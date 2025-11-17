#include "texture.h"

#include "..\..\concurrency\scopedSpinLock.h"

#include "..\..\mainSettings.h"

#include "..\..\serialisation\serialiser.h"

#include "video3d.h"
#include "imageLoaders\il_tga.h"
#ifdef AN_SDL
#include "videoSDL.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace System;

//

#define VER_BASE 0
#define VER_MIP_MAPS 1
#define VER_DATA_PACKED 2
#define CURRENT_VERSION VER_DATA_PACKED

enum PackAlgorithm
{
	PlainData,
	RLE,
};

Importer<Texture, TextureImportOptions> Texture::s_importer;

//

static bool is_power_2(int _a)
{
	_a = abs(_a);
	while (_a > 1)
	{
		if ((_a % 2) == 1)
		{
			return false;
		}
		_a = _a / 2;
	}
	return true;
}

//

TextureCommonSetup::TextureCommonSetup()
: withMipMaps(false)
, wrapU(MainSettings::global().get_default_texture_wrap_u())
, wrapV(MainSettings::global().get_default_texture_wrap_v())
, filteringMag(MainSettings::global().get_default_texture_filtering_mag())
, filteringMin(MainSettings::global().get_default_texture_filtering_min())
{
}

bool TextureCommonSetup::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("withMipMaps")))
	{
		withMipMaps = attr->get_as_bool();
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("wrap")))
	{
		wrapU = ::System::TextureWrap::parse(attr->get_as_string().trim());
		wrapV = wrapU;
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("wrapU")))
	{
		wrapU = ::System::TextureWrap::parse(attr->get_as_string().trim());
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("wrapV")))
	{
		wrapV = ::System::TextureWrap::parse(attr->get_as_string().trim());
	}

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("filtering")))
	{
		filteringMag = ::System::TextureFiltering::parse(attr->get_as_string().trim());
		filteringMin = filteringMag;
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("filteringMag")))
	{
		filteringMag = ::System::TextureFiltering::parse(attr->get_as_string().trim());
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("filteringMin")))
	{
		filteringMin = ::System::TextureFiltering::parse(attr->get_as_string().trim());
	}

	return result;
}

bool TextureCommonSetup::serialise(Serialiser & _serialiser, int _version)
{
	bool result = true;
	result &= serialise_data(_serialiser, withMipMaps);
	result &= serialise_data(_serialiser, wrapU);
	result &= serialise_data(_serialiser, wrapV);
	result &= serialise_data(_serialiser, filteringMag);
	result &= serialise_data(_serialiser, filteringMin);

	return result;
}

//

bool TextureImportOptions::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	autoMipMaps = _node->get_bool_attribute_or_from_child_presence(TXT("autoMipMaps"), autoMipMaps);

	return result;
}

//

TextureImportSetup::TextureImportSetup()
{
}

//

TextureSetup::TextureSetup()
: format(VideoFormat::rgba8)
, mode(BaseVideoFormat::rgba)
, size(0, 0)
, mipMapLevels(1)
{
}

TextureSetup::TextureSetup(Colour const & _colour)
: format(VideoFormat::rgba8)
, mode(BaseVideoFormat::rgba)
, size(1, 1)
, mipMapLevels(1)
{
	// default white texture
	create_pixels(false);
	pixels[0][0] = (int8)((uint8)clamp<int>((int)(_colour.r * 255.0f), 0, 255));
	pixels[0][1] = (int8)((uint8)clamp<int>((int)(_colour.g * 255.0f), 0, 255));
	pixels[0][2] = (int8)((uint8)clamp<int>((int)(_colour.b * 255.0f), 0, 255));
	pixels[0][3] = (int8)((uint8)clamp<int>((int)(_colour.a * 255.0f), 0, 255));
}

void TextureSetup::create_pixels(bool _clear)
{
	pixels.set_size(max(1, mipMapLevels));
	levelSize.set_size(max(1, mipMapLevels));
	int x = size.x;
	int y = size.y;
	for_count(int, level, max(1, mipMapLevels))
	{
		levelSize[level] = VectorInt2(x, y);
		int dataSize = x * y * VideoFormat::get_pixel_size(format);
		pixels[level].set_size(dataSize);
		if (_clear)
		{
			// clear it
			memory_zero(pixels[level].get_data(), dataSize);
		}
		x = max(1, x / 2);
		y = max(1, y / 2);
	}
}

bool TextureSetup::is_pos_valid(int _level, VectorInt2 const& _pos) const
{
	VectorInt2 size = levelSize[_level];
	VectorInt2 pos = _pos;
	bool isValid = true;
	if (wrapU == TextureWrap::clamp)
	{
		isValid &= pos.x >= 0 && pos.x < size.x;
	}
	if (wrapV == TextureWrap::clamp)
	{
		isValid &= pos.y >= 0 && pos.y < size.y;
	}
	return isValid;
}

VectorInt2 TextureSetup::validate_pos(int _level, VectorInt2 const& _pos) const
{
	VectorInt2 size = levelSize[_level];
	VectorInt2 pos = _pos;
	if (wrapU == TextureWrap::clamp)
	{
		pos.x = clamp(pos.x, 0, size.x - 1);
	}
	else
	{
		an_assert(wrapU == TextureWrap::repeat);
		pos.x = mod(pos.x, size.x);
	}
	if (wrapU == TextureWrap::clamp)
	{
		pos.y = clamp(pos.y, 0, size.y - 1);
	}
	else
	{
		an_assert(wrapU == TextureWrap::repeat);
		pos.y = mod(pos.y, size.y);
	}
	return pos;
}

void TextureSetup::draw_pixel(int _level, VectorInt2 const & _pos, Colour const & _colour)
{
	VectorInt2 pos = validate_pos(_level, _pos);
	int at = ((pos.y) * levelSize[_level].x + pos.x) * VideoFormat::get_pixel_size(format);
	if (!pixels[_level].is_index_valid(at) ||
		!pixels[_level].is_index_valid(at + VideoFormat::get_pixel_size(format) - 1))
	{
		return;
	}
	int8* loc = &pixels[_level][at];
	Colour colour = _colour;
	colour.r = clamp(colour.r, 0.0f, 1.0f);
	colour.g = clamp(colour.g, 0.0f, 1.0f);
	colour.b = clamp(colour.b, 0.0f, 1.0f);
	colour.a = clamp(colour.a, 0.0f, 1.0f);
	VideoFormat::encode(format, colour, loc);
}

Colour TextureSetup::get_pixel(int _level, VectorInt2 const & _pos) const
{
	if (!is_pos_valid(_level, _pos))
	{
		return Colour::none;
	}
	VectorInt2 pos = validate_pos(_level, _pos);
	int at = ((pos.y) * levelSize[_level].x + pos.x) * VideoFormat::get_pixel_size(format);
	if (!pixels[_level].is_index_valid(at) ||
		!pixels[_level].is_index_valid(at + VideoFormat::get_pixel_size(format) - 1))
	{
		return Colour::none;
	}
	int8 const* loc = &pixels[_level][at];
	Colour colour;
	VideoFormat::decode(format, OUT_ colour, loc);
	return colour;
}

bool TextureSetup::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	// read format, assume rgba8
	format = ::System::VideoFormat::rgba8;
	if (IO::XML::Attribute const * _attr = _node->get_attribute(TXT("format")))
	{
		format = ::System::VideoFormat::parse(_attr->get_as_string());
	}
	// get default mode
	mode = ::System::VideoFormat::to_base_video_format(format);

	mipMapLevels = _node->get_int_attribute(TXT("mipMapLevels"), mipMapLevels);
	if (mipMapLevels != 1)
	{
		withMipMaps = true; // otherwise mip maps won't be created
	}

	// get size
	if (IO::XML::Attribute const * _attr = _node->get_attribute(TXT("size")))
	{
		result &= size.load_from_string(_attr->get_as_string());
	}
	else if (size.is_zero())
	{
		error_loading_xml(_node, TXT("no size provided"));
		result = false;
	}

	if (size.x < 4 || size.y < 4)
	{
		error_loading_xml(_node, TXT("size is invalid, it should be 4 or greater (when generating texture)"));
		result = false;
	}

	return result;
}

bool TextureSetup::serialise(Serialiser & _serialiser, int _version)
{
	bool result = base::serialise(_serialiser, _version);
	result &= serialise_data(_serialiser, format);
	result &= serialise_data(_serialiser, mode);
	result &= serialise_data(_serialiser, size);
	if (_version >= VER_MIP_MAPS)
	{
		result &= serialise_data(_serialiser, mipMapLevels);
		result &= serialise_data(_serialiser, levelSize);
	}
	else
	{
		mipMapLevels = 1;
		levelSize.set_size(1);
		levelSize[0] = size;
	}
	pixels.set_size(max(1, mipMapLevels));
	for_count(int, level, max(1, mipMapLevels))
	{
		if (_version >= VER_DATA_PACKED)
		{
			if (_serialiser.is_reading())
			{
				int packAlgorithm = 0;
				serialise_data(_serialiser, packAlgorithm);
				if (packAlgorithm == PackAlgorithm::PlainData)
				{
					result &= serialise_data(_serialiser, pixels[level]);
				}
				else if (packAlgorithm == PackAlgorithm::RLE)
				{
					auto& pixelsData = pixels[level];
					int pixelSize = VideoFormat::get_pixel_size(format);
					int pixelCount = 0;
					result &= serialise_data(_serialiser, pixelCount);
					pixelsData.set_size(pixelCount * pixelSize);
					int pixelIdx = 0;
					Array<int8> currPixelBuffer;
					while (pixelIdx < pixelCount)
					{
						int currPixelCount = 0;
						bool currPixelBufferSamePixels = false;
						{
							byte bufferInfo = 0;
							result &= serialise_data(_serialiser, bufferInfo);
							currPixelBufferSamePixels = is_flag_set(bufferInfo, bit(7));
							clear_flag(bufferInfo, bit(7));
							currPixelCount = (int)bufferInfo;
						}
						if (currPixelBufferSamePixels)
						{
							currPixelBuffer.set_size(pixelSize);
						}
						else
						{
							currPixelBuffer.set_size(pixelSize * currPixelCount);
						}
						for_every(d, currPixelBuffer)
						{
							result &= serialise_data(_serialiser, *d);
						}
						if (currPixelBufferSamePixels)
						{
							for_count(int, i, currPixelCount)
							{
								memory_copy(&pixelsData[pixelIdx * pixelSize], currPixelBuffer.get_data(), pixelSize);
								++pixelIdx;
							}
						}
						else
						{
							memory_copy(&pixelsData[pixelIdx * pixelSize], currPixelBuffer.get_data(), pixelSize * currPixelCount);
							pixelIdx += currPixelCount;
						}
					}
				}
				else
				{
					error(TXT("could not recognise pack algorithm"));
					result = false;
				}
			}
			else
			{
				an_assert(_serialiser.is_writing());
				int packAlgorithm = PackAlgorithm::RLE;
				// assume RLE
				serialise_data(_serialiser, packAlgorithm);
				if (packAlgorithm == PackAlgorithm::PlainData)
				{
					result &= serialise_data(_serialiser, pixels[level]);
				}
				else if (packAlgorithm == PackAlgorithm::RLE)
				{
					auto& pixelsData = pixels[level];
					int pixelSize = VideoFormat::get_pixel_size(format);
					int pixelCount = pixelsData.get_size() / pixelSize;
					result &= serialise_data(_serialiser, pixelCount);
					int pixelIdx = 0;
					Array<int8> currPixelBuffer;
					int currPixelCount = 0;
					bool currPixelBufferSamePixels = false;
					while (true)
					{
						bool currPixelSameAsLast = false;
						int8* currPixelData = nullptr;
						if (pixelIdx < pixelCount)
						{
							currPixelData = &pixelsData[pixelIdx * pixelSize];
							if (currPixelCount == 0)
							{
								currPixelBufferSamePixels = false;
								currPixelSameAsLast = false;;
							}
							else 
							{
								an_assert(currPixelCount > 0);
								currPixelSameAsLast = memory_compare_8(&currPixelBuffer[currPixelBufferSamePixels? 0 : (currPixelCount - 1) * pixelSize], currPixelData, pixelSize);
								if (currPixelCount == 1)
								{
									currPixelBufferSamePixels = currPixelSameAsLast;
								}
							}
						}
						//
						bool storeBuffer = false;
						if (pixelIdx >= pixelCount)
						{
							storeBuffer = true;
						}
						else
						{
							if (currPixelCount > 0 &&
								currPixelBufferSamePixels != currPixelSameAsLast)
							{
								storeBuffer = true;
							}
							if (currPixelCount >= 127)
							{
								storeBuffer = true;
							}
						}
						//
						if (storeBuffer)
						{
							if (currPixelData && !currPixelBufferSamePixels && currPixelSameAsLast)
							{
								// we will store last one as unique
								--currPixelCount;
								currPixelBuffer.set_size(currPixelBuffer.get_size() - pixelSize);
							}
							{
								byte bufferInfo = 0;
								bufferInfo = (byte)currPixelCount;
								if (currPixelBufferSamePixels)
								{
									bufferInfo |= bit(7);
								}
								result &= serialise_data(_serialiser, bufferInfo);
							}
							for_every(d, currPixelBuffer)
							{
								serialise_data(_serialiser, *d);
							}
							//
							currPixelBuffer.clear();
							currPixelCount = 0;
							if (currPixelData && !currPixelBufferSamePixels && currPixelSameAsLast)
							{
								// we had the same pixel as the last one, add that last one to our buffer and pretend we're having same pixels now
								currPixelBufferSamePixels = currPixelSameAsLast;
								currPixelBuffer.set_size(pixelSize);
								memory_copy(currPixelBuffer.begin(), currPixelData, pixelSize);
								++currPixelCount;
							}
						}
						//
						if (pixelIdx >= pixelCount)
						{
							break;
						}
						else
						{
							if (!currPixelSameAsLast || currPixelCount == 0)
							{
								currPixelBuffer.grow_size(pixelSize);
								memory_copy(&currPixelBuffer[currPixelCount * pixelSize], currPixelData, pixelSize);
							}
							++currPixelCount;
						}
						//
						++pixelIdx;
					}
				}
			}
		}
		else
		{
			result &= serialise_data(_serialiser, pixels[level]);
		}
	}

	return result;
}

//

Texture::Texture()
: textureObject(texture_id_none())
, isValid(false)
, size(VectorInt2::zero)
{
}

Texture::~Texture()
{
	close();
}

void Texture::close()
{
	if (textureObject != texture_id_none())
	{
		assert_rendering_thread();
		DIRECT_GL_CALL_ glDeleteTextures(1, &textureObject); AN_GL_CHECK_FOR_ERROR
		gpu_memory_info_freed(GPU_MEMORY_INFO_TYPE_TEXTURE, textureObject);
		textureObject = texture_id_none();
	}
	size = VectorInt2::zero;
	isValid = false;
}

bool Texture::init_with_setup_dont_create(TextureImportSetup const & _importSetup)
{
#ifdef AN_SDL
	Concurrency::ScopedSpinLock lock(VideoSDL::imageLock);
#endif

	if (setup.is_set())
	{
		setup.clear();
	}

	close();

	bool loaded = false;

	struct LoadedImage
	{
		ImageLoader::TGA* tga = nullptr;
		bool load(String const& _fileName, String const& _forceType)
		{
			bool loaded = false;
			if (!loaded && (_fileName.to_lower().has_suffix(TXT("tga")) || _forceType == TXT("tga")))
			{
				tga = new ImageLoader::TGA();
				if (tga->load(_fileName))
				{
					loaded = true;
				}
				else
				{
					delete_and_clear(tga);
				}
			}
			return loaded;
		}

		~LoadedImage()
		{
			if (tga)
			{
				delete_and_clear(tga);
			}
		}
	};

	LoadedImage* image = new LoadedImage();
	Array<LoadedImage*> MMs;
	int imageBytesPerPixel = 0;

	VectorInt2 readSize = VectorInt2::zero;
	int mipMapLevels = 1;

	if (!_importSetup.sourceFile.is_empty())
	{
		loaded = image->load(_importSetup.sourceFile, _importSetup.forceType);
		if (loaded)
		{
			if (image->tga)
			{
				imageBytesPerPixel = image->tga->get_bytes_per_pixel();
				readSize = image->tga->get_size();
			}
		}
		if (loaded)
		{
			// check for mip map levels
			int dotAt = _importSetup.sourceFile.find_last_of('.');
			if (dotAt != NONE)
			{
				// NOTE:	of course this way we won't catch changes in those files we're looking for right now.
				while (true)
				{
					String fileName = _importSetup.sourceFile.get_left(dotAt) + String::printf(TXT(" [mip%i]"), mipMapLevels) + _importSetup.sourceFile.get_sub(dotAt);
					LoadedImage* mm = new LoadedImage();
					if (mm->load(fileName, _importSetup.forceType))
					{
						MMs.push_back(mm);
						++mipMapLevels;
					}
					else
					{
						delete mm;
						break;
					}
				}
			}
		}
	}

	if (!loaded)
	{
		delete_and_clear(image);
		close();
		if (!_importSetup.ignoreIfNotAbleToLoad)
		{
			error(TXT("error loading/creating texture"));
		}
		return false;
	}

	bool result = true;

	size = readSize;

	// initialise data
	setup = TextureSetup();
	setup.access().size = size;
	setup.access().format = VideoFormat::rgb8;
	setup.access().mode = BaseVideoFormat::rgb;
	// copy common
	*((::System::TextureCommonSetup*)&setup.access()) = *((::System::TextureCommonSetup*)&_importSetup);

	{
		bool readImage = false;
		if (!readImage && image->tga)
		{
			if (image->tga->get_bytes_per_pixel() == 4)
			{
				setup.access().format = VideoFormat::rgba8;
				setup.access().mode = BaseVideoFormat::rgba; // we shift it when we read it, if it was bgra
			}
			else if (image->tga->get_bytes_per_pixel() == 3)
			{
				setup.access().format = VideoFormat::rgb8;
				setup.access().mode = BaseVideoFormat::rgb; // we shift it when we read it, if it was bgr
			}
			else
			{
				an_assert(false, TXT("format unknown"));
			}
			int pixelsRowSize = image->tga->get_bytes_per_pixel() * image->tga->get_size().x;
			setup.access().withMipMaps |= _importSetup.autoMipMaps || mipMapLevels != 1;
			setup.access().mipMapLevels = _importSetup.autoMipMaps? 0 : mipMapLevels;
			setup.access().create_pixels(false);
			for (int row = 0; row < image->tga->get_size().y; ++row)
			{
				// copy upside down
				memory_copy(&setup.access().pixels[0][pixelsRowSize * row], &((int8*)image->tga->get_data().begin())[pixelsRowSize * (image->tga->get_size().y - 1 - row)], pixelsRowSize);
			}
			readImage = true;
		}
	}

	if (! MMs.is_empty())
	{
		int mmLevel = 1;
		for_every_ptr(mm, MMs)
		{
			bool readMM = false;
			if (!readMM && mm->tga)
			{
				if (mm->tga->get_bytes_per_pixel() != imageBytesPerPixel)
				{
					error(TXT("surfaces of imcomaptible type, level %i"), mmLevel);
					result = false;
					break;
				}
				if (mm->tga->get_size() != setup.access().levelSize[mmLevel])
				{
					error(TXT("level %i size does not match, has %ix%i, expecting %ix%i"), mmLevel, mm->tga->get_size().x, mm->tga->get_size().y, setup.access().levelSize[mmLevel].x, setup.access().levelSize[mmLevel].y);
					result = false;
					break;
				}
				int pixelsRowSize = mm->tga->get_bytes_per_pixel() * mm->tga->get_size().x;
				for (int row = 0; row < mm->tga->get_size().y; ++row)
				{
					// copy upside down
					memory_copy(&setup.access().pixels[mmLevel][pixelsRowSize * row], &((int8*)mm->tga->get_data().begin())[pixelsRowSize * (mm->tga->get_size().y - 1 - row)], pixelsRowSize);
				}
				readMM = true;
			}
			if (readMM)
			{
				++mmLevel;
			}
			else
			{
				break;
			}
		}
	}

	delete_and_clear(image);
	for_every(mm, MMs)
	{
		delete_and_clear(*mm);
	}
	return result;
}

bool Texture::init_with_setup(TextureImportSetup const & _importSetup)
{
	if (init_with_setup_dont_create(_importSetup))
	{
		return init_as_stored();
	}
	else
	{
		return false;
	}
}

bool Texture::init_as_stored()
{
	assert_rendering_thread();

	close();

	isValid = false;
	if (!setup.is_set())
	{
		an_assert(false, TXT("no data to init?"));
		return false;
	}

	an_assert(setup.get().size.x != 0 && setup.get().size.y != 0);

	// generate texture and bind - we will be working on it
	DIRECT_GL_CALL_ glGenTextures(1, &textureObject); AN_GL_CHECK_FOR_ERROR
	Video3D::get()->mark_use_texture(0, textureObject); // do it through this, so video 3d can restore it later
	Video3D::get()->force_send_use_texture_slot(0);
	Video3D::get()->requires_send_use_textures();

	DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TextureFiltering::get_sane_min(setup.get().filteringMin)); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, TextureFiltering::get_sane_mag(setup.get().filteringMag)); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, setup.get().wrapU); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, setup.get().wrapV); AN_GL_CHECK_FOR_ERROR
	int mipMapLevels = setup.get().mipMapLevels;
#ifdef AN_GLES
	if (setup.get().withMipMaps && mipMapLevels != 1)
	{
		if (!is_power_2(setup.get().size.x) ||
			!is_power_2(setup.get().size.y))
		{
			warn(TXT("texture size %ix%i required to be powers of 2 for mipmaps. not using mipmaps for this one"), setup.get().size.x, setup.get().size.y);
			mipMapLevels = 1;
		}
		// it works fine with "repeat"
		/*
		if (setup.get().wrapU != TextureWrap::clamp ||
			setup.get().wrapV != TextureWrap::clamp)
		{
			warn(TXT("for gles we are required to use clamping. not using mipmaps for this one"));
			mipMapLevels = 1;
		}
		*/
	}
#endif
	if (setup.get().withMipMaps)
	{
		if (mipMapLevels == 1)
		{
			DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0); AN_GL_CHECK_FOR_ERROR
		}
		else if (mipMapLevels > 1)
		{
			// only if exact level provided (then we have exact number of data available, otherwise use default (which at the moment is 1000)
			DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapLevels - 1); AN_GL_CHECK_FOR_ERROR
		}
		else
		{
			DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 10); AN_GL_CHECK_FOR_ERROR
#ifdef AN_GL
			DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); AN_GL_CHECK_FOR_ERROR // auto generate all mip map levels
#endif
		}
	}
	else
	{
		DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0); AN_GL_CHECK_FOR_ERROR
	}

	Video3D::get()->mark_disable_blend();
	Video3D::get()->requires_send_all();

	VideoFormatData::Type formatData = VideoFormatData::from(setup.get().format);

	// load levels (as many as we have, no more than mip map levels requested - we may have more mip map levels than pixels, never opposite)
	for_count(int, level, min(setup.get().pixels.get_size(), max(1, setup.get().withMipMaps? mipMapLevels : 1)))
	{
		int8 const * dataToLoad = setup.get().pixels[level].get_data();

		int dataToLoadSize = setup.get().pixels[level].get_data_size();
		bool requiresConversion = false;
		if (VideoFormat::does_require_conversion(setup.get().format))
		{
			requiresConversion = true;
		}
		Array<int8> newDataToLoad;
		if (requiresConversion)
		{
			newDataToLoad.set_size(dataToLoadSize);
			VideoFormat::convert(setup.get().format, newDataToLoad.begin(), dataToLoad, dataToLoadSize);
			dataToLoad = newDataToLoad.begin();
		}
		VectorInt2 size = level == 0 ? setup.get().size : setup.get().levelSize[level];
		DIRECT_GL_CALL_ glTexImage2D(GL_TEXTURE_2D, level, setup.get().format, size.x, size.y, 0, setup.get().mode, formatData, dataToLoad); AN_GL_CHECK_FOR_ERROR
		gpu_memory_info_allocated(GPU_MEMORY_INFO_TYPE_TEXTURE, textureObject, size.x * size.y * ::System::VideoFormat::get_pixel_size(setup.get().format));
	}

	if (setup.get().withMipMaps)
	{
		if (mipMapLevels <= 0)
		{
			DIRECT_GL_CALL_ glGenerateMipmap(GL_TEXTURE_2D); AN_GL_CHECK_FOR_ERROR
		}
	}
	
	size = setup.get().size;
	isValid = true;

	Video3D::get()->mark_use_texture(0, texture_id_none()); // check above
	Video3D::get()->requires_send_use_textures();

	return true;
}

void Texture::use_setup_to_init(TextureSetup const & _setup)
{
	setup = _setup;
}

bool Texture::serialise(Serialiser & _serialiser)
{
	version = CURRENT_VERSION;
	bool result = SerialisableResource::serialise(_serialiser);
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid texture version"));
		return false;
	}

	// data to init might be not present (although that will be an error)
	bool setupPresent = false;
	if (_serialiser.is_writing())
	{
		an_assert(setup.is_set(), TXT("what exactly are we trying to serialise here?"));
		setupPresent = setup.is_set();
	}
	result &= serialise_data(_serialiser, setupPresent);
	if (_serialiser.is_reading())
	{
		an_assert(setupPresent, TXT("what exactly are we trying to serialise here?"));
		if (setupPresent)
		{
			setup = TextureSetup();
		}
		else
		{
			setup.clear();
		}
	}

	if (! setup.is_set())
	{
		error(TXT("texture's setup not present during serialisation"));
		return false;
	}

	result &= setup.access().serialise(_serialiser, version);

	return result;
}

