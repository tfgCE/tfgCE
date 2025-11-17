#pragma once

#include "..\..\core\math\math.h"

namespace Framework
{
	namespace UI
	{
		class Canvas;

		namespace CursorIndex
		{
			enum Type
			{
				Mouse = 0
			};
		};

		struct CanvasInputContext
		{
		public:
			// can be used with anything but by default it has built-in mouse support
			struct Cursor
			{
				Optional<Vector2> at; // from left bottom (logical for canvas), if not provided, should be ignored (might be valid for touch screen)
				Optional<Vector2> movedBy;
				int buttonFlags = 0;
				int prevButtonFlags = 0;
				int scrollV = 0;
				int scrollH = 0;

				Cursor();
				static Cursor use_mouse(Canvas const * _canvas); // will use as it would be rendered in full window
			};
		public:
			CanvasInputContext();

			void set_cursor(int _cursorIdx, Cursor const& _cursor);

			void keep_only_cursor(int _idx);

			ArrayStatic<Cursor, 8> const& get_cursors() const { return cursors; }

		private:
			ArrayStatic<Cursor, 8> cursors;
		};
	};
};
