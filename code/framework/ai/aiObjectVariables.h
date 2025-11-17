#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\concurrency\spinLock.h"
#include "..\..\core\concurrency\scopedSpinLock.h"

//

/**
 *	LATENT_OBJECT_VAR_BEGIN();
 *	LATENT_OBJECT_VAR(Framework::PresencePath, enemy);
 *	LATENT_OBJECT_VAR(float, aggressiveness);
 *	LATENT_OBJECT_VAR_END();
 */

#define LATENT_OBJECT_VAR_BEGIN() \
	LATENT_VAR(bool, objectVarsInitialised);

// uses spin lock to have static name inside declared and registered
#define	LATENT_OBJECT_VAR(type, var) \
	LATENT_VAR(type*, var##Ptr); \
	if (! objectVarsInitialised) \
	{ \
		static Concurrency::SpinLock lockName = Concurrency::SpinLock(TXT("LATENT_OBJECT_VAR")); \
		Concurrency::ScopedSpinLock lock(lockName); \
		{ \
			DEFINE_STATIC_NAME(var); \
			SimpleVariableInfo tempVar = mind->get_owner_as_modules_owner()->access_variables().find<type>(NAME(var)); \
			var##Ptr = &tempVar.access<type>(); \
		} \
	} \
	type & var = *(var##Ptr);

#define LATENT_OBJECT_VAR_END() \
	objectVarsInitialised = true;

//
