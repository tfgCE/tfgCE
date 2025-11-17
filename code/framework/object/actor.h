#pragma once

#include "spaceBlockingObject.h"

#include "actorType.h"

namespace Framework
{
	class Actor
	: public SpaceBlockingObject
	{
		FAST_CAST_DECLARE(Actor);
		FAST_CAST_BASE(SpaceBlockingObject);
		FAST_CAST_END();

		typedef SpaceBlockingObject base;
	public:
		Actor(ActorType const * _actorType, String const & _name); // requires initialisation

		ActorType const* get_actor_type() const { return type; }

	public: // IModulesOwner
		override_ Name const & get_object_type_name() const { return Names::actor; }

	private:
		ActorType const * type = nullptr;
	};
};

DECLARE_REGISTERED_TYPE(Framework::Actor*);
