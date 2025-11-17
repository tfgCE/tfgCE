#pragma once

#include "..\..\framework\world\pointOfInterestInstance.h"

namespace TeaForGodEmperor
{
	struct PointOfInterestInstance
	: public Framework::PointOfInterestInstance
	{
		typedef Framework::PointOfInterestInstance base;
	protected:
		override_ void setup_function_spawn(OUT_ Framework::ObjectType * & _objectType, OUT_ Framework::TemporaryObjectType * & _temporaryObjectType);
		override_ void process_function(Name const& function);
	};
};
