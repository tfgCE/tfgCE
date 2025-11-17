#pragma once

#include "..\..\core\fastCast.h"

#include "activationGroup.h"

struct String;

namespace Framework
{
	class World;

	/**
	 *	Lifetime of an world object
	 *		[sync]	creation
	 *				create object itself
	 *		[async]	initalisation
	 *				initialise all modules, generate meshes etc
	 *		[sync]	place in the world
	 *				place in room
	 *		[async]	ready to world activate
	 *				POIs for Objects are handled here that may create more actors
	 *		[sync]	activation
	 *				activate all readied - this should be just marking them as advanceable
	 */
	class WorldObject
	{
		FAST_CAST_DECLARE(WorldObject);
		FAST_CAST_END();

	public:
		WorldObject();
		virtual ~WorldObject();

		World* get_in_world() const { return world; }

		ActivationGroup* get_activation_group() const { return activationGroup.get(); }
		bool does_belong_to(ActivationGroup* _ag) const { return activationGroup == _ag; }

		inline bool is_ready_to_world_activate() const { return readyToWorldActivate; }
		inline bool is_world_active() const { return worldActive; }

		virtual void ready_to_world_activate() { an_assert(!readyToWorldActivate && !worldActive); readyToWorldActivate = true; }
		virtual void world_activate() { an_assert(readyToWorldActivate); readyToWorldActivate = false; worldActive = true; activationGroup.clear(); }

		// use it for debug purposes only
		virtual String get_world_object_name() const;

	private: friend class World;
		void set_in_world(World* _world) { world = _world; }

	private:
		World* world = nullptr;
		bool readyToWorldActivate = false;
		bool worldActive = false;
		ActivationGroupPtr activationGroup; // this is required to be set (via push_activation_group), see more at ActivationGroup

		friend class World;
	};

};
