#include "uiCanvasRect.h"

#include "..\video\font.h"

#include "uiCanvas.h"
#include "uiCanvasInputContext.h"
#include "uiCanvasRenderContext.h"
#include "uiCanvasUpdateWorktable.h"

#include "..\..\core\system\video\video3dPrimitives.h"

//

using namespace Framework;
using namespace UI;

//

REGISTER_FOR_FAST_CAST(CanvasRect);

CanvasRect* CanvasRect::get(Vector2 const & _leftBottom, Vector2 const & _rightTop, Colour const & _colour)
{
	CanvasRect* rect = get_one();
	rect->on_get_element();
	rect->leftBottom = _leftBottom;
	rect->rightTop = _rightTop;
	rect->colour = _colour;
	return rect;
}

void CanvasRect::on_get()
{
	leftBottom = Vector2::zero;
	rightTop = Vector2::zero;
	colour = Colour::black;
}

void CanvasRect::on_release()
{
}

void CanvasRect::render(CanvasRenderContext & _context) const
{
	::System::Video3DPrimitives::fill_rect_2d(colour,
		_context.get_canvas()->logical_to_real_location(leftBottom + get_global_offset()),
		_context.get_canvas()->logical_to_real_location(rightTop + get_global_offset()));
}

void CanvasRect::update_hover_on(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const
{
	Range2 r = Range2::empty;
	r.include(leftBottom + get_global_offset());
	r.include(rightTop + get_global_offset());
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

void CanvasRect::offset_placement_by(Vector2 const& _offset)
{
	leftBottom += _offset;
	rightTop += _offset;
}

Range2 CanvasRect::get_placement(Canvas const* _canvas) const
{
	Vector2 go = get_global_offset();
	Range2 r = Range2(leftBottom + go);
	r.include(rightTop + go);
	return r;
}
