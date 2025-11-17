#pragma once

#include "..\library\libraryCustomisation.h"
#include "..\render\renderCustomisation.h"
#include "..\world\pointOfInterestInstance.h"

namespace Framework
{
	class ObjectType;
	class TemporaryObjectType;
	struct PointOfInterestInstance;

	struct GameCustomisation
	{
		LibraryCustomisation library;
		Render::Customisation render;

		std::function<PointOfInterestInstance*()> create_point_of_interest_instance = []() { return new PointOfInterestInstance(); };
	};
};

