#pragma once

#include "sprite.h"

#include "..\..\core\math\math.h"
#include "..\..\core\system\video\renderTargetPtr.h"

#define BASIC_DRAW_RENDERING_BUFFER__ALLOW_FORCED_RERENDERING

namespace System
{
	class Video3D;
};

namespace Framework
{
	class Sprite;

	/**
	 *	Allows to batch stuff to render (sprites, lines, rectangles, triangles, points)
	 *	They are drawn in the provided order, which means that if something is drawn later, it draws over the existing stuff
	 */
	class BasicDrawRenderingBuffer
	{
#ifdef BASIC_DRAW_RENDERING_BUFFER__ALLOW_FORCED_RERENDERING
	public:
		static void force_rerendering_of_all();
#endif

	public:
		static void initialise_static();
		static void close_static();

		static ::System::VertexFormat* get_non_sprite_vertex_format() { return s_nonSpriteVertexFormat; }

		BasicDrawRenderingBuffer();
		~BasicDrawRenderingBuffer();

		bool is_ok() const { return renderTarget.get(); }

		void init(VectorInt2 const & _size); // rendering thread!
		void close();

	public:
		void clear(); // clear all draw commands but they are kept there
		void clear_non_sprites(); // sprites remain but everything else is gone
		void prune(); // removes all draw commands

		void clear(int _idx);

		// sprites
		int add(Sprite const* _sprite, VectorInt2 const& _at, Optional<VectorInt2> const& _scale = NP); // anywhere
		int add_back(Sprite const* _sprite, VectorInt2 const& _at, Optional<VectorInt2> const& _scale = NP); // always at the back
		void set(int _idx, Sprite const * _sprite);
		void set(int _idx, Sprite const * _sprite, VectorInt2 const& _at, Optional<VectorInt2> const& _scale = NP);
		void set(int _idx, VectorInt2 const& _at, Optional<VectorInt2> const& _scale = NP);
		void set_stretch_to(int _idx, VectorInt2 const& _stretchTo); // if zero, takes original size

		// non sprites
		int draw_point(Colour const& _colour, VectorInt2 const& _at, Optional<int> const& _idx = NP);
		int draw_line(Colour const& _colour, VectorInt2 const& _at, VectorInt2 const& _to, Optional<int> const& _idx = NP);
		int draw_rect(Colour const& _colour, VectorInt2 const& _at, VectorInt2 const& _to, Optional<int> const& _idx = NP);
		int draw_rect_fill(Colour const& _colour, VectorInt2 const& _at, VectorInt2 const& _to, Optional<int> const& _idx = NP);
		int draw_tri(Colour const& _colour, VectorInt2 const& _at, VectorInt2 const& _to, VectorInt2 const& _to2, Optional<int> const& _idx = NP);
		int draw_tri_fill(Colour const& _colour, VectorInt2 const& _at, VectorInt2 const& _to, VectorInt2 const& _to2, Optional<int> const& _idx = NP);

	public:
		void update(::System::Video3D* _v3d); // rendering thread!

		System::RenderTarget* get_render_target() const { return renderTarget.get(); }

	private:
		static ::System::VertexFormat* s_nonSpriteVertexFormat;

		// no concurrency mechanisms, we expect to be run on a single thread!
#ifdef BASIC_DRAW_RENDERING_BUFFER__ALLOW_FORCED_RERENDERING
		static int s_forcedRerendering;
		int forcedRerendering = 0;
#endif

		struct DrawCommand
		{
			enum Content
			{
				None,
				DrawSprite,		// 6 vertices/2 triangles per sprite, draw as sprite
				DrawPoint,		// 1 point per point, draw as point
				DrawLine,		// 2 vertices/1 line per line, draw as line (in real world we draw it twice, in both directions, see code for explanation)
				DrawRect,		// 8 vertices/4 lines per rectangle, draw as line (x4)
				DrawRectFill,	// 6 vertices/2 triangles per rectangle, draw as triangle fill (x2)
				DrawTri,		// 6 vertices/3 lines per triangle, draw as line (x3)
				DrawTriFill,	// 3 vertices/1 triangle per triangle, draw as triangle fill
			};
			Content content = Content::DrawSprite;
			VectorInt2 at = VectorInt2::zero;
			// non sprite
			VectorInt2 to = VectorInt2::zero; // for lines, rect, rect fills
			VectorInt2 to2 = VectorInt2::zero; // rectangle
			Colour colour = Colour::white;
			// sprite
			Sprite const* sprite = nullptr;
			VectorInt2 scale = VectorInt2::one;
			Optional<VectorInt2> stretchTo; // overrides scale (if zero, takes original size)

			bool operator == (DrawCommand const& _other) const;
			bool operator != (DrawCommand const& _other) const { return ! operator==(_other); }
		};

		struct Entry
		{
			DrawCommand requested;
			DrawCommand rendered;
		};

		Array<Entry> entries;
		int firstEmpty = NONE;

		int drawCommandsRendered = NONE; // will force to be cleared
		::System::RenderTargetPtr renderTarget; // is ok if there is render target

		::System::ShaderProgramInstance* spriteShaderProgramInstance;
		::System::ShaderProgramInstance* nonSpriteShaderProgramInstance;

	private:
		struct NonSpriteVertex
		{
			Vector3 location;
			Colour colour;

			NonSpriteVertex() {}
			NonSpriteVertex(Colour const& _colour, Vector3 const& _loc) : location(_loc), colour(_colour) {}
		};

		Array<SpriteVertex> spriteVertices;
		Array<NonSpriteVertex> nonSpriteVertices;

		void render_sprite_vertices_and_clear(::System::Video3D* _v3d, Texture* _texture);
		void render_non_sprite_vertices_and_clear(::System::Video3D* _v3d, DrawCommand::Content _content);

	private:
		// do not implement
		BasicDrawRenderingBuffer(BasicDrawRenderingBuffer const& _other);
		BasicDrawRenderingBuffer& operator=(BasicDrawRenderingBuffer const& _other);

		int get_empty_draw_command(int _idx = NONE);
	};
};
