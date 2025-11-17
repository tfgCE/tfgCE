#pragma once

#include "..\..\core\containers\array.h"
#include "..\..\core\memory\refCountObject.h"

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef ALLOW_DETAILED_DEBUG
		#define INSPECT_ACTIVATION_GROUP
	#endif
#endif

//#ifndef INSPECT_ACTIVATION_GROUP
//	#define INSPECT_ACTIVATION_GROUP
//#endif

namespace Framework
{
	class Room;
	class World;
	class WorldObject;

	/**
	 *	Activation groups are used to group objects that should be activated together.
	 *	This also allows for objects to create new objects while getting ready to activate.
	 *
	 *	Only if everything is readied to be activated, whole bunch is activated together.
	 *
	 *	This class is just a pointer to an activation group to know to which group does a world object belongs.
	 */ 
	class ActivationGroup
	: public RefCountObject
	{
	public:
		ActivationGroup();

		int get_id() const { return activationGroupId; }
	private:
		static Concurrency::Counter s_activationGroupId;
		int activationGroupId;
	};

	typedef RefCountObjectPtr<ActivationGroup> ActivationGroupPtr;

	struct ActiviationGroupStack
	{
		Array<ActivationGroupPtr> activationGroups;
	};

	struct ScopedAutoActivationGroup
	{
		// you have to provide either world object (to get activation group) or world to know where to activate
		ScopedAutoActivationGroup(WorldObject* _wo, WorldObject* _secondaryWo, World* _world, bool _activateNow = true);
		ScopedAutoActivationGroup(WorldObject* _wo, WorldObject* _secondaryWo = nullptr, bool _activateNow = true);
		ScopedAutoActivationGroup(World* _world, bool _activateNow = true);
		~ScopedAutoActivationGroup();

		bool should_activate_now() { return activateNow; }

		void if_required_delay_until_room_not_visible(Room* _room);

	private:
		ActivationGroupPtr activationGroup;
		bool activateNow = true;
		World* world = nullptr;

		void initial_setup(WorldObject* _wo, WorldObject* _secondaryWo, World* _world, bool _activateNow);

	private:
		// do not implement!
		ScopedAutoActivationGroup(ScopedAutoActivationGroup const& _saag);
		ScopedAutoActivationGroup& operator=(ScopedAutoActivationGroup const& _saag);
	};
};

