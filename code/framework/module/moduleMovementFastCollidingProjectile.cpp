#include "moduleMovementFastCollidingProjectile.h"

#include "moduleCollisionProjectile.h"

#include "registeredModule.h"
#include "modules.h"
#include "moduleDataImpl.inl"

#include "..\collision\collisionTrace.h"
#include "..\collision\checkCollisionContext.h"
#include "..\collision\checkSegmentResult.h"
#include "..\collision\physicalMaterial.h"

#include "..\advance\advanceContext.h"

#include "..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(againstCollision);

//

REGISTER_FOR_FAST_CAST(ModuleMovementFastCollidingProjectile);

static ModuleMovement* create_module(IModulesOwner* _owner)
{
	return new ModuleMovementFastCollidingProjectile(_owner);
}

RegisteredModule<ModuleMovement> & ModuleMovementFastCollidingProjectile::register_itself()
{
	return Modules::movement.register_module(String(TXT("fastCollidingProjectile")), create_module);
}

ModuleMovementFastCollidingProjectile::ModuleMovementFastCollidingProjectile(IModulesOwner* _owner)
: base(_owner)
{
	requiresContinuousMovement = true;
}

void ModuleMovementFastCollidingProjectile::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (collisionPushOutTime != 0.0f)
	{
		warn(TXT("for fast colliding, push out time is 0, \"%S\""), get_owner()->get_library_name().to_string().to_char());
		collisionPushOutTime = 0.0f;
	}
}

void ModuleMovementFastCollidingProjectile::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context)
{
	alter_velocity(_deltaTime, _context);

	an_assert(! fast_cast<ModuleCollisionProjectile>(get_owner()->get_collision()), TXT("shouldn't be used with collision projectile!"));

	// push out time requires to be 0 to properly apply displacement
	an_assert(collisionPushOutTime == 0.0f);
	ModuleMovement::apply_changes_to_velocity_and_displacement(_deltaTime, _context); // skip projectile!

	IModulesOwner * modulesOwner = get_owner();

	// FastCollidingProjectile and FastColliding use the same code!

	// get from collision
	if (_context.applyCollision)
	{
		if (ModuleCollision const * collision = modulesOwner->get_collision())
		{
			Vector3 accelerationFromGradient = collision->get_gradient();
			if (accelerationFromGradient.is_zero())
			{
				// if not colliding check if maybe we would be colliding on the way, if so, try to get to the distance to stop at the collision
				// then we will detect collision and collide
				Vector3 displacement = _context.currentDisplacementLinear;
				float displacementLength = displacement.length();
				float step = collision->get_radius_for_gradient() * magic_number 0.8f;
				bool mayCollide = true;
				if (mayCollide && displacementLength > step * 2.0f)
				{
					MEASURE_PERFORMANCE(fastCollidingProjectile_largeDisplacement_stretched);
					// a very large one, try checking if we hit anything along the displacement at all
					if (!collision->check_if_colliding_stretched(displacement))
					{
						mayCollide = false;
					}
				}
				if (mayCollide && displacementLength > step)
				{
					MEASURE_PERFORMANCE(fastCollidingProjectile_largeDisplacement);
					Vector3 dir = displacement.normal();
					float testDist = step;
					while (testDist < displacementLength)
					{
						if (collision->check_if_colliding(dir * testDist))
						{
							float stopAt = testDist / displacementLength;
							_context.currentDisplacementLinear *= stopAt;
							_context.currentDisplacementRotation = Quat::lerp(stopAt, Quat::identity, _context.currentDisplacementRotation);
							an_assert(_context.currentDisplacementRotation.is_normalised(0.1f));
							break;
						}

						testDist += step;
					}
				}
			}
		}
	}
}
