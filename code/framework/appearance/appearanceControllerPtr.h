#pragma once

#include "..\..\core\memory\refCountObjectPtr.h"

namespace Framework
{
	class AppearanceController;
	class AppearanceControllerData;

	typedef RefCountObjectPtr<AppearanceController> AppearanceControllerPtr;
	typedef RefCountObjectPtr<AppearanceControllerData> AppearanceControllerDataPtr;
};
