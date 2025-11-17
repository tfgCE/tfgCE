#include "uiCanvasWindow.h"

#include "..\video\font.h"

#include "uiCanvas.h"
#include "uiCanvasButton.h"
#include "uiCanvasInputContext.h"
#include "uiCanvasRenderContext.h"
#include "uiCanvasUpdateWorktable.h"

#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace UI;

//

REGISTER_FOR_FAST_CAST(CanvasWindow);

CanvasWindow::CanvasWindow()
: elements(this)
{
}

CanvasWindow* CanvasWindow::get()
{
	CanvasWindow* window = get_one();
	window->on_get_element();
	window->leftBottom = Vector2::zero;
	window->size = Vector2::one * 100.0f;
	return window;
}

void CanvasWindow::on_get()
{
	leftBottom = Vector2::zero;
	size.clear();
	innerSize.clear();
	title = String::empty();

	onMoved = nullptr;

	closable = false;
	movable = false;
	modal = false;
	scrollVertical = Scroll();
	scrollHorizontal = Scroll();

	elements.reset();
}

void CanvasWindow::on_release()
{
	elements.reset();
}

CanvasWindow* CanvasWindow::set_at(Vector2 const& _leftBottom)
{
	leftBottom = _leftBottom;
	return this;
}

CanvasWindow* CanvasWindow::set_size(Vector2 const& _size)
{
	an_assert(isfinite(_size.x) && isfinite(_size.y));
	size = _size;
	innerSize.clear();
	return this;
}

CanvasWindow* CanvasWindow::set_inner_size(Vector2 const& _size)
{
	an_assert(isfinite(_size.x) && isfinite(_size.y));
	size.clear();
	innerSize = _size;
	return this;
}

CanvasWindow* CanvasWindow::set_at_pt(Canvas const* _canvas, Vector2 const& _atPt)
{
	{
		if (auto* w = get_in_window())
		{
			Range2 innerW = w->get_inner_placement(_canvas);
			Vector2 useSize = calculate_size(_canvas);
			innerW.x.max -= useSize.x;
			innerW.y.max -= useSize.y;
			Vector2 globalLeftBottom = innerW.get_at(_atPt);
			leftBottom = globalLeftBottom - get_global_offset();
		}
		else
		{
			Range2 canvasSpace = Range2::zero;
			canvasSpace.include(_canvas->get_logical_size());
			Vector2 useSize = calculate_size(_canvas);
			canvasSpace.x.max -= useSize.x;
			canvasSpace.y.max -= useSize.y;
			Vector2 globalLeftBottom = canvasSpace.get_at(_atPt);
			leftBottom = globalLeftBottom - get_global_offset();
		}
	}

	return this;
}

void CanvasWindow::update_for_canvas(Canvas const* _canvas)
{
	{
		auto& cs = _canvas->get_setup();
		Vector2 lb = get_global_offset() + leftBottom + Vector2::one * cs.windowFrameThickness;
		if (scrollHorizontal.inUse)
		{
			lb.y += cs.windowScrollThickness;
		}
		elements.set_global_offset(lb);
	}

	elements.update_for_canvas(_canvas);

	update_scrolls(_canvas);
}

void CanvasWindow::update_scrolls(Canvas const* _canvas)
{
	if (!scrollHorizontal.inUse &&
		!scrollVertical.inUse)
	{
		return;
	}

	auto& cs = _canvas->get_setup();

	Range2 innerPlacement = get_inner_placement(_canvas);
	Range2 elementsPlacement = Range2::empty;

	Concurrency::ScopedSpinLock lock(elements.access_lock(), true);
	for_every_ptr(e, elements.get_elements())
	{
		Range2 ep = e->get_placement(_canvas);
		elementsPlacement.include(ep);
	}

	for_count(int, i, 2)
	{
		auto& scroll = i == 0 ? scrollHorizontal : scrollVertical;
		if (scroll.inUse)
		{
			auto& innerRange = i == 0 ? innerPlacement.x : innerPlacement.y;
			auto& elementsRange = i == 0 ? elementsPlacement.x : elementsPlacement.y;
			elementsRange.include(innerRange); // if fits in window, keep it as it is
			float scrollPossible = max(0.0f, elementsRange.length() - innerRange.length());
			scroll.scrollableLength = scrollPossible;
			scroll.nonScrollableLength = innerRange.length();
			scroll.at = innerRange.min - elementsRange.min;

			scroll.scrollArea = innerPlacement;
			if (i == 0)
			{
				scroll.scrollArea.y.max = innerPlacement.y.min - 1.0f;
				scroll.scrollArea.y.min = innerPlacement.y.min - cs.windowScrollThickness;
			}
			else 
			{
				scroll.scrollArea.x.min = innerPlacement.x.max + 1.0f;
				scroll.scrollArea.x.max = innerPlacement.x.max + cs.windowScrollThickness;
			}
		}
	}
}

Range2 CanvasWindow::get_inner_placement(Canvas const* _canvas) const
{
	auto& cs = _canvas->get_setup();
	Vector2 lb = leftBottom + get_global_offset();
	Vector2 rt = lb;
	if (size.is_set())
	{
		rt = lb + size.get();
		if (scrollHorizontal.inUse)
		{
			lb.y += cs.windowScrollThickness;
		}
		if (scrollVertical.inUse)
		{
			rt.x -= cs.windowScrollThickness;
		}
		lb.x += cs.windowFrameThickness;
		lb.y += cs.windowFrameThickness;
		rt.x -= cs.windowFrameThickness;
		rt.y -= cs.windowFrameThickness;
		if (!title.is_empty())
		{
			rt.y -= cs.windowTitleBarHeight; // title bar
		}
	}
	else if (innerSize.is_set())
	{
		lb.x += cs.windowFrameThickness;
		lb.y += cs.windowFrameThickness;
		rt = lb + innerSize.get();
	}
	Range2 r = Range2(lb);
	r.include(rt);
	return r;
}

Vector2 CanvasWindow::calculate_size(Canvas const * _canvas) const
{
	auto& canvasSetup = _canvas->get_setup();
	Vector2 useSize = Vector2::zero;
	if (size.is_set())
	{
		useSize = size.get();
	}
	else if (innerSize.is_set())
	{
		useSize = innerSize.get();
		useSize.x += 2.0f * canvasSetup.windowFrameThickness; // frame sides
		useSize.y += 2.0f * canvasSetup.windowFrameThickness; // frame top and bottom
		if (scrollVertical.inUse)
		{
			useSize.x += canvasSetup.windowScrollThickness;
		}
		if (scrollHorizontal.inUse)
		{
			useSize.y += canvasSetup.windowScrollThickness;
		}
		if (!title.is_empty())
		{
			useSize.y += canvasSetup.windowTitleBarHeight; // title bar
		}
	}
	return useSize;
}

void CanvasWindow::render(CanvasRenderContext & _context) const
{
	auto* c = _context.get_canvas();
	auto& cs = c->get_setup();

	Vector2 useSize = calculate_size(_context.get_canvas());
	Vector2 lb = _context.get_canvas()->logical_to_real_location(leftBottom + get_global_offset());
	Vector2 rt = _context.get_canvas()->logical_to_real_location(leftBottom + useSize + get_global_offset());

	::System::Video3DPrimitives::fill_rect_2d(cs.colours.windowPaper, lb, rt);

	RangeInt2 prevScissors = _context.get_v3d()->get_scissors();

	{
		RangeInt2 scissors = RangeInt2::empty;
		Vector2 mlb = Vector2::one * cs.windowFrameThickness;
		Vector2 mrt = -Vector2::one * cs.windowFrameThickness;
		if (!title.is_empty())
		{
			mrt.y -= cs.windowTitleBarHeight;
		}
		if (scrollVertical.inUse)
		{
			mrt.x -= cs.windowScrollThickness;
		}
		if (scrollHorizontal.inUse)
		{
			mlb.y += cs.windowScrollThickness;
		}
		Vector2 ilb = lb + c->logical_to_real_size(mlb);
		Vector2 irt = rt + c->logical_to_real_size(mrt);
		scissors.include(TypeConversions::Normal::f_i_floor(ilb));
		scissors.include(TypeConversions::Normal::f_i_cells(irt));
		bool renderDueToScissors = true;
		if (!prevScissors.is_empty())
		{
			scissors.intersect_with(prevScissors);
			renderDueToScissors = !scissors.is_empty();
		}
		_context.get_v3d()->set_scissors(scissors);

		if (renderDueToScissors)
		{
			elements.render(_context);
		}
	}

	bool renderDueToScissors = true;
	{
		RangeInt2 scissors = RangeInt2::empty;
		scissors.include(TypeConversions::Normal::f_i_floor(lb));
		scissors.include(TypeConversions::Normal::f_i_cells(rt));
		if (!prevScissors.is_empty())
		{
			scissors.intersect_with(prevScissors);
			renderDueToScissors = !scissors.is_empty();
		}
		_context.get_v3d()->set_scissors(scissors);
	}

	if (renderDueToScissors)
	{
		// title
		if (!title.is_empty())
		{
			float useHeight = cs.windowTitleBarTextHeight;
			useHeight = _context.get_canvas()->logical_to_real_height(useHeight);
			if (auto* font = _context.get_canvas()->get_font_for_real_size(useHeight))
			{
				::System::Video3DPrimitives::fill_rect_2d(cs.colours.windowTitleBarPaper, Vector2(lb.x, rt.y - c->logical_to_real_height(cs.windowTitleBarHeight + cs.windowFrameThickness)), rt);
				font->font->draw_text_at(System::Video3D::get(), title, cs.colours.windowTitleBarInk,
					Vector2(lb.x + c->logical_to_real_height(cs.windowFrameThickness + 2.0f), rt.y - c->logical_to_real_height(cs.windowFrameThickness + cs.windowTitleBarHeight * 0.5f)),
					Vector2(useHeight / font->size.y) * _context.get_canvas()->get_setup().fontScale, Vector2(0.0f, 0.5f), false);
			}
		}

		{
			for_count(int, i, 2)
			{
				auto& scroll = i == 0 ? scrollHorizontal : scrollVertical;
				if (scroll.inUse)
				{
					Vector2 slb = _context.get_canvas()->logical_to_real_location(scroll.scrollArea.bottom_left());
					Vector2 srt = _context.get_canvas()->logical_to_real_location(scroll.scrollArea.top_right());
					::System::Video3DPrimitives::rect_2d(cs.colours.windowFrame, slb, srt);
					if (scroll.scrollableLength + scroll.nonScrollableLength != 0.0f)
					{
						float startPt = scroll.at / (scroll.scrollableLength + scroll.nonScrollableLength);
						float endPt = (scroll.at + scroll.nonScrollableLength) / (scroll.scrollableLength + scroll.nonScrollableLength);
						Vector2 sblb;
						Vector2 sbrt;
						if (i == 0)
						{
							sblb = Vector2(lerp(startPt, slb.x, srt.x), slb.y);
							sbrt = Vector2(lerp(endPt, slb.x, srt.x), srt.y);
						}
						else
						{
							sblb = Vector2(slb.x, lerp(startPt, slb.y, srt.y));
							sbrt = Vector2(srt.x, lerp(endPt, slb.y, srt.y));
						}
						::System::Video3DPrimitives::fill_rect_2d(Colour::lerp(0.5f, cs.colours.windowPaper, cs.colours.windowFrame), sblb, sbrt);
						::System::Video3DPrimitives::rect_2d(cs.colours.windowFrame, sblb, sbrt);
					}
				}
			}
		}

		{
			float w = max(0.0f, c->logical_to_real_height(cs.windowFrameThickness - 1.0f));
			::System::Video3DPrimitives::fill_rect_2d(cs.colours.windowFrame, Vector2(lb.x, lb.y), Vector2(lb.x + w, rt.y));
			::System::Video3DPrimitives::fill_rect_2d(cs.colours.windowFrame, Vector2(rt.x - w, lb.y), Vector2(rt.x, rt.y));
			::System::Video3DPrimitives::fill_rect_2d(cs.colours.windowFrame, Vector2(lb.x, lb.y), Vector2(rt.x, lb.y + w));
			::System::Video3DPrimitives::fill_rect_2d(cs.colours.windowFrame, Vector2(lb.x, rt.y - w), Vector2(rt.x, rt.y));
		}

		if (closable)
		{
			Vector2 crb = rt;
			crb.x -= cs.windowFrameThickness;
			crb.y -= cs.windowFrameThickness;
			Vector2 clb = rt;
			clb.x -= cs.windowTitleBarHeight;
			clb.y -= cs.windowTitleBarHeight;
			float i = 2.0f;
			float o = cs.windowTitleBarHeight * 0.1f;
			clb.x += i;
			clb.y += i;
			crb.x -= i;
			crb.y -= i;
			::System::Video3DPrimitives::fill_rect_2d(cs.colours.windowTitleBarPaper, clb, crb);
			::System::Video3DPrimitives::rect_2d(cs.colours.windowFrame, clb, crb);
			::System::Video3DPrimitives::line_2d(cs.colours.windowFrame, Vector2(clb.x + o, crb.y - o), Vector2(crb.x - o, clb.y + o));
			::System::Video3DPrimitives::line_2d(cs.colours.windowFrame, Vector2(clb.x + o, clb.y + o), Vector2(crb.x - o, crb.y - o));
		}
	}

	_context.get_v3d()->set_scissors(prevScissors);
}

void CanvasWindow::add(ICanvasElement* _element)
{
	elements.add(_element);
}

bool CanvasWindow::process_shortcuts(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const
{
	if (elements.process_shortcuts(_cic, _cuw))
	{
		return true;
	}

	return base::process_shortcuts(_cic, _cuw);
}

bool CanvasWindow::process_controls(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw)
{
	if (elements.process_controls(_cic, _cuw))
	{
		return true;
	}

	if (scrollVertical.inUse)
	{
		Range2 r = Range2::empty;
		Vector2 lb = leftBottom + get_global_offset();
		r.include(lb);
		r.include(lb + calculate_size(_cuw.get_canvas()));
		for_every(c, _cic.get_cursors())
		{
			if (c->at.is_set())
			{
				if (r.does_contain(c->at.get()))
				{
					if (auto* input = ::System::Input::get())
					{
#ifdef AN_STANDARD_INPUT
						float const MOUSE_WHEEL_ZOOM_SPEED = 0.2f;
						if (input->has_key_been_pressed(System::Key::MouseWheelUp))
						{
							scroll_by(_cuw.get_canvas(), 3.0f * Vector2(0.0f, _cuw.get_canvas()->get_default_spacing().y));
							return true;
						}
						if (input->has_key_been_pressed(System::Key::MouseWheelDown))
						{
							scroll_by(_cuw.get_canvas(), 3.0f * Vector2(0.0f, -_cuw.get_canvas()->get_default_spacing().y));
							return true;
						}
#endif
					}
				}
			}
		}
	}
	
	return base::process_controls(_cic, _cuw);
}

void CanvasWindow::update_hover_on(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const
{
	Range2 r = Range2::empty;
	Vector2 lb = leftBottom + get_global_offset();
	r.include(lb);
	r.include(lb + calculate_size(_cuw.get_canvas()));
	for_every(c, _cic.get_cursors())
	{
		if (c->at.is_set())
		{
			if (r.does_contain(c->at.get()))
			{
				CanvasInputContext cic(_cic);
				cic.keep_only_cursor(for_everys_index(c));

				// first go through elements inside
				elements.update_hover_on(cic, _cuw);

				// then mark us, we should be later, 
				_cuw.hovers_on(for_everys_index(c), this);
			}
		}
	}
}

void CanvasWindow::on_hold(REF_ HoldContext& _context)
{
	if (! _context.isHolding &&
		_context.at.is_set())
	{
		auto& cs = _context.canvas->get_setup();

		// check if should hold
		Vector2 useSize = calculate_size(_context.canvas);
		Vector2 lb = leftBottom + get_global_offset();
		Vector2 rt = leftBottom + useSize + get_global_offset();

		if (closable)
		{
			Vector2 crb = rt;
			crb.x -= cs.windowFrameThickness;
			crb.y -= cs.windowFrameThickness;
			Vector2 clb = crb;
			clb.x -= cs.windowTitleBarHeight;
			clb.y -= cs.windowTitleBarHeight;
			Range2 closeRegion = Range2::empty;
			closeRegion.include(clb);
			closeRegion.include(crb);
			if (closeRegion.does_contain(_context.at.get()))
			{
				access_in_elements().remove(this);
				return;
			}
		}

		if (!_context.isHolding && movable)
		{
			Range2 holdRegion = Range2::empty;

			if (!title.is_empty())
			{
				holdRegion.include(Vector2(lb.x, rt.y - cs.windowTitleBarHeight - cs.windowFrameThickness));
				holdRegion.include(rt);
			}
			else
			{
				holdRegion.include(lb);
				holdRegion.include(rt);
			}

			_context.isHolding = holdRegion.does_contain(_context.at.get());
			_context.holdingCustomId = 0;
		}
		if (!_context.isHolding && scrollVertical.inUse)
		{
			_context.isHolding = scrollVertical.scrollArea.does_contain(_context.at.get());
			_context.holdingCustomId = 1;
		}
		if (!_context.isHolding && scrollHorizontal.inUse)
		{
			_context.isHolding = scrollHorizontal.scrollArea.does_contain(_context.at.get());
			_context.holdingCustomId = 2;
		}
	}

	if (_context.isHolding)
	{
		if (_context.holdingCustomId == 0)
		{
			leftBottom += _context.movedBy;
			if (onMoved)
			{
				onMoved(this);
			}
		}
		if (_context.holdingCustomId == 1)
		{
			scroll_by(_context.canvas, _context.movedBy * Vector2(0.0f, 1.0f));
		}
		if (_context.holdingCustomId == 2)
		{
			scroll_by(_context.canvas, _context.movedBy * Vector2(1.0f, 0.0f));
		}
	}
}

ICanvasElement* CanvasWindow::find_by_id(Name const& _id, Optional<int> const& _additionalId)
{
	if (auto* e = base::find_by_id(_id, _additionalId))
	{
		return e;
	}

	return elements.find_by_id(_id, _additionalId);
}

Vector2 CanvasWindow::get_size(Canvas const* _canvas) const
{
	return calculate_size(_canvas);
}

void CanvasWindow::offset_placement_by(Vector2 const& _offset)
{
	leftBottom += _offset;
}

Range2 CanvasWindow::get_placement(Canvas const* _canvas) const
{
	Vector2 lb = leftBottom + get_global_offset();
	Range2 r = Range2(lb);
	r.include(lb + calculate_size(_canvas));
	return r;
}

CanvasWindow* CanvasWindow::set_size_to_fit_all(Canvas const* _canvas, Vector2 const& _margin)
{
	Range2 content = Range2::empty;

	Concurrency::ScopedSpinLock lock(elements.access_lock(), true);
	Vector2 globalOffset = get_global_offset();
	for_every_ptr(e, elements.get_elements())
	{
		Range2 eAt = e->get_placement(_canvas);
		content.include(eAt.bottom_left() - globalOffset);
		content.include(eAt.top_right() - globalOffset);
	}

	Vector2 offset = _margin - content.bottom_left();

	for_every_ptr(e, elements.get_elements())
	{
		e->offset_placement_by(offset);
	}

	float topExtraHeight = 0.0f;
	if (closable && title.is_empty())
	{
		topExtraHeight = max(0.0f, _canvas->get_setup().windowTitleBarHeight - _margin.y);
	}
	set_inner_size(content.length() + _margin * 2.0f + Vector2::yAxis * topExtraHeight);

	return this;
}

CanvasWindow* CanvasWindow::set_centre_at(Canvas const* _canvas, Vector2 const& _centre)
{
	Vector2 s = calculate_size(_canvas);
	return set_at(_centre - s * 0.5f);
}

CanvasWindow* CanvasWindow::set_align_vertically(Canvas const* _canvas, ICanvasElement const* _rel, Vector2 const& _pt)
{
	Range2 relAt = _rel->get_placement(_canvas);
	Vector2 s = calculate_size(_canvas);

	Vector2 lb;
	lb.x = relAt.x.get_at(_pt.x) - s.x * _pt.x;
	lb.y = relAt.y.get_at(1.0f - _pt.y) - s.y * _pt.y;
	lb = lb - get_global_offset();
	set_at(lb);

	return this;
}

CanvasWindow* CanvasWindow::set_align_horizontally(Canvas const* _canvas, ICanvasElement const* _rel, Vector2 const& _pt)
{
	Range2 relAt = _rel->get_placement(_canvas);
	Vector2 s = calculate_size(_canvas);

	Vector2 lb;
	lb.x = relAt.x.get_at(1.0f - _pt.x) - s.x * _pt.x;
	lb.y = relAt.y.get_at(_pt.y) - s.y * _pt.y;
	lb = lb - get_global_offset();
	set_at(lb);

	return this;
}

void CanvasWindow::place_content_vertically(Canvas const* _canvas, Optional<Vector2> const& _spacing, Optional<bool> const& _fromTop)
{
	Vector2 spacing = _spacing.get(_canvas->get_default_spacing());
	bool fromTop = _fromTop.get(true);

	Vector2 beAt = spacing;

	Concurrency::ScopedSpinLock lock(elements.access_lock(), true);
	float maxWidth = 0.0f;
	for_every_ptr(e, elements.get_elements())
	{
		Range2 eAt = e->get_placement(_canvas);
		maxWidth = max(maxWidth, eAt.x.length());
	}

	for_every_ptr(e, elements.get_elements())
	{
		if (auto* b = fast_cast<CanvasButton>(e))
		{
			b->set_width(maxWidth);
		}
	}

	Vector2 eGlobalOffset = elements.get_global_offset();
	struct Utils
	{
		static void place(REF_ Vector2& beAt, ICanvasElement* e, Vector2 const& eGlobalOffset, Canvas const* _canvas, Vector2 const& _spacing, float maxWidth)
		{
			Range2 eAt = e->get_placement(_canvas);
			eAt.move_by(-eGlobalOffset);
			Vector2 offset = beAt - eAt.bottom_left();
			offset.x += (eAt.x.length() - maxWidth) * 0.5f;
			e->offset_placement_by(offset);
			beAt.y += eAt.y.length() + _spacing.y;
		}
	};
	if (fromTop)
	{
		for_every_reverse_ptr(e, elements.get_elements())
		{
			Utils::place(REF_ beAt, e, eGlobalOffset, _canvas, spacing, maxWidth);
		}
	}
	else
	{
		for_every_ptr(e, elements.get_elements())
		{
			Utils::place(REF_ beAt, e, eGlobalOffset, _canvas, spacing, maxWidth);
		}
	}

	beAt.x = maxWidth + spacing.x * 2.0f;
	set_inner_size(beAt);
}

void CanvasWindow::place_content_horizontally(Canvas const* _canvas, Optional<Vector2> const& _spacing)
{
	Vector2 spacing = _spacing.get(_canvas->get_default_spacing());

	Vector2 beAt = spacing;

	Concurrency::ScopedSpinLock lock(elements.access_lock(), true);
	Vector2 eGlobalOffset = elements.get_global_offset();
	float maxHeight = 0.0f;
	for_every_ptr(e, elements.get_elements())
	{
		Range2 eAt = e->get_placement(_canvas);
		eAt.move_by(-eGlobalOffset);
		Vector2 offset = beAt - eAt.bottom_left();
		e->offset_placement_by(offset);
		beAt.x += eAt.x.length() + spacing.x;
		maxHeight = max(maxHeight, eAt.y.length());
	}

	beAt.y = maxHeight + spacing.y * 2.0f;
	set_inner_size(beAt);
}

void CanvasWindow::place_content_on_grid(Canvas const* _canvas, Optional<VectorInt2> const& _grid, Optional<Vector2> const & _size, Optional<Vector2> const& _spacing, Optional<bool> const & _fromTop, Optional<bool> const& _keepSizeAsIs)
{
	Vector2 spacing = _spacing.get(_canvas->get_default_spacing());
	bool fromTop = _fromTop.get(true);
	bool keepSizeAsIs = _keepSizeAsIs.get(false);

	auto& cs = _canvas->get_setup();

	Concurrency::ScopedSpinLock lock(elements.access_lock(), true);

	VectorInt2 grid = _grid.get(VectorInt2::zero);
	if (grid.x == 0)
	{
		int amount = elements.get_elements().get_size();
		if (grid.y == 0)
		{
			grid.x = max(1, (int)(sqrt((float)amount)));
		}
		else
		{
			int leftOvers = mod(amount, grid.y);
			int roundedAmount;
			if (leftOvers == 0)
			{
				roundedAmount = amount;
			}
			else
			{
				roundedAmount = amount + grid.y - leftOvers;
			}
			grid.x = roundedAmount / grid.y;
		}
	}
	if (grid.y == 0)
	{
		int amount = elements.get_elements().get_size();
		int leftOvers = mod(amount, grid.x);
		int roundedAmount;
		if (leftOvers == 0)
		{
			roundedAmount = amount;
		}
		else
		{
			roundedAmount = amount + grid.x - leftOvers;
		}
		grid.y = roundedAmount / grid.x;
	}

	Vector2 maxSize = Vector2::zero;
	for_every_ptr(e, elements.get_elements())
	{
		Range2 eAt = e->get_placement(_canvas);
		maxSize.x = max(maxSize.x, eAt.x.length());
		maxSize.y = max(maxSize.y, eAt.y.length());
	}

	Optional<Vector2> innerSize = _size;
	if (innerSize.is_set())
	{
		if (innerSize.get().x != 0)
		{
			innerSize.access().x -= cs.windowFrameThickness * 2.0f;
			if (scrollVertical.inUse)
			{
				innerSize.access().x -= cs.windowScrollThickness;
			}
		}
		if (innerSize.get().y != 0)
		{
			innerSize.access().y -= cs.windowFrameThickness * 2.0f;
			if (! title.is_empty())
			{
				innerSize.access().y -= cs.windowTitleBarHeight;
			}
			if (scrollHorizontal.inUse)
			{
				innerSize.access().y -= cs.windowScrollThickness;
			}
		}
	}

	if (innerSize.is_set())
	{
		if (innerSize.get().x != 0)
		{
			maxSize.x = (innerSize.get().x - (float)(grid.x - 1) * spacing.x - spacing.x * 2.0f) / (float)grid.x;
		}
		if (innerSize.get().y != 0)
		{
			maxSize.y = (innerSize.get().y - (float)(grid.y - 1) * spacing.y - spacing.y * 2.0f) / (float)grid.y;
		}
	}

	for_every_ptr(e, elements.get_elements())
	{
		if (auto* b = fast_cast<CanvasButton>(e))
		{
			b->set_size(maxSize);
		}
	}

	VectorInt2 coord = VectorInt2::zero;
	Vector2 zeroAt = Vector2::zero;
	if (fromTop)
	{
		coord.y = grid.y - 1;
		if (keepSizeAsIs)
		{
			Range2 innerPlacement = get_inner_placement(_canvas);
			zeroAt.y = innerPlacement.y.length() - (grid.y * (maxSize.y + spacing.y) + spacing.y); // + spacing as we remove one between but add two on borders
		}
	}
	Vector2 eGlobalOffset = elements.get_global_offset();
	Vector2 newWindowSize = spacing * 2.0f;
	for_every_ptr(e, elements.get_elements())
	{
		Range2 eAt = e->get_placement(_canvas);
		eAt.move_by(-eGlobalOffset);
		Vector2 beAt = spacing + (maxSize + spacing) * coord.to_vector2() + zeroAt;
		Vector2 offset = beAt - eAt.bottom_left();
		e->offset_placement_by(offset);
		newWindowSize.x = max(newWindowSize.x, beAt.x + maxSize.x + spacing.x);
		newWindowSize.y = max(newWindowSize.y, beAt.y + maxSize.y + spacing.y);
		beAt.x += eAt.x.length() + spacing.x;

		++coord.x;
		if (coord.x >= grid.x)
		{
			coord.x = 0;
			coord.y = fromTop ? (coord.y - 1) : (coord.y + 1);
		}
	}

	if (!keepSizeAsIs)
	{
		if (innerSize.is_set() && innerSize.get().x != 0)
		{
			newWindowSize.x = innerSize.get().x;
		}
		if (innerSize.is_set() && innerSize.get().y != 0)
		{
			newWindowSize.y = innerSize.get().y;
		}
	
		set_inner_size(newWindowSize);
	}
}

void CanvasWindow::window_to_top()
{
	//if (!title.is_empty())
	{
		to_top();
	}
	base::window_to_top();
}

void CanvasWindow::scroll_by(Canvas const* _canvas, Vector2 const& _by)
{
	Vector2 by = _by;
	if (scrollVertical.inUse)
	{
		by.y = clamp(by.y, 0.0f - scrollVertical.at, scrollVertical.scrollableLength - scrollVertical.at);
		by.y *= (scrollVertical.scrollableLength + scrollVertical.nonScrollableLength) / scrollVertical.nonScrollableLength;
	}
	else
	{
		by.y = 0.0f;
	}
	if (scrollHorizontal.inUse)
	{
		by.x = clamp(by.x, 0.0f - scrollHorizontal.at, scrollHorizontal.scrollableLength - scrollHorizontal.at);
		by.x *= (scrollHorizontal.scrollableLength + scrollHorizontal.nonScrollableLength) / scrollHorizontal.nonScrollableLength;
	}
	else
	{
		by.x = 0.0f;
	}

	elements.offset_placement_by(-by);

	update_scrolls(_canvas);
}

void CanvasWindow::scroll_to_show_element(Canvas const* _canvas, ICanvasElement const* _element)
{
	if (!_element)
	{
		return;
	}
	Range2 iAt = get_inner_placement(_canvas);
	Range2 eAt = _element->get_placement(_canvas);
	if (eAt.is_empty())
	{
		return;
	}

	Vector2 by = Vector2::zero;
	if (scrollVertical.inUse)
	{
		if (eAt.y.min < iAt.y.min)
		{
			by.y = eAt.y.min - iAt.y.min;
		}
		if (eAt.y.max > iAt.y.max)
		{
			by.y = eAt.y.max - iAt.y.max;
		}
		by.y = clamp(by.y, 0.0f - scrollVertical.at, scrollVertical.scrollableLength - scrollVertical.at);
	}
	if (scrollHorizontal.inUse)
	{
		if (eAt.x.min < iAt.x.min)
		{
			by.x = eAt.x.min - iAt.x.min;
		}
		if (eAt.x.max > iAt.x.max)
		{
			by.x = eAt.x.max - iAt.x.max;
		}
		by.x = clamp(by.x, 0.0f - scrollHorizontal.at, scrollHorizontal.scrollableLength - scrollHorizontal.at);
	}

	elements.offset_placement_by(-by);

	update_scrolls(_canvas);
}
