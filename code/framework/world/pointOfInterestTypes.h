#pragma once

#include "..\..\core\memory\refCountObjectPtr.h"

namespace Framework
{
	struct PointOfInterest;
	struct PointOfInterestInstance;

	typedef RefCountObjectPtr<PointOfInterest> PointOfInterestPtr;
	typedef RefCountObjectPtr<PointOfInterestInstance> PointOfInterestInstancePtr;

	typedef std::function< void(PointOfInterestInstance * _poi) > OnFoundPointOfInterest;
	typedef std::function< bool(PointOfInterestInstance const * _poi) > IsPointOfInterestValid;
};
