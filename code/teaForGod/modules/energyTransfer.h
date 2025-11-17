#pragma once

#include "..\..\core\fastCast.h"
#include "..\game\energy.h"
#include "..\teaEnums.h"

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	struct EnergyTransferRequest
	{
		enum Type
		{
			Query, // does handle at all
			QueryDeposit, // does handle deposit
			QueryWithdraw, // does handle withdraw
			Deposit, // add energy
			Withdraw, // remove energy
		};

		EnergyTransferRequest(Type _type = Query) : type(_type) {}

		Type type = Query;

		bool fillOrKill = false; // do the request only if able to transfer whole energy

		int priority = 0; // priority for querry, to know whether given object is more or less important
		Energy minLeft = Energy(0);			// minimum amount of energy that can be left
		Energy energyRequested = Energy(0);	// different for different options
											// for deposit it is an amount to deposit
											// for withdraw it is an amount to withdraw
		Energy energyResult = Energy(0);	// different for different options
											// for deposit it is how much was deposit
											// for withdraw it is how much was withdrawn

		Framework::IModulesOwner* instigator = nullptr;
	};

	/**
	 *	This is not a module but rather an interface of a module.
	 *	Allows to treat an object as source (or destination) of energy.
	 *	Can also be used to transfer energy to different object.
	 */
	interface_class IEnergyTransfer
	{
		FAST_CAST_DECLARE(IEnergyTransfer);
		FAST_CAST_END();

	public:
		virtual bool handle_ammo_energy_transfer_request(EnergyTransferRequest & _request) { return false; } // returns true if valid, handable etc
		virtual bool handle_health_energy_transfer_request(EnergyTransferRequest & _request) { return false; } // as above
		bool handle_energy_transfer_request(EnergyType::Type _energyType, EnergyTransferRequest& _request);
		bool handle_any_energy_transfer_request(EnergyTransferRequest& _request);
		virtual Energy calculate_total_energy_available(EnergyType::Type _type) const { return Energy::zero(); }
		virtual Energy calculate_total_max_energy_available(EnergyType::Type _type) const { return Energy::zero(); }

		static IEnergyTransfer* get_energy_transfer_module_from(Framework::IModulesOwner* _owner, EnergyType::Type _energyType);
		static IEnergyTransfer* get_any_energy_transfer_module_from(Framework::IModulesOwner* _owner);
		static Energy calculate_total_energy_available_for(Framework::IModulesOwner* _owner);
		// if not deposit, withdraw, if not set, general query
		static bool does_accept_energy_transfer(EnergyType::Type _energyType, Framework::IModulesOwner* _owner, Optional<bool> const & _deposit = NP);
		static bool does_accept_energy_transfer_ammo(Framework::IModulesOwner* _owner, Optional<bool> const & _deposit = NP);
		static bool does_accept_energy_transfer_health(Framework::IModulesOwner* _owner, Optional<bool> const & _deposit = NP);
		static bool does_accept_energy_transfer(EnergyType::Type _energyType, IEnergyTransfer* _iet, Optional<bool> const & _deposit = NP);
		static bool does_accept_energy_transfer_ammo(IEnergyTransfer* _iet, Optional<bool> const & _deposit = NP);
		static bool does_accept_energy_transfer_health(IEnergyTransfer* _iet, Optional<bool> const & _deposit = NP);

		static bool transfer_energy_between_two(Framework::IModulesOwner* _instigator, Energy const& _amount, IEnergyTransfer* _from, EnergyType::Type _fromType, IEnergyTransfer* _to, EnergyType::Type _toType); // returns true if anything transferred
		static bool transfer_energy_between_two_monitor_energy(Framework::IModulesOwner* _instigator, Energy& _amount, IEnergyTransfer* _from, EnergyType::Type _fromType, IEnergyTransfer* _to, EnergyType::Type _toType); // returns true if anything transferred, alters _amount
	};
};
