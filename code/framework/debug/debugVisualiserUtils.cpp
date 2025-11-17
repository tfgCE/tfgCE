#include "debugVisualiserUtils.h"

#include "..\world\doorInRoom.h"
#include "..\world\room.h"

#include "..\..\core\vr\vrZone.h"

using namespace Framework;

void DebugVisualiserUtils::add_door_to_debug_visualiser(DebugVisualiserPtr & dv, Framework::DoorInRoom const * dA, Vector2 const & doorALoc, Vector2 const & doorADir, float _doorWidth, Colour const & _colour)
{
	// doorADir is outbound
	Vector2 tileA = doorADir.rotated_right() * _doorWidth;
	dv->add_line(doorALoc, doorALoc + (-doorADir * tileA.length() - tileA) * 0.2f, _colour);
	dv->add_line(doorALoc, doorALoc + (-doorADir * tileA.length() + tileA) * 0.2f, _colour);
	dv->add_line(doorALoc - tileA * 0.5f, doorALoc + tileA * 0.5f, _colour);
	dv->add_circle(doorALoc, 0.1f, _colour);
	if (dA)
	{
		dv->add_text(doorALoc, dA->get_room_on_other_side() ? dA->get_room_on_other_side()->get_name() : String::empty(), _colour, 0.02f);
	}
}

void DebugVisualiserUtils::add_doors_to_debug_visualiser(DebugVisualiserPtr & dv, VR::Zone const & playAreaZone, Framework::DoorInRoom const * dA, Vector2 const & doorALoc, Vector2 const & doorADir, Framework::DoorInRoom const * dB, Vector2 const & doorBLoc, Vector2 const & doorBDir, float _doorWidth)
{
	playAreaZone.debug_visualise(dv.get(), Colour::black.with_alpha(0.4f));
	add_door_to_debug_visualiser(dv, dA, doorALoc, doorADir, _doorWidth, Colour::red);
	add_door_to_debug_visualiser(dv, dB, doorBLoc, doorBDir, _doorWidth, Colour::blue);
}

void DebugVisualiserUtils::add_range2_to_debug_visualiser(DebugVisualiserPtr& dv, Range2 const& _range, Colour const& _colour)
{
	dv->add_line(Vector2(_range.x.min, _range.y.min), Vector2(_range.x.min, _range.y.max), _colour);
	dv->add_line(Vector2(_range.x.min, _range.y.max), Vector2(_range.x.max, _range.y.max), _colour);
	dv->add_line(Vector2(_range.x.max, _range.y.max), Vector2(_range.x.max, _range.y.min), _colour);
	dv->add_line(Vector2(_range.x.max, _range.y.min), Vector2(_range.x.min, _range.y.min), _colour);
}

void DebugVisualiserUtils::add_poly_to_debug_visualiser(DebugVisualiserPtr& dv, Array<Vector2> const& _points, Colour const& _colour)
{
	Vector2 lp = _points.get_last();
	for_every(p, _points)
	{
		dv->add_line(lp, *p, _colour);
		lp = *p;
	}
}
