#pragma once

#include "texture.h"
#include "vertexFormat.h"

#include "..\..\mesh\mesh3dRenderingContext.h"

namespace Meshes
{
	class Mesh3D;
};

namespace System
{
	class Video3D;
	struct ShaderProgramInstance;

	namespace FontType
	{
		enum Type
		{
			FixedSize,
			VariableWidth
		};
	};

	struct CharacterDefinition
	{
		int textureIdx = NONE;
		tchar ch = NONE;
		bool colourCharacter = false; // the rgb won't be affected - will draw in full colour clamped to max value of provided colour
		Vector2 leftBottom;
		Vector2 size;
		Vector2 wholeLeftBottom; // whole place, this is bigger than leftBottom
		Vector2 wholeSize; // as above
		Vector2 gridLeftBottom; // total grid cell
		Vector2 gridSize; // total grid cell
		Vector2 leftBottomSpacing;
		Vector2 rightTopSpacing;

		CACHED_ Vector2 sizeWithSpacing;

		CharacterDefinition();
		CharacterDefinition(int _textureIdx, tchar _ch, bool _colourCharacter, Vector2 const& _gridLeftBottom, Vector2 const& _gridSize, Vector2 const & _leftBottom, Vector2 const & _size, Vector2 const & _leftBottomSpacing, Vector2 const & _rightTopSpacing);

		void recalculate();
	};

	struct FontDrawingContext
	{
		Optional<bool> forceFixedSize;
		Optional<bool> blend;
		Optional<float> smallerDecimals;
		ShaderProgramInstance const* shaderProgramInstance = nullptr;
		Meshes::Mesh3DRenderingContext  renderingContext = Meshes::Mesh3DRenderingContext::none();
	};

	class Font
	: public SerialisableResource
	{
	public:
		static void set_variable_space_width_pt(Optional<float> const& _pt) { s_variableSpaceWidthPt = _pt; }
	public:
		struct TextureDefinition
		{
			Texture const * texture;
			String characters; // not used at runtime+release by font, might be skipped filling
		};
	public:
		Font();
		~Font();

		static Vector2 const & h_centred() { return c_hCentred; }
		static Vector2 const & h_left() { return c_hLeft; }
		static Vector2 const & h_right() { return c_hRight; }

		bool load_from_xml(IO::XML::Node const * _node);
		void use_textures_and_prepare(Array<TextureDefinition> const & _textureDefinitions, String const & _fixedWidthCharacters, String const & _colourCharacters, std::function<Texture*(int _textureDefinitionIdx, int _subTextureIdx)> get_texture_for_texture_definition);

		int get_width(tchar _ch, bool _forceFixedSize = false) const;
		Vector2 calculate_char_size() const;
		Vector2 calculate_text_size(String const & _text, Optional<Vector2> const & _scale = NP, Optional<FontDrawingContext> const& _drawingContext = NP) const;
		Vector2 calculate_text_size(tchar const * _text, Optional<Vector2> const & _scale = NP, Optional<FontDrawingContext> const& _drawingContext = NP) const;

		// (rel placement is where leftBottom is to be treated = 0,0 is left bottom, 1,1 is right top)
		void draw_text_at(Video3D* _v3d, String const & _text, Colour const & _colour, Vector2 const & _leftBottom,
			Vector2 const & _scale = Vector2::one, Vector2 const & _anchorRelPlacement = Vector2(0.0f, 0.0f), Optional<bool> _alignToPixels = NP,
			FontDrawingContext const & _drawingContext = FontDrawingContext()) const;
		void draw_text_at(Video3D* _v3d, tchar const * _text, Colour const & _colour, Vector2 const & _leftBottom,
			Vector2 const & _scale = Vector2::one, Vector2 const & _anchorRelPlacement = Vector2(0.0f, 0.0f), Optional<bool> _alignToPixels = NP,
			FontDrawingContext const& _drawingContext = FontDrawingContext()) const;
		void draw_text_at(Video3D* _v3d, tchar const * _text, Colour const & _colour, Vector3 const & _leftBottom,
			Vector2 const & _scale = Vector2::one, Vector2 const & _anchorRelPlacement = Vector2(0.0f, 0.0f), Optional<bool> _alignToPixels = NP,
			FontDrawingContext const& _drawingContext = FontDrawingContext()) const;

		void draw_text_at_with_border(::System::Video3D* _v3d, String const & _text, Colour const & _colour, Colour const & _borderColour, float _border, Vector2 const & _leftBottom, Vector2 const & _scale = Vector2::one, Vector2 const & _anchorRelPlacement = Vector2(0.0f, 0.0f), Optional<bool> _alignToPixels = NP,
			FontDrawingContext const& _drawingContext = FontDrawingContext()) const;

		// ATM construct mesh creates mesh without colours
		void construct_mesh(Meshes::Mesh3D* _mesh, Array<TextureID> & _usingTextures, String const & _text, Optional<bool> const & _for3d = NP, float const & _letterHeight = 1.0f, Vector2 const & _anchorRelPlacement = Vector2::half) const;

		float get_height() const;

		ShaderProgramInstance* access_fallback_shader_program_instance() const { return fallbackShaderProgramInstance; }
		ShaderProgramInstance* access_fallback_no_blend_shader_program_instance() const { return fallbackNoBlendShaderProgramInstance; }

	public:
		void debug_draw() const;
		bool preview_render() const;

	private:
		static Vector2 const c_hCentred;
		static Vector2 const c_hLeft;
		static Vector2 const c_hRight;
		static Optional<float> s_variableSpaceWidthPt;

		ShaderProgramInstance* fallbackShaderProgramInstance; // used if no shader program instance provided
		ShaderProgramInstance* fallbackNoBlendShaderProgramInstance; // used if no shader program instance provided

		::System::VertexFormat vertexFormat;
		::System::VertexFormat vertexFormatNoCharacterColour;

		Array<TextureDefinition> textureDefinitions;
		FontType::Type fontType;
		Vector2 charSize = Vector2::zero; // char size - it's either space or max char size
		int variableWidthLeftMargin = 1; // extra margin
		int variableWidthRightMargin = 1;
		int topMargin = 1;
		int bottomMargin = 1;
		Array<CharacterDefinition> ansiCharacterDefinitions; // -32
		Array<CharacterDefinition> unicodeCharacterDefinitions; // extra unicode
		
		bool alignToPixels;
		VectorInt2 fixedFontSize; // as provided, also returned by get_height
		VectorInt2 fixedFontSpacing;

		float baseLinePt = 0.2f; // relative to charSize

		float offsetY = 0.0f; // offsets rendering of the font (scaled)

		void set_fallback_shader_texture(Texture const * _texture) const;

		CharacterDefinition const * get_character_definition(int _ch) const;
		CharacterDefinition * access_character_definition(int _ch);

		void get_fixed_character_range(CharacterDefinition& _chd, OUT_ RangeInt& _fixedCharactersRange, String const& _fixedWidthCharacters);
		void calculate_variable_placement(CharacterDefinition& _chd, RangeInt const & _fixedCharactersRange, String const& _fixedWidthCharacters);

		void draw_text_or_construct_internal(Meshes::Mesh3D* _drawIntoMesh, OUT_ OPTIONAL_ Array<TextureID> * _usingTextures, Video3D* _v3d, tchar const* _text,
			Optional<bool> const& _for3d, Colour const& _colour, Vector3 const& _leftBottom,
			Vector2 const& _scale = Vector2::one, Vector2 const& _anchorRelPlacement = Vector2(0.0f, 0.0f), Optional<bool> _alignToPixels = NP,
			FontDrawingContext const& _drawingContext = FontDrawingContext()) const;
	};

};
