#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\functionParamsStruct.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\memory\safeObject.h"

#include "gameInput.h"

namespace Framework
{
	class Actor;
	interface_class IModulesOwner;

	class Player
	{
		FAST_CAST_DECLARE(Player);
		FAST_CAST_END();
	public:
		Player();
		virtual ~Player();

		struct BindActorParams
		{
			ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(BindActorParams, bool, lazyBind, lazy_bind, false, true);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(BindActorParams, bool, keepVRAnchor, keep_vr_anchor, false, true);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(BindActorParams, bool, findVRAnchor, find_vr_anchor, false, true);

			BindActorParams() {}
		};
		void bind_actor(Actor* _actor, BindActorParams const & _params = BindActorParams());
		void unbind_actor();
		void update_for_sliding_locomotion();
		Actor* get_actor() const;

		GameInput & access_input() { return input; }
		GameInput & access_input_non_vr() { return input; }

		GameInput const & get_input() const { return input; } // non vr!
		GameInput const & get_input_non_vr() const { return input; } // non vr!
		GameInput const & get_input_vr_left() const { return inputVRLeft; }
		GameInput const & get_input_vr_right() const { return inputVRRight; }

		void use_input_definition(GameInputDefinition* _gid);
		void use_input_definition(Name const & _gidName);

		Optional<Transform> const& get_vr_anchor_os_lock() const { return vrAnchorOSLock; }
		void start_vr_anchor_os_lock(Optional<Transform> const & _vrAnchorOSLock = NP);
		void end_vr_anchor_os_lock();

		void advance_vr_anchor();
		void zero_vr_anchor_for_sliding_locomotion();

	protected:
		virtual void on_actor_change() {}

	private:
		SafePtr<IModulesOwner> actor;
		bool lazilyBound = false; // if lazily bound, thins are not changed inside, we just bind then to know about the actor

		GameInput input; // non vr!
		GameInput inputVRLeft;
		GameInput inputVRRight;

		void setup_input_vr();

		Optional<Transform> vrAnchorOSLock; // this stretches beyond currently controlled actor
	};

};
