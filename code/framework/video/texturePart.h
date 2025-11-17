#pragma once

#include "texture.h"
#include "texturePartUse.h"
#include "..\..\core\types\colour.h"
#include "..\..\core\system\video\vertexFormat.h"
#include "..\..\core\system\video\renderTargetPtr.h"

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
	class TexturePart;

	struct TexturePartDrawingContext
	{
		bool alignToPixels = true;
		Colour colour = Colour::white;
		Range verticalRange = Range(0.0f, 1.0f); // range starts at bottom
		Vector2 scale = Vector2::one;
		Optional<Range2> limits;
		Optional<Range2> showPt;
		Optional<bool> blend;
		Optional<TexturePartUse> use;
		::System::ShaderProgramInstance const * shaderProgramInstance = nullptr;
		Meshes::Mesh3DRenderingContext const * renderingContext = nullptr;
	};

	class TexturePart
	: public LibraryStored
	{
		FAST_CAST_DECLARE(TexturePart);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		static void initialise_static();
		static void close_static();

		TexturePart(Library * _library, LibraryName const & _name);
		virtual ~TexturePart();

		void draw_at(::System::Video3D* _v3d, Vector2 const & _at, TexturePartDrawingContext const & _context = TexturePartDrawingContext()) const;
		void draw_at(::System::Video3D* _v3d, Vector3 const & _at, TexturePartDrawingContext const & _context = TexturePartDrawingContext()) const;

		bool is_valid() const { return get_texture() != nullptr; }
		Texture* get_texture() const { return texture.get(); }
		::System::TextureID get_texture_id() const { return textureID; }
		Vector2 const & get_uv_0() const { return uv0; }
		Vector2 const & get_uv_1() const { return uv1; }
		Vector2 const & get_uv_anchor() const { return uvAnchor; }
		Vector2 const & get_anchor() const { return anchor; } // anchor within texture
		Vector2 get_rel_anchor() const { return -leftBottomOffset; } // anchor relative to left bottom
		Vector2 const & get_left_bottom_offset() const { return leftBottomOffset; }
		Vector2 const & get_right_top_offset() const { return rightTopOffset; }
		Vector2 const & get_size() const { return size; }
		RangeInt2 calc_rect() const { return RangeInt2(RangeInt((int)leftBottomOffset.x, (int)rightTopOffset.x), RangeInt((int)leftBottomOffset.y, (int)rightTopOffset.y)); }

		void setup(Texture* _texture, Vector2 const& _uv0, Vector2 const& _uv1, Vector2 const& _uvAnchor);
		void setup(::System::RenderTarget* _renderTarget, int _renderTargetIdx, Vector2 const& _uv0, Vector2 const& _uv1, Vector2 const& _uvAnchor);

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	private:
		static ::System::VertexFormat* s_vertexFormat;

		::System::ShaderProgramInstance* fallbackShaderProgramInstance; // used if no shader program instance provided
		::System::ShaderProgramInstance* fallbackNoBlendShaderProgramInstance; // used if no shader program instance provided

		UsedLibraryStored<Texture> texture;
		::System::RenderTargetPtr renderTarget;
		int renderTargetIdx = 0;
		CACHED_ ::System::TextureID textureID = ::System::texture_id_none();
		Vector2 uv0 = Vector2::zero;
		Vector2 uv1 = Vector2::one;
		Vector2 uvAnchor = Vector2::zero; // will be automatically set to range between uv0 and uv1
		CACHED_ Vector2 size = Vector2::zero;
		CACHED_ Vector2 anchor = Vector2::zero;
		CACHED_ Vector2 leftBottomOffset = Vector2::zero;
		CACHED_ Vector2 rightTopOffset = Vector2::zero;

		void set_fallback_shader_texture(::System::Texture const* _texture) const;
		void set_fallback_shader_texture(::System::RenderTarget const* _rt, int _rtIdx) const;
		void cache_auto();
	};
};
