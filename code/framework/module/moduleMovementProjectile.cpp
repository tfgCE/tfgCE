#include "moduleMovementProjectile.h"

#include "moduleCollisionProjectile.h"

#include "registeredModule.h"
#include "modules.h"
#include "moduleDataImpl.inl"

#include "..\advance\advanceContext.h"
#include "..\collision\collisionTrace.h"
#include "..\collision\checkCollisionContext.h"
#include "..\collision\checkSegmentResult.h"
#include "..\collision\physicalMaterial.h"
#include "..\object\temporaryObject.h"
#include "..\world\presenceLink.h"
#include "..\world\room.h"

#include "..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef ALLOW_DETAILED_DEBUG
		//#define DEBUG_PROJECTILE
	#endif
#endif

//#define DEBUG_RENDER_PROJECTILE

//#define AUTO_PROJECTILE_AVOIDS_SHOOTER

//

using namespace Framework;

//

// module params
DEFINE_STATIC_NAME(alignToVelocity);
DEFINE_STATIC_NAME(dontAlignToVelocity);
DEFINE_STATIC_NAME(againstCollision);
DEFINE_STATIC_NAME(ignoreCollision);
DEFINE_STATIC_NAME(canRicochet);
DEFINE_STATIC_NAME(canPenetrate);
DEFINE_STATIC_NAME(autoRotation);
DEFINE_STATIC_NAME(minSpeed);
DEFINE_STATIC_NAME(maxSpeed);
DEFINE_STATIC_NAME(maintainSpeed);
DEFINE_STATIC_NAME(deceleration);
DEFINE_STATIC_NAME(acceleration);

// rare advance for move
DEFINE_STATIC_NAME(raMoveProjectile);

//

REGISTER_FOR_FAST_CAST(ModuleMovementProjectile);

static ModuleMovement* create_module(IModulesOwner* _owner)
{
	return new ModuleMovementProjectile(_owner);
}

RegisteredModule<ModuleMovement> & ModuleMovementProjectile::register_itself()
{
	return Modules::movement.register_module(String(TXT("projectile")), create_module);
}

ModuleMovementProjectile::ModuleMovementProjectile(IModulesOwner* _owner)
: base(_owner)
{
	SET_EXTRA_DEBUG_INFO(contacted, TXT("ModuleMovementProjectile.contacted"));
}

void ModuleMovementProjectile::reset()
{
	base::reset();
	contacted.clear();
	framesAlive = 0;
	justRicocheted = false;
	justPenetrated = false;
#ifdef CATCH_HANGING_PROJECTILE
	placement.clear();
#endif
}

void ModuleMovementProjectile::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		againstCollision = AgainstCollision::parse(_moduleData->get_parameter<String>(this, NAME(againstCollision), String::empty()), againstCollision);
		ignoreCollision = _moduleData->get_parameter<bool>(this, NAME(ignoreCollision), ignoreCollision);
		canRicochet = _moduleData->get_parameter<bool>(this, NAME(canRicochet), canRicochet);
		canPenetrate = _moduleData->get_parameter<bool>(this, NAME(canPenetrate), canPenetrate);
		alignToVelocity = _moduleData->get_parameter<bool>(this, NAME(alignToVelocity), alignToVelocity);
		alignToVelocity = ! _moduleData->get_parameter<bool>(this, NAME(dontAlignToVelocity), ! alignToVelocity);
		autoRotation = _moduleData->get_parameter<Rotator3>(this, NAME(autoRotation), autoRotation);
		autoRotation.pitch *= Random::get_float(-1.0f, 1.0f);
		autoRotation.yaw *= Random::get_float(-1.0f, 1.0f);
		autoRotation.roll *= Random::get_float(-1.0f, 1.0f);
		minSpeed = _moduleData->get_parameter<float>(this, NAME(minSpeed), minSpeed);
		maxSpeed = _moduleData->get_parameter<float>(this, NAME(maxSpeed), maxSpeed);
		maintainSpeed = _moduleData->get_parameter<float>(this, NAME(maintainSpeed), maintainSpeed);
		deceleration = _moduleData->get_parameter<float>(this, NAME(deceleration), deceleration);
		acceleration = _moduleData->get_parameter<float>(this, NAME(acceleration), acceleration);
	}
	get_owner()->allow_rare_move_advance(false, NAME(raMoveProjectile)); // every frame
}

void ModuleMovementProjectile::alter_velocity(float _deltaTime, VelocityAndDisplacementContext& _context)
{
	if (maintainSpeed != 1.0f || deceleration != 0.0f || acceleration != 0.0f)
	{
		Vector3 dir = _context.velocityLinear.is_zero()? Vector3::yAxis : _context.velocityLinear.normal();
		float speed = _context.velocityLinear.length();
		if (maintainSpeed != 1.0f)
		{
			speed *= 1.0f - (1.0f - maintainSpeed) * _deltaTime;
		}
		if (deceleration != 0.0f)
		{
			speed -= deceleration * _deltaTime;
		}
		if (acceleration != 0.0f)
		{
			speed += acceleration * _deltaTime;
		}
		speed = max(max(minSpeed, 0.01f), speed);
		if (maxSpeed != 0.0f)
		{
			speed = min(speed, maxSpeed);
		}
		_context.velocityLinear = dir * speed;
		_context.currentDisplacementLinear = _context.velocityLinear * _deltaTime;
	}

	IModulesOwner* modulesOwner = get_owner();

	auto const* presence = modulesOwner->get_presence();

	if (alignToVelocity && presence)
	{	// reorientate to be along velocity
		Vector3 velocityDir = _context.velocityLinear;
		if (auto* to = get_owner()->get_as_temporary_object())
		{
			velocityDir -= to->get_initial_additional_absolute_velocity().get(Vector3::zero);
		}
		velocityDir = velocityDir.normal();
		if (Vector3::dot(presence->get_placement().get_axis(Axis::Forward), velocityDir) < 0.9995f)
		{
			Quat isAt = presence->get_placement().get_orientation();

			Vector3 up = presence->get_placement().get_axis(Axis::Up).normal();
			if (abs(Vector3::dot(velocityDir, up)) > 0.9f)
			{
				Vector3 right = presence->get_placement().get_axis(Axis::Right).normal();
				up = Vector3::cross(right, velocityDir).normal();
			}
			Quat shouldBeAt = look_matrix(Vector3::zero, velocityDir, up).get_orientation_matrix().to_quat();
			_context.currentDisplacementRotation = isAt.to_local(shouldBeAt);
		}
		if (!autoRotation.is_zero())
		{
			_context.currentDisplacementRotation = _context.currentDisplacementRotation.to_world((autoRotation * Rotator3(0.0f, 0.0f, 1.0f) * _deltaTime).to_quat());
		}
	}
	else if (!autoRotation.is_zero())
	{
		_context.currentDisplacementRotation = (autoRotation * _deltaTime).to_quat();
	}
}

void ModuleMovementProjectile::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context)
{
	IModulesOwner * modulesOwner = get_owner();

#ifndef BUILD_PUBLIC_RELEASE
	if (!get_owner()->get_presence()->get_centre_of_presence_os().get_translation().is_zero() &&
		get_owner()->get_presence()->can_change_rooms())
	{
		error_stop(TXT("projectiles (%S) should not have centre of presence offset as it may mess up ricochets and movement (invisible walls when moving through doors etc), it's just easier if the mesh is offset"), get_owner()->get_library_name().to_string().to_char());
	}
#endif

#ifdef CATCH_HANGING_PROJECTILE
	Framework::Room* nowInRoom = get_owner()->get_presence()->get_in_room();
	Transform nowPlacement = get_owner()->get_presence()->get_placement();
	if (framesAlive > 0 &&
		nowInRoom == inRoom &&
		placement.is_set() &&
		Transform::same_with_orientation(nowPlacement, placement.get()))
	{
		warn_dev_ignore(TXT("hanging projectile to'%p"), get_owner());
		debug_context(get_owner()->get_presence()->get_in_room());
		debug_draw_time_based(2.0f, debug_draw_sphere(true, false, Colour::yellow, 0.5f, Sphere(placement.get().get_translation(), 0.01f)));
		debug_no_context();
	}
	inRoom = nowInRoom;
	placement = nowPlacement;
#endif

	an_assert(ignoreCollision || fast_cast<ModuleCollisionProjectile>(get_owner()->get_collision()), TXT("should be used with collision projectile only!"));

	alter_velocity(_deltaTime, _context);

	auto const * presence = modulesOwner->get_presence();
	auto * collision = modulesOwner->get_collision();
	if (presence && collision && _context.applyCollision && ! ignoreCollision)
	{
		Optional<Vector3> backCheckLocation;
		Vector3 startLocation = presence->get_placement().get_translation();
		Vector3 trueStartLocation = startLocation;
		
		if (oneBackCheckOffset.is_set())
		{
			backCheckLocation = presence->get_placement().location_to_world(oneBackCheckOffset.get());
			oneBackCheckOffset.clear();
		}

#ifdef DEBUG_PROJECTILE
		output(TXT("[debug-projectile] START to'%p:%i"), get_owner(), framesAlive);
		output(TXT("[debug-projectile] to'%p at %S"), get_owner(), startLocation.to_string().to_char());
		output(TXT("[debug-projectile] to'%p in room r%p %S"), get_owner(), presence->get_in_room(), presence->get_in_room()->get_name().to_char());
		Vector3 p = startLocation;
		Optional<Vector3> pbct;
#endif

#ifdef DEBUG_PROJECTILE
		output(TXT("[debug-projectile] to'%p to %S"), get_owner(), (startLocation + _context.currentDisplacementLinear).to_string().to_char());
		output(TXT("[debug-projectile] to'%p currentDisplacementLinear %S"), get_owner(), _context.currentDisplacementLinear.to_string().to_char());
		output(TXT("[debug-projectile] to'%p distance to cover %.3f"), get_owner(), _context.currentDisplacementLinear.length());
#ifdef AN_DEBUG_RENDERER
		Vector3 s = startLocation;
		Vector3 e = startLocation + _context.currentDisplacementLinear;
#endif
#endif

#ifdef DEBUG_RENDER_PROJECTILE
		{
			debug_context(presence->get_in_room());
			Vector3 s = startLocation;
			Vector3 e = startLocation + _context.currentDisplacementLinear;
			debug_draw_time_based(0.1f, debug_draw_arrow(true, Colour::green, s, e));
			debug_no_context();
		}
#endif

		// move back a little in case we would end up right on the wall and due to floating point we would skip over it
		Vector3 currentStartLocation = startLocation;
		if (! justRicocheted && ! justPenetrated && framesAlive > 0)
		{
			currentStartLocation -= _context.currentDisplacementLinear.normal() * 0.0025f; // we may move through a collision, cover a distance we already covered
		}

		justRicocheted = false;
		justPenetrated = false;

#ifdef DEBUG_PROJECTILE
		{
			Rotator3 dir = _context.currentDisplacementLinear.to_rotator();
			output(TXT("[debug-projectile] to'%p pitch:%.0f yaw:%.0f"), get_owner(), dir.pitch, dir.yaw);
		}
#endif
		Optional<Vector3> forceLinearDisplacement;

		float totalDistanceToCover = _context.currentDisplacementLinear.length();
		Optional<float> totalDistanceCovered;
		bool keepGoing = true;
		int const keepGoingsAmount = 10;
		int keepGoingsLeft = keepGoingsAmount;
#ifdef DEBUG_PROJECTILE
		output(TXT("[debug-projectile] to'%p totalDistanceToCover %.3f"), get_owner(), totalDistanceToCover);
#endif
		while (keepGoing && keepGoingsLeft > 0 && (totalDistanceToCover < 0.005f || totalDistanceCovered.get(0.0f) < totalDistanceToCover - 0.001f))
		{
			keepGoing = false;
			--keepGoingsLeft;

			bool backCheckTrace = false;
			CollisionTrace collisionTrace(CollisionTraceInSpace::WS);
			if (backCheckLocation.is_set())
			{
#ifdef DEBUG_PROJECTILE
				pbct = backCheckLocation.get();
#endif
				backCheckTrace = true;
				collisionTrace.add_location(backCheckLocation.get());
				collisionTrace.add_location(currentStartLocation);
#ifdef DEBUG_PROJECTILE
				output(TXT("[debug-projectile] to'%p going (back check trace) %S to %S"), get_owner(), backCheckLocation.get().to_string().to_char(), currentStartLocation.to_string().to_char());
#endif
				backCheckLocation.clear();
			}
			else
			{
				collisionTrace.add_location(currentStartLocation);
				collisionTrace.add_location(startLocation + _context.currentDisplacementLinear);
#ifdef DEBUG_PROJECTILE
				output(TXT("[debug-projectile] to'%p going %S to %S"), get_owner(), currentStartLocation.to_string().to_char(), (startLocation + _context.currentDisplacementLinear).to_string().to_char());
				output(TXT("[debug-projectile] to'%p totalDistanceCovered %.3f"), get_owner(), totalDistanceCovered.get(0.0f));
#endif
			}

			float const avoidForTime = 0.1f;

			CheckSegmentResult result;
			int collisionTraceFlags = CollisionTraceFlag::ResultInFinalRoom;
			CheckCollisionContext checkCollisionContext;
			checkCollisionContext.collision_info_needed();
			checkCollisionContext.use_collision_flags(collision->get_collides_with_flags());
			checkCollisionContext.ignore_reversed_normal(backCheckTrace); // this way we may skip through a shield if it faces away from us
			collision->add_not_colliding_with_to(checkCollisionContext);
			if (can_penetrate())
			{
				for_every(c, contacted)
				{
					if (c->collidedTS.get_time_since() <= avoidForTime &&
						c->object)
					{
#ifdef DEBUG_PROJECTILE
						output(TXT("[debug-projectile] to'%p avoid o'%p"), get_owner(), c->object);
#endif
						checkCollisionContext.avoid(c->object);
					}
				}
			}

			if (presence->trace_collision(AgainstCollision::Precise, collisionTrace, result, collisionTraceFlags, checkCollisionContext))
			{
				Vector3 hitStartRoomWS = result.intoInRoomTransform.location_to_local(result.hitLocation);
#ifdef DEBUG_RENDER_PROJECTILE
				{
					debug_context(presence->get_in_room());
					Vector3 s = startLocation;
					Vector3 e = hitStartRoomWS;
					debug_draw_time_based(2.0f, debug_draw_arrow(true, Colour::red, s, e));
					debug_no_context();
				}
#endif
#ifdef DEBUG_PROJECTILE
				output(TXT("[debug-projectile] to'%p hit at %S"), get_owner(), hitStartRoomWS.to_string().to_char());
				output(TXT("[debug-projectile] to'%p hit in room r%p %S"), get_owner(), result.inRoom, result.inRoom->get_name().to_char());
				output(TXT("[debug-projectile] to'%p hit o%p %S"), get_owner(),
					result.presenceLink? result.presenceLink->get_modules_owner() : nullptr,
					result.presenceLink? result.presenceLink->get_modules_owner()->ai_get_name().to_char() : TXT("--"));
#endif
				float distanceCoveredSinceStart = backCheckTrace? 0.0f : (hitStartRoomWS - startLocation).length();
				an_assert(distanceCoveredSinceStart <= totalDistanceToCover);

				// if we already hit, ignore collision and keep going in this frame
				bool ignoreCollision = false;
#ifdef AUTO_PROJECTILE_AVOIDS_SHOOTER
				if (result.object)
				{
					todo_hack(TXT("avoid hitting gun that fired"));
					// 
					if (backCheckTrace || (framesAlive == 0 && distanceCoveredSinceStart < 0.1f))
					{
						auto* inst = get_owner()->get_instigator();
						int goUp = 1;
						while (inst && goUp > 0)
						{
							if (inst->get_as_collision_collidable_object() == result.object)
							{
								ignoreCollision = true;
								break;
							}
							inst = inst->get_presence()->get_attached_to();
							--goUp;
						}
					}
				}
#endif
				if (can_ricochet() || can_penetrate())
				{
					for (int i = 0; i < contacted.get_size(); ++i)
					{
						auto& r = contacted[i];
						if (r.collidedTS.get_time_since() > avoidForTime)
						{
							contacted.remove_at(i);
							--i;
						}
						else if (r.shape == result.shape &&
							(r.contactLocWS - result.hitLocation).length() < 0.01f)
						{
							ignoreCollision = true;
							break;
						}
					}
					if (!ignoreCollision)
					{
						if (!contacted.has_place_left())
						{
							contacted.pop_front();
						}
						if (result.shape)
						{
							contacted.grow_size(1);
							contacted.get_last().shape = result.shape;
							contacted.get_last().object = result.object;
							contacted.get_last().collidedTS.reset();
							contacted.get_last().contactLocWS = result.hitLocation;
						}
					}
				}

				if (!ignoreCollision)
				{
					keepGoing = false;
					justRicocheted = can_ricochet();
					justPenetrated = can_penetrate();

					if (backCheckTrace)
					{
						forceLinearDisplacement = hitStartRoomWS - startLocation;
						forceLinearDisplacement = forceLinearDisplacement.get().normal() * (forceLinearDisplacement.get().length() + (justRicocheted ? 0.01f : 0.0f));
#ifdef DEBUG_PROJECTILE
						output(TXT("[debug-projectile] to'%p back check trace%S"), get_owner(), justRicocheted? TXT(" ricocheted") : TXT(" hit/stop"));
#endif
					}
					else
					{
						if (justRicocheted)
						{
#ifdef DEBUG_PROJECTILE
							output(TXT("[debug-projectile] to'%p ricocheted"), get_owner());
#endif
							totalDistanceCovered = max(0.0f, distanceCoveredSinceStart - 0.01f);
						}
						else
						{
#ifdef DEBUG_PROJECTILE
							output(TXT("[debug-projectile] to'%p stop"), get_owner());
#endif
							totalDistanceCovered = distanceCoveredSinceStart;
						}
					}
				}
				else if (backCheckTrace)
				{
#ifdef DEBUG_PROJECTILE
					output(TXT("[debug-projectile] to'%p hit ignored (back check trace, continue with normal)"), get_owner());
#endif
					keepGoing = true;
				}
				else
				{
#ifdef DEBUG_PROJECTILE
					output(TXT("[debug-projectile] to'%p hit ignored"), get_owner());
#endif
					totalDistanceCovered = distanceCoveredSinceStart;
					currentStartLocation = startLocation + _context.currentDisplacementLinear.normal() * (distanceCoveredSinceStart + 0.001f);
					keepGoing = true;
				}
#ifdef DEBUG_PROJECTILE
				output(TXT("[debug-projectile] to'%p post hit currentDisplacementLinear %S"), get_owner(), _context.currentDisplacementLinear.to_string().to_char());
#endif

				if (!ignoreCollision)
				{
					// we store collision here as if we have projectile collision module
					CollisionInfo ci;
					ci.collidedInRoom = result.inRoom;
					ci.presenceLink = result.presenceLink;
					ci.material = PhysicalMaterial::cast(result.material);
					ci.collisionLocation = result.hitLocation;
					ci.collisionNormal = result.hitNormal;
					ci.collidedWithObject = result.object;
					an_assert_immediate(! result.presenceLink ||
									 result.presenceLink->get_modules_owner()->get_as_collision_collidable_object() == result.object);
					ci.collidedWithShape = result.shape;
					collision->store_collided_with(ci);
				}
			}
			else
			{
				if (backCheckTrace)
				{
					keepGoing = true;
				}
			}

#ifdef DEBUG_PROJECTILE
#ifdef AN_DEBUG_RENDERER
			Vector3 h = startLocation + _context.currentDisplacementLinear;
			if (totalDistanceCovered.is_set())
			{
				h = startLocation + _context.currentDisplacementLinear.normal() * (totalDistanceCovered.get());
			}

			debug_context(presence->get_in_room());
			if (forceLinearDisplacement.is_set())
			{
				if (pbct.is_set())
				{
					debug_draw_time_based(2.0f, debug_draw_arrow(true, Colour::red, pbct.get(), s + forceLinearDisplacement.get()));
				}
				debug_draw_time_based(2.0f, debug_draw_arrow(true, Colour::blue, s + forceLinearDisplacement.get(), s));
			}
			else
			{
				if (p != s)
				{
					debug_draw_time_based(2.0f, debug_draw_arrow(true, Colour::blue, p, s));
				}
				if (pbct.is_set() && backCheckTrace && pbct.get() != s)
				{
					debug_draw_time_based(2.0f, debug_draw_arrow(true, Colour::blue, pbct.get(), s));
				}
				if (!backCheckTrace)
				{
					debug_draw_time_based(2.0f, debug_draw_arrow(true, Colour::green, s, h));
					if (h != e)
					{
						debug_draw_time_based(2.0f, debug_draw_line(true, Colour::red, h, e));
					}
					debug_draw_time_based(2.0f, debug_draw_text(true, Colour::green, h, Vector2::half, true, 0.1f, false, TXT("%i:%i %c%c"), framesAlive, keepGoingsAmount - keepGoingsLeft - 1, justRicocheted ? 'r' : ' ', justPenetrated ? 'p' : ' '));
				}
			}
			debug_no_context();
#endif
#endif
		}

		if (keepGoingsLeft <= 0)
		{
#ifdef DEBUG_PROJECTILE
			output(TXT("[debug-projectile] to'%p exceeded keep goings left"), get_owner());
#endif
			output(TXT("[projectile] to'%p exceeded keep goings left"), get_owner());
		}

		if (forceLinearDisplacement.is_set())
		{
			_context.currentDisplacementLinear = forceLinearDisplacement.get();
#ifdef DEBUG_PROJECTILE
			output(TXT("[debug-projectile] to'%p travelled %.3f (forced linear displacement"), get_owner(), forceLinearDisplacement.get().length());
#endif
		}
		else if (totalDistanceCovered.is_set())
		{
			_context.currentDisplacementLinear = _context.currentDisplacementLinear.normal() * (totalDistanceCovered.get());
#ifdef DEBUG_PROJECTILE
			output(TXT("[debug-projectile] to'%p travelled %.3f"), get_owner(), totalDistanceCovered.get());
#endif
		}
		else
		{
#ifdef DEBUG_PROJECTILE
			output(TXT("[debug-projectile] to'%p travelled %.3f peacefully"), get_owner(), _context.currentDisplacementLinear.length());
#endif
		}

		Vector3 endLocation = startLocation + _context.currentDisplacementLinear;
		
		_context.currentDisplacementLinear = endLocation - trueStartLocation;
	}
	++framesAlive;
}
