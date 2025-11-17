#pragma once

#include "texture.h"
#include "..\..\core\types\colour.h"

namespace Meshes
{
	struct Mesh3DRenderingContext;
};

namespace System
{
	class Video3D;
	struct ShaderProgramInstance;
};

namespace Framework
{
	class ColourClashSprite
	: public LibraryStored
	{
		FAST_CAST_DECLARE(ColourClashSprite);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		ColourClashSprite(Library * _library, LibraryName const & _name);

		void draw_graphics_at(::System::Video3D* _v3d, VectorInt2 const & _at) const;
		void draw_colour_grid_at(::System::Video3D* _v3d, VectorInt2 const & _at, Optional<Colour> const & _overrideForegroundColour = NP, Optional<Colour> const & _overrideBackgroundColour = NP, Optional<Colour> const & _overrideBrightColour = NP) const;

		Texture* get_texture() const { return texture.get(); }
		Vector2 const & get_uv_0() const { return uv0; }
		Vector2 const & get_uv_1() const { return uv1; }
		Vector2 const & get_uv_anchor() const { return uvAnchor; }
		Vector2 const & get_anchor() const { return anchor; }
		Vector2 const & get_size() const { return size; }
		Vector2 const & get_rel_anchor() const { return leftBottomOffset; }
		RangeInt2 calc_rect() const { return RangeInt2(RangeInt((int)floor(leftBottomOffset.x), (int)floor(rightTopOffset.x) - 1), RangeInt((int)floor(leftBottomOffset.y), (int)floor(rightTopOffset.y) - 1)); }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	private:
		UsedLibraryStored<Texture> texture;
		Vector2 uv0 = Vector2::zero;
		Vector2 uv1 = Vector2::one;
		Vector2 uvAnchor = Vector2::zero; // will be automatically set to range between uv0 and uv1
		Colour foregroundColour = Colour::none;
		Colour backgroundColour = Colour::none;
		Colour brightColour = Colour::none;
		CACHED_ Vector2 size = Vector2::zero;
		CACHED_ Vector2 anchor = Vector2::zero;
		CACHED_ Vector2 relAnchor = Vector2::zero; // reverse of left bottom offset
		CACHED_ Vector2 leftBottomOffset = Vector2::zero;
		CACHED_ Vector2 rightTopOffset = Vector2::zero;
	};
};
