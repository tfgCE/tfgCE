#include "mc_switchSidesHandler.h"

#include "mc_emissiveControl.h"
#include "mc_pickup.h"

#include "..\moduleAI.h"
#include "..\gameplay\modulePilgrim.h"

#include "..\..\game\gameDirector.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiSocial.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\object.h"

//#define AN_HACK_AUTO_SWITCH_SIDES_ALWAYS
//#define AN_HACK_AUTO_SWITCH_SIDES_IN_ROOM
//#define AN_HACK_AUTO_SWITCH_SIDES_IN_ROOM_KEY_REQUIRED

#ifdef AN_HACK_AUTO_SWITCH_SIDES_ALWAYS
#define AN_HACK_AUTO_SWITCH_SIDES
#else
#ifdef AN_HACK_AUTO_SWITCH_SIDES_IN_ROOM
#define AN_HACK_AUTO_SWITCH_SIDES
#endif
#endif

#ifdef AN_HACK_AUTO_SWITCH_SIDES
#include "..\..\game\game.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\core\vr\iVR.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// module params
DEFINE_STATIC_NAME(hasToBeHeld);
DEFINE_STATIC_NAME(switchOnUseUsableByHolder);
DEFINE_STATIC_NAME(interactiveDeviceIdVar);
DEFINE_STATIC_NAME(emissiveOnSwitchSides);

//

REGISTER_FOR_FAST_CAST(SwitchSidesHandler);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new SwitchSidesHandler(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & SwitchSidesHandler::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("switchSidesHandler")), create_module);
}

SwitchSidesHandler::SwitchSidesHandler(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

SwitchSidesHandler::~SwitchSidesHandler()
{
}

void SwitchSidesHandler::reset()
{
	base::reset();
}

void SwitchSidesHandler::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	hasToBeHeld = _moduleData->get_parameter<bool>(this, NAME(hasToBeHeld), hasToBeHeld);
	switchOnUseUsableByHolder = _moduleData->get_parameter<bool>(this, NAME(switchOnUseUsableByHolder), switchOnUseUsableByHolder);
	interactiveDeviceIdVar = _moduleData->get_parameter<Name>(this, NAME(interactiveDeviceIdVar), interactiveDeviceIdVar);
	emissiveOnSwitchSides = _moduleData->get_parameter<Name>(this, NAME(emissiveOnSwitchSides), emissiveOnSwitchSides);

	if (!interactiveDeviceIdVar.is_valid())
	{
		error(TXT("no \"interactiveDeviceIdVar\" set for \"%S\""), get_owner()->ai_get_name().to_char());
	}
}

void SwitchSidesHandler::on_owner_destroy()
{
	base::on_owner_destroy();
}

void SwitchSidesHandler::activate()
{
	base::activate();

	interactiveButtonHandler.has_to_be_held(hasToBeHeld);
	interactiveButtonHandler.initialise(get_owner(), interactiveDeviceIdVar);

	mark_requires(all_customs__advance_post);

#ifdef AN_HACK_AUTO_SWITCH_SIDES_ALWAYS
	if (!VR::IVR::can_be_used() && Game::get_as<Game>())
	{
		switchSidesTo = Game::get_as<Game>()->access_player().get_actor();
	}
#endif
}

void SwitchSidesHandler::advance_post(float _deltaTime)
{
	base::advance_post(_deltaTime);

	rarer_post_advance_if_not_visible();

	bool prevSwitchSidesTo = get_switch_sides_to() != nullptr;

#ifdef AN_HACK_AUTO_SWITCH_SIDES_IN_ROOM
#ifdef AN_HACK_AUTO_SWITCH_SIDES_IN_ROOM_KEY_REQUIRED
#ifdef AN_STANDARD_INPUT
	if (::System::Input::get()->has_key_been_pressed(::System::Key::G))
#endif
#endif
	if (!get_switch_sides_to())
	{
		if (auto* g = Game::get_as<Game>())
		{
			if (auto* pa = g->access_player().get_actor())
			{
				if (pa->get_presence()->get_in_room() == get_owner()->get_presence()->get_in_room())
				{
					switchSidesTo = pa;
					// copied
					if (auto* ai = get_owner()->get_ai())
					{
						auto* mind = ai->get_mind();
						mind->access_social().no_longer_be_enemy(switchSidesTo.get());
						Optional<Name> faction;
						if (auto* imo = switchSidesTo.get())
						{
							if (auto* ai = imo->get_ai())
							{
								auto* mind = ai->get_mind();
								faction = mind->get_social().get_faction();
							}
						}
						mind->access_social().set_faction(faction); // set or reset
						if (auto* mai = fast_cast<ModuleAI>(ai))
						{
							mai->mark_non_hostile_for_game_director(Framework::GameUtils::is_controlled_by_player(switchSidesTo.get()));
						}
					}
				}
			}
		}
	}
#endif

	interactiveButtonHandler.advance(_deltaTime);

	bool buttonTriggered = interactiveButtonHandler.is_triggered();
	if (switchOnUseUsableByHolder)
	{
		if (auto* asPickup = get_owner()->get_custom<CustomModules::Pickup>())
		{
			if (auto* heldBy = asPickup->get_held_by_owner())
			{
				if (auto* pilgrim = heldBy->get_gameplay_as<ModulePilgrim>())
				{
					auto heldingHand = pilgrim->get_helding_hand(get_owner());
					if (heldingHand.is_set())
					{
						if (pilgrim->has_controls_use_as_usable_equipment_been_pressed(heldingHand.get()))
						{
							buttonTriggered = true;
						}
					}
				}
			}
		}
		else
		{
			an_assert(false, TXT("works only for pickups"));
		}
	}

	if (buttonTriggered)
	{
		if (switchSidesTo.is_pointing_at_something())
		{
			get_switch_sides_to(nullptr);
		}
		else
		{
			if (auto* asPickup = get_owner()->get_custom<CustomModules::Pickup>())
			{
				switchSidesTo = asPickup->get_held_by_owner();
			}
			get_switch_sides_to(switchSidesTo.get());
		}
	}

	if (emissiveOnSwitchSides.is_valid())
	{
		bool newSwitchSidesTo = get_switch_sides_to() != nullptr;
		if (newSwitchSidesTo ^ prevSwitchSidesTo)
		{
			update_emissives();
		}
	}
}

void SwitchSidesHandler::get_switch_sides_to(Framework::IModulesOwner* _toSide)
{
	if (!_toSide)
	{
		get_owner()->clear_game_related_system_flag(ModulesOwnerFlags::BlockSpawnManagerAutoCease);
		switchSidesTo.clear();
		if (auto* ai = get_owner()->get_ai())
		{
			auto* mind = ai->get_mind();
			mind->access_social().set_sociopath();
			mind->access_social().set_endearing();
			mind->access_social().set_faction(); // reset
			if (auto* mai = fast_cast<ModuleAI>(ai))
			{
				mai->mark_non_hostile_for_game_director(false);
			}
		}
	}
	else
	{
		get_owner()->set_game_related_system_flag(ModulesOwnerFlags::BlockSpawnManagerAutoCease);
		switchSidesTo = _toSide;
		if (auto* ai = get_owner()->get_ai())
		{
			auto* mind = ai->get_mind();
			mind->access_social().no_longer_be_enemy(switchSidesTo.get());
			Optional<Name> faction;
			if (auto* imo = switchSidesTo.get())
			{
				if (auto* ai = imo->get_ai())
				{
					auto* mind = ai->get_mind();
					faction = mind->get_social().get_faction();
				}
			}
			mind->access_social().set_sociopath(faction.is_set() ? Optional<bool>(false) : NP); // set or reset
			mind->access_social().set_endearing(faction.is_set() ? Optional<bool>(false) : NP); // set or reset
			mind->access_social().set_faction(faction); // set or reset
			if (auto* mai = fast_cast<ModuleAI>(ai))
			{
				mai->mark_non_hostile_for_game_director(Framework::GameUtils::is_controlled_by_player(switchSidesTo.get()));
			}
		}
	}

	update_emissives();
}

void SwitchSidesHandler::update_emissives()
{
	bool newSwitchSidesTo = get_switch_sides_to() != nullptr;

	if (auto* em = get_owner()->get_custom<CustomModules::EmissiveControl>())
	{
		if (newSwitchSidesTo)
		{
			em->emissive_activate(emissiveOnSwitchSides);
		}
		else
		{
			em->emissive_deactivate(emissiveOnSwitchSides);
		}
	}
	for_every_ref(imo, interactiveButtonHandler.get_buttons())
	{
		if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
		{
			if (newSwitchSidesTo)
			{
				em->emissive_activate(emissiveOnSwitchSides);
			}
			else
			{
				em->emissive_deactivate(emissiveOnSwitchSides);
			}
		}
	}
}
