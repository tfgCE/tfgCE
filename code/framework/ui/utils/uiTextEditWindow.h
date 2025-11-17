#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\types\optional.h"
#include "..\..\..\core\system\input.h"

#include <functional>

namespace Framework
{
	namespace UI
	{
		interface_class ICanvasElement;
		class Canvas;
		struct CanvasButton;
		struct CanvasWindow;

		namespace Utils
		{
			void edit_text(Canvas* _canvas, tchar const* _info, REF_ String& _text, std::function<void()> _on_done);
			void edit_text(Canvas* _c, tchar const* _info, String const& _text, std::function<void(String const& _text)> _on_accepted, std::function<void()> _on_canceled = nullptr);

			struct TextEditWindow
			{
				CanvasWindow* step_1_create_window(Canvas* _canvas, Optional<float> const& _textScale = NP, Optional<float> const& _buttonTextScale = NP);
				CanvasWindow* step_2_setup(String const& _textToEdit, std::function<void(String const &)> _on_ok, std::function<void()> _on_back = nullptr);
				CanvasWindow* step_3_finalise();

			private:
				Canvas* canvas = nullptr;
				CanvasWindow* window = nullptr;
				float textScale;
				float buttonTextScale;
				float spacing;

				CanvasButton* add_button(REF_ Vector2& at, tchar const* _caption, Optional<float> const & _width = NP, Optional<float> const & _overrideScale = NP);
				CanvasButton* add_key_button(REF_ Vector2& at, Optional<float> const& _width, System::Key::Type _key, tchar _add, tchar _addShifted = 0);

				static void enter_char(Canvas* _canvas, tchar _add, tchar _addShifted = 0);
				static void remove_char(Canvas* _canvas, int _dir);
				static void clear_box(Canvas* _canvas);
				static void move_caret(Canvas* _canvas, int _by);
				static void move_caret_to_end(Canvas* _canvas, int _end);

				static void remove_window(Canvas* _canvas);
			};
		};
	};
};
