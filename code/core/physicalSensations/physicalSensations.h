#pragma once

#include "sensations.h"

namespace PhysicalSensations
{
	void initialise(bool _autoInit = false);
	void terminate();

	bool is_active();

	Sensation::ID start_sensation(SingleSensation const& _sensation);
	Sensation::ID start_sensation(OngoingSensation const& _sensation);
	void stop_sensation(Sensation::ID _id);
	void stop_all_sensations();

	void advance(float _deltaTime);
};
