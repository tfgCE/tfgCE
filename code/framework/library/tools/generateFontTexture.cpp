#include "libraryTools.h"

#ifdef AN_WINDOWS
#define WITH_FREE_TYPE
#endif

#ifdef WITH_FREE_TYPE
	#include <ft2build.h>

	#include <freetype/freetype.h>
	#include <freetype/ftoutln.h>
#endif

#include "..\..\text\localisedCharacters.h"

#include "..\..\..\core\mainSettings.h"
#include "..\..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\..\core\io\xml.h"
#include "..\..\..\core\random\random.h"
#include "..\..\..\core\types\colour.h"
#include "..\..\..\core\types\unicode.h"
#include "..\..\..\core\serialisation\serialiser.h"
#include "..\..\..\core\system\video\textureUtils.h"

#include "..\..\video\texture.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace ::Framework;

//

struct GenerateFontTexture
{
	String fileName;
	String fontFileName;
	::System::TextureSetup textureSetup;
	Array<::System::TextureSetup> textureSetups;
	int textureLimit = 1;
	VectorInt2 gridSize = VectorInt2(8, 8);
	VectorInt2 gridCharacterSize = VectorInt2(10, 10);
	VectorInt2 characterSize = VectorInt2(8, 8);
	VectorInt2 makeBold = VectorInt2(1, 1);
	int baseLine = 2;
	Array<String> lines;
	bool usesLocalisedCharacters = false;
	bool compressCharacters = false;
	::System::TextureUtils::AddBorder addBorder;
	float contrast = 0.0f;
	float useFullSourceThreshold = 1.0f; // values are scaled from this to 1.0 -> s -> min(s / ufst, 1)
	bool monochromatic = false;
	Colour fontColour = Colour::white;

	IO::Digested digestedSource;

	bool load_from_xml(IO::XML::Node const * _node)
	{
		digestedSource.digest(_node);

		fileName = _node->get_string_attribute(TXT("file"));
		fontFileName = _node->get_string_attribute(TXT("fontFile"));
		if (fileName.is_empty() ||
			fontFileName.is_empty())
		{
			error(TXT("no names provided for library tool \"generate_font_texture\""));
			return false;
		}
		gridSize.load_from_xml_child_node(_node, TXT("gridSize"));
		gridCharacterSize.load_from_xml_child_node(_node, TXT("gridCharacterSize"));
		characterSize.load_from_xml_child_node(_node, TXT("characterSize"));
		makeBold.load_from_xml_child_node(_node, TXT("makeBold"));
		baseLine = _node->get_int_attribute_or_from_child(TXT("baseLine"), baseLine);
		textureSetup.size = gridSize * gridCharacterSize;
		if (!textureSetup.load_from_xml(_node))
		{
			return false;
		}
		compressCharacters = _node->get_bool_attribute_or_from_child_presence(TXT("compressCharacters"), compressCharacters);
		addBorder.load_from_xml(_node);
		contrast = _node->get_float_attribute_or_from_child(TXT("contrast"), contrast);
		useFullSourceThreshold = _node->get_float_attribute_or_from_child(TXT("useFullSourceThreshold"), useFullSourceThreshold);
		monochromatic = _node->get_bool_attribute_or_from_child_presence(TXT("monochromatic"), monochromatic);
		monochromatic = _node->get_bool_attribute_or_from_child_presence(TXT("mono"), monochromatic);
		for_every(n, _node->all_children())
		{
			if (n->get_name() == TXT("rowAnsi") ||
				n->get_name() == TXT("ansi"))
			{
				String ansiCharacters;
				ansiCharacters = String::empty();
				for (tchar ch = compressCharacters? 33 : 32; ch < 128; ++ch)
				{
					ansiCharacters += ch;
				}
				lines.push_back(ansiCharacters);
			}
			if (n->get_name() == TXT("row"))
			{
				lines.push_back(n->get_text());
			}
			if (n->get_name() == TXT("localisedCharacters") ||
				n->get_name() == TXT("rowLocalisedCharacters"))
			{
				Name id = n->get_name_attribute(TXT("setId"));
				if (id.is_valid())
				{
					lines.push_back(LocalisedCharacters::get_set(id));
					usesLocalisedCharacters = true;
				}
			}
		}
		fontColour.load_from_xml_child_or_attr(_node, TXT("colour"));
		fontColour.load_from_xml_child_or_attr(_node, TXT("fontColour"));

		textureLimit = usesLocalisedCharacters ? 0 : 1;

		if (usesLocalisedCharacters)
		{
			digestedSource.add(LocalisedCharacters::get_digested_source()); // accumulate both
		}

		return true;
	}

	void clear(::System::TextureSetup& _textureSetup)
	{
		for (int y = 0; y < _textureSetup.levelSize[0].y; ++y)
		{
			for (int x = 0; x < _textureSetup.levelSize[0].x; ++x)
			{
				_textureSetup.draw_pixel(0, VectorInt2(x, y), fontColour.with_alpha(0.0f));
			}
		}
	}

	void process(LibraryLoadingContext& _lc)
	{
#ifdef WITH_FREE_TYPE
		static Concurrency::SpinLock generateFontTextureLock;
		Concurrency::ScopedSpinLock lock(generateFontTextureLock);

		FT_Error errorCode;

		FT_Library library;
		FT_Face face;

		errorCode = FT_Init_FreeType(&library);
		if (errorCode) { error(TXT("FT_Init_FreeType: error %i"), errorCode); return; }

		String fontFilePath = IO::Utils::make_path(_lc.get_dir(), fontFileName);

		Array<char> fontFilePath8 = fontFilePath.to_char8_array();
		errorCode = FT_New_Face(library, fontFilePath8.get_data(), 0, &face);
		if (errorCode) { error(TXT("FT_New_Face: error %i"), errorCode); return; }

		FT_Set_Pixel_Sizes(face, characterSize.x - max(0, makeBold.x - 1), characterSize.y - max(0, makeBold.y - 1));

		FT_GlyphSlot slot;
		slot = face->glyph;

		VectorInt2 charAt = VectorInt2::zero;

		FT_Matrix matrix;
		matrix.xx = (FT_Fixed)(0x10000L);
		matrix.xy = (FT_Fixed)(0x00000L);
		matrix.yx = (FT_Fixed)(0x00000L);
		matrix.yy = (FT_Fixed)(0x10000L);

		int textureIdx = NONE;
		::System::TextureSetup* drawToTextureSetup = nullptr;

		bool atNewLine = true;
		for_every(line, lines)
		{
			if (!atNewLine && !compressCharacters)
			{
				atNewLine = true;
				charAt.x = 0;
				++charAt.y;
			}
			for (int i = 0; i < line->get_length(); ++i)
			{
				if (charAt.x >= gridSize.x)
				{
					atNewLine = true;
					charAt.x = 0;
					++charAt.y;
				}

				if (charAt.y >= gridSize.y)
				{
					if (textureLimit > 0 && textureSetups.get_size() >= textureLimit)
					{
						error(TXT("exceeded grid size for \"%S\" and available texture count"), fileName.to_char());
						break;
					}
					// new texture required!
					textureIdx = NONE;
				}

				// start new texture
				if (textureIdx == NONE)
				{
					textureIdx = textureSetups.get_size();
					textureSetups.push_back(textureSetup);
					drawToTextureSetup = &textureSetups.get_last();
					drawToTextureSetup->create_pixels();
					clear(*drawToTextureSetup);
					charAt = VectorInt2::zero;
				}

				/* set transformation */
				FT_Set_Transform(face, &matrix, nullptr);

				/* load glyph image into the slot (erase previous one) */
				errorCode = FT_Load_Char(face, Unicode::tchar_to_unicode((*line)[i]), FT_LOAD_RENDER | (monochromatic? FT_LOAD_TARGET_MONO : 0));
				bool ok = true;
				if (errorCode)
				{
					error(TXT("could not find character unicode %i of font %S"), Unicode::tchar_to_unicode((*line)[i]), fontFileName.to_char());
					ok = false;
				}
				else if (slot->bitmap.width == 0 ||
						 slot->bitmap.rows == 0)
				{
					error(TXT("empty character unicode %i of font %S"), Unicode::tchar_to_unicode((*line)[i]), fontFileName.to_char());
					ok = false;
				}
				if (ok)
				{
					VectorInt2 drawAt = charAt * gridCharacterSize;
					drawAt.x += gridCharacterSize.x / 2;
					drawAt.y = gridSize.y * gridCharacterSize.y - 1 - drawAt.y;
					drawAt.y += -gridCharacterSize.y / 2 - characterSize.y / 2 + baseLine;

					// drawAt is now at centre at line at base

					struct ReadPixel
					{
						static float read(FT_Bitmap& bitmap, VectorInt2 const& at)
						{
							int pixelSize = 1;
							if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
							{
								int bigPixel = (at.x / 8);
								int smallPixel = 7 - (at.x - (bigPixel * 8));
								int srcIdx = (bitmap.rows-1 - at.y) * bitmap.pitch + bigPixel;
								int pixelData = (bitmap.buffer[srcIdx] >> smallPixel) & 1;
								return (float)pixelData;
							}
							else if (bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
							{
								int srcIdx = (bitmap.rows-1 - at.y) * bitmap.pitch + at.x;
								int pixelData = bitmap.buffer[srcIdx];
								float p = (float)pixelData / max(1.0f, (float)bitmap.num_grays);
								return clamp(p, 0.0f, 1.0f);
							}
							else
							{
								error(TXT("NOT HANDLED"));
								return 0.0f;
							}
						}
					};

					VectorInt2 copySize(slot->bitmap.width, slot->bitmap.rows);
					{
						float maxP = 0.0f;
						for (int y = 0; y < copySize.y; ++y)
						{
							for (int x = 0; x < copySize.x; ++x)
							{
								float p = ReadPixel::read(slot->bitmap, VectorInt2(x, y));
								if (monochromatic) p = p < 0.5f ? 0.0f : 1.0f;
								p = lerp(contrast, p, p < 0.25f ? 0.0f : 1.0f);
								if (useFullSourceThreshold > 0.0f)
								{
									p = min(p / useFullSourceThreshold, 1.0f);
								}
								maxP = max(maxP, p);
								VectorInt2 at = drawAt + VectorInt2((x) - copySize.x / 2, slot->bitmap_top - (slot->bitmap.rows - (y)));
								for (int bx = 0; bx < max(1, makeBold.x); ++bx)
								{
									for (int by = 0; by < max(1, makeBold.y); ++by)
									{
										VectorInt2 bat = at;
										bat.x += bx;
										bat.y += by;
										drawToTextureSetup->draw_pixel(0, bat, Colour::lerp(p, drawToTextureSetup->get_pixel(0, bat), fontColour));
									}
								}
							}
						}
						if (maxP == 0.0f)
						{
							error(TXT("no pixels for character unicode %i of font %S"), Unicode::tchar_to_unicode((*line)[i]), fontFileName.to_char());
						}
					}
				}

				++charAt.x;
			}

			if (charAt.y >= gridSize.y)
			{
				break;
			}
		}

		for_every(ts, textureSetups)
		{
			addBorder.process(*ts); // will add only if required, no need to check if required
		}

		output(TXT("generated font texture \"%S\""), fileName.to_char());
		FT_Done_Face(face);
		FT_Done_FreeType(library);
#endif
	}
};

bool LibraryTools::generate_font_texture(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (!MainSettings::global().should_allow_importing())
	{
		// no importing? don't create texture, as this is almost same as importing - otherwise force importing
		return true;
	}

#ifdef WITH_FREE_TYPE
	bool result = true;

	struct AddIndexToFileName
	{
		static String add(String const& _fileName, int _idx)
		{
			String fileName = _fileName;
			String extraFileInfo = String::printf(TXT(" [%i]"), _idx);
			int dotAt = fileName.find_last_of('.');
			if (dotAt != NONE)
			{
				fileName = fileName.get_left(dotAt) + extraFileInfo + fileName.get_sub(dotAt);
			}
			else
			{
				fileName = fileName + extraFileInfo;
			}
			return fileName;
		}
	};

	Array<GenerateFontTexture> generateFontTextures;

	for_every(dataNode, _node->all_children())
	{
		if (dataNode->get_name() == TXT("texture"))
		{
			GenerateFontTexture gft;
			if (gft.load_from_xml(dataNode))
			{
				generateFontTextures.push_back(gft);
			}
			else
			{
				result = false;
			}
		}
	}

	// compare to file
	if (! MainSettings::global().should_force_reimporting() &&
		! MainSettings::global().should_force_reimporting_using_tools())
	{
		bool needsToRegenerate = false;
		for_every(gft, generateFontTextures)
		{
			if (gft->fileName.is_empty())
			{
				error(TXT("no name provided for library tool \"generate_texture\""));
				return false;
			}
			String fileName = gft->fileName;
			if (gft->textureSetups.get_size() > 1 || gft->usesLocalisedCharacters)
			{
				fileName = AddIndexToFileName::add(fileName, 0);
			}
			String path = IO::Utils::make_path(_lc.get_dir(), fileName);
			if (IO::File::does_exist(path))
			{
				::System::Texture texture;
				if (texture.serialise(Serialiser::reader(IO::File(path).temp_ptr()).temp_ref()))
				{
					if (!(gft->digestedSource == texture.get_digested_source()))
					{
						needsToRegenerate = true;
						break;
					}
				}
				else
				{
					needsToRegenerate = true;
					break;
				}
			}
			else
			{
				needsToRegenerate = true;
				break;
			}
		}
		if (!needsToRegenerate)
		{
			return true;
		}
	}

	// create place for pixels and generate each one
	for_every(gft, generateFontTextures)
	{
		if (gft->fileName.is_empty())
		{
			error(TXT("no name provided for library tool \"generate_texture\""));
			return false;
		}
		gft->process(_lc);
	}

	// create texture(s)
	for_every(gft, generateFontTextures)
	{
		for_every(ts, gft->textureSetups)
		{
			::System::Texture* texture = new ::System::Texture();
			texture->use_setup_to_init(*ts);

			// serialise texture to file
			texture->set_digested_source(gft->digestedSource);
			String fileName = gft->fileName;
			if (gft->textureSetups.get_size() > 1 || gft->usesLocalisedCharacters)
			{
				fileName = AddIndexToFileName::add(fileName, for_everys_index(ts));
			}
			texture->serialise_resource_to_file(IO::Utils::make_path(_lc.get_dir(), fileName));

			// we no longer need it
			delete texture;
		}
	}

	return result;
#else
	warn(TXT("can't generate font texture"));
	return true;
#endif
}
