#pragma once

#include "..\..\..\framework\module\moduleCustom.h"

namespace TeaForGodEmperor
{
	struct PilgrimageDeviceStateVariable
	{
		TypeId typeId = RegisteredType::none();
		Name inStateVariables;
		Name inDevice;
		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
	};

	namespace CustomModules
	{
		class PilgrimageDeviceData;

		class PilgrimageDevice
		: public Framework::ModuleCustom // ModuleMultiplePilgrimageDevice is ModuleGameplay!
		{
			FAST_CAST_DECLARE(PilgrimageDevice);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			PilgrimageDevice(Framework::IModulesOwner* _owner);
			virtual ~PilgrimageDevice();

			void set_guide_towards(Framework::IModulesOwner* _guideTowards); // will also inform open world about changing the guide towards
			Framework::IModulesOwner* get_guide_towards() const { return guideTowards.get(); }

		public:
			void store_pilgrim_device_state_variables(SimpleVariableStorage & _stateVariablesStorage); // get from spawned PDs and store in this one
			void restore_pilgrim_device_state_variables(SimpleVariableStorage const & _stateVariablesStorage); // get from this one and set in spawned PDs

		protected: // Module
			override_ void setup_with(Framework::ModuleData const* _moduleData);

			override_ void initialise();

		protected:
			PilgrimageDeviceData const* pilgrimageDeviceData = nullptr;

			SafePtr<Framework::IModulesOwner> guideTowards;

			struct SubDevice
			{
				Name varId; // var in this object to get value from
				Name objectVarId; // var in object to check
				SafePtr<Framework::IModulesOwner> device;

				Array<PilgrimageDeviceStateVariable> stateVariables; // copied from data
			};
			Array<SubDevice> subDevices;

			void find_sub_devices();
		};

		class PilgrimageDeviceData
		: public Framework::ModuleData
		{
			FAST_CAST_DECLARE(PilgrimageDeviceData);
			FAST_CAST_BASE(Framework::ModuleData);
			FAST_CAST_END();

			typedef Framework::ModuleData base;
		public:
			PilgrimageDeviceData(Framework::LibraryStored* _inLibraryStored);
			virtual ~PilgrimageDeviceData();

		public: // ModuleData
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			override_ void prepare_to_unload();

		protected: // ModuleData
			override_ bool read_parameter_from(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

		private:
			Array<PilgrimageDeviceStateVariable> stateVariables;

			struct SubDevice
			{
				Name varId; // var in this object to get value from
				Name objectVarId; // var in object to check

				Array<PilgrimageDeviceStateVariable> stateVariables; // may remain empty, will use values from
			};
			Array<SubDevice> subDevices;

			friend class PilgrimageDevice;
		};
	};
};

