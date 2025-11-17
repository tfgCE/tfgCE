#pragma once

#include "..\custom\mc_pilgrimageDevice.h"

#include "..\moduleGameplay.h"

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\object\sceneryType.h"

namespace TeaForGodEmperor
{
	class ModuleMultiplePilgrimageDeviceData;

	struct MultiplePilgrimageDevice
	: public RefCountObject
	{
		Name id;
		Framework::UsedLibraryStored<Framework::SceneryType> sceneryType;
		Name poi;
		
		Array<PilgrimageDeviceStateVariable> stateVariables;

		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
	};

	class ModuleMultiplePilgrimageDevice
	: public ModuleGameplay // ModulePilgrimageDevice is ModuleCustom!
	{
		FAST_CAST_DECLARE(ModuleMultiplePilgrimageDevice);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

		ModuleMultiplePilgrimageDevice(Framework::IModulesOwner* _owner);
		virtual ~ModuleMultiplePilgrimageDevice();

	public:
		void spawn_pilgrimage_devices(); // async

		void store_pilgrim_device_state_variables(SimpleVariableStorage& _stateVariablesStorage); // get from spawned PDs and store in this one
		void restore_pilgrim_device_state_variables(SimpleVariableStorage const & _stateVariablesStorage); // get from this one and set in spawned PDs

		bool is_one_of_pilgrimage_devices(Framework::IModulesOwner* _imo) const;

	protected: // Module
		override_ void setup_with(Framework::ModuleData const* _moduleData);

		override_ void initialise();

	protected: // ModuleGameplay
		override_ void advance_post_move(float _deltaTime);

	protected:
		ModuleMultiplePilgrimageDeviceData const* multiplePilgrimageDeviceData = nullptr;

		struct SpawnedDevice
		{
			SafePtr<::Framework::IModulesOwner> object;
			RefCountObjectPtr<MultiplePilgrimageDevice> deviceData;
		};
		Array<SpawnedDevice> devices;
	};

	class ModuleMultiplePilgrimageDeviceData
	: public Framework::ModuleData
	{
		FAST_CAST_DECLARE(ModuleMultiplePilgrimageDeviceData);
		FAST_CAST_BASE(Framework::ModuleData);
		FAST_CAST_END();

		typedef Framework::ModuleData base;
	public:
		ModuleMultiplePilgrimageDeviceData(Framework::LibraryStored* _inLibraryStored);
		virtual ~ModuleMultiplePilgrimageDeviceData();

	public: // ModuleData
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

	protected: // ModuleData
		override_ bool read_parameter_from(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

	private:
		Array<RefCountObjectPtr<MultiplePilgrimageDevice>> devices;

		friend class ModuleMultiplePilgrimageDevice;
	};
};

