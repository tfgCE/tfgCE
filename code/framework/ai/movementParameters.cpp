#include "movementParameters.h"

//

using namespace Framework;

//

MovementParameters::MovementParameters()
{
}

MovementParameters & MovementParameters::stop()
{
	absoluteSpeed.set(0.0f);
	relativeSpeed.clear();

	return *this;
}

MovementParameters & MovementParameters::full_speed()
{
	return relative_speed(1.0f);
}

MovementParameters & MovementParameters::absolute_speed(float _absoluteSpeed)
{
	absoluteSpeed.set(_absoluteSpeed);
	relativeSpeed.clear();

	return *this;
}

MovementParameters& MovementParameters::relative_speed(float _relativeSpeed)
{
	absoluteSpeed.clear();
	relativeSpeed.set(_relativeSpeed);

	return *this;
}

MovementParameters & MovementParameters::gait(Name const & _gaitName)
{
	gaitName = _gaitName;

	return *this;
}

MovementParameters & MovementParameters::default_gait()
{
	return gait(Name::invalid());
}

MovementParameters & MovementParameters::relative_z(float _relativeZ)
{
	relativeZ = _relativeZ;
	return *this;
}
