#pragma once

#include "..\moduleGameplay.h"

#include "..\..\ai\logics\components\aiLogicComponent_energyTransferPhysSens.h"
#include "..\..\game\energy.h"

#include "..\..\..\framework\presence\relativeToPresencePlacement.h"

namespace TeaForGodEmperor
{
	class ModuleDuctEnergy
	: public ModuleGameplay
	{
		FAST_CAST_DECLARE(ModuleDuctEnergy);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static Framework::RegisteredModule<Framework::ModuleGameplay>& register_itself();

		ModuleDuctEnergy(Framework::IModulesOwner* _owner);
		virtual ~ModuleDuctEnergy();

		void be_active(bool _active) { isActive = _active; }

		static void be_active(Framework::IModulesOwner* imo, bool _active);

	public: // Logic
		implement_ void advance_post_move(float _deltaTime);

	protected: // Module
		override_ void reset(); 
		override_ void setup_with(Framework::ModuleData const* _moduleData);

	private:
		float bayRadius = 0.25f;

		bool isActive = true;

		struct EnergyHandling
		{
			float timeToNextTransfer = 0.0f;
			Energy transferSpeed = Energy(4);
			::Framework::RelativeToPresencePlacement objectInBay;
		};
		EnergyHandling bay;

		enum EnergyTransfer
		{
			None,
			Withdraw,
		};
		EnergyTransfer energyTransfer = EnergyTransfer::None;
		AI::Logics::EnergyTransferPhysSens energyTransferPhysSens;
		Array<SafePtr<Framework::IModulesOwner>> energyTransferTemporaryObjects;

		bool temporaryObjectsRequireReset = false;

		Optional<int> determine_priority(::Framework::IModulesOwner* _object);
					
		void update_object_in_bay(::Framework::IModulesOwner* imo);
		void handle_transfer_energy(float _deltaTime, ::Framework::IModulesOwner* imo);
		void handle_energy_transfer_temporary_objects(EnergyTransfer _prevEnergyTransfer, EnergyTransfer _energyTransfer, ::Framework::IModulesOwner* imo, bool _force);

		Name get_energy_transfer_temporary_object_name(EnergyTransfer _energyTransfer);
	};
};