#pragma once

#include "..\..\..\core\types\optional.h"

namespace Framework
{
	namespace UI
	{
		class Canvas;
		struct CanvasButton;
		struct CanvasWindow;

		namespace Utils
		{
			struct Grid2Menu
			{
				CanvasWindow* step_1_create_window(Canvas * _canvas, Optional<float> const& _textScale = NP);
				CanvasButton* step_2_add_text_and_button(tchar const * _text, tchar const * _buttonText); // multiple steps
				CanvasButton* step_3_add_button(tchar const * _buttonText); // multiple steps
				CanvasButton* step_3_add_no_button(); // multiple steps
				CanvasWindow* step_4_finalise(Optional<float> const & _width = NP);

			private:
				Canvas* canvas = nullptr;
				CanvasWindow* window = nullptr;
				float textScale = 1.0f;

				bool oddElementAdded = false;
			};
		};
	};
};
