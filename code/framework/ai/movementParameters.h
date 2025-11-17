#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\types\name.h"
#include "..\..\core\types\optional.h"

namespace Framework
{
	struct MovementParameters
	{
		Optional<float> absoluteSpeed; // overrides relative speed as request for something more accurate
		Optional<float> relativeSpeed;
		Optional<float> relativeZ; // relative z location (useful for flyers)
		Name gaitName;

		MovementParameters();
		MovementParameters & stop();
		MovementParameters & full_speed();
		MovementParameters & absolute_speed(float _absoluteSpeed);
		MovementParameters & relative_speed(float _relativeSpeed);
		MovementParameters & gait(Name const & _gaitName);
		MovementParameters & default_gait();
		MovementParameters & relative_z(float _relativeZ);
	};
};
