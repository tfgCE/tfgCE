#pragma once

#include "iPhysicalSensations.h"

#include "..\network\netClientTCP.h"

#include "bhapticsJava\bhj_bhapticsModule.h"

namespace PhysicalSensations
{
	class BhapticsJava
	: public IPhysicalSensations
	{
		FAST_CAST_DECLARE(BhapticsJava);
		FAST_CAST_BASE(IPhysicalSensations);
		FAST_CAST_END();

		typedef IPhysicalSensations base;

	public:
		static void splash_logo();
		
		static Name const& system_id();
		static bool may_auto_start() { return true; }

		static void start();

		BhapticsJava();
		virtual ~BhapticsJava();

		bhapticsJava::BhapticsModule* get_bhaptics() const { return bhapticsModule; }

	protected: // IPhysicalSensations
		implement_ void async_init();

		implement_ void internal_start_sensation(REF_ SingleSensation & _sensation);
		implement_ void internal_start_sensation(REF_ OngoingSensation & _sensation);
		implement_ void internal_sustain_sensation(Sensation::ID _id);
		implement_ void internal_stop_sensation(Sensation::ID _id);
		implement_ bool internal_stop_all_sensations();

	protected:
		Concurrency::SpinLock initialising = Concurrency::SpinLock(TXT("PhysicalSensations.BhapticsJava.initialising"));

		bhapticsJava::BhapticsModule* bhapticsModule = nullptr;

		struct OngoingSensationInfo
		{
			Sensation::ID id = NONE;
			Name sensationId;
			bool vest = false;
			float offsetYaw = 0.0f;
			float offsetZ = 0.0f;
		};
		Array<OngoingSensationInfo> ongoingSensations;
	};

};
