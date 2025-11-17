#include "moduleMovementFastColliding.h"

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
DEFINE_STATIC_NAME(stopAtVelocity);
DEFINE_STATIC_NAME(getVelocityFromCollidedWith);

//

REGISTER_FOR_FAST_CAST(ModuleMovementFastColliding);

static ModuleMovement* create_module(IModulesOwner* _owner)
{
	return new ModuleMovementFastColliding(_owner);
}

RegisteredModule<ModuleMovement> & ModuleMovementFastColliding::register_itself()
{
	return Modules::movement.register_module(String(TXT("fastColliding")), create_module);
}

ModuleMovementFastColliding::ModuleMovementFastColliding(IModulesOwner* _owner)
: base(_owner)
{
	requiresContinuousMovement = true;
}

void ModuleMovementFastColliding::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (collisionPushOutTime != 0.0f)
	{
		warn(TXT("for fast colliding, push out time is 0, \"%S\""), get_owner()->get_library_name().to_string().to_char());
		collisionPushOutTime = 0.0f;
	}
	stopAtVelocity = _moduleData->get_parameter<float>(this, NAME(stopAtVelocity), stopAtVelocity);
	getVelocityFromCollidedWith = _moduleData->get_parameter<float>(this, NAME(getVelocityFromCollidedWith), getVelocityFromCollidedWith);
}

void ModuleMovementFastColliding::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context)
{
	// push out time requires to be 0 to properly apply displacement
	an_assert(collisionPushOutTime == 0.0f);
	base::apply_changes_to_velocity_and_displacement(_deltaTime, _context);

	IModulesOwner * modulesOwner = get_owner();

	// FastCollidingProjectile and FastColliding use the same code!
	// although now FastColliding also uses a way to stop

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
					MEASURE_PERFORMANCE(fastColliding_largeDisplacement_stretched);
					// a very large one, try checking if we hit anything along the displacement at all
					if (! collision->check_if_colliding_stretched(displacement))
					{
						mayCollide = false;
					}
				}
				if (mayCollide && displacementLength > step)
				{
					MEASURE_PERFORMANCE(fastColliding_largeDisplacement);
					Vector3 dir = displacement.normal();
					float testDist = step;
					Collision::ICollidableObject const* detectedCollisionWith = nullptr;
					while (testDist < displacementLength)
					{
						ModuleCollision::CheckIfCollidingContext context;
						if (collision->check_if_colliding(dir * testDist, 1.0f, &context))
						{
							float stopAt = testDist / displacementLength;
							_context.currentDisplacementLinear *= stopAt;
							_context.currentDisplacementRotation = Quat::lerp(stopAt, Quat::identity, _context.currentDisplacementRotation);
							an_assert(_context.currentDisplacementRotation.is_normalised(0.1f));
							detectedCollisionWith = context.detectedCollisionWith;
							break;
						}

						testDist += step;
					}
					if (getVelocityFromCollidedWith)
					{
						if (IModulesOwner const * colImo = fast_cast<IModulesOwner>(detectedCollisionWith))
						{
							if (modulesOwner->get_presence()->get_based_on() != colImo)
							{
								_context.velocityLinear = lerp(getVelocityFromCollidedWith, _context.velocityLinear, colImo->get_presence()->get_velocity_linear());
							}
						}
					}
				}
			}
			else
			{
				Vector3 baseVelocity = Vector3::zero;
				if (getVelocityFromCollidedWith)
				{
					if (!collision->get_collided_with().is_empty())
					{
						if (IModulesOwner const* colImo = fast_cast<IModulesOwner>(collision->get_collided_with()[0].collidedWithObject.get()))
						{
							baseVelocity = colImo->get_presence()->get_velocity_linear();
						}
					}
					_context.velocityLinear = lerp(getVelocityFromCollidedWith, _context.velocityLinear, baseVelocity);
				}
				if ((_context.velocityLinear - baseVelocity).length() <= stopAtVelocity &&
					Vector3::dot(accelerationFromGradient.normal(), modulesOwner->get_presence()->get_gravity_dir()) < 0.7f)
				{
					_context.velocityLinear = baseVelocity;
					_context.velocityRotation = Rotator3::zero;
				}
			}
		}
	}
}
