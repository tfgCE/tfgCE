#pragma once

#include "iPhysicalSensations.h"

#include "..\network\netClientTCP.h"

namespace PhysicalSensations
{
	class OWO
	: public IPhysicalSensations
	{
		FAST_CAST_DECLARE(OWO);
		FAST_CAST_BASE(IPhysicalSensations);
		FAST_CAST_END();

		typedef IPhysicalSensations base;

	public:
		static void splash_logo();
		
		static Name const& system_id();
		static bool may_auto_start() { return false; }

		static void start();
		static int default_owo_port() { return 54010; }

		OWO(Network::Address const& _address);
		virtual ~OWO();

		bool is_connecting() const { return isConnecting; }

	protected: // IPhysicalSensations
		implement_ void async_init();

		implement_ void internal_start_sensation(REF_ SingleSensation & _sensation);
		implement_ void internal_start_sensation(REF_ OngoingSensation & _sensation);
		implement_ void internal_stop_sensation(Sensation::ID _id);
		implement_ bool internal_stop_all_sensations();

	protected:
		Concurrency::SpinLock initialising = Concurrency::SpinLock(TXT("PhysicalSensations.OWO.initialising"));

		Network::Address address;
		Network::ClientTCP client;
		bool isConnecting = false;
	};

};
