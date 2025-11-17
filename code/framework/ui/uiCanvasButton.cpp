#include "uiCanvasButton.h"

#include "..\video\font.h"
#include "..\video\sprite.h"

#include "uiCanvas.h"
#include "uiCanvasInputContext.h"
#include "uiCanvasRenderContext.h"
#include "uiCanvasUpdateWorktable.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\system\core.h"
#include "..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace UI;

//

REGISTER_FOR_FAST_CAST(CanvasButton);

CanvasButton* CanvasButton::set_shortcut(System::Key::Type _key, Optional<ShortcutParams> const& _params, std::function<void(ActContext const&)> _perform, String const& _info)
{
	add_shortcut(_key, _params, _perform, _info);

	return this;
}

CanvasButton* CanvasButton::get()
{
	CanvasButton* button = get_one();
	button->on_get_element();
	return button;
}

void CanvasButton::on_get()
{
	at = Vector2::zero;
	size = Vector2::zero;
	anchorRelPlacement = Vector2::half;
	alignment = Vector2::half;
	caption = String::empty();
	userData = nullptr;
	fontScale = Vector2::one;
	scale = 1.0f;
	colour.clear();
	sprite.clear();
	hasFrame = true;
	useFrameWidthOnlyForAction = false;
	innerFrameWithBackground = false;
	frameWidthPt.clear();
	useWindowPaper = false;
	textOnly = false;

	highlighted = false;
	enabled = true;

	do_on_press = nullptr;
	do_on_click = nullptr;
	do_on_double_click = nullptr;
}

void CanvasButton::on_release()
{
	do_on_press = nullptr;
	do_on_click = nullptr;
	do_on_double_click = nullptr;
}

CanvasButton* CanvasButton::set_auto_width(Canvas const* _canvas, float _margin)
{
	Vector2 measuredSize = Vector2::zero;
	{
		auto& cs = _canvas->get_setup();
		float useHeight = cs.buttonTextHeight * scale;
		useHeight = _canvas->logical_to_real_height(useHeight);
		if (auto* font = _canvas->get_font_for_real_size(useHeight))
		{
			if (auto* f = font->font->get_font())
			{
				measuredSize = f->calculate_text_size(caption, Vector2(useHeight / font->size.y) * _canvas->get_setup().fontScale * fontScale);
				measuredSize = _canvas->real_to_logical_size(measuredSize);
			}
		}
	}
	size.x = measuredSize.x + _margin * 2.0f;
	return this;
}

CanvasButton* CanvasButton::set_default_height(Canvas const* _canvas)
{
	size.y = _canvas->get_setup().buttonDefaultHeight * scale;
	return this;
}

void CanvasButton::render(CanvasRenderContext& _context) const
{
	auto* c = _context.get_canvas();
	auto& cs = c->get_setup();

	Vector2 lb = at - size * anchorRelPlacement + get_global_offset();
	Vector2 rt = lb + size;

	lb = c->logical_to_real_location(lb);
	rt = c->logical_to_real_location(rt);

	bool hoversOnThis = c->does_hover_on(this);

	Optional<Colour> fillColour;
	if (!textOnly && enabled)
	{
		if (colour.is_set())
		{
			fillColour = colour.get();
		}
		else
		{
			fillColour = hoversOnThis && enabled ? cs.colours.buttonPaperHoverOn : (highlighted ? cs.colours.buttonPaperHighlighted : (useWindowPaper? cs.colours.windowPaper : cs.colours.buttonPaper));
		}
		::System::Video3DPrimitives::fill_rect_2d(fillColour.get(), lb, rt);
	}

	if (!textOnly && enabled)
	{
		if (auto* s = sprite.get())
		{
			Range2 r = Range2::empty;
			r.include(lb);
			r.include(rt);
			s->draw_stretch_at(System::Video3D::get(), r, true);

			if (highlighted && ::System::Core::get_timer_mod(1.0f) < 0.5f && ! frameWidthPt.is_set())
			{
				::System::Video3DPrimitives::fill_rect_2d(cs.colours.buttonPaperHighlighted.with_alpha(0.5f), lb, rt);
			}
		}
	}

	String const* shortcutInfo = (!shortcuts.is_empty() && !shortcuts[0].info.is_empty()) ? &shortcuts[0].info : nullptr;
	if (!caption.is_empty() || shortcutInfo)
	{
		RangeInt2 prevScissors = _context.get_v3d()->get_scissors();

		RangeInt2 scissors = RangeInt2::empty;
		scissors.include(TypeConversions::Normal::f_i_floor(lb) + VectorInt2::one);
		scissors.include(TypeConversions::Normal::f_i_cells(rt) - VectorInt2::one);
		bool renderDueToScissors = true;
		if (!prevScissors.is_empty())
		{
			scissors.intersect_with(prevScissors);
			renderDueToScissors = !scissors.is_empty();
		}
		if (renderDueToScissors)
		{
			_context.get_v3d()->set_scissors(scissors);

			Vector2 buttonTextMargin = c->logical_to_real_size(cs.buttonTextMargin);

			if (!caption.is_empty())
			{
				float useHeight = cs.buttonTextHeight * scale;
				useHeight = c->logical_to_real_height(useHeight);
				if (auto* font = c->get_font_for_real_size(useHeight))
				{
					Vector2 useAt;
					useAt.x = lerp(alignment.x, lb.x + buttonTextMargin.x, rt.x - buttonTextMargin.x);
					useAt.y = lerp(alignment.y, lb.y + buttonTextMargin.y, rt.y - buttonTextMargin.y);
					Vector2 scale = Vector2(useHeight / font->size.y) * c->get_setup().fontScale * fontScale;
					Vector2 textSize = font->font->get_font()->calculate_text_size(caption.to_char(), scale);
					if (textSize.x > 0.0f)
					{
						scale.x *= min(1.0f, (rt.x - lb.x) / textSize.x);
					}
					if (shortcutInfo)
					{
						useAt.y += textSize.y * 0.15f;
					}
					font->font->draw_text_at(System::Video3D::get(), caption, enabled? cs.colours.buttonInk : Colour::lerp(0.5f, cs.colours.buttonInk, cs.colours.buttonPaper),
						useAt, scale, alignment, false);
				}
			}

			if (shortcutInfo && enabled)
			{
				float useHeight = cs.buttonTextHeight * scale * 0.45f;
				useHeight = c->logical_to_real_height(useHeight);
				if (auto* font = c->get_font_for_real_size(useHeight))
				{
					Vector2 useAt;
					useAt.x = lb.x + buttonTextMargin.x * 0.1f;
					useAt.y = lb.y + buttonTextMargin.y * 0.1f;
					Vector2 scale = Vector2(useHeight / font->size.y) * c->get_setup().fontScale * fontScale;
					if (fillColour.is_set())
					{
						int off = 2;
						for_range(int, oy, -off, off)
						{
							for_range(int, ox, -off, off)
							{
								font->font->draw_text_at(System::Video3D::get(), *shortcutInfo, fillColour.get(), useAt + Vector2((float)ox, (float)oy), scale, Vector2(0.0f, 0.0f), false);
							}
						}
					}
					font->font->draw_text_at(System::Video3D::get(), *shortcutInfo, cs.colours.buttonShortcutInk, useAt, scale, Vector2(0.0f, 0.0f), false);
				}
			}

			_context.get_v3d()->set_scissors(prevScissors);
		}
	}

	if (!textOnly)
	{
		Optional<float> useFrameWidthPt;
		Colour frameColour = cs.colours.buttonFrame;
		Colour backgroundColour = cs.colours.buttonPaper;
		if (hasFrame)
		{
			useFrameWidthPt = useFrameWidthOnlyForAction? 0.0f : frameWidthPt.get(0.0f);
		}
		{
			if (highlighted)
			{
				useFrameWidthPt = frameWidthPt.get(0.0f);
				frameColour = cs.colours.buttonFrameHighlighted;
				backgroundColour = cs.colours.buttonPaperHighlighted;
			}
			if (hoversOnThis && enabled)
			{
				useFrameWidthPt = frameWidthPt.get(0.0f);
				frameColour = cs.colours.buttonFrameHoverOn;
				backgroundColour = cs.colours.buttonPaperHoverOn;
			}
		}
		if (highlighted && frameWidthPt.is_set())
		{
			useFrameWidthPt = frameWidthPt.get();
			frameColour = cs.colours.buttonFrameHighlighted;
		}
		if (useFrameWidthPt.is_set())
		{
			float fw = useFrameWidthPt.get() * (min(rt.y - lb.y, rt.x - lb.x));
			for(int i = innerFrameWithBackground ? 0 : 1; i < 2; ++ i)
			{
				Colour c = i == 0? backgroundColour : frameColour;
				::System::Video3DPrimitives::fill_rect_2d(c, Vector2(lb.x, lb.y), Vector2(lb.x + fw, rt.y));
				::System::Video3DPrimitives::fill_rect_2d(c, Vector2(lb.x, lb.y), Vector2(rt.x, lb.y + fw));
				::System::Video3DPrimitives::fill_rect_2d(c, Vector2(lb.x, rt.y - fw), Vector2(rt.x, rt.y));
				::System::Video3DPrimitives::fill_rect_2d(c, Vector2(rt.x - fw, lb.y), Vector2(rt.x, rt.y));
				fw *= 0.5f;
			}
		}
	}
}

void CanvasButton::execute_shortcut(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const
{
	bool doOnPress = true;
	if (shortcuts.is_index_valid(_cuw.get_shortcut_index_to_activate()))
	{
		auto& shortcut = shortcuts[_cuw.get_shortcut_index_to_activate()];
		if (shortcut.perform)
		{
			doOnPress = false;
		}
	}
	if (doOnPress)
	{
		ActContext context;
		prepare_context(OUT_ context, _cuw.get_canvas(), NP);
		if (do_on_press)
		{
			do_on_press(context);
			return;
		}
		else if (do_on_click)
		{
			do_on_click(context);
			return;
		}
	}
	base::execute_shortcut(_cic, _cuw);
}

void CanvasButton::update_hover_on(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const
{
	Range2 r = Range2::empty;
	Vector2 lb = at - size * anchorRelPlacement + get_global_offset();
	r.include(lb);
	r.include(lb + size);
	for_every(c, _cic.get_cursors())
	{
		if (c->at.is_set())
		{
			if (r.does_contain(c->at.get()))
			{
				_cuw.hovers_on(for_everys_index(c), this);
			}
		}
	}
}

Vector2 CanvasButton::get_centre() const
{
	Vector2 lb = at - size * anchorRelPlacement + get_global_offset();
	return (lb + size * 0.5f);
}

Vector2 CanvasButton::get_centre_local() const
{
	Vector2 lb = at - size * anchorRelPlacement;
	return (lb + size * 0.5f);
}

void CanvasButton::prepare_context(ActContext& _context, Canvas* _canvas, Optional<Vector2> const & _actAt) const
{
	_context.canvas = _canvas;
	_context.element = cast_to_nonconst(this);
	_context.at = _actAt;
	if (_actAt.is_set())
	{
		_context.relToCentre = _actAt.get() - get_centre();
	}
}

void CanvasButton::on_press(REF_ PressContext& _context)
{
	if (do_on_press)
	{
		ActContext context;
		prepare_context(OUT_ context, _context.canvas, _context.at);
		do_on_press(context);
	}
}

void CanvasButton::on_click(REF_ ClickContext& _context)
{
	if (do_on_click)
	{
		ActContext context;
		prepare_context(OUT_ context, _context.canvas, _context.at);
		do_on_click(context);
	}
}

bool CanvasButton::on_double_click(REF_ ClickContext& _context)
{
	if (do_on_double_click)
	{
		ActContext context;
		prepare_context(OUT_ context, _context.canvas, _context.at);
		do_on_double_click(context);
		return true;
	}
	return false;
}

void CanvasButton::offset_placement_by(Vector2 const& _offset)
{
	at += _offset;
}

Range2 CanvasButton::get_placement(Canvas const* _canvas) const
{
	Vector2 lb = at - size * anchorRelPlacement + get_global_offset();
	Range2 r = Range2(lb);
	r.include(lb + size);
	return r;
}
