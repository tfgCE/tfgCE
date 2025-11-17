#pragma once

#include "..\moduleGameplay.h"
#include "..\energyTransfer.h"
#include "..\..\..\core\types\hand.h"

namespace Framework
{
	class GameInput;
	struct DoorInRoomArrayPtr;
	struct RelativeToPresencePlacement;
};

namespace TeaForGodEmperor
{
	class ModuleEnergyQuantum;
	struct ContinuousDamage;
	struct Damage;
	struct DamageInfo;

	/**
	 *	Hands are currently handled with Pilgrim module
	 *	This is only for transferring energy
	 */
	class ModulePilgrimHand
	: public ModuleGameplay
	, public IEnergyTransfer
	{
		FAST_CAST_DECLARE(ModulePilgrimHand);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_BASE(IEnergyTransfer);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

		ModulePilgrimHand(Framework::IModulesOwner* _owner);
		virtual ~ModulePilgrimHand();

		Hand::Type get_hand() const { return hand; }

		void set_pilgrim(Framework::IModulesOwner* _pilgrim, Hand::Type _hand) { pilgrim = _pilgrim; hand = _hand; }
		void set_health_energy_transfer(Framework::IModulesOwner* _health) { health = _health; }

		void process_energy_quantum_health(ModuleEnergyQuantum* _eq);
		void process_energy_quantum_ammo(ModuleEnergyQuantum* _eq);

	public: // IEnergyTransfer
		implement_ bool handle_ammo_energy_transfer_request(EnergyTransferRequest & _request);
		implement_ bool handle_health_energy_transfer_request(EnergyTransferRequest & _request);
		implement_ Energy calculate_total_energy_available(EnergyType::Type _type) const;
		implement_ Energy calculate_total_max_energy_available(EnergyType::Type _type) const;

	private:
		Hand::Type hand = Hand::MAX;
		SafePtr<Framework::IModulesOwner> pilgrim;
		SafePtr<Framework::IModulesOwner> health;
	};
};

