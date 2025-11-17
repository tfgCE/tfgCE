#include "uiCanvasText.h"

#include "..\video\font.h"

#include "uiCanvas.h"
#include "uiCanvasRenderContext.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace UI;

//

REGISTER_FOR_FAST_CAST(CanvasText);

CanvasText* CanvasText::get()
{
	CanvasText* text = get_one();
	text->on_get_element();
	return text;
}

void CanvasText::on_get()
{
	text = String::empty();
	at = Vector2::zero;
	anchorRelPlacement = Vector2::half;
	colour.clear();
	height.clear();
	scale = 1.0f;
}

void CanvasText::on_release()
{
}

float CanvasText::calculate_use_height(Canvas const* _canvas) const
{
	float useHeight = 0.0f;
	if (height.is_set())
	{
		useHeight = height.get();
	}
	else
	{
		auto& cs = _canvas->get_setup();
		useHeight = cs.textHeight;
	}
	return useHeight * scale;
}

void CanvasText::render(CanvasRenderContext & _context) const
{
	auto& cs = _context.get_canvas()->get_setup();

	float useHeight = calculate_use_height(_context.get_canvas());

	useHeight = _context.get_canvas()->logical_to_real_height(useHeight);

	if (auto* font = _context.get_canvas()->get_font_for_real_size(useHeight))
	{
		Vector2 rAt = at + get_global_offset();
		rAt = _context.get_canvas()->logical_to_real_location(rAt);
		font->font->draw_text_at(System::Video3D::get(), text, colour.get(cs.colours.ink),
			rAt, Vector2(useHeight / font->size.y) * _context.get_canvas()->get_setup().fontScale, anchorRelPlacement, false);
	}
}

void CanvasText::offset_placement_by(Vector2 const& _offset)
{
	at += _offset;
}

Range2 CanvasText::get_placement(Canvas const* _canvas) const
{
	Vector2 measuredSize = Vector2::zero;

	float useHeight = calculate_use_height(_canvas);
	useHeight = _canvas->logical_to_real_height(useHeight);
	if (auto* font = _canvas->get_font_for_real_size(useHeight))
	{
		if (auto* f = font->font->get_font())
		{
			measuredSize = f->calculate_text_size(text, Vector2(useHeight / font->size.y) * _canvas->get_setup().fontScale);
			measuredSize = _canvas->real_to_logical_size(measuredSize);
		}
	}

	Vector2 size = measuredSize;
	Vector2 lb = at - size * anchorRelPlacement + get_global_offset();
	Range2 r = Range2(lb);
	r.include(lb + size);
	return r;
}
