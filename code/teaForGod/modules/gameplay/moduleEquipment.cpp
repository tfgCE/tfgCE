#include "moduleEquipment.h"

#include "moduleEnergyQuantum.h"
#include "modulePilgrim.h"

#include "..\..\..\framework\module\modules.h"

using namespace TeaForGodEmperor;

//

// module param
DEFINE_STATIC_NAME(nonInteractive);

//

REGISTER_FOR_FAST_CAST(ModuleEquipment);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleEquipment(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModuleEquipmentData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & ModuleEquipment::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("equipment")), create_module, create_module_data);
}

ModuleEquipment::ModuleEquipment(Framework::IModulesOwner* _owner)
: base( _owner )
{
}

ModuleEquipment::~ModuleEquipment()
{
}

void ModuleEquipment::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	equipmentData = fast_cast<ModuleEquipmentData>(_moduleData);

	nonInteractive = _moduleData->get_parameter<bool>(this, NAME(nonInteractive), nonInteractive);

	releaseVelocity = equipmentData->releaseVelocity;
}

void ModuleEquipment::reset()
{
	base::reset();
}

void ModuleEquipment::initialise()
{
	base::initialise();
}

void ModuleEquipment::advance_post_move(float _deltaTime)
{
	timeSinceLastKill += _deltaTime;
	if (timeInHand.is_set())
	{
		timeInHand = timeInHand.get() + _deltaTime;
	}
}

void ModuleEquipment::advance_user_controls()
{
}

void ModuleEquipment::be_main_equipment(bool _isMainEquipment, Optional<Hand::Type> const& _hand)
{
	MODULE_OWNER_LOCK(TXT("ModuleEquipment::be_main_equipment"));
	
	isMainEquipment = _isMainEquipment;
	if (isMainEquipment)
	{
		an_assert(_hand.is_set());
		mainEquipmentHand = _hand;
	}
	else
	{
		mainEquipmentHand.clear();
	}
}

void ModuleEquipment::set_user(ModulePilgrim* _user, Optional<Hand::Type> const & _hand)
{
	MODULE_OWNER_LOCK(TXT("ModuleEquipment::set_user")); // otherwise we may end with a situation when we set user while gun is advancing (post move)

	an_assert(!_user || _hand.is_set(), TXT("if user is set, hand to be set too"));
	auto* prevUser = user;
	auto prevHand = hand;
	user = _user;
	hand = _hand;
	if (_user && _hand.is_set())
	{
		timeInHand = 0.0f;
	}
	else
	{
		timeInHand.clear();
	}
	set_update_with_attached_to(user != nullptr); // if possible we may want to update with the owner
	on_change_user(_user, _hand, prevUser, prevHand);
}

Framework::IModulesOwner* ModuleEquipment::get_user_as_modules_owner() const
{
	return user ? user->get_owner() : nullptr;
}

void ModuleEquipment::ready_energy_quantum_context(EnergyQuantumContext & _eqc) const
{
	// won't be used?
}

void ModuleEquipment::process_energy_quantum_ammo(ModuleEnergyQuantum* _eq)
{
	an_assert(_eq->is_being_processed_by_us());
	// won't be used?
}

void ModuleEquipment::process_energy_quantum_health(ModuleEnergyQuantum* _eq)
{
	an_assert(_eq->is_being_processed_by_us());
	// won't be used?
}

//

REGISTER_FOR_FAST_CAST(ModuleEquipmentData);

ModuleEquipmentData::ModuleEquipmentData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

ModuleEquipmentData::~ModuleEquipmentData()
{
}

bool ModuleEquipmentData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	releaseVelocity.load_from_xml_child_node(_node, TXT("releaseVelocity"));

	return result;
}

bool ModuleEquipmentData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

void ModuleEquipmentData::prepare_to_unload()
{
	base::prepare_to_unload();
}