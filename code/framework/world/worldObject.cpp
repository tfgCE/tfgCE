#include "worldObject.h"

#include "..\game\game.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(WorldObject);

WorldObject::WorldObject()
{
	activationGroup = Game::get()->get_current_activation_group();
}

WorldObject::~WorldObject()
{
	an_assert(!world);
}

String WorldObject::get_world_object_name() const
{
	return String::printf(TXT("world object x%p"), this);
}
