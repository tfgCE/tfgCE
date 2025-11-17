#pragma once

#include "..\..\core\system\video\font.h"

#include "..\library\libraryStored.h"

namespace Concurrency
{
	class JobExecutionContext;
};

namespace Framework
{
	class Material;
	class Mesh;
	class Texture;
	class TextureArray;

	class Font
	: public LibraryStored
	{
		FAST_CAST_DECLARE(Font);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		Font(Library * _library, LibraryName const & _name);

		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		::System::Font* get_font() const { return font; }

	public: // drawing
		// note that we disallow accessing font directly, as we may want to hide font selection (rel placement is where leftBottom is to be treated = 0,0 is left bottom, 1,1 is right top)
		void draw_text_at(::System::Video3D* _v3d, String const & _text, Colour const & _colour, Vector2 const & _leftBottom, Vector2 const & _scale = Vector2::one, Vector2 const & _anchorRelPlacement = Vector2(0.0f, 0.0f), Optional<bool> const & _alignToPixels = NP, ::System::FontDrawingContext const& _fontDrawingContext = ::System::FontDrawingContext()) const;
		void draw_text_at(::System::Video3D* _v3d, tchar const * _text, Colour const & _colour, Vector2 const & _leftBottom, Vector2 const & _scale = Vector2::one, Vector2 const & _anchorRelPlacement = Vector2(0.0f, 0.0f), Optional<bool> const& _alignToPixels = NP, ::System::FontDrawingContext const& _fontDrawingContext = ::System::FontDrawingContext()) const;
		void draw_text_at_with_border(::System::Video3D* _v3d, String const & _text, Colour const & _colour, Colour const & _borderColour, float _border, Vector2 const & _leftBottom, Vector2 const & _scale = Vector2::one, Vector2 const & _anchorRelPlacement = Vector2(0.0f, 0.0f), Optional<bool> const& _alignToPixels = NP, ::System::FontDrawingContext const& _fontDrawingContext = ::System::FontDrawingContext()) const;
		float get_height() const;
		Vector2 calculate_char_size() const;

	public: // create a mesh (with materials)
		void construct_mesh(Mesh* _mesh, String const& _text, Optional<bool> const& _for3d = NP, float const& _letterHeight = 1.0f, Vector2 const& _anchorRelPlacement = Vector2::half) const; // will use default, from global references
		void construct_mesh(Mesh* _mesh, Material* _usingMaterial, Name const & _textureIdParam, String const& _text, Optional<bool> const& _for3d = NP, float const& _letterHeight = 1.0f, Vector2 const& _anchorRelPlacement = Vector2::half) const;

	public: // LibraryStored
		virtual void debug_draw() const;
		virtual bool preview_render() const;

	protected:
		virtual ~Font();

	private:
		struct TextureDefinition
		{
			String characters;
			bool usesLocalisedCharacters = false;
			// uses one of two but defined in the file as one
			UsedLibraryStored<Texture> texture;
			UsedLibraryStored<TextureArray> textureArray;
		};

		// todo add support for different sizes that will be automatically selected depending on size of character?
		::System::Font* font;
		Array<TextureDefinition> textureDefinitions; // used only while loading
		String fixedWidthCharacters;
		String colourCharacters;

		void defer_font_delete();
	};
};
