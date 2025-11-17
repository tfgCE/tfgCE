#include "moduleAI.h"

#include "..\ai\aiCommon.h"

#include "..\game\gameDirector.h"

#include "..\..\framework\module\modules.h"
#include "..\..\framework\module\moduleDataImpl.inl"
#include "..\..\framework\object\object.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// tags
DEFINE_STATIC_NAME(hostileAtGameDirector);
DEFINE_STATIC_NAME(immobileHostileAtGameDirector);

// socket
DEFINE_STATIC_NAME(investigateTarget);

//

REGISTER_FOR_FAST_CAST(ModuleAI);

static Framework::ModuleAI* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleAI(_owner);
}

Framework::RegisteredModule<Framework::ModuleAI> & ModuleAI::register_itself()
{
	return Framework::Modules::ai.register_module(String(TXT("ai")), create_module);
}

ModuleAI::ModuleAI(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

ModuleAI::~ModuleAI()
{
}

void ModuleAI::activate()
{
	register_hostile_to_game_director();
	register_immobile_hostile_to_game_director();

	base::activate();

	if (auto* a = get_owner()->get_appearance())
	{
		investigateSocket.look_up(a->get_mesh(), AllowToFail);
	}
}

void ModuleAI::reset_ai()
{
	unregister_hostile_from_game_director();
	unregister_immobile_hostile_from_game_director();
	update_immobile_hostile_hidden_at_game_director(false, false);
	update_non_hostile_at_game_director(false, false);
	base::reset_ai();
}

void ModuleAI::setup_with(Framework::ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		registerHostileAtGameDirector = false;
		registerImmobileHostileAtGameDirector = false;
		if (auto* ob = get_owner_as_object())
		{
			registerHostileAtGameDirector = ob->get_tags().get_tag_as_int(NAME(hostileAtGameDirector));
			registerImmobileHostileAtGameDirector = ob->get_tags().get_tag_as_int(NAME(immobileHostileAtGameDirector));
		}
		investigateSocket.set_name(NAME(investigateTarget));
	}
}

Transform ModuleAI::calculate_investigate_placement_os() const
{
	if (investigateSocket.is_valid())
	{
		if (auto* a = get_owner()->get_appearance())
		{
			return a->calculate_socket_os(investigateSocket.get_index());
		}
	}
	return get_owner()->get_presence()->get_centre_of_presence_os();
}

void ModuleAI::on_owner_destroy()
{
	unregister_hostile_from_game_director();
	unregister_immobile_hostile_from_game_director();
	update_immobile_hostile_hidden_at_game_director(false, false);
	update_non_hostile_at_game_director(false, false);

	base::on_owner_destroy();
}

void ModuleAI::register_hostile_to_game_director()
{
	if (!registerHostileAtGameDirector ||
		registeredHostileAtGameDirector)
	{
		return;
	}
	if (auto* gd = GameDirector::get())
	{
		gd->register_hostile_ai(get_owner());
		registeredHostileAtGameDirector = true;
	}
}

void ModuleAI::unregister_hostile_from_game_director()
{
	if (!registeredHostileAtGameDirector)
	{
		return;
	}
	if (auto* gd = GameDirector::get())
	{
		gd->unregister_hostile_ai(get_owner());
		registeredHostileAtGameDirector = false;
	}
}

void ModuleAI::update_non_hostile_at_game_director(Optional<bool> const& _switchedSidesToPlayer, Optional<bool> const& _suspended)
{
	if (!registerHostileAtGameDirector)
	{
		return;
	}

	switchedSidesToPlayer = _switchedSidesToPlayer.get(switchedSidesToPlayer);
	suspendedSwitchSides = _suspended.get(suspendedSwitchSides);

	bool shouldBeAsSwitchedSidesToPlayer = switchedSidesToPlayer || suspendedSwitchSides;
	if (registeredAsSwitchedSidesToPlayerAtGameDirector ^ shouldBeAsSwitchedSidesToPlayer)
	{
		if (auto* gd = GameDirector::get())
		{
			gd->mark_hostile_ai_non_hostile(get_owner(), shouldBeAsSwitchedSidesToPlayer);
			registeredAsSwitchedSidesToPlayerAtGameDirector = shouldBeAsSwitchedSidesToPlayer;
		}
	}
}

void ModuleAI::register_immobile_hostile_to_game_director()
{
	if (!registerImmobileHostileAtGameDirector ||
		registeredImmobileHostileAtGameDirector)
	{
		return;
	}
	if (auto* gd = GameDirector::get())
	{
		gd->register_immobile_hostile_ai(get_owner());
		registeredImmobileHostileAtGameDirector = true;
	}
}

void ModuleAI::unregister_immobile_hostile_from_game_director()
{
	if (!registeredImmobileHostileAtGameDirector)
	{
		return;
	}
	if (auto* gd = GameDirector::get())
	{
		gd->unregister_immobile_hostile_ai(get_owner());
		registeredImmobileHostileAtGameDirector = false;
	}
}

void ModuleAI::update_immobile_hostile_hidden_at_game_director(Optional<bool> const& _hidden, Optional<bool> const& _suspended)
{
	if (!registerImmobileHostileAtGameDirector)
	{
		return;
	}

	hiddenImmobileHostile = _hidden.get(hiddenImmobileHostile);
	suspendedImmobileHostile = _suspended.get(suspendedImmobileHostile);

	bool shouldBeHidden = hiddenImmobileHostile || suspendedImmobileHostile;
	if (registeredAsHiddenImmobileHostileAtGameDirector ^ shouldBeHidden)
	{
		if (auto* gd = GameDirector::get())
		{
			gd->mark_immobile_hostile_ai_hidden(get_owner(), shouldBeHidden);
			registeredAsHiddenImmobileHostileAtGameDirector = shouldBeHidden;
		}
	}
}

void ModuleAI::mark_visible_for_game_director(bool _visible)
{
	update_immobile_hostile_hidden_at_game_director(! _visible, NP);
}

void ModuleAI::mark_non_hostile_for_game_director(bool _nonHostile)
{
	update_non_hostile_at_game_director(_nonHostile, NP);
}

void ModuleAI::on_advancement_suspended()
{
	base::on_advancement_suspended();

	update_immobile_hostile_hidden_at_game_director(NP, true);
	update_non_hostile_at_game_director(NP, true);
}

void ModuleAI::on_advancement_resumed()
{
	base::on_advancement_resumed();

	update_immobile_hostile_hidden_at_game_director(NP, false);
	update_non_hostile_at_game_director(NP, false);
}
