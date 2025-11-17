#include "uiQuestionWindow.h"

#include "..\uiCanvas.h"
#include "..\uiCanvasButton.h"
#include "..\uiCanvasWindow.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#define CARET '|'

//

using namespace Framework;
using namespace UI;

//

void Utils::QuestionWindow::show_message(Canvas* _canvas, tchar const* _info, std::function<void()> _on_done)
{
	UI::Utils::QuestionWindow question;

	float textScale = 2.0f;
	float menuButtonScale = 2.0f;
	question.step_1_create_window(_canvas, textScale, menuButtonScale);

	question.step_2_add_text(_info);
	question.step_2_add_answer(TXT("ok"), [_on_done]() {if (_on_done) { _on_done(); }})
#ifdef AN_STANDARD_INPUT
		->set_shortcut(System::Key::Return)
#endif
		;

	question.step_3_finalise();
}

void Utils::QuestionWindow::ask_question_yes_no(Canvas* _canvas, tchar const* _info, std::function<void()> _on_yes, std::function<void()> _on_no)
{
	UI::Utils::QuestionWindow question;

	float textScale = 2.0f;
	float menuButtonScale = 2.0f;
	question.step_1_create_window(_canvas, textScale, menuButtonScale);

	question.step_2_add_text(_info);
	question.step_2_add_answer(TXT("yes"), [_on_yes]() {if (_on_yes) { _on_yes(); }})
#ifdef AN_STANDARD_INPUT
		->set_shortcut(System::Key::Return)
#endif
		;
	question.step_2_add_answer(TXT("no"), [_on_no]() {if (_on_no) { _on_no(); }})
#ifdef AN_STANDARD_INPUT
		->set_shortcut(System::Key::Esc)
#endif
		;

	question.step_3_finalise();
}

//

CanvasWindow* Utils::QuestionWindow::step_1_create_window(Canvas * _canvas, Optional<float> const& _textScale, Optional<float> const & _buttonTextScale)
{
	canvas = _canvas;
	textScale = _textScale.get(2.0f);
	buttonTextScale = _buttonTextScale.get(textScale * 1.5f);

	window = new UI::CanvasWindow();
	window->set_closable(false)->set_movable(false)->set_modal(true);
	canvas->add(window);

	messagePanel = new UI::CanvasWindow();
	messagePanel->set_closable(false)->set_movable(false);
	window->add(messagePanel);

	answerPanel = new UI::CanvasWindow();
	answerPanel->set_closable(false)->set_movable(false);
	window->add(answerPanel);

	return window;
}

CanvasButton* Utils::QuestionWindow::step_2_add_text(tchar const * _text)
{
	auto* b = new UI::CanvasButton();
	b->set_caption(_text);
	b->set_scale(textScale);
	b->set_text_only();
	b->set_auto_width(canvas)->set_default_height(canvas);
	messagePanel->add(b);
	return b;
}

CanvasButton* Utils::QuestionWindow::step_2_add_answer(tchar const* _text, std::function<void()> _on_press)
{
	auto* b = new UI::CanvasButton();
	b->set_caption(_text);
	b->set_scale(buttonTextScale);
	b->set_auto_width(canvas)->set_default_height(canvas);
	SafePtr<ICanvasElement> keepWindow;
	keepWindow = window;
	b->set_on_press([_on_press, keepWindow](Framework::UI::ActContext const& _context)
		{
			_on_press();
			if (auto* w = keepWindow.get())
			{
				w->remove();
			}
		});
	answerPanel->add(b);
	return b;
}

CanvasWindow* Utils::QuestionWindow::step_3_finalise()
{
	messagePanel->place_content_vertically(canvas);
	answerPanel->place_content_on_grid(canvas, VectorInt2(0, 1), Vector2(messagePanel->get_placement(canvas).x.length(), 0.0f));

	window->place_content_vertically(canvas);

	window->set_size_to_fit_all(canvas, canvas->get_default_spacing());
	window->set_at_pt(canvas, Vector2::half);

	return window;
}
