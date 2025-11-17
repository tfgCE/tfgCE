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
			struct QuestionWindow
			{
			public:
				static void show_message(Canvas* _canvas, tchar const* _info, std::function<void()> _on_done = nullptr);
				static void ask_question_yes_no(Canvas* _canvas, tchar const* _info, std::function<void()> _on_yes, std::function<void()> _on_no = nullptr);

			public:
				CanvasWindow* step_1_create_window(Canvas* _canvas, Optional<float> const& _textScale = NP, Optional<float> const& _buttonTextScale = NP);
				CanvasButton* step_2_add_text(tchar const* _text);
				CanvasButton* step_2_add_answer(tchar const* _text, std::function<void()> _on_press);
				CanvasWindow* step_3_finalise();

			private:
				Canvas* canvas = nullptr;
				CanvasWindow* window = nullptr;
				CanvasWindow* messagePanel = nullptr;
				CanvasWindow* answerPanel = nullptr;
				float textScale;
				float buttonTextScale;
			};
		};
	};
};
