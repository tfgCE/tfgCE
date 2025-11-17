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
			struct Grid1Menu
			{
				CanvasWindow* step_1_create_window(Canvas * _canvas, Optional<float> const& _textScale = NP);
				CanvasButton* step_2_add_text(tchar const * _text); // multiple steps
				CanvasButton* step_2_add_button(tchar const * _buttonText); // multiple steps
				CanvasWindow* step_3_finalise(Optional<float> const & _width = NP);

			private:
				Canvas* canvas = nullptr;
				CanvasWindow* window = nullptr;
				float textScale = 1.0f;
			};
		};
	};
};
