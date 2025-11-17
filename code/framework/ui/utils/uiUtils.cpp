#include "uiUtils.h"

#include "..\uiCanvas.h"
#include "..\uiCanvasWindow.h"

//

using namespace Framework;
using namespace UI;

//

void Utils::place_below(Canvas* _c, CanvasWindow* _e, ICanvasElement*& _below, float _horizontalAlignmentPt, Optional<float> const& _spacing)
{
	if (_below)
	{
		_e->set_align_vertically(_c, _below, Vector2(_horizontalAlignmentPt, 1.0f));
	}
	else
	{
		_e->set_at_pt(_c, Vector2(_horizontalAlignmentPt, 1.0f));
	}
	if (_spacing.is_set())
	{
		_e->set_at(_e->get_at() - Vector2(0.0f, _spacing.get(0.0f)));
	}
	_below = _e;
}

void Utils::place_above(Canvas* _c, CanvasWindow* _e, ICanvasElement*& _above, float _horizontalAlignmentPt, Optional<float> const& _spacing)
{
	if (_above)
	{
		_e->set_align_vertically(_c, _above, Vector2(_horizontalAlignmentPt, 0.0f));
	}
	else
	{
		_e->set_at_pt(_c, Vector2(_horizontalAlignmentPt, 0.0f));
	}
	if (_spacing.is_set())
	{
		_e->set_at(_e->get_at() + Vector2(0.0f, _spacing.get(0.0f)));
	}
	_above = _e;
}

void Utils::place_on_left(Canvas* _c, CanvasWindow* _e, ICanvasElement*& _of, float _verticalAlignmentPt, Optional<float> const& _spacing)
{
	if (_of)
	{
		_e->set_align_horizontally(_c, _of, Vector2(1.0f, _verticalAlignmentPt));
	}
	else
	{
		_e->set_at_pt(_c, Vector2(1.0f, _verticalAlignmentPt));
	}
	if (_spacing.is_set())
	{
		_e->set_at(_e->get_at() - Vector2(_spacing.get(0.0f), 0.0f));
	}
	_of = _e;
}

void Utils::place_on_right(Canvas* _c, CanvasWindow* _e, ICanvasElement*& _of, float _verticalAlignmentPt, Optional<float> const& _spacing)
{
	if (_of)
	{
		_e->set_align_horizontally(_c, _of, Vector2(0.0f, _verticalAlignmentPt));
	}
	else
	{
		_e->set_at_pt(_c, Vector2(0.0f, _verticalAlignmentPt));
	}
	if (_spacing.is_set())
	{
		_e->set_at(_e->get_at() + Vector2(_spacing.get(0.0f), 0.0f));
	}
	_of = _e;
}

void Utils::place_below(Canvas* _c, CanvasWindow* _e, Name const & _belowId, float _horizontalAlignmentPt, Optional<float> const& _spacing)
{
	auto* ce = _c->find_by_id(_belowId);
	place_below(_c, _e, ce, _horizontalAlignmentPt, _spacing);
}

void Utils::place_above(Canvas* _c, CanvasWindow* _e, Name const & _aboveId, float _horizontalAlignmentPt, Optional<float> const& _spacing)
{
	auto* ce = _c->find_by_id(_aboveId);
	place_above(_c, _e, ce, _horizontalAlignmentPt, _spacing);
}

void Utils::place_on_left(Canvas* _c, CanvasWindow* _e, Name const & _ofId, float _horizontalAlignmentPt, Optional<float> const& _spacing)
{
	auto* ce = _c->find_by_id(_ofId);
	place_on_left(_c, _e, ce, _horizontalAlignmentPt, _spacing);
}

void Utils::place_on_right(Canvas* _c, CanvasWindow* _e, Name const & _ofId, float _horizontalAlignmentPt, Optional<float> const& _spacing)
{
	auto* ce = _c->find_by_id(_ofId);
	place_on_right(_c, _e, ce, _horizontalAlignmentPt, _spacing);
}

