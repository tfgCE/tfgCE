#pragma once

namespace System
{
	class Video3D;
};

namespace Framework
{
	namespace UI
	{
		class Canvas;

		struct CanvasRenderContext
		{
		public:
			CanvasRenderContext(::System::Video3D* _v3d, Canvas const * _canvas);

			::System::Video3D* get_v3d() const { return v3d; }
			Canvas const * get_canvas() const { return canvas; }

		private:
			::System::Video3D* v3d;
			Canvas const* canvas;
		};
	};
};
