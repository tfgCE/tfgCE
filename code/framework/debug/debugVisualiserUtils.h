#pragma once

#include "..\..\core\debug\debugVisualiser.h"

struct Vector2;
struct Colour;

namespace VR
{
	struct Zone;
};

namespace Framework
{
	class DoorInRoom;

	class DebugVisualiserUtils
	{
	public:
		// doorDirs are outbound
		static void add_door_to_debug_visualiser(DebugVisualiserPtr & dv, Framework::DoorInRoom const * dA, Vector2 const & doorALoc, Vector2 const & doorADir, float _doorWidth, Colour const & _colour);
		static void add_doors_to_debug_visualiser(DebugVisualiserPtr & dv, VR::Zone const & playAreaZone, Framework::DoorInRoom const * dA, Vector2 const & doorALoc, Vector2 const & doorADir, Framework::DoorInRoom const * dB, Vector2 const & doorBLoc, Vector2 const & doorBDir, float _doorWidth);

		static void add_range2_to_debug_visualiser(DebugVisualiserPtr& dv, Range2 const& _range, Colour const& _colour);
		static void add_poly_to_debug_visualiser(DebugVisualiserPtr& dv, Array<Vector2> const& _points, Colour const& _colour);
	};
};

