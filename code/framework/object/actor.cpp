#include "actor.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(Actor);

Actor::Actor(ActorType const * _actorType, String const & _name)
: base(_actorType, !_name.is_empty() ? _name : _actorType->create_instance_name())
, type(_actorType)
{
}
