#pragma once

#include "..\energyTransfer.h"

#include "..\..\game\gameStateSensitive.h"

#include "..\..\..\framework\module\moduleCustom.h"

namespace Framework
{
	class GameInput;
	struct DoorInRoomArrayPtr;
	struct RelativeToPresencePlacement;
};

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class EnergyStorageData;

		class EnergyStorage
		: public Framework::ModuleCustom
		, public IEnergyTransfer
		, public IGameStateSensitive
		{
			FAST_CAST_DECLARE(EnergyStorage);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_BASE(IEnergyTransfer);
			FAST_CAST_BASE(IGameStateSensitive);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			EnergyStorage(Framework::IModulesOwner* _owner);
			virtual ~EnergyStorage();

		public: // continuous transfer
			bool begin_transfer(Framework::IModulesOwner* _imo);
			Energy transfer_energy_to(Framework::IModulesOwner* _imo, float _deltaTime); // returns transferred energy
			void end_transfer(Framework::IModulesOwner* _imo);

		public: // IEnergyTransfer
			implement_ bool handle_ammo_energy_transfer_request(EnergyTransferRequest & _request);
			implement_ bool handle_health_energy_transfer_request(EnergyTransferRequest & _request);
			implement_ Energy calculate_total_energy_available(EnergyType::Type _type) const;
			implement_ Energy calculate_total_max_energy_available(EnergyType::Type _type) const;

		protected: // Module
			override_ void reset();
			override_ void setup_with(Framework::ModuleData const* _moduleData);

			override_ void initialise();

		public: // IGameStateSensitive
			implement_ void store_for_game_state(SimpleVariableStorage& _variables) const;
			implement_ void restore_from_game_state(SimpleVariableStorage const& _variables);

		private:
			EnergyStorageData const* energyStorageData = nullptr;

			EnergyType::Type energyType = EnergyType::Ammo;
			Energy amount = Energy::zero();
			EnergyRange startingAmount = EnergyRange::empty;
			Energy capacity = Energy(10);

			Name transferTemporaryObjectId;
			RUNTIME_ SafePtr< Framework::IModulesOwner> transfertemporaryObject; // only one at the time

			Name emissivePowerLevel;

			Energy transferSpeed = Energy(10);
			RUNTIME_ SafePtr< Framework::IModulesOwner> transferRecipent; // only one at the time
			RUNTIME_ float timeToNextTransfer = 0.0f;

			void reset_energy();

			bool handle_energy_transfer_request(EnergyType::Type _transferEnergy, EnergyTransferRequest& _request);

			void update_emissive(Optional<bool> const & _active = NP);

			void transferring_energy();
			void not_transferring_energy();

			void manage_auto_clear_update_emissive();
		};

		class EnergyStorageData
		: public Framework::ModuleData
		{
			FAST_CAST_DECLARE(EnergyStorageData);
			FAST_CAST_BASE(Framework::ModuleData);
			FAST_CAST_END();

			typedef Framework::ModuleData base;
		public:
			static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored);
				
			EnergyStorageData(Framework::LibraryStored* _inLibraryStored);
			virtual ~EnergyStorageData();

		protected: // Framework::ModuleData
			override_ bool read_parameter_from(IO::XML::Attribute const * _attr, Framework::LibraryLoadingContext & _lc);
			override_ bool read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

		private:
			Random::Number<Energy> startingAmount;

			friend class EnergyStorage;
		};
	};
};

