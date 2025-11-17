#include "moduleCollisionProjectile.h"

#include "modules.h"

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(ModuleCollisionProjectile);

static ModuleCollision* create_module(IModulesOwner* _owner)
{
	return new ModuleCollisionProjectile(_owner);
}

RegisteredModule<ModuleCollision> & ModuleCollisionProjectile::register_itself()
{
	return Modules::collision.register_module(String(TXT("projectile")), create_module);
}

ModuleCollisionProjectile::ModuleCollisionProjectile(IModulesOwner* _owner)
: base(_owner)
{
	allowCollisionsWithSelf = false;
}

ModuleCollisionProjectile::~ModuleCollisionProjectile()
{
}

void ModuleCollisionProjectile::update_gradient_and_detection()
{
	collisionGradient = Vector3::zero;
}