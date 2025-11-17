#include "me_shield.h"

#include "..\..\..\game\game.h"

#include "..\moduleEnergyQuantum.h"

#include "..\..\custom\mc_pickup.h"
#include "..\..\custom\health\mc_health.h"

#include "..\..\..\..\framework\debug\previewGame.h"
#include "..\..\..\..\framework\display\display.h"
#include "..\..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\..\framework\display\displayText.h"
#include "..\..\..\..\framework\display\displayUtils.h"
#include "..\..\..\..\framework\module\modules.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"


using namespace TeaForGodEmperor;
using namespace ModuleEquipments;

//

// module params
DEFINE_STATIC_NAME(beRetractedVarID);
DEFINE_STATIC_NAME(beShieldVarID);
DEFINE_STATIC_NAME(beShieldLeftHandVarID);
DEFINE_STATIC_NAME(beShieldRightHandVarID);
DEFINE_STATIC_NAME(shieldCoverSocket);

// movement
DEFINE_STATIC_NAME(immovable);

//

REGISTER_FOR_FAST_CAST(Shield);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new Shield(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ShieldData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & Shield::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("shield")), create_module, create_module_data);
}

Shield::Shield(Framework::IModulesOwner* _owner)
: base( _owner )
{
	allowEasyReleaseForAutoInterimEquipment = true;
	rotatedDisplay = true;
}

Shield::~Shield()
{
}

void Shield::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	shieldData = fast_cast<ShieldData>(_moduleData);

	beRetractedVar.set_name(_moduleData->get_parameter<Name>(this, NAME(beRetractedVarID), beRetractedVar.get_name()));
	beShieldVar.set_name(_moduleData->get_parameter<Name>(this, NAME(beShieldVarID), beShieldVar.get_name()));
	beShieldLeftHandVar.set_name(_moduleData->get_parameter<Name>(this, NAME(beShieldLeftHandVarID), beShieldLeftHandVar.get_name()));
	beShieldRightHandVar.set_name(_moduleData->get_parameter<Name>(this, NAME(beShieldRightHandVarID), beShieldRightHandVar.get_name()));

	shieldCoverSocket.set_name(_moduleData->get_parameter<Name>(this, NAME(shieldCoverSocket), shieldCoverSocket.get_name()));
}

void Shield::reset()
{
	base::reset();
}

void Shield::initialise()
{
	base::initialise();

	beRetractedVar.look_up<float>(get_owner()->access_variables());
	beShieldVar.look_up<float>(get_owner()->access_variables());
	beShieldLeftHandVar.look_up<float>(get_owner()->access_variables());
	beShieldRightHandVar.look_up<float>(get_owner()->access_variables());

	if (auto* ap = get_owner()->get_appearance())
	{
		shieldCoverSocket.look_up(ap->get_mesh());
	}
}

void Shield::advance_post_move(float _deltaTime)
{
	base::advance_post_move(_deltaTime);

#ifdef AN_DEVELOPMENT
	if (Game::get_as<Framework::PreviewGame>())
	{
		return;
	}
#endif

	auto & beRetracted = beRetractedVar.access<float>();
	auto & beShield = beShieldVar.access<float>();
	auto & beShieldLeftHand = beShieldLeftHandVar.access<float>();
	auto & beShieldRightHand = beShieldRightHandVar.access<float>();

	beRetracted = 0.0f;

	if (!user)
	{
		if (auto * m = get_owner()->get_movement())
		{
			if (m->get_name() == NAME(immovable))
			{
				if (auto * c = get_owner()->get_collision())
				{
					if (!c->get_collided_with().is_empty())
					{
						beRetracted = 1.0f;
					}
				}
			}
		}
	}
	
	if (auto* pickup = get_owner()->get_custom<CustomModules::Pickup>())
	{
		if (pickup->is_in_pocket() || pickup->is_in_holder())
		{
			beRetracted = 1.0f;
		}
	}


	beShield = user ? 1.0f : 0.0f;
	beShieldLeftHand = user && get_hand() == Hand::Left ? 1.0f : 0.0f;
	beShieldRightHand = user && get_hand() == Hand::Right ? 1.0f : 0.0f;
}

void Shield::advance_user_controls()
{
	base::advance_user_controls();

	if (!user)
	{
		return;
	}
}

void Shield::ready_energy_quantum_context(EnergyQuantumContext & _eqc) const
{
	base::ready_energy_quantum_context(_eqc);

	// consume as health
	_eqc.energy_type_override(EnergyType::Health); // this doubles functionality with grabAnyEnergyAsHealth variable
	_eqc.health_receiver(get_owner());
	_eqc.side_effects_health_receiver(get_owner());
}

Optional<Energy> Shield::get_primary_state_value() const
{
	if (auto* h = get_owner()->get_custom<CustomModules::Health>())
	{
		return h->get_health();
	}
	else
	{
		return NP;
	}
}

Vector3 Shield::get_cover_dir_os() const
{
	if (shieldCoverSocket.is_valid())
	{
		if (auto* ap = get_owner()->get_appearance())
		{
			Transform placement = ap->calculate_socket_os(shieldCoverSocket.get_index());
			return placement.get_axis(Axis::Forward);
		}
	}
	return Vector3::yAxis;
}

//

REGISTER_FOR_FAST_CAST(ShieldData);

ShieldData::ShieldData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

ShieldData::~ShieldData()
{
}

bool ShieldData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool ShieldData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

void ShieldData::prepare_to_unload()
{
	base::prepare_to_unload();
}