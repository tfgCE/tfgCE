#pragma once

#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\framework\module\moduleData.h"
#include "..\..\..\core\types\colour.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class CarrierData;

		/**
		 *	Carries other objects (drop ships, drop capsules)
		 */
		class Carrier
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(Carrier);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			Carrier(Framework::IModulesOwner* _owner);
			virtual ~Carrier();

		public:
			bool can_carry_more() const;
			bool carry(Framework::IModulesOwner* _imo);
			void drop(Framework::IModulesOwner* _imo);

			void give_zero_velocity_to_dropped() { giveZeroVelocityToDropped = true; }

		public:	// Module
			override_ void setup_with(::Framework::ModuleData const * _moduleData);
			override_ void on_owner_destroy();

		private:
			CarrierData const * carrierData = nullptr;

			bool giveZeroVelocityToDropped = false;

			Array<SafePtr<Framework::IModulesOwner>> atCarryPoint;
		};

		class CarrierData
		: public Framework::ModuleData
		{
			FAST_CAST_DECLARE(CarrierData);
			FAST_CAST_BASE(Framework::ModuleData);
			FAST_CAST_END();

			typedef Framework::ModuleData base;
		public:
			CarrierData(Framework::LibraryStored* _inLibraryStored);
			virtual ~CarrierData();

		protected: // Framework::ModuleData
			override_ bool read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);

		private:
			struct CarryPoint
			{
				Name id;
				Name socket;

				Name carriedVarID; // variable in carried object to be set

				bool suspendAI = false;
				bool hidden = false;

				bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			};
			Array<CarryPoint> carryPoints;

			friend class Carrier;
		};
	};
};

