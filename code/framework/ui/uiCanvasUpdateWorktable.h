#pragma once

#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\memory\safeObject.h"

namespace Framework
{
	namespace UI
	{
		interface_class ICanvasElement;
		class Canvas;

		struct CanvasUpdateWorktable
		{
		public:
			CanvasUpdateWorktable(Canvas* _canvas);

			Canvas* get_canvas() const { return canvas; }

			void hovers_on(int _cursorIdx, ICanvasElement const* _element);
			ArrayStatic<::SafePtr<ICanvasElement>, 8> const& get_hovers_on() const { return hoversOn; }

			void set_shortcut_to_activate(ICanvasElement const* _element, int _idx);
			ICanvasElement const* get_shortcut_to_activate() const;
			int get_shortcut_index_to_activate() const;

		public:
			bool get_in_modal_window() const { return inModalWindow; }
			void set_in_modal_window(bool _inModalWindow = true) { inModalWindow = _inModalWindow; }

		private:
			Canvas* canvas;
			ArrayStatic<::SafePtr<ICanvasElement>, 8> hoversOn;
			::SafePtr<ICanvasElement> shortcutToActivate;
			int shortcutIndexToActivate = 0;
			bool inModalWindow = false;
		};
	};
};
