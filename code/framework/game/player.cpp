#include "player.h"

#include "..\game\game.h"
#include "..\module\moduleAI.h"
#include "..\module\moduleCollision.h"
#include "..\module\moduleMovement.h"
#include "..\module\modulePresence.h"
#include "..\object\actor.h"
#include "..\world\room.h"

#include "..\..\core\vr\iVR.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(playerCollidesWithFlags);
DEFINE_STATIC_NAME(playerSlidingLocomotionCollidesWithFlags);

//

REGISTER_FOR_FAST_CAST(Player);

Player::Player()
: actor(nullptr)
{
}

Player::~Player()
{
	bind_actor(nullptr);
}

void Player::unbind_actor()
{
	bind_actor(nullptr);
}

Actor* Player::get_actor() const
{
	if (auto* imo = actor.get())
	{
		if (imo->is_advanceable())
		{
			return fast_cast<Actor>(imo);
		}
	}
	return nullptr;
}

void Player::bind_actor(Actor* _actor, BindActorParams const& _params)
{
	if (actor == _actor && _params.lazyBind == lazilyBound)
	{
		on_actor_change(); // if we requested, do the basic/default behaviour anyway
		return;
	}

	Optional<Transform> vrPlacement;
	Optional<Transform> vrAnchorRelToActor;

	if (actor.is_set() && !lazilyBound)
	{
		if (ModuleAI* ai = actor->get_ai())
		{
			ai->set_controlled_by_player(false);
		}
		if (ModuleCollision* mc = actor->get_collision())
		{
			mc->access_collides_with_flags()
				.remove(NAME(playerCollidesWithFlags))
				.remove(NAME(playerSlidingLocomotionCollidesWithFlags));
			mc->update_collidable_object();
		}
		if (ModulePresence* p = actor->get_presence())
		{
			vrPlacement = p->get_placement_in_vr_space();
			if (_params.keepVRAnchor)
			{
				vrAnchorRelToActor = p->get_placement().to_local(p->get_vr_anchor());
			}
			p->set_vr_movement(false); // just clear
			p->set_player_movement(false); // just clear
			p->set_required_forced_update_look_via_controller(false); // rely on default behaviour
		}
	}

	actor = _actor;
	lazilyBound = _params.lazyBind;

	if (actor.is_set() && ! _params.lazyBind)
	{
		if (ModuleAI* ai = actor->get_ai())
		{
			ai->set_controlled_by_player(true);
		}
		if (ModuleCollision* mc = actor->get_collision())
		{
			mc->access_collides_with_flags()
				.add(NAME(playerCollidesWithFlags));
			if (Game::is_using_sliding_locomotion())
			{
				mc->access_collides_with_flags()
					.add(NAME(playerSlidingLocomotionCollidesWithFlags));
			}
			mc->update_collidable_object();
		}
		if (ModulePresence* p = actor->get_presence())
		{
			if (_params.findVRAnchor)
			{
				p->set_vr_anchor(p->get_in_room()->get_vr_anchor(p->get_placement(), vrPlacement));
			}
			else if (_params.keepVRAnchor && vrAnchorRelToActor.is_set())
			{
				p->set_vr_anchor(p->get_placement().to_world(vrAnchorRelToActor.get()));
			}
			if (VR::IVR::get()->can_be_used())
			{
				p->set_vr_movement();
			}
			p->set_player_movement();
			p->set_required_forced_update_look_via_controller(true); // if we're immovable, we may need to update look at least
		}
	}

	if (!_params.keepVRAnchor)
	{
		vrAnchorOSLock.clear();
	}

	on_actor_change();
}

void Player::update_for_sliding_locomotion()
{
	if (actor.is_set())
	{
		if (ModuleCollision* mc = actor->get_collision())
		{
			if (Game::is_using_sliding_locomotion())
			{
				mc->access_collides_with_flags()
					.add(NAME(playerSlidingLocomotionCollidesWithFlags));
			}
			else
			{
				mc->access_collides_with_flags()
					.remove(NAME(playerSlidingLocomotionCollidesWithFlags));
			}
			mc->update_collidable_object();
		}
	}
}

void Player::use_input_definition(GameInputDefinition* _gid)
{
	input.use(_gid);
	setup_input_vr();
}

void Player::use_input_definition(Name const & _gidName)
{
	input.use(_gidName);
	setup_input_vr();
}

void Player::setup_input_vr()
{
	inputVRLeft = input;
	inputVRLeft.use(GameInputIncludeVR::Left);
	inputVRRight = input;
	inputVRRight.use(GameInputIncludeVR::Right);
}

void Player::start_vr_anchor_os_lock(Optional<Transform> const& _vrAnchorOSLock)
{
	if (_vrAnchorOSLock.is_set())
	{
		vrAnchorOSLock = _vrAnchorOSLock;
		return;
	}

	if (auto* playerActor = get_actor())
	{
		auto* p = playerActor->get_presence();
		Transform vrAnchor = p->get_vr_anchor();
		Transform placement = p->get_placement();
		vrAnchorOSLock = placement.to_local(vrAnchor);
	}
}

void Player::end_vr_anchor_os_lock()
{
	vrAnchorOSLock.clear();
}

void Player::advance_vr_anchor()
{
	if (auto* playerActor = get_actor())
	{
		bool continuousZeroVRAnchor = Game::is_using_sliding_locomotion();

		if (vrAnchorOSLock.is_set())
		{
			auto* p = playerActor->get_presence();
			Transform placement = p->get_placement();
			p->set_vr_anchor(placement.to_world(vrAnchorOSLock.get()));
			continuousZeroVRAnchor = false;
		}

		// zero vr anchor, so we always start as we would not move, check gameSceneLayer_World for more info
		// do this here, before process controls
		if (continuousZeroVRAnchor)
		{
			if (playerActor->get_presence()->is_vr_movement() &&
				playerActor->get_movement() &&
				playerActor->get_movement()->does_use_controller_for_movement())
			{
				playerActor->get_presence()->zero_vr_anchor(true);
			}
		}
	}
}

void Player::zero_vr_anchor_for_sliding_locomotion()
{
	if (Game::is_using_sliding_locomotion())
	{
		if (auto* playerActor = get_actor())
		{
			if (playerActor->get_presence()->is_vr_movement() &&
				playerActor->get_movement() &&
				playerActor->get_movement()->does_use_controller_for_movement())
			{
				playerActor->get_presence()->zero_vr_anchor();
			}
		}
	}
}
