#pragma once

#include "physicalSensationsSettings.h"

//

#include "iPhysicalSensations.h"

#ifdef WITH_PHYSICAL_SENSATIONS
#ifdef PHYSICAL_SENSATIONS_OWO
#include "psOWO.h"
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
#include "psBhapticsJava.h"
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
#include "psBhapticsWindows.h"
#endif

#include "bhaptics\bh_library.h"

namespace PhysicalSensations
{
	inline void initialise_static()
	{
#ifdef PHYSICAL_SENSATIONS_OWO
		PhysicalSensations::OWO::splash_logo();
#endif
		bool registerBhapticsLibrary = false;
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
		PhysicalSensations::BhapticsJava::splash_logo();
		registerBhapticsLibrary = true;
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
		PhysicalSensations::BhapticsWindows::splash_logo();
		registerBhapticsLibrary = true;
#endif
		if (registerBhapticsLibrary)
		{
			an_bhaptics::Library::initialise_static();
		}
	}
	inline void close_static()
	{
		an_bhaptics::Library::close_static();
	}
};

#endif
