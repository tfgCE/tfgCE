
#include "font.h"

#include "video3d.h"

#include "..\..\containers\arrayStack.h"
#include "..\..\debug\debug.h"
#include "..\..\mesh\mesh3d.h"
#include "..\..\mesh\mesh3dBuilder.h"
#include "..\..\other\typeConversions.h"
#include "..\..\system\core.h"
#include "..\..\system\video\renderTarget.h"
#include "..\..\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace ::System;

//

DEFINE_STATIC_NAME(inTexture);

// system tag
DEFINE_STATIC_NAME(lowGraphics);

//

CharacterDefinition::CharacterDefinition()
: leftBottom(Vector2::zero)
, size(Vector2::zero)
, wholeLeftBottom(Vector2::zero)
, wholeSize(Vector2::zero)
{}

CharacterDefinition::CharacterDefinition(int _textureIdx, tchar _ch, bool _colourCharacter, Vector2 const& _gridLeftBottom, Vector2 const& _gridSize, Vector2 const & _leftBottom, Vector2 const & _size, Vector2 const & _leftBottomSpacing, Vector2 const & _rightTopSpacing)
: textureIdx(_textureIdx)
, ch(_ch)
, colourCharacter(_colourCharacter)
, leftBottom(_leftBottom)
, size(_size)
, wholeLeftBottom(_leftBottom)
, wholeSize(_size)
, gridLeftBottom(_gridLeftBottom)
, gridSize(_gridSize)
, leftBottomSpacing(_leftBottomSpacing)
, rightTopSpacing(_rightTopSpacing)
{}

void CharacterDefinition::recalculate()
{
	sizeWithSpacing = size + leftBottomSpacing + rightTopSpacing;
}

//

Vector2 const Font::c_hCentred = Vector2(0.5f, 0.0f);
Vector2 const Font::c_hLeft = Vector2(0.0f, 0.0f);
Vector2 const Font::c_hRight = Vector2(1.0f, 0.0f);
Optional<float> Font::s_variableSpaceWidthPt;

Font::Font()
: fontType(FontType::FixedSize)
, alignToPixels(true)
, fixedFontSize(8, 8)
, fixedFontSpacing(0, 0)
{
	fallbackShaderProgramInstance = new ShaderProgramInstance();
	fallbackNoBlendShaderProgramInstance = new ShaderProgramInstance();

	an_assert(Video3D::get());
	fallbackShaderProgramInstance->set_shader_program(Video3D::get()->get_fallback_shader_program(Video3DFallbackShader::For3D | Video3DFallbackShader::WithTexture));
	fallbackNoBlendShaderProgramInstance->set_shader_program(Video3D::get()->get_fallback_shader_program(Video3DFallbackShader::For3D | Video3DFallbackShader::WithTexture | Video3DFallbackShader::DiscardsBlending));

	vertexFormat.with_colour_rgba().with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float).no_padding().calculate_stride_and_offsets();
	vertexFormatNoCharacterColour.with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float).no_padding().calculate_stride_and_offsets();
}

Font::~Font()
{
	delete_and_clear(fallbackShaderProgramInstance);
	delete_and_clear(fallbackNoBlendShaderProgramInstance);
}

bool Font::load_from_xml(IO::XML::Node const * _node)
{
	bool result = false;
	if (IO::XML::Node const * childNode = _node->first_child_named(TXT("fixedSize")))
	{
		fontType = FontType::FixedSize;
		fixedFontSize.load_from_xml(childNode);
		fixedFontSpacing.load_from_xml(childNode, TXT("spacingx"), TXT("spacingy"));
	}
	if (IO::XML::Node const * childNode = _node->first_child_named(TXT("variableWidth")))
	{
		fontType = FontType::VariableWidth;
		fixedFontSize.load_from_xml(childNode);
		fixedFontSpacing.load_from_xml(childNode, TXT("spacingx"), TXT("spacingy"));
		variableWidthLeftMargin = childNode->get_int_attribute(TXT("leftMargin"), variableWidthLeftMargin);
		variableWidthRightMargin = childNode->get_int_attribute(TXT("rightMargin"), variableWidthRightMargin);
	}

	for_every(childNode, _node->all_children())
	{
		if (childNode->get_name() == TXT("fixedSize") ||
			childNode->get_name() == TXT("variableWidth"))
		{
			topMargin = childNode->get_int_attribute(TXT("verticalMargin"), topMargin);
			bottomMargin = childNode->get_int_attribute(TXT("verticalMargin"), bottomMargin);
			topMargin = childNode->get_int_attribute(TXT("topMargin"), topMargin);
			bottomMargin = childNode->get_int_attribute(TXT("bottomMargin"), bottomMargin);

			baseLinePt = childNode->get_float_attribute(TXT("baseLinePt"), baseLinePt);

			offsetY = childNode->get_float_attribute(TXT("offsetY"), offsetY);
		}
	}
	return result;
}

void Font::set_fallback_shader_texture(Texture const * _texture) const
{
	fallbackShaderProgramInstance->set_uniform(NAME(inTexture), _texture ? _texture->get_texture_id() : texture_id_none());
	fallbackNoBlendShaderProgramInstance->set_uniform(NAME(inTexture), _texture ? _texture->get_texture_id() : texture_id_none());
}

void Font::use_textures_and_prepare(Array<TextureDefinition> const & _textureDefinitions, String const& _fixedWidthCharacters, String const& _colourCharacters, std::function<Texture* (int _textureDefinitionIdx, int _subTextureIdx)> get_texture_for_texture_definition)
{
	textureDefinitions.clear();
	ansiCharacterDefinitions.clear();
	unicodeCharacterDefinitions.clear();

	if (fontType == FontType::FixedSize ||
		fontType == FontType::VariableWidth)
	{
		Vector2 const fixedFontSizeAsVector2 = fixedFontSize.to_vector2();
		Vector2 const fixedFontSpacingAsVector2 = fixedFontSpacing.to_vector2();

		ansiCharacterDefinitions.clear();
		unicodeCharacterDefinitions.clear();

		ansiCharacterDefinitions.set_size(1);
		{	// space is first char
			CharacterDefinition chd(NONE, ' ', false, Vector2::zero, fixedFontSizeAsVector2, Vector2::zero, fixedFontSizeAsVector2, fixedFontSpacingAsVector2, fixedFontSpacingAsVector2);
			ansiCharacterDefinitions[0] = chd;
		}

		int textureIdx = 0;
		for_every(sourceTextureDefinition, _textureDefinitions)
		{
			VectorInt2 gridCellSize(fixedFontSize.x + fixedFontSpacing.x * 2, fixedFontSize.y + fixedFontSpacing.y * 2);
			VectorInt2 textureSize;
			VectorInt2 gridLBAt;
			VectorInt2 lbAt;
			bool skippedToNextLineDueToLimitExceeded = false;

			int sourceTextureIdx = 0;
			Texture const* sourceTexture = nullptr;
			TextureDefinition* destTextureDefinition = nullptr;

			for_every(ch, sourceTextureDefinition->characters.get_data())
			{
				if (*ch == 0)
				{
					break;
				}
				if (!sourceTexture)
				{
					if (sourceTextureIdx == 0)
					{
						sourceTexture = sourceTextureDefinition->texture;
					}
					if (!sourceTexture)
					{
						sourceTexture = get_texture_for_texture_definition(for_everys_index(sourceTextureDefinition), sourceTextureIdx);
					}
					if (!sourceTexture)
					{
						error(TXT("no texture data for font"));
						break;
					}
					textureSize = sourceTexture->get_size();
					gridLBAt = VectorInt2(0, textureSize.y - gridCellSize.y);
					lbAt = VectorInt2(fixedFontSpacing.x, textureSize.y - gridCellSize.y + fixedFontSpacing.y);
					
					// for a different sourceTexture we need a different destTextureDefinition
					textureDefinitions.grow_size(1);
					destTextureDefinition = &textureDefinitions.get_last();
					destTextureDefinition->texture = sourceTexture;
					textureIdx = textureDefinitions.get_size() - 1;
				}
				an_assert(destTextureDefinition);
				bool skipToNextLine = false;
				if (*ch == '\t' ||
					*ch == '\r')
				{
					// skip special characters
					continue; 
				}
#ifdef AN_DEVELOPMENT
				// if not in development, won't be visible anyway
				destTextureDefinition->characters += *ch;
#endif
				if (*ch == '\n')
				{
					skipToNextLine = ! skippedToNextLineDueToLimitExceeded;
					skippedToNextLineDueToLimitExceeded = false;
				}
				else
				{
					CharacterDefinition chd(textureIdx, *ch, _colourCharacters.does_contain(*ch), gridLBAt.to_vector2(), gridCellSize.to_vector2(), lbAt.to_vector2(), fixedFontSizeAsVector2, fixedFontSpacingAsVector2, fixedFontSpacingAsVector2);
					if (CharacterDefinition* existing = access_character_definition(*ch))
					{
						*existing = chd;
					}
					else
					{
						if (*ch < 128)
						{
							int chOffset = *ch - 32;
							ansiCharacterDefinitions.set_size(max(ansiCharacterDefinitions.get_size(), chOffset + 1));
							ansiCharacterDefinitions[chOffset] = chd;
						}
						else
						{
							unicodeCharacterDefinitions.push_back(chd);
						}
					}
					lbAt.x += gridCellSize.x;
					gridLBAt.x += gridCellSize.x;
				}
				if (lbAt.x >= textureSize.x || skipToNextLine)
				{
					skippedToNextLineDueToLimitExceeded = !skipToNextLine;
					lbAt.x = fixedFontSpacing.x;
					lbAt.y -= gridCellSize.y;
					gridLBAt.x = 0;
					gridLBAt.y -= gridCellSize.y;
					if (lbAt.y < fixedFontSpacing.y)
					{
						++sourceTextureIdx;
						sourceTexture = nullptr;
					}
				}
			}
		}
	}
	else
	{
		todo_important(TXT("other font types!"));
	}

	// update using texture data
	if (fontType == FontType::VariableWidth)
	{
		RangeInt filledRange = RangeInt::empty;

		if (!_fixedWidthCharacters.is_empty())
		{
			for_every(characterDefinition, ansiCharacterDefinitions)
			{
				get_fixed_character_range(*characterDefinition, OUT_ filledRange, _fixedWidthCharacters);
			}
			for_every(characterDefinition, unicodeCharacterDefinitions)
			{
				get_fixed_character_range(*characterDefinition, OUT_ filledRange, _fixedWidthCharacters);
			}
		}

		{
			for_every(characterDefinition, ansiCharacterDefinitions)
			{
				calculate_variable_placement(*characterDefinition, filledRange, _fixedWidthCharacters);
			}
			for_every(characterDefinition, unicodeCharacterDefinitions)
			{
				calculate_variable_placement(*characterDefinition, filledRange, _fixedWidthCharacters);
			}
		}
	}

	// update character size
	charSize = Vector2::zero;
	for_every(characterDefinition, ansiCharacterDefinitions)
	{
		characterDefinition->recalculate();
		charSize.x = max(charSize.x, characterDefinition->size.x);
		charSize.y = max(charSize.y, characterDefinition->size.y);
	}
	if (ansiCharacterDefinitions.get_size() > 0 && !ansiCharacterDefinitions[0].size.is_zero())
	{
		// use space for char size
		charSize = ansiCharacterDefinitions[0].size;
	}
	// do not depend on unicode characters, just use base
	for_every(characterDefinition, unicodeCharacterDefinitions)
	{
		characterDefinition->recalculate();
	}
}

void Font::get_fixed_character_range(CharacterDefinition& _chd, OUT_ RangeInt & _fixedCharactersRange, String const & _fixedWidthCharacters)
{
	if (_fixedWidthCharacters.get_data().does_contain(_chd.ch))
	{
		_chd.leftBottom = _chd.wholeLeftBottom;
		_chd.size = _chd.wholeSize;

		_chd.leftBottom.y -= (float)bottomMargin;
		_chd.size.y += (float)(bottomMargin + topMargin);

		if (_chd.textureIdx != NONE)
		{
			auto& textureDefinition = textureDefinitions[_chd.textureIdx];
			auto* texture = textureDefinition.texture;

			if (auto* ts = texture->get_setup())
			{
				//measure width
				{
					RangeInt filledRange = RangeInt::empty;
					int leftMost = TypeConversions::Normal::f_i_closest(_chd.leftBottom.x);
					int rightMost = TypeConversions::Normal::f_i_closest(_chd.leftBottom.x + _chd.size.x - 1);
					int bottom = TypeConversions::Normal::f_i_closest(_chd.leftBottom.y);
					int top = TypeConversions::Normal::f_i_closest(_chd.leftBottom.y + _chd.size.y - 1);
					for (int x = leftMost; x <= rightMost; ++x)
					{
						bool filled = false;
						for (int y = bottom; y <= top; ++y)
						{
							if (ts->get_pixel(0, VectorInt2(x, y)).a > 0.0f)
							{
								filled = true;
							}
						}
						if (filled)
						{
							filledRange.include(x);
						}
					}

					if (!filledRange.is_empty())
					{
						int x = TypeConversions::Normal::f_i_closest(_chd.wholeLeftBottom.x);
						filledRange.min -= x;
						filledRange.max -= x;
						_fixedCharactersRange.include(filledRange);
					}
				}
				// make characters as tall as needed
				{
					RangeInt filledRange = RangeInt::empty;
					int leftMost = TypeConversions::Normal::f_i_closest(_chd.leftBottom.x);
					int rightMost = TypeConversions::Normal::f_i_closest(_chd.leftBottom.x + _chd.size.x - 1);
					int bottom = TypeConversions::Normal::f_i_closest(_chd.leftBottom.y);
					int top = TypeConversions::Normal::f_i_closest(_chd.leftBottom.y + _chd.size.y - 1);
					for (int y = bottom; y <= top; ++y)
					{
						bool filled = false;
						for (int x = leftMost; x <= rightMost; ++x)
						{
							if (ts->get_pixel(0, VectorInt2(x, y)).a > 0.0f)
							{
								filled = true;
							}
						}
						if (filled)
						{
							filledRange.include(y);
						}
					}

					if (!filledRange.is_empty())
					{
						filledRange.include(TypeConversions::Normal::f_i_closest(_chd.wholeLeftBottom.y));
						filledRange.include(TypeConversions::Normal::f_i_closest(_chd.wholeLeftBottom.y + _chd.wholeSize.y) - 1);
						_chd.leftBottom.y = (float)filledRange.min;
						_chd.size.y = (float)(filledRange.max - filledRange.min + 1);
					}
					else
					{
						_chd.leftBottom.y = _chd.wholeLeftBottom.y;
						_chd.size.y = _chd.wholeSize.y;
					}
				}
			}
		}
	}
}

void Font::calculate_variable_placement(CharacterDefinition& _chd, RangeInt const & _fixedCharactersRange, String const& _fixedWidthCharacters)
{
	_chd.leftBottom = _chd.wholeLeftBottom;
	_chd.size = _chd.wholeSize;

	_chd.leftBottom.y -= (float)bottomMargin;
	_chd.size.y += (float)(bottomMargin + topMargin);

	if (_chd.textureIdx != NONE)
	{
		auto& textureDefinition = textureDefinitions[_chd.textureIdx];
		auto* texture = textureDefinition.texture;

		if (auto* ts = texture->get_setup())
		{
			// measure width
			{
				RangeInt filledRange = RangeInt::empty;

				if (_fixedWidthCharacters.get_data().does_contain(_chd.ch))
				{
					int x = TypeConversions::Normal::f_i_closest(_chd.wholeLeftBottom.x);
					filledRange.min = _fixedCharactersRange.min + x;
					filledRange.max = _fixedCharactersRange.max + x;
				}
				else
				{
					// scan whole grid cell
					int leftMost = TypeConversions::Normal::f_i_closest(min(_chd.leftBottom.x, _chd.gridLeftBottom.x + 1));
					int rightMost = TypeConversions::Normal::f_i_closest(max(_chd.leftBottom.x + _chd.size.x - 1, _chd.gridLeftBottom.x + _chd.gridSize.x - 1 - 1));
					int bottom = TypeConversions::Normal::f_i_closest(_chd.leftBottom.y);
					int top = TypeConversions::Normal::f_i_closest(_chd.leftBottom.y + _chd.size.y - 1);
					for (int x = leftMost; x <= rightMost; ++x)
					{
						bool filled = false;
						for (int y = bottom; y <= top; ++y)
						{
							float a = ts->get_pixel(0, VectorInt2(x, y)).a;
							if (a > 0.0f)
							{
								filled = true;
							}
						}
						if (filled)
						{
							filledRange.include(x);
						}
					}
				}

				if (!filledRange.is_empty())
				{
					_chd.leftBottom.x = (float)filledRange.min - variableWidthLeftMargin;
					_chd.size.x = (float)(filledRange.max - filledRange.min + 1) + variableWidthLeftMargin + variableWidthRightMargin;
				}
			}
			// make characters as tall as needed
			{
				RangeInt filledRange = RangeInt::empty;
				int leftMost = TypeConversions::Normal::f_i_closest(_chd.leftBottom.x);
				int rightMost = TypeConversions::Normal::f_i_closest(_chd.leftBottom.x + _chd.size.x - 1);
				int bottom = TypeConversions::Normal::f_i_closest(_chd.leftBottom.y);
				int top = TypeConversions::Normal::f_i_closest(_chd.leftBottom.y + _chd.size.y - 1);
				for (int y = bottom; y <= top; ++y)
				{
					bool filled = false;
					for (int x = leftMost; x <= rightMost; ++x)
					{
						if (ts->get_pixel(0, VectorInt2(x, y)).a > 0.0f)
						{
							filled = true;
						}
					}
					if (filled)
					{
						filledRange.include(y);
					}
				}

				if (!filledRange.is_empty())
				{
					filledRange.include(TypeConversions::Normal::f_i_closest(_chd.wholeLeftBottom.y));
					filledRange.include(TypeConversions::Normal::f_i_closest(_chd.wholeLeftBottom.y + _chd.wholeSize.y) - 1);
					_chd.leftBottom.y = (float)filledRange.min;
					_chd.size.y = (float)(filledRange.max - filledRange.min + 1);
				}
				else
				{
					_chd.leftBottom.y = _chd.wholeLeftBottom.y;
					_chd.size.y = _chd.wholeSize.y;
				}
			}
		}
	}
}

int Font::get_width(tchar _ch, bool _forceFixedSize) const
{
	if (_ch < 128)
	{
		_ch -= 32;
		if (ansiCharacterDefinitions.is_index_valid(_ch))
		{
			float sizeX = 0.0f;
			if (_ch == 0) // space
			{
				sizeX = _forceFixedSize ? ansiCharacterDefinitions[_ch].wholeSize.x : round(ansiCharacterDefinitions[_ch].size.x * s_variableSpaceWidthPt.get(1.0f));
			}
			else
			{
				sizeX = _forceFixedSize ? ansiCharacterDefinitions[_ch].wholeSize.x : ansiCharacterDefinitions[_ch].size.x;
			}
			return TypeConversions::Normal::f_i_closest(sizeX);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		for_every(ucd, unicodeCharacterDefinitions)
		{
			if (ucd->ch == _ch)
			{
				return TypeConversions::Normal::f_i_closest(_forceFixedSize? ucd->wholeSize.x : ucd->size.x);
			}
		}
		return 0;
	}
}

CharacterDefinition const * Font::get_character_definition(int _ch) const
{
	if (_ch < 128)
	{
		_ch -= 32;
		if (ansiCharacterDefinitions.is_index_valid(_ch))
		{
			return &ansiCharacterDefinitions[_ch];
		}
		else
		{
			return nullptr;
		}
	}
	else
	{
		for_every(ucd, unicodeCharacterDefinitions)
		{
			if (ucd->ch == _ch)
			{
				return ucd;
			}
		}
		return nullptr;
	}
}

CharacterDefinition * Font::access_character_definition(int _ch)
{
	if (_ch < 128)
	{
		_ch -= 32;
		if (ansiCharacterDefinitions.is_index_valid(_ch))
		{
			return &ansiCharacterDefinitions[_ch];
		}
		else
		{
			return nullptr;
		}
	}
	else
	{
		for_every(ucd, unicodeCharacterDefinitions)
		{
			if (ucd->ch == _ch)
			{
				return ucd;
			}
		}
		return nullptr;
	}
}

Vector2 Font::calculate_text_size(String const& _text, Optional<Vector2> const& _scale, Optional<FontDrawingContext> const& _drawingContext) const
{
	return calculate_text_size(_text.to_char(), _scale, _drawingContext);
}

Vector2 Font::calculate_text_size(tchar const * _text, Optional<Vector2> const& _scale, Optional<FontDrawingContext> const& _drawingContext) const
{
	Vector2 scale = _scale.get(Vector2::one);
	bool forceFixedSize = _drawingContext.is_set()? _drawingContext.get().forceFixedSize.get(false) : false;
	Vector2 size(0.0f, get_height() * scale.y);
	bool smallerDecimals = false;
	for (tchar const * character = _text; *character != 0; ++character)
	{
		if (_drawingContext.is_set() && _drawingContext.get().smallerDecimals.is_set())
		{
			if (*character == '.' &&
				*(character + 1) >= '0' &&
				*(character + 1) <= '9')
			{
				smallerDecimals = true;
				continue; // skip dot
			}
			if (!(*(character) >= '0' && *(character) <= '9'))
			{
				smallerDecimals = false;
			}
		}

		Vector2 charSize = Vector2::zero;
		if (CharacterDefinition const * charDef = get_character_definition((int)*character))
		{
			if (*character == ' ')
			{
				charSize = (forceFixedSize ? charDef->wholeSize : round((charDef->size * s_variableSpaceWidthPt.get(1.0f)))) * scale;
			}
			else
			{
				charSize = (forceFixedSize? charDef->wholeSize : charDef->size) * scale;
			}
		}
		float characterScale = 1.0f;
		if (smallerDecimals)
		{
			characterScale *= _drawingContext.get().smallerDecimals.get(0.5f);
		}
		size.x += charSize.x * characterScale;
	}
	return size;
}

struct FontVertex
{
	Vector3 location;
	Colour colour;
	Vector2 uv;

	FontVertex() {}
	FontVertex(Vector2 const & _uv, Vector3 const & _loc, Colour const & _colour) : location(_loc), colour(_colour), uv(_uv) {}
};

struct FontVertexNoColour
{
	Vector3 location;
	Vector2 uv;

	FontVertexNoColour() {}
	FontVertexNoColour(Vector2 const & _uv, Vector3 const & _loc) : location(_loc), uv(_uv) {}
	FontVertexNoColour(FontVertex const& fv) : location(fv.location), uv(fv.uv) {}
};

void Font::draw_text_at_with_border(::System::Video3D* _v3d, String const & _text, Colour const & _colour, Colour const & _borderColour, float _border, Vector2 const & _leftBottom, Vector2 const & _scale, Vector2 const & _anchorRelPlacement, Optional<bool> _alignToPixels, FontDrawingContext const& _drawingContext) const
{
	if (! ::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
	{
		Vector2 at;
		at = _leftBottom + Vector2(-_border, -_border);	draw_text_at(_v3d, _text, _borderColour, at, _scale, _anchorRelPlacement, _alignToPixels, _drawingContext);
		at = _leftBottom + Vector2(_border, -_border);	draw_text_at(_v3d, _text, _borderColour, at, _scale, _anchorRelPlacement, _alignToPixels, _drawingContext);
		at = _leftBottom + Vector2(-_border, 0.0f);		draw_text_at(_v3d, _text, _borderColour, at, _scale, _anchorRelPlacement, _alignToPixels, _drawingContext);
		at = _leftBottom + Vector2(0.0f, -_border);		draw_text_at(_v3d, _text, _borderColour, at, _scale, _anchorRelPlacement, _alignToPixels, _drawingContext);
		at = _leftBottom + Vector2(_border, 0.0f);		draw_text_at(_v3d, _text, _borderColour, at, _scale, _anchorRelPlacement, _alignToPixels, _drawingContext);
		at = _leftBottom + Vector2(-_border, _border);	draw_text_at(_v3d, _text, _borderColour, at, _scale, _anchorRelPlacement, _alignToPixels, _drawingContext);
		at = _leftBottom + Vector2(0.0f, _border);		draw_text_at(_v3d, _text, _borderColour, at, _scale, _anchorRelPlacement, _alignToPixels, _drawingContext);
		at = _leftBottom + Vector2(_border, _border);	draw_text_at(_v3d, _text, _borderColour, at, _scale, _anchorRelPlacement, _alignToPixels, _drawingContext);
	}
	// text itself
	draw_text_at(_v3d, _text, _colour, _leftBottom, _scale, _anchorRelPlacement, _alignToPixels, _drawingContext);
}

void Font::draw_text_at(Video3D* _v3d, String const & _text, Colour const & _colour, Vector2 const & _leftBottom, Vector2 const & _scale, Vector2 const & _anchorRelPlacement, Optional<bool> _alignToPixels, FontDrawingContext const& _drawingContext) const
{
	draw_text_at(_v3d, _text.get_data().get_data(), _colour, _leftBottom, _scale, _anchorRelPlacement, _alignToPixels, _drawingContext);
}

void Font::draw_text_at(Video3D* _v3d, tchar const * _text, Colour const & _colour, Vector2 const & _leftBottom, Vector2 const & _scale, Vector2 const & _anchorRelPlacement, Optional<bool> _alignToPixels, FontDrawingContext const& _drawingContext) const
{
	draw_text_at(_v3d, _text, _colour, _v3d->is_for_3d()? _leftBottom.to_vector3_xz() : _leftBottom.to_vector3(), _scale, _anchorRelPlacement, _alignToPixels, _drawingContext);
}

void Font::draw_text_at(Video3D* _v3d, tchar const* _text, Colour const& _colour, Vector3 const& _leftBottom, Vector2 const& _scale, Vector2 const& _anchorRelPlacement, Optional<bool> _alignToPixels, FontDrawingContext const& _drawingContext) const
{
	draw_text_or_construct_internal(nullptr, nullptr, _v3d, _text, NP, _colour, _leftBottom, _scale, _anchorRelPlacement, _alignToPixels, _drawingContext);
}

void Font::construct_mesh(Meshes::Mesh3D* _mesh, Array<TextureID>& _usingTextures, String const& _text, Optional<bool> const& _for3d, float const& _letterHeight, Vector2 const& _anchorRelPlacement) const
{
	float scale = _letterHeight / get_height();
	draw_text_or_construct_internal(_mesh, &_usingTextures, nullptr, _text.to_char(), _for3d, Colour::none /* to allow using fallback colour */, Vector3::zero, Vector2::one * scale, _anchorRelPlacement, false);
}

void Font::draw_text_or_construct_internal(Meshes::Mesh3D* _drawIntoMesh, OUT_ OPTIONAL_ Array<TextureID>* _usingTextures, Video3D* _v3d, tchar const* _text, Optional<bool> const& _for3d, Colour const& _colour, Vector3 const& _leftBottom, Vector2 const& _scale, Vector2 const& _anchorRelPlacement, Optional<bool> _alignToPixels, FontDrawingContext const& _drawingContext) const
{
	if (_drawIntoMesh)
	{
		_v3d = nullptr;
	}

	if (textureDefinitions.is_empty())
	{
		return;
	}

	bool isFor3D = _for3d.get(!_v3d || _v3d->is_for_3d());

	assert_rendering_thread();

	bool alignToPixelsNow = alignToPixels;
	if (_alignToPixels.is_set())
	{
		alignToPixelsNow = _alignToPixels.get();
	}

	Vector3 startingAt = alignToPixelsNow ? _leftBottom.to_vector_int3().to_vector3() : _leftBottom;

	if (!_anchorRelPlacement.is_zero())
	{
		if (isFor3D)
		{
			startingAt -= (calculate_text_size(_text, _scale, _drawingContext) * _anchorRelPlacement).to_vector3_xz();
		}
		else
		{
			startingAt -= (calculate_text_size(_text, _scale, _drawingContext) * _anchorRelPlacement).to_vector3();
		}
	}

	if (_v3d)
	{
		_v3d->set_fallback_colour(_colour);
	}

	bool isBlending = _drawingContext.blend.get(true);
	if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
	{
		isBlending = _drawingContext.blend.get(false);
	}
	if (_v3d && isBlending)
	{
		_v3d->mark_enable_blend();
	}

	ARRAY_STACK(FontVertex, vertices, (int)tstrlen(_text) * 6);
	ARRAY_STACK(FontVertexNoColour, verticesNoColour, (int)tstrlen(_text) * 6);

	ARRAY_STACK(bool, tryTextures, textureDefinitions.get_size());
	tryTextures.set_size(textureDefinitions.get_size());
	for_every(tryTexture, tryTextures)
	{
		*tryTexture = false;
	}
	bool firstOne = true; // first one being processed

	bool forceFixedSize = _drawingContext.forceFixedSize.get(false);

	for_every(textureDefinition, textureDefinitions)
	{
		int const textureIdx = for_everys_index(textureDefinition);
		if (!textureDefinition->texture)
		{
			// skip it, we won't be able to render anyway
			continue;
		}

		if (! firstOne && !tryTextures[textureIdx])
		{
			// skip this one, there's nothing for us here
			continue;
		}

		Texture const * texture = textureDefinition->texture;

		Vector2 const textureSize = texture->get_size().to_vector2();
		Vector2 const texelSize = Vector2::one / textureSize;

		vertices.clear();
		verticesNoColour.clear();
		Vector2 at = isFor3D? Vector2(startingAt.x, startingAt.z) : Vector2(startingAt.x, startingAt.y);
		bool smallerDecimals = false;
		for (tchar const * character = _text; *character != 0; ++character)
		{
			if (_drawingContext.smallerDecimals.is_set())
			{
				if (*character == '.' &&
					*(character + 1) >= '0' &&
					*(character + 1) <= '9')
				{
					smallerDecimals = true;
					continue; // skip dot
				}
				if (! (*(character) >= '0' && *(character) <= '9'))
				{
					smallerDecimals = false;
				}
			}
			Vector2 charSize = Vector2::zero;
			if (CharacterDefinition const * charDef = get_character_definition((int)*character))
			{
				Vector2 useSize;
				if (*character == ' ')
				{
					useSize = forceFixedSize ? charDef->wholeSize : round(charDef->size * s_variableSpaceWidthPt.get(1.0f));
				}
				else
				{
					useSize = forceFixedSize ? charDef->wholeSize : charDef->size;
				}
				float characterScale = 1.0f;
				if (smallerDecimals)
				{
					characterScale *= _drawingContext.smallerDecimals.get(0.5f);
				}
				Vector2 useLeftBottom = forceFixedSize ? charDef->wholeLeftBottom : charDef->leftBottom;
				useLeftBottom.y = min(charDef->wholeLeftBottom.y, charDef->leftBottom.y);
				charSize = useSize * _scale * characterScale;
				charSize.y = max(charDef->wholeLeftBottom.y + charDef->wholeSize.y, charDef->leftBottom.y + charDef->size.y) - useLeftBottom.y;
				if (charDef->textureIdx != NONE)
				{
					if (charDef->textureIdx == textureIdx)
					{
						Vector2 const charEnd = useSize * _scale * characterScale;
						Vector2 const uv0 = useLeftBottom * texelSize;
						Vector2 const uv1 = uv0 + useSize * texelSize;

						Vector2 lb = at;
						lb.y += (charSize.y * _scale.y) * baseLinePt * (1.0f - characterScale);
						lb.y += (charDef->wholeLeftBottom.y - useLeftBottom.y) * _scale.y * characterScale;
						lb.y += offsetY * _scale.y;
						Range2 chAt(Range(lb.x, lb.x + charEnd.x), Range(lb.y, lb.y + charEnd.y));

						Colour chColour = charDef->colourCharacter? Colour::white.mul_rgb(max(_colour.r, max(_colour.g, _colour.b))).with_alpha(_colour.a) : _colour;
						if (isFor3D)
						{
							vertices.push_back(FontVertex(Vector2(uv0.x, uv0.y), Vector3(chAt.x.min, startingAt.y, chAt.y.min), chColour));
							vertices.push_back(FontVertex(Vector2(uv0.x, uv1.y), Vector3(chAt.x.min, startingAt.y, chAt.y.max), chColour));
							vertices.push_back(FontVertex(Vector2(uv1.x, uv1.y), Vector3(chAt.x.max, startingAt.y, chAt.y.max), chColour));
							vertices.push_back(FontVertex(Vector2(uv1.x, uv1.y), Vector3(chAt.x.max, startingAt.y, chAt.y.max), chColour)); // same as previous
							vertices.push_back(FontVertex(Vector2(uv1.x, uv0.y), Vector3(chAt.x.max, startingAt.y, chAt.y.min), chColour));
							vertices.push_back(FontVertex(Vector2(uv0.x, uv0.y), Vector3(chAt.x.min, startingAt.y, chAt.y.min), chColour)); // same as first
						}
						else
						{
							vertices.push_back(FontVertex(Vector2(uv0.x, uv0.y), Vector3(chAt.x.min, chAt.y.min, 0.0f), chColour));
							vertices.push_back(FontVertex(Vector2(uv0.x, uv1.y), Vector3(chAt.x.min, chAt.y.max, 0.0f), chColour));
							vertices.push_back(FontVertex(Vector2(uv1.x, uv1.y), Vector3(chAt.x.max, chAt.y.max, 0.0f), chColour));
							vertices.push_back(FontVertex(Vector2(uv1.x, uv1.y), Vector3(chAt.x.max, chAt.y.max, 0.0f), chColour)); // same as previous
							vertices.push_back(FontVertex(Vector2(uv1.x, uv0.y), Vector3(chAt.x.max, chAt.y.min, 0.0f), chColour));
							vertices.push_back(FontVertex(Vector2(uv0.x, uv0.y), Vector3(chAt.x.min, chAt.y.min, 0.0f), chColour)); // same as first
						}
					}
					else if (firstOne)
					{
						// mark that we will be interested in that texture
						tryTextures[charDef->textureIdx] = true;
					}
				}
			}
			at.x += charSize.x;
		}

		if (!vertices.is_empty())
		{
			void* verticesData = vertices.get_data();
			int verticesCount = vertices.get_size();
			auto const* useVertexFormat = &vertexFormat;
			if (_drawIntoMesh && _colour.is_none())
			{
				for_every(v, vertices)
				{
					verticesNoColour.push_back(*v);
				}
				verticesData = verticesNoColour.get_data();
				verticesCount = verticesNoColour.get_size();
				useVertexFormat = &vertexFormatNoCharacterColour;
			}
			if (_drawIntoMesh)
			{
				_drawIntoMesh->load_part_data(verticesData, Meshes::Primitive::Triangle, verticesCount / 3, *useVertexFormat);
				an_assert(_usingTextures, TXT("should be called with \"_usingTextures\" when constructing a mesh"));
				_usingTextures->push_back(texture->get_texture_id());
			}
			if (_v3d)
			{
				if (_drawingContext.shaderProgramInstance)
				{
					int textureIdx = _drawingContext.shaderProgramInstance->get_shader_program()->get_in_texture_uniform_index();
					if (textureIdx != NONE)
					{
						_drawingContext.shaderProgramInstance->set_uniform(textureIdx, texture->get_texture_id());
					}
				}
				else
				{
					set_fallback_shader_texture(texture);
				}
				_v3d->set_fallback_texture(texture->get_texture_id());
			
				// keep existing blend
				Meshes::Mesh3DRenderingContext renderingContext = _drawingContext.renderingContext;
				renderingContext.with_existing_blend();
			
				Meshes::Mesh3D::render_data(_v3d,
					_drawingContext.shaderProgramInstance ? _drawingContext.shaderProgramInstance : (isBlending? fallbackShaderProgramInstance : fallbackNoBlendShaderProgramInstance),
					nullptr, renderingContext, verticesData, ::Meshes::Primitive::Triangle, verticesCount / 3, *useVertexFormat);
			}
		}

		firstOne = false;
	}

	if (_v3d)
	{
		if (_drawingContext.blend.get(true))
		{
			_v3d->mark_disable_blend();
		}

		_v3d->set_fallback_colour();
		_v3d->set_fallback_texture();
	}
}

Vector2 Font::calculate_char_size() const
{
	if (fontType == FontType::FixedSize ||
		fontType == FontType::VariableWidth)
	{
		return Vector2(charSize.x, (float)fixedFontSize.y);
	}
	else
	{
		return calculate_text_size(TXT(" "));
	}
}

float Font::get_height() const
{
	if (fontType == FontType::FixedSize ||
		fontType == FontType::VariableWidth)
	{
		an_assert(!fixedFontSize.is_zero());
		return (float)fixedFontSize.y;
	}
	else
	{
		todo_important(TXT("implement_"));
	}
	return 0.0f;
}

void Font::debug_draw() const
{
}

bool Font::preview_render() const
{
	System::Video3D* v3d = System::Video3D::get();
	Vector2 rtSize = v3d->get_screen_size().to_vector2();
	System::RenderTarget::bind_none();
	v3d->set_viewport(VectorInt2::zero, rtSize.to_vector_int2());
	v3d->setup_for_2d_display();
	v3d->set_2d_projection_matrix_ortho(rtSize);
	v3d->access_model_view_matrix_stack().clear();

	struct Draw
	{
		::System::VertexFormat const * vertexFormat;
		ShaderProgramInstance* shaderInstance;
		void rect(System::Texture const * _texture, Range2 const& _at, Range2 const& _uv)
		{
			System::Video3D* v3d = System::Video3D::get();

			ARRAY_STACK(FontVertex, vertices, (int)6);

			vertices.clear();
			vertices.push_back(FontVertex(Vector2(_uv.x.min, _uv.y.min), Vector3(_at.x.min, _at.y.min, 0.0f), Colour::white));
			vertices.push_back(FontVertex(Vector2(_uv.x.min, _uv.y.max), Vector3(_at.x.min, _at.y.max, 0.0f), Colour::white));
			vertices.push_back(FontVertex(Vector2(_uv.x.max, _uv.y.max), Vector3(_at.x.max, _at.y.max, 0.0f), Colour::white));
			vertices.push_back(FontVertex(Vector2(_uv.x.max, _uv.y.max), Vector3(_at.x.max, _at.y.max, 0.0f), Colour::white)); // same as previous
			vertices.push_back(FontVertex(Vector2(_uv.x.max, _uv.y.min), Vector3(_at.x.max, _at.y.min, 0.0f), Colour::white));
			vertices.push_back(FontVertex(Vector2(_uv.x.min, _uv.y.min), Vector3(_at.x.min, _at.y.min, 0.0f), Colour::white)); // same as first

			if (!vertices.is_empty())
			{
				void* verticesData = vertices.get_data();
				int verticesCount = vertices.get_size();
				if (v3d)
				{
					shaderInstance->set_uniform(NAME(inTexture), _texture ? _texture->get_texture_id() : texture_id_none());

					v3d->set_fallback_colour(Colour::white);
					v3d->set_fallback_texture(_texture->get_texture_id());

					Meshes::Mesh3DRenderingContext renderingContext = Meshes::Mesh3DRenderingContext::none();
					renderingContext.with_existing_blend();

					Meshes::Mesh3D::render_data(v3d,
						shaderInstance,
						nullptr, renderingContext, verticesData, ::Meshes::Primitive::Triangle, verticesCount / 3, *vertexFormat);

					v3d->set_fallback_colour();
					v3d->set_fallback_texture();
				}
			}
		}
	} draw;

	bool isBlending = true;
	draw.vertexFormat = &vertexFormat;
	draw.shaderInstance = (isBlending ? fallbackShaderProgramInstance : fallbackNoBlendShaderProgramInstance);

	float topY = rtSize.y - 1.0f;
	float scale = 2.0f;

	for_every(td, textureDefinitions)
	{
		auto* texture = td->texture;

		Range2 wholeTextureAt = Range2(Range(0.0f, texture->get_size().x * scale), Range(topY - texture->get_size().y * scale, topY));
		Range2 wholeUV = Range2(Range(0.0f, 1.0f), Range(0.0f, 1.0f));

		v3d->set_fallback_colour(Colour::white);
		v3d->mark_disable_blend();
		System::Video3DPrimitives::fill_rect_2d(Colour::lerp(0.5f,Colour::grey, Colour::mint), wholeTextureAt);

		if (isBlending)
		{
			v3d->mark_enable_blend();
		}
		else
		{
			v3d->mark_disable_blend();
		}

		draw.rect(texture, wholeTextureAt, wholeUV);

		v3d->set_fallback_colour(Colour::white);
		v3d->mark_disable_blend();

		System::Video3DPrimitives::rect_2d(Colour::red, wholeTextureAt);

		Vector2 const textureSize = texture->get_size().to_vector2();
		Vector2 const texelSize = Vector2::one / textureSize;

		for_count(int, chIdx, 2)
		{
			auto& cds = chIdx == 0 ? ansiCharacterDefinitions : unicodeCharacterDefinitions;

			for_every(cd, cds)
			{
				if (cd->textureIdx == for_everys_index(td))
				{
					struct Char
					{
						static void draw(Range2 const& wholeTextureAt, Vector2 const& _ptLB, Vector2 const _ptRT, Colour const & _colour)
						{
							Range2 charAt = Range2::empty;
							charAt.include(wholeTextureAt.get_at(_ptLB));
							charAt.include(wholeTextureAt.get_at(_ptRT));

							// to show its contents fully
							charAt.x.min -= 1.0f;
							charAt.y.min -= 1.0f;

							System::Video3DPrimitives::rect_2d(_colour, charAt);
						}
					};
					{
						Vector2 charPtLB = (cd->gridLeftBottom) * texelSize;
						Vector2 charPtRT = (cd->gridLeftBottom + cd->gridSize) * texelSize;
						Char::draw(wholeTextureAt, charPtLB, charPtRT, Colour::blue);
					}
					{
						Vector2 charPtLB = (cd->wholeLeftBottom) * texelSize;
						Vector2 charPtRT = (cd->wholeLeftBottom + cd->wholeSize) * texelSize;
						Char::draw(wholeTextureAt, charPtLB, charPtRT, Colour::red);
					}
					{
						Vector2 charPtLB = (cd->leftBottom) * texelSize;
						Vector2 charPtRT = (cd->leftBottom + cd->size) * texelSize;
						Char::draw(wholeTextureAt, charPtLB, charPtRT, Colour::green);
					}
					{
						Vector2 charPtLB = (cd->gridLeftBottom) * texelSize;
						Vector2 charPtRT = (cd->gridLeftBottom + cd->gridSize) * texelSize;
						charPtLB.y = (cd->leftBottom.y + cd->size.y * baseLinePt) * texelSize.y;
						charPtRT.y = charPtLB.y;

						Vector2 charLB = wholeTextureAt.get_at(charPtLB);
						Vector2 charRT = wholeTextureAt.get_at(charPtRT);
						System::Video3DPrimitives::line_2d(Colour::cyan, charLB, charRT);
					}
				}
			}
		}

		topY -= texture->get_size().y * scale;
	}

	return true;
}
