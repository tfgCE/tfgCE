#include "uiGrid2Menu.h"

#include "..\uiCanvas.h"
#include "..\uiCanvasButton.h"
#include "..\uiCanvasWindow.h"

//

using namespace Framework;
using namespace UI;

//

CanvasWindow* Utils::Grid2Menu::step_1_create_window(Canvas * _canvas, Optional<float> const& _textScale)
{
	canvas = _canvas;
	textScale = _textScale.get(1.0f);

	window = new UI::CanvasWindow();
	window->set_closable(false)->set_movable(false)->set_modal(true);
	canvas->add(window);

	return window;
}

CanvasButton* Utils::Grid2Menu::step_2_add_text_and_button(tchar const* _text, tchar const* _buttonText)
{
	if (oddElementAdded)
	{
		step_3_add_no_button();
	}
	{
		auto* b = new UI::CanvasButton();
		b->set_caption(_text);
		b->set_text_only()->set_alignment(Vector2(1.0f, 0.5f));
		b->set_scale(textScale);
		b->set_auto_width(canvas)->set_default_height(canvas);
		window->add(b);
	}
	{
		auto* b = new UI::CanvasButton();
		b->set_caption(_buttonText);
		b->set_scale(textScale);
		b->set_auto_width(canvas)->set_default_height(canvas);
		window->add(b);
		return b;
	}
}

CanvasButton* Utils::Grid2Menu::step_3_add_button(tchar const* _buttonText)
{
	auto* b = new UI::CanvasButton();
	b->set_caption(_buttonText);
	b->set_scale(textScale);
	b->set_auto_width(canvas)->set_default_height(canvas);
	window->add(b);
	return b;
}

CanvasButton* Utils::Grid2Menu::step_3_add_no_button()
{
	oddElementAdded = !oddElementAdded;
	auto* b = new UI::CanvasButton();
	b->set_scale(textScale);
	b->set_caption(TXT(" "));
	b->set_auto_width(canvas)->set_default_height(canvas);
	b->set_text_only();
	window->add(b);
	return b;
}

CanvasWindow* Utils::Grid2Menu::step_4_finalise(Optional<float> const& _width)
{
	Optional<Vector2> size;
	if (_width.is_set())
	{
		size = Vector2(_width.get(), 0.0f);
	}
	window->place_content_on_grid(canvas, VectorInt2(2, 0), size, NP);
	window->set_at_pt(canvas, Vector2::half);

	return window;
}
