#include "activationGroup.h"

#include "world.h"
#include "worldObject.h"

#include "..\framework.h"

#include "..\game\game.h"

#include "..\..\core\performance\performanceUtils.h"

using namespace Framework;

//

Concurrency::Counter ActivationGroup::s_activationGroupId;

ActivationGroup::ActivationGroup()
: activationGroupId(s_activationGroupId.increase())
{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("[ActivationGroup] new activation group [%i]"), get_id());
#endif
}

//

ScopedAutoActivationGroup::ScopedAutoActivationGroup(WorldObject* _wo, WorldObject* _secondaryWo, World* _world, bool _activateNow)
{
	an_assert(_wo || _secondaryWo || _world);
	initial_setup(_wo, _secondaryWo, _world, _activateNow);
}

ScopedAutoActivationGroup::ScopedAutoActivationGroup(WorldObject* _wo, WorldObject* _secondaryWo, bool _activateNow)
{
	an_assert(_wo || _secondaryWo);
	initial_setup(_wo, _secondaryWo, nullptr, _activateNow);
}

ScopedAutoActivationGroup::ScopedAutoActivationGroup(World* _world, bool _activateNow)
{
	an_assert(_world);

	initial_setup(nullptr, nullptr, _world, _activateNow);
}

void ScopedAutoActivationGroup::initial_setup(WorldObject* _wo, WorldObject* _secondaryWo, World* _world, bool _activateNow)
{
	activateNow = _activateNow;
	world = _world;
	if (_wo || _secondaryWo)
	{
		world = _wo ? _wo->get_in_world() : (_secondaryWo ? _secondaryWo->get_in_world() : nullptr);
	}
	an_assert(world);
	if (!activateNow && _wo)
	{
		activateNow |= _wo->is_world_active();
	}
	if (!activateNow && _secondaryWo)
	{
		activateNow |= _secondaryWo->is_world_active();
	}
	// being a part of an activation group will set activateNow to false
	if (!activationGroup.is_set() && _wo)
	{
		activationGroup = _wo->get_activation_group();
#ifdef INSPECT_ACTIVATION_GROUP
		if (activationGroup.get())
		{
			output(TXT("[ScopedAutoActivationGroup] sharing activation group with first world object [%i]"), activationGroup->get_id());
		}
#endif
	}
	if (!activationGroup.is_set() && _secondaryWo)
	{
		activationGroup = _secondaryWo->get_activation_group();
#ifdef INSPECT_ACTIVATION_GROUP
		if (activationGroup.get())
		{
			output(TXT("[ScopedAutoActivationGroup] sharing activation group with second world object [%i]"), activationGroup->get_id());
		}
#endif
	}

	//

	if (activationGroup.is_set())
	{
		// we're part of an activation group, no need to activate now
		activateNow = false;
	}

	an_assert(activateNow || activationGroup.is_set(), TXT("if not activating now, we are required to have activation group of owner"));
	// new object has to have access to activation group through get_current_activation_group
	if (!activationGroup.is_set())
	{
		activationGroup = Game::get()->push_new_activation_group();
#ifdef INSPECT_ACTIVATION_GROUP
		if (activationGroup.get())
		{
			output(TXT("[ScopedAutoActivationGroup] using new activation group (wo[%S] swo[%S]) [%i]"), _wo ? _wo->get_world_object_name().to_char() : TXT("--"), _secondaryWo? _secondaryWo->get_world_object_name().to_char() : TXT("--"), activationGroup->get_id());
		}
#endif
	}
	else
	{
		Game::get()->push_activation_group(activationGroup.get());
	}
}

ScopedAutoActivationGroup::~ScopedAutoActivationGroup()
{
#ifdef INSPECT_ACTIVATION_GROUP
	if (activationGroup.get())
	{
		output(TXT("[ScopedAutoActivationGroup] leaving activation group [%i] with intend to %S"), activationGroup->get_id(), activateNow && world? TXT("activate objects now") : TXT("activate objects later"));
	}
#endif
	if (activateNow && world)
	{
		while (!world->ready_to_activate_all_inactive(activationGroup.get())) {}
		Game::get()->perform_sync_world_job(TXT("activate"), [this]()
		{
			MEASURE_PERFORMANCE(scopedActivationGroup);
			world->activate_all_inactive(activationGroup.get());
		});
	}
	Game::get()->pop_activation_group(activationGroup.get());
}

void delayed_activation(SafePtr<Room> _room, ActivationGroup* _activationGroup)
{
	ActivationGroupPtr keepActivationGroup(_activationGroup);

	if (_room.is_set())
	{
		if (AVOID_CALLING_ACK_ _room->was_recently_seen_by_player()
#ifdef AN_DEVELOPMENT_OR_PROFILER
			&& ! Framework::is_preview_game()
#endif
			)
		{
			Game::get()->add_async_world_job(TXT("delayed activation"),
				[_room, keepActivationGroup]()
			{
				if (Game::get()->get_number_of_world_jobs() < 3)
				{
					MEASURE_PERFORMANCE(waitForOtherWorldJobs);
					System::Core::sleep(0.1f); // make space for other things to kick in, if we have more, then just skim over
				}
				delayed_activation(_room, keepActivationGroup.get());
			});
		}
		else
		{
			while (!_room->get_in_world()->ready_to_activate_all_inactive(_activationGroup)) {}
			Game::get()->perform_sync_world_job(TXT("activate"), [_room, _activationGroup]()
			{
				MEASURE_PERFORMANCE(delayedActivationGroup);
				_room->get_in_world()->activate_all_inactive(_activationGroup);
			});
		}
	}
}

void ScopedAutoActivationGroup::if_required_delay_until_room_not_visible(Room * _room)
{
	if (activateNow && AVOID_CALLING_ACK_ _room->was_recently_seen_by_player())
	{
		delayed_activation(SafePtr<Room>(_room), activationGroup.get());
		activateNow = false;
	}
}
