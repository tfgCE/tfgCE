#include "moduleCollision.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#include "registeredModule.h"
#include "modules.h"
#include "moduleCollisionData.h"
#include "moduleDataImpl.inl"

#include "..\advance\advanceContext.h"
#include "..\library\usedLibraryStored.inl"
#include "..\object\temporaryObjectType.h"
#include "..\video\texture.h"
#include "..\world\environmentType.h"
#include "..\world\presenceLink.h"
#include "..\world\room.h"
#include "..\world\doorInRoom.h"

#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\collision\checkGradientPoints.h"
#include "..\..\core\collision\checkGradientDistances.h"
#include "..\..\core\collision\checkGradientContext.h"
#include "..\..\core\collision\collisionFlags.h"
#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\types\names.h"

#include "..\object\actor.h"
#include "..\object\object.h"

#include "..\collision\collisionModel.h"
#include "..\collision\physicalMaterial.h"

//

#ifdef AN_DEVELOPMENT
//#define INVESTIGATE__SHOULD_CONSIDER_FOR_COLLISION
#endif

//

using namespace Framework;

//

// params
DEFINE_STATIC_NAME(frameDelayOnActualUpdatingCollisions);
DEFINE_STATIC_NAME(allowCollapsingSegmentToPoint);
DEFINE_STATIC_NAME(enforceStiffCollisions);
DEFINE_STATIC_NAME(stepHole);
DEFINE_STATIC_NAME(gradientStepHole);
DEFINE_STATIC_NAME(movementStepHole);
DEFINE_STATIC_NAME(navObstructor);
DEFINE_STATIC_NAME(navObstructorOnlyRoot);
DEFINE_STATIC_NAME(allowCollisionsWithSelf);
DEFINE_STATIC_NAME(doEarlyCheckForUpdateGradient);
DEFINE_STATIC_NAME(radiusForGradient);
DEFINE_STATIC_NAME(radiusForMovement);
DEFINE_STATIC_NAME(collisionPrimitiveRadius);
DEFINE_STATIC_NAME(preciseCollisionPrimitiveRadius);
DEFINE_STATIC_NAME(radiusForDetection);
DEFINE_STATIC_NAME(collisionPrimitiveHeight);
DEFINE_STATIC_NAME(collisionPrimitiveLength);
DEFINE_STATIC_NAME(collisionPrimitiveVerticalOffset);
DEFINE_STATIC_NAME(collisionPrimitiveCentreDistanceX);
DEFINE_STATIC_NAME(collisionPrimitiveCentreDistanceY);
DEFINE_STATIC_NAME(collisionPrimitiveCentreDistanceZ);
DEFINE_STATIC_NAME(collisionPrimitiveCentreDistance);
DEFINE_STATIC_NAME(collisionPrimitiveCentreOffsetX);
DEFINE_STATIC_NAME(collisionPrimitiveCentreOffsetY);
DEFINE_STATIC_NAME(collisionPrimitiveCentreOffsetZ);
DEFINE_STATIC_NAME(collisionPrimitiveCentreOffset);
DEFINE_STATIC_NAME(preciseCollisionPrimitiveCentreAdditionalOffsetX);
DEFINE_STATIC_NAME(preciseCollisionPrimitiveCentreAdditionalOffsetY);
DEFINE_STATIC_NAME(preciseCollisionPrimitiveCentreAdditionalOffsetZ);
DEFINE_STATIC_NAME(preciseCollisionPrimitiveCentreAdditionalOffset);
DEFINE_STATIC_NAME(height);
DEFINE_STATIC_NAME(length);
DEFINE_STATIC_NAME(radius);
DEFINE_STATIC_NAME(verticalOffset);
DEFINE_STATIC_NAME(collisionFlags);
DEFINE_STATIC_NAME(collidesWithFlags);
DEFINE_STATIC_NAME(detectsFlags);
DEFINE_STATIC_NAME(mass);
DEFINE_STATIC_NAME(ikTraceFlags);
DEFINE_STATIC_NAME(ikTraceRejectFlags);
DEFINE_STATIC_NAME(presenceTraceFlags);
DEFINE_STATIC_NAME(presenceTraceRejectFlags);
DEFINE_STATIC_NAME(updateCollisionPrimitiveBasingOnBoundingBox);
DEFINE_STATIC_NAME(updateCollisionPrimitiveBasingOnBoundingBoxAsSphere);
DEFINE_STATIC_NAME(playCollisionSoundUsingAttachedToSetup);
DEFINE_STATIC_NAME(playCollisionSoundThroughMaterial);
DEFINE_STATIC_NAME(playCollisionSoundMinSpeed);
DEFINE_STATIC_NAME(playCollisionSoundFullVolumeSpeed);
DEFINE_STATIC_NAME(collisionSound);
DEFINE_STATIC_NAME(collisionSoundMinSpeed);
DEFINE_STATIC_NAME(collisionSoundNoOftenThan);
DEFINE_STATIC_NAME(noMovementCollision);
DEFINE_STATIC_NAME(cantBePushedThroughCollision);
DEFINE_STATIC_NAME(throughCollisionPusher);

//

REGISTER_FOR_FAST_CAST(ModuleCollision);

static ModuleCollision* create_module(IModulesOwner* _owner)
{
	return new ModuleCollision(_owner);
}

RegisteredModule<ModuleCollision> & ModuleCollision::register_itself()
{
	return Modules::collision.register_module(String(TXT("base")), create_module);
}

ModuleCollision::ModuleCollision(IModulesOwner* _owner)
: Module(_owner)
, mass(0.0f)
, radiusForGradient(0.0f)
, radiusForDetection(0.0f)
, gradientStepHole(0.0f)
, collisionPrimitiveRadius(0.0f)
, collisionPrimitiveCentreDistance(Vector3::zero)
, collisionPrimitiveCentreOffset(Vector3::zero)
, preciseCollisionPrimitiveCentreAdditionalOffset(Vector3::zero)
, movementQueryPoint(nullptr)
, movementQuerySegment(nullptr)
, movementQuerySegmentCollapsedToPoint(nullptr)
, collisionGradient(Vector3::zero)
, collisionStiffGradient(Vector3::zero)
{
	collidesWithFlagsStack.push_back(Collision::Flags::defaults());
	detectsFlagsStack.push_back(Collision::Flags::none());
	collisionFlagsStack.push_back(Collision::Flags::defaults());

	ikTraceFlags = Collision::DefinedFlags::get_ik_trace();
	ikTraceRejectFlags = Collision::DefinedFlags::get_ik_trace_reject();
	presenceTraceFlags = Collision::DefinedFlags::get_presence_trace();
	presenceTraceRejectFlags = Collision::DefinedFlags::get_presence_trace_reject();

	collidedWith.make_space_for(32);
}

ModuleCollision::~ModuleCollision()
{
	if (collisionUpdatedForPoseMS)
	{
		collisionUpdatedForPoseMS->release();
	}
	delete_and_clear(movementQueryPoint);
	delete_and_clear(movementQuerySegment);
	delete_and_clear(movementQuerySegmentCollapsedToPoint);
}

void ModuleCollision::push_collides_with_flags()
{
	Collision::Flags last = collidesWithFlagsStack.get_last().flags;
	collidesWithFlagsStack.push_back(last);
#ifdef AN_DEVELOPMENT
	updateCollidableObjectRequired = true;
	if (auto* co = fast_cast<Collision::ICollidableObject>(get_owner()))
	{
		co->mark_requires_updating_collides_with_flags();
	}
#endif
}

void ModuleCollision::push_collides_with_flags(Collision::Flags const & _flags)
{
	collidesWithFlagsStack.push_back(_flags);
#ifdef AN_DEVELOPMENT
	updateCollidableObjectRequired = true;
	if (auto* co = fast_cast<Collision::ICollidableObject>(get_owner()))
	{
		co->mark_requires_updating_collides_with_flags();
	}
#endif
}

void ModuleCollision::pop_collides_with_flags()
{
	an_assert(collidesWithFlagsStack.get_size() > 1);
	collidesWithFlagsStack.pop_back();
#ifdef AN_DEVELOPMENT
	updateCollidableObjectRequired = true;
	if (auto* co = fast_cast<Collision::ICollidableObject>(get_owner()))
	{
		co->mark_requires_updating_collides_with_flags();
	}
#endif
}

void ModuleCollision::push_detects_flags()
{
	Collision::Flags last = detectsFlagsStack.get_last().flags;
	detectsFlagsStack.push_back(last);
}

void ModuleCollision::push_detects_flags(Collision::Flags const & _flags)
{
	detectsFlagsStack.push_back(_flags);
}

void ModuleCollision::pop_detects_flags()
{
	an_assert(detectsFlagsStack.get_size() > 1);
	detectsFlagsStack.pop_back();
}

void ModuleCollision::push_collision_flags()
{
	Collision::Flags last = collisionFlagsStack.get_last().flags;
	collisionFlagsStack.push_back(last);
#ifdef AN_DEVELOPMENT
	updateCollidableObjectRequired = true;
	if (auto* co = fast_cast<Collision::ICollidableObject>(get_owner()))
	{
		co->mark_requires_updating_collision_flags();
	}
#endif
}

void ModuleCollision::push_collision_flags(Collision::Flags const & _flags)
{
	collisionFlagsStack.push_back(_flags);
#ifdef AN_DEVELOPMENT
	updateCollidableObjectRequired = true;
	if (auto* co = fast_cast<Collision::ICollidableObject>(get_owner()))
	{
		co->mark_requires_updating_collision_flags();
	}
#endif
}

void ModuleCollision::pop_collision_flags()
{
	an_assert(collisionFlagsStack.get_size() > 1);
	collisionFlagsStack.pop_back();
#ifdef AN_DEVELOPMENT
	updateCollidableObjectRequired = true;
	if (auto* co = fast_cast<Collision::ICollidableObject>(get_owner()))
	{
		co->mark_requires_updating_collision_flags();
	}
#endif
}

void ModuleCollision::push_collides_with_flags(Name const & _id, Collision::Flags const & _flags)
{
	collidesWithFlagsStack.push_back(CollisionFlagsStackElement(_id, _flags));
#ifdef AN_DEVELOPMENT
	updateCollidableObjectRequired = true;
	if (auto* co = fast_cast<Collision::ICollidableObject>(get_owner()))
	{
		co->mark_requires_updating_collides_with_flags();
	}
#endif
}

void ModuleCollision::pop_collides_with_flags(Name const & _id)
{
	for_every_reverse(se, collidesWithFlagsStack)
	{
		if (se->id == _id)
		{
			collidesWithFlagsStack.remove_at(for_everys_index(se));
			break;
		}
	}
#ifdef AN_DEVELOPMENT
	updateCollidableObjectRequired = true;
	if (auto* co = fast_cast<Collision::ICollidableObject>(get_owner()))
	{
		co->mark_requires_updating_collides_with_flags();
	}
#endif
}

void ModuleCollision::push_detects_flags(Name const & _id, Collision::Flags const & _flags)
{
	detectsFlagsStack.push_back(CollisionFlagsStackElement(_id, _flags));
}

void ModuleCollision::pop_detects_flags(Name const & _id)
{
	for_every_reverse(se, detectsFlagsStack)
	{
		if (se->id == _id)
		{
			detectsFlagsStack.remove_at(for_everys_index(se));
			break;
		}
	}
}

void ModuleCollision::push_collision_flags(Name const & _id, Collision::Flags const & _flags)
{
	collisionFlagsStack.push_back(CollisionFlagsStackElement(_id, _flags));
#ifdef AN_DEVELOPMENT
	updateCollidableObjectRequired = true;
	if (auto* co = fast_cast<Collision::ICollidableObject>(get_owner()))
	{
		co->mark_requires_updating_collision_flags();
	}
#endif
}

void ModuleCollision::pop_collision_flags(Name const & _id)
{
	for_every_reverse(se, collisionFlagsStack)
	{
		if (se->id == _id)
		{
			collisionFlagsStack.remove_at(for_everys_index(se));
			break;
		}
	}
#ifdef AN_DEVELOPMENT
	updateCollidableObjectRequired = true;
	if (auto* co = fast_cast<Collision::ICollidableObject>(get_owner()))
	{
		co->mark_requires_updating_collision_flags();
	}
#endif
}

void ModuleCollision::setup_with(ModuleData const * _moduleData)
{
	Module::setup_with(_moduleData);
	moduleCollisionData = fast_cast<ModuleCollisionData>(_moduleData);
	if (_moduleData)
	{
		noMovementCollision = _moduleData->get_parameter<bool>(this, NAME(noMovementCollision), noMovementCollision);
		cantBePushedThroughCollision = _moduleData->get_parameter<bool>(this, NAME(cantBePushedThroughCollision), cantBePushedThroughCollision);
		throughCollisionPusher = _moduleData->get_parameter<bool>(this, NAME(throughCollisionPusher), throughCollisionPusher);

		movementCollisionMaterial = moduleCollisionData->movementCollisionMaterial;
		preciseCollisionMaterial = moduleCollisionData->preciseCollisionMaterial;
		if (!movementCollisionMaterial.is_set())
		{
			todo_note(TXT("actor? scenery?"));
			movementCollisionMaterial = PhysicalMaterial::get();
		}

		frameDelayOnActualUpdatingCollisions = _moduleData->get_parameter<int>(this, NAME(frameDelayOnActualUpdatingCollisions), frameDelayOnActualUpdatingCollisions);

		allowCollapsingSegmentToPoint = _moduleData->get_parameter<bool>(this, NAME(allowCollapsingSegmentToPoint), allowCollapsingSegmentToPoint);
		
		enforceStiffCollisions = _moduleData->get_parameter<bool>(this, NAME(enforceStiffCollisions), enforceStiffCollisions);
		
		radiusForGradient = _moduleData->get_parameter<float>(this, NAME(radius), radiusForGradient);
		gradientStepHole = _moduleData->get_parameter<float>(this, NAME(stepHole), gradientStepHole);
		collisionPrimitiveRadius = _moduleData->get_parameter<float>(this, NAME(radius), collisionPrimitiveRadius);
		{
			preciseCollisionPrimitiveRadius.clear();
			float temp = -1.0f;
			temp = _moduleData->get_parameter<float>(this, NAME(preciseCollisionPrimitiveRadius), temp);
			if (temp >= 0.0f)
			{
				preciseCollisionPrimitiveRadius = temp;
			}
		}
		//
		navObstructor = _moduleData->get_parameter<bool>(this, NAME(navObstructor), navObstructor);
		navObstructorOnlyRoot = _moduleData->get_parameter<bool>(this, NAME(navObstructorOnlyRoot), navObstructorOnlyRoot);
		navObstructor |= navObstructorOnlyRoot;
		allowCollisionsWithSelf = _moduleData->get_parameter<bool>(this, NAME(allowCollisionsWithSelf), allowCollisionsWithSelf);
		doEarlyCheckForUpdateGradient = _moduleData->get_parameter<bool>(this, NAME(doEarlyCheckForUpdateGradient), doEarlyCheckForUpdateGradient);
		radiusForDetection = _moduleData->get_parameter<float>(this, NAME(radiusForDetection), radiusForDetection);
		radiusForGradient = _moduleData->get_parameter<float>(this, NAME(radiusForGradient), radiusForGradient);
		radiusForGradient = _moduleData->get_parameter<float>(this, NAME(radiusForMovement), radiusForGradient);
		gradientStepHole = _moduleData->get_parameter<float>(this, NAME(gradientStepHole), gradientStepHole);
		gradientStepHole = _moduleData->get_parameter<float>(this, NAME(movementStepHole), gradientStepHole);
		collisionPrimitiveRadius = _moduleData->get_parameter<float>(this, NAME(collisionPrimitiveRadius), collisionPrimitiveRadius);
		collisionPrimitiveCentreDistance.z = _moduleData->get_parameter<float>(this, NAME(collisionPrimitiveHeight), collisionPrimitiveCentreDistance.z + collisionPrimitiveRadius * 2.0f) - collisionPrimitiveRadius * 2.0f;
		collisionPrimitiveCentreDistance.z = _moduleData->get_parameter<float>(this, NAME(height), collisionPrimitiveCentreDistance.z + collisionPrimitiveRadius * 2.0f) - collisionPrimitiveRadius * 2.0f;
		collisionPrimitiveCentreDistance.z = max(0.0f, collisionPrimitiveCentreDistance.z);
		collisionPrimitiveCentreDistance.y = _moduleData->get_parameter<float>(this, NAME(collisionPrimitiveLength), collisionPrimitiveCentreDistance.y + collisionPrimitiveRadius * 2.0f) - collisionPrimitiveRadius * 2.0f;
		collisionPrimitiveCentreDistance.y = _moduleData->get_parameter<float>(this, NAME(length), collisionPrimitiveCentreDistance.y + collisionPrimitiveRadius * 2.0f) - collisionPrimitiveRadius * 2.0f;
		collisionPrimitiveCentreDistance.y = max(0.0f, collisionPrimitiveCentreDistance.y);
		collisionPrimitiveCentreOffset.z = _moduleData->get_parameter<float>(this, NAME(collisionPrimitiveVerticalOffset), collisionPrimitiveCentreOffset.z);
		collisionPrimitiveCentreOffset.z = _moduleData->get_parameter<float>(this, NAME(verticalOffset), collisionPrimitiveCentreOffset.z);
		{
			collisionPrimitiveCentreDistance.x = _moduleData->get_parameter<float>(this, NAME(collisionPrimitiveCentreDistanceX), collisionPrimitiveCentreDistance.x);
			collisionPrimitiveCentreDistance.y = _moduleData->get_parameter<float>(this, NAME(collisionPrimitiveCentreDistanceY), collisionPrimitiveCentreDistance.y);
			collisionPrimitiveCentreDistance.z = _moduleData->get_parameter<float>(this, NAME(collisionPrimitiveCentreDistanceZ), collisionPrimitiveCentreDistance.z);
			collisionPrimitiveCentreDistance = _moduleData->get_parameter<Vector3>(this, NAME(collisionPrimitiveCentreDistance), collisionPrimitiveCentreDistance);
		}
		{
			collisionPrimitiveCentreOffset.x = _moduleData->get_parameter<float>(this, NAME(collisionPrimitiveCentreOffsetX), collisionPrimitiveCentreOffset.x);
			collisionPrimitiveCentreOffset.y = _moduleData->get_parameter<float>(this, NAME(collisionPrimitiveCentreOffsetY), collisionPrimitiveCentreOffset.y);
			collisionPrimitiveCentreOffset.z = _moduleData->get_parameter<float>(this, NAME(collisionPrimitiveCentreOffsetZ), collisionPrimitiveCentreOffset.z);
			collisionPrimitiveCentreOffset = _moduleData->get_parameter<Vector3>(this, NAME(collisionPrimitiveCentreOffset), collisionPrimitiveCentreOffset);
		}
		{
			preciseCollisionPrimitiveCentreAdditionalOffset.x = _moduleData->get_parameter<float>(this, NAME(preciseCollisionPrimitiveCentreAdditionalOffsetX), preciseCollisionPrimitiveCentreAdditionalOffset.x);
			preciseCollisionPrimitiveCentreAdditionalOffset.y = _moduleData->get_parameter<float>(this, NAME(preciseCollisionPrimitiveCentreAdditionalOffsetY), preciseCollisionPrimitiveCentreAdditionalOffset.y);
			preciseCollisionPrimitiveCentreAdditionalOffset.z = _moduleData->get_parameter<float>(this, NAME(preciseCollisionPrimitiveCentreAdditionalOffsetZ), preciseCollisionPrimitiveCentreAdditionalOffset.z);
			preciseCollisionPrimitiveCentreAdditionalOffset = _moduleData->get_parameter<Vector3>(this, NAME(preciseCollisionPrimitiveCentreAdditionalOffset), preciseCollisionPrimitiveCentreAdditionalOffset);
		}
		mass = _moduleData->get_parameter<float>(this, NAME(mass), mass);

		updateCollisionPrimitiveBasingOnBoundingBox = _moduleData->get_parameter<bool>(this, NAME(updateCollisionPrimitiveBasingOnBoundingBox), updateCollisionPrimitiveBasingOnBoundingBox);
		updateCollisionPrimitiveBasingOnBoundingBoxAsSphere = _moduleData->get_parameter<bool>(this, NAME(updateCollisionPrimitiveBasingOnBoundingBoxAsSphere), updateCollisionPrimitiveBasingOnBoundingBoxAsSphere);

		playCollisionSoundUsingAttachedToSetup = _moduleData->get_parameter<bool>(this, NAME(playCollisionSoundUsingAttachedToSetup), playCollisionSoundUsingAttachedToSetup);
		playCollisionSoundThroughMaterial = _moduleData->get_parameter<bool>(this, NAME(playCollisionSoundThroughMaterial), playCollisionSoundThroughMaterial);
		playCollisionSoundMinSpeed = _moduleData->get_parameter<float>(this, NAME(playCollisionSoundMinSpeed), playCollisionSoundMinSpeed);
		playCollisionSoundFullVolumeSpeed = _moduleData->get_parameter<float>(this, NAME(playCollisionSoundFullVolumeSpeed), playCollisionSoundFullVolumeSpeed);
		collisionSound = _moduleData->get_parameter<Name>(this, NAME(collisionSound), collisionSound);
		collisionSoundMinSpeed = _moduleData->get_parameter<float>(this, NAME(collisionSoundMinSpeed), collisionSoundMinSpeed);
		collisionSoundNoOftenThan = _moduleData->get_parameter<float>(this, NAME(collisionSoundNoOftenThan), collisionSoundNoOftenThan);

		collisionFlagsStack.get_first().flags.apply(_moduleData->get_parameter<String>(this, NAME(collisionFlags)));
		collidesWithFlagsStack.get_first().flags.apply(_moduleData->get_parameter<String>(this, NAME(collidesWithFlags)));
		detectsFlagsStack.get_first().flags.apply(_moduleData->get_parameter<String>(this, NAME(detectsFlags)));
		ikTraceFlags.apply(_moduleData->get_parameter<String>(this, NAME(ikTraceFlags)));
		ikTraceRejectFlags.apply(_moduleData->get_parameter<String>(this, NAME(ikTraceRejectFlags)));
		presenceTraceFlags.apply(_moduleData->get_parameter<String>(this, NAME(presenceTraceFlags)));
		presenceTraceRejectFlags.apply(_moduleData->get_parameter<String>(this, NAME(presenceTraceRejectFlags)));
	}

	update_collision_model_and_query();

	update_collidable_object();
}

void ModuleCollision::update_collidable_object()
{
	collidableOwner = fast_cast<Collision::ICollidableObject>(get_owner());

	if (collidableOwner)
	{
		collidableOwner->set_mass_for_collision(mass);
		collidableOwner->set_collision_flags(collisionFlagsStack.get_last().flags);
		collidableOwner->set_collides_with_flags(collidesWithFlagsStack.get_last().flags);
	}

#ifdef AN_DEVELOPMENT
	updateCollidableObjectRequired = false;
#endif

	update_should_update_collisions();
}

void ModuleCollision::update_should_update_collisions()
{
	shouldUpdateCollisions = (!get_collides_with_flags().is_empty() && (collisionPrimitiveRadius > 0.0f || get_radius_for_detection() > 0.0f || radiusForGradient > 0.0f)) ||
							 (!get_detects_flags().is_empty() && (get_radius_for_detection() > 0.0f));
}

template <typename Class>
void ModuleCollision::add_primitive_to_own_collision_models(Class const & _primitiveMovement, Class const& _primitivePrecise, bool _usePrimitiveForPreciseCollision)
{
	if (!noMovementCollision)
	{
		auto primitiveCopy = _primitiveMovement;
		if (auto* material = get_movement_collision_material())
		{
			primitiveCopy.use_material(material);
		}
		ownMovementCollisionModel.add(primitiveCopy);
	}
	if (_usePrimitiveForPreciseCollision)
	{
		auto primitiveCopy = _primitivePrecise;
		if (auto* material = get_precise_collision_material())
		{
			primitiveCopy.use_material(material);
		}
		ownPreciseCollisionModel.add(primitiveCopy);
	}
}

void ModuleCollision::update_collision_model_and_query()
{
	bool usePrimitiveForMovementCollision = !usingProvidedMovementCollision;
	bool usePrimitiveForPreciseCollision = (!usingProvidedPreciseCollision && get_precise_collision_material() != nullptr) || preciseCollisionPrimitiveRadius.is_set() || !preciseCollisionPrimitiveCentreAdditionalOffset.is_zero();
	ownMovementCollisionModel.clear();
	ownPreciseCollisionModel.clear();

	if (!usingProvidedMovementCollision)
	{
		clear_movement_collision();
	}
	if (!usingProvidedPreciseCollision)
	{
		clear_precise_collision();
	}

	// create movement query and add collision
	if (moduleCollisionData)
	{
		if (CollisionModel* module = moduleCollisionData->get_movement_collision_model())
		{
			if (!usingProvidedMovementCollision)
			{
				if (noMovementCollision)
				{
					fallbackPreciseCollision.add(module->get_model());
				}
				else
				{
					movementCollision.add(module->get_model());
				}
				usePrimitiveForMovementCollision = false;
			}
		}
		if (CollisionModel* module = moduleCollisionData->get_precise_collision_model())
		{
			if (!usingProvidedPreciseCollision)
			{
				preciseCollision.add(module->get_model());
				usePrimitiveForPreciseCollision = false;
			}
		}
	}
	if (collisionPrimitiveRadius > 0.0f ||
		radiusForGradient > 0.0f ||
		radiusForDetection > 0.0f)
	{
		if (!collisionPrimitiveCentreDistance.is_zero())
		{
			// capsule
			Vector3 locationA, locationB;
			update_movement_query(OUT_ &locationA, OUT_ &locationB);
			if (collisionPrimitiveRadius > 0.0f)
			{
				Vector3 precLocationA = locationA + preciseCollisionPrimitiveCentreAdditionalOffset;
				Vector3 precLocationB = locationB + preciseCollisionPrimitiveCentreAdditionalOffset;
				float preciseRadius = preciseCollisionPrimitiveRadius.get(collisionPrimitiveRadius);
				if (preciseRadius != collisionPrimitiveRadius)
				{
					// we want ends to match, even if radius is slimmar
					Vector3 a2b = (locationB - locationA).normal();
					Vector3 extraOff = a2b * (collisionPrimitiveRadius - preciseRadius);
					precLocationA -= extraOff;
					precLocationB += extraOff;
				}
				add_primitive_to_own_collision_models(Collision::Capsule().set(Capsule(locationA, locationB, collisionPrimitiveRadius)),
					Collision::Capsule().set(Capsule(precLocationA, precLocationB, preciseRadius)), usePrimitiveForPreciseCollision);
			}
			else if (preciseCollisionPrimitiveRadius.is_set())
			{
				error(TXT("don't use precise collision primitive if normal not used [capsule] %S"), get_owner()->ai_get_name().to_char());
			}
		}
		else
		{
			// sphere
			Vector3 location = collisionPrimitiveCentreOffset;
			update_movement_query(OUT_ & location);
			if (collisionPrimitiveRadius > 0.0f)
			{
				Vector3 precLocation = location + preciseCollisionPrimitiveCentreAdditionalOffset;
				add_primitive_to_own_collision_models(Collision::Sphere().set(Sphere(location, collisionPrimitiveRadius)), Collision::Sphere().set(Sphere(precLocation, preciseCollisionPrimitiveRadius.get(collisionPrimitiveRadius))), usePrimitiveForPreciseCollision);
			}
			else if (preciseCollisionPrimitiveRadius.is_set())
			{
				error(TXT("don't use precise collision primitive if normal not used [sphere] %S"), get_owner()->ai_get_name().to_char());
			}
		}
		if (usePrimitiveForMovementCollision && !usingProvidedMovementCollision)
		{
			if (noMovementCollision)
			{
				fallbackPreciseCollision.add(&ownMovementCollisionModel);
			}
			else
			{
				movementCollision.add(&ownMovementCollisionModel);
			}
		}
		if (usePrimitiveForPreciseCollision && !usingProvidedPreciseCollision)
		{
			preciseCollision.add(&ownPreciseCollisionModel);
		}
	}
	else if (preciseCollisionPrimitiveRadius.is_set() || ! preciseCollisionPrimitiveCentreAdditionalOffset.is_zero())
	{
		error(TXT("don't use precise collision primitive if normal not used [nothing] %S"), get_owner()->ai_get_name().to_char());
	}

	update_bounding_box();
}

void ModuleCollision::update_movement_query(OPTIONAL_ OUT_ Vector3 * _locA, OPTIONAL_ OUT_ Vector3* _locB)
{
	if (radiusForGradient > 0.0f || radiusForDetection > 0.0f)
	{
		if (!collisionPrimitiveCentreDistance.is_zero())
		{
			// capsule
			Vector3 locationA = collisionPrimitiveCentreOffset - collisionPrimitiveCentreDistance * 0.5f;
			if (collisionPrimitiveCentreDistance.x == 0.0f && collisionPrimitiveCentreDistance.y == 0.0f)
			{
				locationA.z += gradientStepHole;
			}
			Vector3 locationB = collisionPrimitiveCentreOffset + collisionPrimitiveCentreDistance * 0.5f;
			use_movement_query_segment(Collision::QueryPrimitiveSegment(locationA, locationB));
			assign_optional_out_param(_locA, locationA);
			assign_optional_out_param(_locB, locationB);
		}
		else
		{
			// sphere
			Vector3 location = collisionPrimitiveCentreOffset;
			use_movement_query_point(Collision::QueryPrimitivePoint(location));
			assign_optional_out_param(_locA, location);
			assign_optional_out_param(_locB, location);
		}
	}
	else
	{
		assign_optional_out_param(_locA, collisionPrimitiveCentreOffset);
		assign_optional_out_param(_locB, collisionPrimitiveCentreOffset);
	}
}

float ModuleCollision::get_collision_primitive_height() const
{
	return collisionPrimitiveCentreOffset.z + collisionPrimitiveCentreDistance.z * 0.5f + collisionPrimitiveRadius;
}

void ModuleCollision::update_collision_primitive_height(float _height)
{
	/**
	 *				 _  top / height
	 *				/ \
	 *				 +  top sphere
	 *
	 *
	 *				 +  centre offset
	 *
	 *
	 *				 +  bottom sphere
	 *				\_/ bottom
	 *
	 *		------------------- ground level
	 */
	float wholeLength = collisionPrimitiveCentreDistance.z + collisionPrimitiveRadius * 2.0f;
	float bottomZ = collisionPrimitiveCentreOffset.z - wholeLength * 0.5f;
	Vector3 new_collisionPrimitiveCentreOffset = Vector3::zero;
	Vector3 new_collisionPrimitiveCentreDistance = Vector3::zero;
	// we need sane values, prevent offset to be to low and distance to be negative)
	new_collisionPrimitiveCentreOffset.z = max(collisionPrimitiveRadius, bottomZ + (_height - bottomZ) * 0.5f);
	new_collisionPrimitiveCentreDistance.z = max(0.0f, (_height - collisionPrimitiveRadius) - (bottomZ + collisionPrimitiveRadius));
	if ((new_collisionPrimitiveCentreOffset - collisionPrimitiveCentreOffset).length() > 0.01f ||
		(new_collisionPrimitiveCentreDistance - collisionPrimitiveCentreDistance).length() > 0.01f)
	{
		collisionPrimitiveCentreOffset = new_collisionPrimitiveCentreOffset;
		collisionPrimitiveCentreDistance = new_collisionPrimitiveCentreDistance;

		update_collision_model_and_query();
	}
}

void ModuleCollision::clear_movement_collision()
{
	movementCollision.clear();
	fallbackPreciseCollision.clear();
	// we will require full update this way
	if (collisionUpdatedForPoseMS)
	{
		collisionUpdatedForPoseMS->release();
		collisionUpdatedForPoseMS = nullptr;
	}
	usingProvidedMovementCollision = false;
}

void ModuleCollision::clear_precise_collision()
{
	preciseCollision.clear();
	// we will require full update this way
	if (collisionUpdatedForPoseMS)
	{
		collisionUpdatedForPoseMS->release();
		collisionUpdatedForPoseMS = nullptr;
	}
	usingProvidedPreciseCollision = false;
}

Collision::ModelInstance* ModuleCollision::use_movement_collision(Collision::Model const * _model, Transform const & _placement)
{
	clear_movement_collision();
	if (!usingProvidedPreciseCollision)
	{
		clear_precise_collision(); // clear precise collision too, if we have one, we will provide it, if not, we shall use movement collision
	}

	if (!noMovementCollision)
	{
		usingProvidedMovementCollision = true;

		return movementCollision.add(_model, _placement);
	}
	else
	{
		return fallbackPreciseCollision.add(_model, _placement);
	}
}

Collision::ModelInstance* ModuleCollision::use_precise_collision(Collision::Model const * _model, Transform const & _placement)
{
	clear_precise_collision();

	usingProvidedPreciseCollision = true;

	return preciseCollision.add(_model, _placement);
}

void ModuleCollision::use_movement_query_point(Collision::QueryPrimitivePoint const & _qpp)
{
	delete_and_clear(movementQuerySegment);
	delete_and_clear(movementQuerySegmentCollapsedToPoint);
	if (!movementQueryPoint)
	{
		movementQueryPoint = new Collision::QueryPrimitivePoint(_qpp);
	}
	else
	{
		*movementQueryPoint = _qpp;
	}
}

void ModuleCollision::use_movement_query_segment(Collision::QueryPrimitiveSegment const & _qps)
{
	delete_and_clear(movementQueryPoint);
	if (!movementQuerySegment)
	{
		movementQuerySegment = new Collision::QueryPrimitiveSegment(_qps);
	}
	else
	{
		*movementQuerySegment = _qps;
	}
	if (allowCollapsingSegmentToPoint)
	{
		Collision::QueryPrimitivePoint collapsed;
		segmentCollapsedToPointDistance = Vector3::distance(_qps.locationA, _qps.locationB);
		collapsed.location = (_qps.locationA + _qps.locationB) * 0.5f;
		if (!movementQuerySegmentCollapsedToPoint)
		{
			movementQuerySegmentCollapsedToPoint = new Collision::QueryPrimitivePoint(collapsed);
		}
		else
		{
			*movementQuerySegmentCollapsedToPoint = collapsed;
		}
	}
	else
	{
		delete_and_clear(movementQuerySegmentCollapsedToPoint);
	}
}

///

template <typename CollisionQueryPrimitive>
class CheckGradient
{
public:
	// distances will be relative to placement, result gradient is in _placement's reference frame
	CheckGradient(CollisionQueryPrimitive const & _queryPrimitive, Matrix44 const & _placement, float _maxDistanceGradient, float _maxDistanceDetect, IModulesOwner const * _forModulesOwner, float _epsilon = 0.005f);

	void update_stiff_gradient() { updateStiffGradient = true; }
	void check_first_collision_only() { firstCollisionCheckOnly = true; }

	void run();

	Vector3 get_gradient() const;
	Vector3 get_stiff_gradient() const;
	void store_collided_with_in(ModuleCollision* _collision) const;

	int get_detected_collision_with_count() const { return detectedCollisionWith.get_size(); }
	Collision::ICollidableObject const* get_detected_collision_with(int _idx) const { return detectedCollisionWith.is_index_valid(_idx) ? detectedCollisionWith[_idx] : nullptr; }

#ifdef AN_DEVELOPMENT
	Collision::CheckGradientDistances const & get_distances() const { return distances; }
	float get_epsilon() const { return epsilon; }
#endif

	bool does_collide() const { return hasCollided; }

private:
	IModulesOwner const * forModulesOwner;
	Collision::CheckGradientPoints<CollisionQueryPrimitive> points;
	Collision::CheckGradientDistances distances;
	Collision::CheckGradientDistances distancesStiff;
	float distanceGradient = 0.0f;
	float distanceDetect = 0.0f;
	bool checkGradient = false;
	bool checkDetect = false;
	float epsilon;
	bool firstCollisionCheckOnly = false;
	bool updateStiffGradient = false;

	Collision::Flags collidesWithFlags = Collision::Flags::defaults();
	Collision::Flags detectsFlags = Collision::Flags::none();

	bool hasCollided = false;
	ArrayStatic<Collision::ICollidableObject const*, 128> detectedCollisionWith;
	ArrayStatic<DetectedCollidable, 128> detected;

	void run_for_room(IModulesOwner const * _forModulesOwner, Collision::CheckGradientContext<CollisionQueryPrimitive> & _scgc, Collision::CheckGradientContext<CollisionQueryPrimitive> & _scgcStiff, PresenceLink const * forLink); // _scgcStiff is only for stiff result
	inline bool should_consider_collidable_object_for_collision(Collision::CheckGradientContext<CollisionQueryPrimitive> & _scgc, Collision::ICollidableObject const * _collidableObject);
	inline bool should_consider_collidable_object_for_detect(Collision::CheckGradientContext<CollisionQueryPrimitive> & _scgc, Collision::ICollidableObject const * _collidableObject);
};

template <typename CollisionQueryPrimitive>
CheckGradient<CollisionQueryPrimitive>::CheckGradient(CollisionQueryPrimitive const & _queryPrimitive, Matrix44 const & _placement, float _maxDistanceGradient, float _maxDistanceDetect, IModulesOwner const * _forModulesOwner, float _epsilon)
: forModulesOwner(_forModulesOwner)
, points(_queryPrimitive, _placement, _epsilon)
, distances(_maxDistanceGradient, _epsilon)
, distancesStiff(_maxDistanceGradient, _epsilon)
, distanceGradient(_maxDistanceGradient)
, distanceDetect(_maxDistanceDetect)
, checkGradient(_maxDistanceGradient > 0.0f)
, checkDetect(_maxDistanceDetect > 0.0f)
, epsilon(_epsilon)
{
	SET_EXTRA_DEBUG_INFO(detectedCollisionWith, TXT("CheckGradient.detectedCollisionWith"));
	SET_EXTRA_DEBUG_INFO(detected, TXT("CheckGradient.detected"));
}

template <typename CollisionQueryPrimitive>
void CheckGradient<CollisionQueryPrimitive>::run()
{
	scoped_call_stack_info(TXT("CheckGradient<CollisionQueryPrimitive>::run"));
	MEASURE_PERFORMANCE(checkGradient_run);
	// TODO change to start at current room and go through neighbouring doors
	PresenceLink const * presenceLink = forModulesOwner && forModulesOwner->get_presence() ? forModulesOwner->get_presence()->get_presence_links() : nullptr;
	collidesWithFlags = Collision::Flags::defaults();
	detectsFlags = Collision::Flags::none();
	if (auto* mc = forModulesOwner->get_collision())
	{
		if (checkGradient)
		{
			collidesWithFlags = mc->get_collides_with_flags();
		}
		if (checkDetect)
		{
			detectsFlags = mc->get_detects_flags();
			an_assert(!detectsFlags.is_empty());
		}
	}
	while (presenceLink)
	{
		bool ok = true;
		{
			scoped_call_stack_info(TXT("is presence link ok?"));
			MEASURE_PERFORMANCE(checkIfPresenceOk);

			scoped_call_stack_ptr(presenceLink);

			PresenceLink const * p = presenceLink;
			while (p)
			{
				scoped_call_stack_ptr(p);
				if (p->get_modules_owner() == forModulesOwner) // it could get invalidated
				{
					if (auto* door = p->get_through_door())
					{
						debug_context(p->get_in_room());
						debug_subject(p->get_modules_owner());
						door->debug_draw_hole(true, Colour::green);
						debug_draw_sphere(true, true, Colour::orange, 0.2f, Sphere(p->get_placement_in_room_for_presence_primitive().get_translation(), 0.1f));
						debug_no_subject();
						debug_no_context();
						Vector3 presenceLoc = p->get_placement_in_room_for_presence_primitive().get_translation();
						REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info_str_printf(TXT("is inside hole dr'%p (d%p) in room r%p"), door, door->get_door(), p->get_in_room());
						if (!door->is_inside_hole(presenceLoc))
						{
							// we do not care about 
							ok = false;
							break;
						}
					}
				}
				else
				{
					ok = false; // invalidated
					break;
				}
				p = p->get_link_towards_owner();
			}
		}
		if (ok)
		{
			scoped_call_stack_info(TXT("pl ok - run for room"));
			Collision::Flags checkFlags = Collision::Flags::none();
			{
				MEASURE_PERFORMANCE(checkGradient_setupPresenceLinkFlags);
				if (checkGradient && presenceLink->is_valid_for_collision_gradient())
				{
					checkFlags |= collidesWithFlags;
				}
				if (checkDetect && presenceLink->is_valid_for_collision_detection())
				{
					checkFlags |= detectsFlags;
				}
			}
			if (!checkFlags.is_empty())
			{
				Collision::CheckGradientContext<CollisionQueryPrimitive> cgc(points.transformed_to_local_of(presenceLink->get_transform_to_room_matrix()), distances, presenceLink->get_clip_planes_for_collision());
				Collision::CheckGradientContext<CollisionQueryPrimitive> cgcStiff(points.transformed_to_local_of(presenceLink->get_transform_to_room_matrix()), distancesStiff, presenceLink->get_clip_planes_for_collision());
				cgc.use_collision_flags(checkFlags);
				cgc.first_collision_check_only(firstCollisionCheckOnly);
				cgcStiff.use_collision_flags(checkFlags);
				cgcStiff.first_collision_check_only(firstCollisionCheckOnly);
				run_for_room(forModulesOwner, cgc, cgcStiff, presenceLink);
				if (firstCollisionCheckOnly && !detectedCollisionWith.is_empty())
				{
					return;
				}
				distances = cgc.distances;
				distancesStiff = cgcStiff.distances;
			}
		}
		presenceLink = presenceLink->get_next_in_object();
	}
}

template <typename CollisionQueryPrimitive>
bool CheckGradient<CollisionQueryPrimitive>::should_consider_collidable_object_for_collision(Collision::CheckGradientContext<CollisionQueryPrimitive> & _cgc, Collision::ICollidableObject const * _collidableObject)
{
	if (_collidableObject)
	{
		return Collision::Flags::check(collidesWithFlags, _collidableObject->get_collision_flags());
	}
	else
	{
		return true;
	}
}

template <typename CollisionQueryPrimitive>
bool CheckGradient<CollisionQueryPrimitive>::should_consider_collidable_object_for_detect(Collision::CheckGradientContext<CollisionQueryPrimitive> & _cgc, Collision::ICollidableObject const * _collidableObject)
{
	if (_collidableObject)
	{
		return Collision::Flags::check(detectsFlags, _collidableObject->get_collision_flags());
	}
	else
	{
		return false;
	}
}

template <typename CollisionQueryPrimitive>
void CheckGradient<CollisionQueryPrimitive>::run_for_room(IModulesOwner const * _forModulesOwner, Collision::CheckGradientContext<CollisionQueryPrimitive> & _cgc, Collision::CheckGradientContext<CollisionQueryPrimitive> & _cgcStiff, PresenceLink const * forLink)
{
	scoped_call_stack_info(TXT("CheckGradient<CollisionQueryPrimitive>::run_for_room"));

	if (!forLink->get_modules_owner())
	{
		// invalidated?
		return;
	}

	Room const * room = forLink->get_in_room();

	MEASURE_PERFORMANCE_CONTEXT(room->get_performance_context_info());

	debug_context(forLink->get_in_room());
	debug_subject(forLink->get_modules_owner());

	{
		scoped_call_stack_info(TXT("against room"));
		MEASURE_PERFORMANCE_COLOURED(updateGradientAgainstRoom, Colour::zxRed);
		if (should_consider_collidable_object_for_collision(_cgc, room))
		{
			_cgc.minDistanceFound.clear();
			_cgc.maxDistanceToCheck = distanceGradient; // we won't store detected anyway!
			_cgc.maxDistanceToUpdateGradient = distanceGradient;
			_cgc.focus_on_gradient_only(true);
			if (_cgc.update(Matrix44::identity, room, Meshes::PoseMatBonesRef(), room->get_movement_collision(), 1.0f)) // with room? difference is always 1.0
			{
				an_assert(_cgc.minDistanceFound.is_set());
				hasCollided |= _cgc.minDistanceFound.get() < distanceGradient;
			}
			_cgc.focus_on_gradient_only(false);
		}
	}

	Collision::ICollidableObject const * forICOOwner = fast_cast<Collision::ICollidableObject>(_forModulesOwner);
	ModuleCollision const * mcOwner = _forModulesOwner->get_collision();

	Range3 queryPrimitiveBoundingBox = _cgc.points.queryPrimitiveInPlace.calculate_bounding_box(checkDetect? max(distanceDetect, _cgc.distances.maxDistance) : _cgc.distances.maxDistance);

	{
		scoped_call_stack_info(TXT("against presence links"));
		MEASURE_PERFORMANCE_COLOURED(updateGradientAgainstPresenceLinks, Colour::zxMagenta);
		PresenceLink const * link = room->get_presence_links();
		while (link)
		{
			if (link != forLink &&
				link->is_valid_for_collision())
			{
				if (queryPrimitiveBoundingBox.overlaps(link->get_collision_bounding_box()))
				{
					if (IModulesOwner const * object = link->get_modules_owner())
					{
						scoped_call_stack_ptr(object);
						an_assert_immediate(_forModulesOwner == forLink->get_modules_owner());
						if (object == _forModulesOwner && !mcOwner->should_allow_collisions_with_self())
						{
							// skip collisions with self
							continue;
						}
						auto* collidableObject = fast_cast<Collision::ICollidableObject>(object);
						bool checkingCollision = checkGradient && should_consider_collidable_object_for_collision(_cgc, collidableObject);
						bool checkingDetect = checkDetect && should_consider_collidable_object_for_detect(_cgc, collidableObject);
						if (checkingCollision || checkingDetect)
						{
							ShouldIgnoreCollisionWithResult::Type whatToDo = mcOwner->should_ignore_collision_with(object);
							if (whatToDo == ShouldIgnoreCollisionWithResult::Collide ||
								(whatToDo == ShouldIgnoreCollisionWithResult::IgnoreButCheck && ! detectedCollisionWith.does_contain(collidableObject) && DetectedCollidable::does_contain(detected, collidableObject) && !_cgc.should_only_check_first_collision()))
							{
								scoped_call_stack_info(TXT("ok, check"));
								if (ModuleCollision const * mc = object->get_collision())
								{
									scoped_call_stack_info(TXT("collision module"));
									_cgc.only_check(!checkingCollision || whatToDo == ShouldIgnoreCollisionWithResult::IgnoreButCheck);
									_cgc.focus_on_gradient_only(!checkingDetect);
									_cgc.minDistanceFound.clear();
									_cgc.maxDistanceToCheck = max(distanceGradient, distanceDetect);
									_cgc.maxDistanceToUpdateGradient = distanceGradient;
									/* assume that if it is not attached, we should treat it as an object with infinite mass */
									auto poseRef = object->get_appearance() ? object->get_appearance()->get_final_pose_mat_MS_ref() : Meshes::PoseMatBonesRef();
									float massDifference = Collision::ICollidableObject::calculate_mass_difference(
											_forModulesOwner->get_presence()->is_attached() ? nullptr : forICOOwner,
											object->get_presence()->is_attached() ? nullptr : collidableObject);
									bool collided = _cgc.update(link->get_placement_in_room_matrix(), collidableObject, poseRef, mc->get_movement_collision(), massDifference);
									if (collided)
									{
										if (updateStiffGradient)
										{
											bool enforcesStiffCollisions = mc->does_enforce_stiff_collisions();
											if (!enforcesStiffCollisions)
											{
												auto* imo = object->get_instigator();
												while (imo)
												{
													if (ModuleCollision const* mc = imo->get_collision())
													{
														if (mc->does_enforce_stiff_collisions())
														{
															enforcesStiffCollisions = true;
															break;
														}
													}
													auto* up = imo->get_instigator();
													if (up == imo)
													{
														break;
													}
													imo = up;
												}
											}
											if (enforcesStiffCollisions)
											{
												_cgcStiff.update(link->get_placement_in_room_matrix(), collidableObject, poseRef, mc->get_movement_collision(), massDifference);
											}
										}
										an_assert(_cgc.minDistanceFound.is_set());
										if (checkingCollision && _cgc.minDistanceFound.get() < distanceGradient)
										{
											hasCollided = true;
											detectedCollisionWith.push_back_unique(collidableObject);
										}
										if (checkingDetect && _cgc.minDistanceFound.get() < distanceDetect)
										{
											detected.push_back_unique(DetectedCollidable().for_object(collidableObject).at_distance(_cgc.minDistanceFound.get()));
										}
										if (firstCollisionCheckOnly)
										{
											// that's it, we just checked first collision
											return;
										}
									}
									_cgc.only_check(false);
									_cgc.focus_on_gradient_only(false);
								}
							}
						}
					}
				}
			}
			link = link->get_next_in_room();
		}
	}

	{
		scoped_call_stack_info(TXT("against doors"));
		MEASURE_PERFORMANCE_COLOURED(updateGradientAgainstDoors, Colour::zxBlue);
		for_every_ptr(door, room->get_doors())
		{
			if (! door->is_visible())
			{
				continue;
			}
			if (door->are_meshes_ready() &&
				!door->get_movement_collision().is_empty())
			{
				if (Collision::Flags::check(_cgc.get_collision_flags(), door->get_collision_flags()))
				{
					if (should_consider_collidable_object_for_collision(_cgc, door))
					{
						_cgc.update(door->get_scaled_placement_matrix(), door, Meshes::PoseMatBonesRef(), door->get_movement_collision(), Collision::ICollidableObject::calculate_mass_difference(forICOOwner, door));
					}
				}
			}
		}
	}

	debug_no_subject(); 
	debug_no_context();
}

template <typename CollisionQueryPrimitive>
Vector3 CheckGradient<CollisionQueryPrimitive>::get_gradient() const
{
	return distances.get_gradient(epsilon);
}

template <typename CollisionQueryPrimitive>
Vector3 CheckGradient<CollisionQueryPrimitive>::get_stiff_gradient() const
{
	return distancesStiff.get_gradient(epsilon);
}

template <typename CollisionQueryPrimitive>
void CheckGradient<CollisionQueryPrimitive>::store_collided_with_in(ModuleCollision* _collision) const
{
	Collision::CheckGradientDistances::Dir const * dir = distances.dirs;
	for (int i = 0; i < 6; ++i, ++dir)
	{
		if (dir->object || dir->shape)
		{
			CollisionInfo ci;
			ci.collidedWithObject = dir->object;
			ci.collidedWithShape = dir->shape;
			_collision->store_collided_with(ci);
		}
	}
	for_every_ptr(c, detectedCollisionWith)
	{
		_collision->store_detected_collision_with(c);
	}
	for_every(dc, detected)
	{
		_collision->store_detected(*dc);
	}
}

///

ShouldIgnoreCollisionWithResult::Type ModuleCollision::should_ignore_collision_with(IModulesOwner const * _object) const
{
	// ignore when attached to each other
	if (_object->get_presence()->is_attached_at_all_to(get_owner()) ||
		get_owner()->get_presence()->is_attached_at_all_to(_object))
	{
		return ShouldIgnoreCollisionWithResult::Ignore;
	}
	auto* collidableObject = fast_cast<Collision::ICollidableObject>(_object);
	auto* mc = _object->get_collision();
	{
		if (!dontCollideWithIfAttachedTo.is_empty())
		{
			// should be just one
			for_every(d, dontCollideWithIfAttachedTo)
			{
				auto* dimo = fast_cast<IModulesOwner>(d->object);
				auto* o = _object;
				while (o)
				{
					if (o == dimo)
					{
						return ShouldIgnoreCollisionWithResult::IgnoreButCheck;
					}
					if (auto* op = o->get_presence())
					{
						o = op->get_attached_to();
					}
					else
					{
						o = nullptr;
					}
				}
			}
		}
		if ((collidableObject && dontCollideWithUntilNoCollision.does_contain(collidableObject)) ||
			(mc && mc->dontCollideWithUntilNoCollision.does_contain(collidableOwner)))
		{
			return ShouldIgnoreCollisionWithResult::IgnoreButCheck;
		}
		if ((collidableObject && dontCollideWith.does_contain(collidableObject)) ||
			(mc && mc->dontCollideWith.does_contain(collidableOwner)))
		{
			return ShouldIgnoreCollisionWithResult::Ignore;
		}
	}
	return ShouldIgnoreCollisionWithResult::Collide;
}

void ModuleCollision::update_collision_skipped()
{
	for_every(ci, collidedWith)
	{
		ci->presenceLink = nullptr; // no longer valid!
		ci->collidedWithShape = nullptr;
	}
}

void ModuleCollision::update_collisions(float _deltaTime)
{
	MEASURE_PERFORMANCE(update_collisions);
	scoped_call_stack_info(TXT("update_collisions"));

	int wasColliding = collidedWith.get_size();
	collidedWith.clear();
	detectedCollisionWith.clear();
	detected.clear();
	if (frameDelayOnActualUpdatingCollisions <= 0)
	{
		update_gradient_and_detection();
	}
	else
	{
		--frameDelayOnActualUpdatingCollisions;
	}
	int isColliding = collidedWith.get_size();

	scoped_call_stack_info(TXT("update_collisions [1]"));

	{
		// timeLeft is post check to allow for a single frame to always be processed
		scoped_call_stack_info(TXT("dont collide with"));
		// update - when we do not collide, we should drop them
		for (int i = 0; i < dontCollideWithUntilNoCollision.get_size(); ++i)
		{
			if (dontCollideWithUntilNoCollision[i].timeLeft.is_set())
			{
				if (dontCollideWithUntilNoCollision[i].timeLeft.get() <= 0.0f)
				{
					dontCollideWithUntilNoCollision[i].timeLeft.clear();
				}
				else
				{
					dontCollideWithUntilNoCollision[i].timeLeft = dontCollideWithUntilNoCollision[i].timeLeft.get() - _deltaTime;
				}
			}
			if (!dontCollideWithUntilNoCollision[i].timeLeft.is_set())
			{
				if (!detectedCollisionWith.does_contain(dontCollideWithUntilNoCollision[i].object))
				{
					dontCollideWithUntilNoCollision.remove_fast_at(i);
					--i;
				}
			}
		}
		// remove dontCollideWith
		for (int i = 0; i < dontCollideWith.get_size(); ++i)
		{
			if (dontCollideWith[i].timeLeft.is_set())
			{
				if (dontCollideWith[i].timeLeft.get() <= 0.0f)
				{
					dontCollideWith.remove_fast_at(i);
					--i;
				}
				else
				{
					dontCollideWith[i].timeLeft = dontCollideWith[i].timeLeft.get() - _deltaTime;
				}
			}
		}
		// remove dontCollideWithIfAttachedTo
		for (int i = 0; i < dontCollideWithIfAttachedTo.get_size(); ++i)
		{
			if (dontCollideWithIfAttachedTo[i].timeLeft.is_set())
			{
				if (dontCollideWithIfAttachedTo[i].timeLeft.get() <= 0.0f)
				{
					dontCollideWithIfAttachedTo.remove_fast_at(i);
					--i;
				}
				else
				{
					dontCollideWithIfAttachedTo[i].timeLeft = dontCollideWithIfAttachedTo[i].timeLeft.get() - _deltaTime;
				}
			}
		}
	}

	{
		scoped_call_stack_info(TXT("colliding with non scenery?"));
		collidingWithNonScenery = false;
		for_every(collided, collidedWith)
		{
			if (auto imo = fast_cast<IModulesOwner>(collided->collidedWithObject.get()))
			{
				imo = imo->get_instigator(); // to catch items held by actors
				if (auto * actor = fast_cast<Actor>(imo))
				{
					collidingWithNonScenery = true;
					break;
				}
			}
		}
	}
	scoped_call_stack_info(TXT("update_collisions [2]"));
	timeSinceLastCollisionSound += _deltaTime;
	if ((collisionSound.is_valid() || playCollisionSoundThroughMaterial) &&
		timeSinceLastCollisionSound >= collisionSoundNoOftenThan &&
		(collisionSoundMinSpeed == 0.0f || get_owner()->get_presence()->get_velocity_linear().length_squared() >= sqr(collisionSoundMinSpeed)) &&
		!collidedWith.is_empty() && 
		isColliding && ! wasColliding)
	{
		scoped_call_stack_info(TXT("new collision"));
		if (collisionSound.is_valid())
		{
			if (auto * ms = get_owner()->get_sound())
			{
				ms->play_sound(collisionSound);
			}
		}
		if (playCollisionSoundThroughMaterial)
		{
			scoped_call_stack_info(TXT("playCollisionSoundThroughMaterial"));
			for_every(cw, collidedWith)
			{
				PhysicalMaterial const * material = cw->material;
				if (!material && cw->collidedWithShape)
				{
					material = fast_cast<PhysicalMaterial>(cw->collidedWithShape->get_material());
				}
				if (!material && cw->collidedWithObject.get())
				{
					if (auto* imo = fast_cast<IModulesOwner>(cw->collidedWithObject.get()))
					{
						if (auto* mc = imo->get_collision())
						{
							material = mc->get_movement_collision_material();
							if (!material)
							{
								material = mc->get_precise_collision_material();
							}
						}
					}
				}
				if (!material && cw->presenceLink)
				{
					if (auto* mc = cw->presenceLink->get_modules_owner()->get_collision())
					{
						material = mc->get_movement_collision_material();
						if (!material)
						{
							material = mc->get_precise_collision_material();
						}
					}
				}
				if (material)
				{
					float volume = 1.0f;
					float minSpeed = playCollisionSoundMinSpeed;
					float fullVolSpeed = playCollisionSoundFullVolumeSpeed;
					bool play = playCollisionSound;
					if (playCollisionSoundUsingAttachedToSetup)
					{
						if (auto* a = get_owner()->get_presence()->get_attached_to())
						{
							if (auto* c = a->get_collision())
							{
								play = c->playCollisionSound;
								if (c->playCollisionSoundThroughMaterial)
								{
									minSpeed = c->playCollisionSoundMinSpeed;
									fullVolSpeed = c->playCollisionSoundFullVolumeSpeed;
								}
								else
								{
									// do not process anymore as we shouldn't be playing sound anyway
									break;
								}
							}
						}
					}
					if (play)
					{
						if (minSpeed >= 0.0f || fullVolSpeed > 0.0f)
						{
							if (auto * p = get_owner()->get_presence())
							{
								float speed = p->get_velocity_linear().length();
								if (speed <= minSpeed)
								{
									volume = 0.0f;
								}
								else if (fullVolSpeed > minSpeed)
								{
									volume = clamp((speed - minSpeed) / max(0.01f, fullVolSpeed - minSpeed), 0.0f, 1.5f);
								}
							}
						}
						if (volume > 0.0f)
						{
							material->play_collision(get_owner(), cw->collidedInRoom.get(), cw->collisionLocation, volume);
						}
					}
					break;
				}
			}
		}
		timeSinceLastCollisionSound = 0.0f;
	}
}

bool ModuleCollision::check_if_colliding(Vector3 const & _addOffsetWS, float _radiusCoef, CheckIfCollidingContext* _context) const
{
	MEASURE_PERFORMANCE(check_if_colliding);

	if (movementQueryPoint || movementQuerySegmentCollapsedToPoint)
	{
		auto const * useMovementQuery = movementQueryPoint;
		float useRadiusForGradient = get_radius_for_gradient() * _radiusCoef;
		
		if (!movementQueryPoint)
		{
			an_assert(movementQuerySegmentCollapsedToPoint);
			useMovementQuery = movementQuerySegmentCollapsedToPoint;
			useRadiusForGradient += segmentCollapsedToPointDistance * 0.5f; // some approximation?
		}
		debug_filter(moduleCollisionCheckIfColliding);
		debug_subject(get_owner());

		Transform placement = get_owner()->get_presence()->get_placement();
		placement.set_translation(placement.get_translation() + _addOffsetWS);
		CheckGradient<Collision::QueryPrimitivePoint> cg(*useMovementQuery, placement.to_matrix(), useRadiusForGradient, 0.0f, get_owner());

		cg.check_first_collision_only();

		cg.run();

		debug_subject(nullptr);
		debug_no_filter();

		if (_context)
		{
			_context->detectedCollisionWith = cg.get_detected_collision_with(0);
		}

		return cg.does_collide();
	}
	else if (movementQuerySegment)
	{
		todo_important(TXT("not supported and never may be, maybe you should add allowCollapsingSegmentToPoint to collision module?"));
	}

	return false;
}

bool ModuleCollision::check_if_colliding_stretched(Vector3 const & _endOffsetWS, float _radiusCoef, CheckIfCollidingContext* _context) const
{
	MEASURE_PERFORMANCE(check_if_colliding_stretch);

	if (movementQueryPoint || movementQuerySegmentCollapsedToPoint)
	{
		auto const * useMovementQuery = movementQueryPoint;
		float useRadiusForGradient = get_radius_for_gradient() * _radiusCoef;
		
		if (!movementQueryPoint)
		{
			an_assert(movementQuerySegmentCollapsedToPoint);
			useMovementQuery = movementQuerySegmentCollapsedToPoint;
			useRadiusForGradient += segmentCollapsedToPointDistance * 0.5f; // some approximation?
		}
		debug_filter(moduleCollisionCheckIfCollidingStretched);
		debug_subject(get_owner());

		Transform placement = get_owner()->get_presence()->get_placement();

		Collision::QueryPrimitiveSegment movementQueryPointStretchedToSegment(useMovementQuery->location, useMovementQuery->location + placement.vector_to_local(_endOffsetWS));

		CheckGradient<Collision::QueryPrimitiveSegment> cg(movementQueryPointStretchedToSegment, placement.to_matrix(), useRadiusForGradient, 0.0f, get_owner());

		cg.check_first_collision_only();

		cg.run();

		debug_subject(nullptr);
		debug_no_filter();

		if (_context)
		{
			_context->detectedCollisionWith = cg.get_detected_collision_with(0);
		}

		return cg.does_collide();
	}
	else if (movementQuerySegment)
	{
		todo_important(TXT("not supported and never may be, maybe you should add allowCollapsingSegmentToPoint to collision module?"));
	}

	return false;
}

void ModuleCollision::update_gradient_and_detection()
{
	scoped_call_stack_info(TXT("update_gradient_and_detection"));

	MEASURE_PERFORMANCE(update_gradient_and_detection);

	if (doEarlyCheckForUpdateGradient)
	{
		MEASURE_PERFORMANCE_COLOURED(updateGradientEarlyCheck, Colour::zxYellow);
		if (auto* imo = get_owner())
		{
			if (auto* p = imo->get_presence())
			{
				if (auto* r = p->get_in_room())
				{
					// we assume that this is used for small objects interacting with small objects, therefore 3x radius should be enough
					if (!r->is_close_to_collidable(p->get_placement().get_translation(), get_collides_with_flags(), 3.0f * max(get_radius_for_gradient(), get_radius_for_detection())))
					{
						// ignore gradients
						collisionGradient = Vector3::zero;
						collisionStiffGradient = Vector3::zero;
						return;
					}
				}
			}
		}
	}

	debug_filter(moduleCollisionUpdateGradient);
	debug_subject(get_owner());

	Vector3 newCollisionGradient = Vector3::zero;
	Vector3 newCollisionStiffGradient = Vector3::zero;
	if (movementQueryPoint)
	{
		CheckGradient<Collision::QueryPrimitivePoint> cg(*movementQueryPoint, get_owner()->get_presence()->get_placement().to_matrix(), get_radius_for_gradient(), get_radius_for_detection(), get_owner());

		cg.update_stiff_gradient();

		cg.run();

#ifdef AN_DEBUG_RENDERER
		debug_context(get_owner()->get_presence()->get_in_room());
		debug_subject(get_owner());
		debug_push_transform(get_owner()->get_presence()->get_placement());
		debug_draw_sphere(true, false, Colour::yellow, 0.2f, Sphere(movementQueryPoint->location, get_radius_for_gradient()));
		if (get_radius_for_detection())
		{
			debug_draw_sphere(true, false, Colour::cyan, 0.2f, Sphere(movementQueryPoint->location, get_radius_for_detection()));
		}
#ifdef AN_DEVELOPMENT
		for_count(int, i, 6)
		{
			float dist = cg.get_distances().dirs[i].distance;
			Vector3 off = dist * Collision::CheckGradientPoint::get_dir((Collision::CheckGradientPoint::Type)i);
			debug_draw_line(true, dist < get_radius_for_gradient() - cg.get_epsilon() ? Colour::orange.with_alpha(0.5f) : Colour::green.with_alpha(0.5f), Vector3::zero, off);
		}
#endif
		debug_draw_line(true, Colour::orange, Vector3::zero, cg.get_gradient());
		debug_pop_transform();
		debug_no_subject(); 
		debug_no_context();
#endif

		// get from placement's reference frame to room's/global
		newCollisionGradient = get_owner()->get_presence()->get_placement().vector_to_world(cg.get_gradient());
		newCollisionStiffGradient = get_owner()->get_presence()->get_placement().vector_to_world(cg.get_stiff_gradient());

		if (storeGradientCollisionInfo)
		{
			cg.store_collided_with_in(this);
		}
	}
	else if (movementQuerySegment)
	{
		CheckGradient<Collision::QueryPrimitiveSegment> cg(*movementQuerySegment, get_owner()->get_presence()->get_placement().to_matrix(), get_radius_for_gradient(), get_radius_for_detection(), get_owner());

		cg.update_stiff_gradient();

		cg.run();

#ifdef AN_DEBUG_RENDERER
		debug_context(get_owner()->get_presence()->get_in_room());
		debug_subject(get_owner());
		debug_push_transform(get_owner()->get_presence()->get_placement());
		debug_draw_line(true, Colour::yellow, movementQuerySegment->locationA, movementQuerySegment->locationB);
		debug_draw_capsule(true, false, Colour::yellow, 0.2f, Capsule(movementQuerySegment->locationA, movementQuerySegment->locationB, get_radius_for_gradient()));
#ifdef AN_DEVELOPMENT
		for_count(int, i, 6)
		{
			float dist = cg.get_distances().dirs[i].distance;
			Vector3 off = dist * Collision::CheckGradientPoint::get_dir((Collision::CheckGradientPoint::Type)i);
			debug_draw_line(true, dist < get_radius_for_gradient() - cg.get_epsilon() ? Colour::orange.with_alpha(0.5f) : Colour::green.with_alpha(0.5f), Vector3::zero, off);
		}
#endif
		debug_draw_line(true, Colour::orange, Vector3::zero, cg.get_gradient());
		debug_pop_transform();
		debug_no_subject();
		debug_no_context();
#endif

		// get from placement's reference frame to room's/global
		newCollisionGradient = get_owner()->get_presence()->get_placement().vector_to_world(cg.get_gradient());
		newCollisionStiffGradient = get_owner()->get_presence()->get_placement().vector_to_world(cg.get_stiff_gradient());

		if (storeGradientCollisionInfo)
		{
			cg.store_collided_with_in(this);
		}
	}

	if (! newCollisionGradient.is_zero() && ! collisionGradient.is_zero())
	{
		// prevent over compensation, this should work nicely for in-corner cases, for 90fps
		collisionGradient = newCollisionGradient * clamp(0.25f + Vector3::dot(newCollisionGradient.normal(), collisionGradient.normal()), 0.25f, 1.0f);
	}
	else
	{
		collisionGradient = newCollisionGradient;
	}

	if (! newCollisionStiffGradient.is_zero() && ! collisionStiffGradient.is_zero())
	{
		// prevent over compensation, this should work nicely for in-corner cases, for 90fps
		collisionStiffGradient = newCollisionStiffGradient * clamp(0.25f + Vector3::dot(newCollisionStiffGradient.normal(), collisionStiffGradient.normal()), 0.25f, 1.0f);
	}
	else
	{
		collisionStiffGradient = newCollisionStiffGradient;
	}

	debug_subject(nullptr);
	debug_no_filter();
}

void ModuleCollision::advance__update_collisions(IMMEDIATE_JOB_PARAMS)
{
	AdvanceContext const * ac = plain_cast<AdvanceContext const>(_executionContext);
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE_COLOURED(updateGradient, Colour::zxGreen);
		if (ModuleCollision* self = modulesOwner->get_collision())
		{
			scoped_call_stack_info(TXT("advance__update_collisions"));
			scoped_call_stack_ptr(modulesOwner);
			scoped_call_stack_info(modulesOwner->ai_get_name().to_char());
			if (auto* room = modulesOwner->get_presence()->get_in_room())
			{
				MEASURE_PERFORMANCE_COLOURED(updateGradient_update, Colour::zxGreenBright);
				self->update_collisions(ac->get_delta_time());
			}
		}
		else
		{
			an_assert(false, TXT("shouldn't be called"));
		}
	}
}

void ModuleCollision::log_collision_models(LogInfoContext& _log, bool _movement, bool _precise) const
{
	if (_movement)
	{
		_log.log(TXT("movement collision"));
		LOG_INDENT(_log);
		movementCollision.log(_log);
	}
	if (_precise && preciseCollision.is_empty())
	{
		_log.log(TXT("fallback precise collision"));
		LOG_INDENT(_log);
		fallbackPreciseCollision.log(_log);
	}
	if (_precise)
	{
		_log.log(TXT("precise collision"));
		LOG_INDENT(_log);
		preciseCollision.log(_log);
	}
}

void ModuleCollision::debug_draw(int _againstCollision) const
{
	debug_context(get_owner()->get_presence()->get_in_room());
	debug_subject(get_owner());
	debug_push_transform(get_owner()->get_presence()->get_placement());
	if (_againstCollision == NONE || _againstCollision == AgainstCollision::Movement)
	{
		if (_againstCollision == AgainstCollision::Movement)
		{
			if (!get_movement_collision().get_bounding_box().is_empty())
			{
				debug_draw_box(true, false, Colour::white, 0.0f, Box(get_movement_collision().get_bounding_box()));
			}
		}
		get_movement_collision().debug_draw(true, false, Colour::red, 0.2f, get_owner()->get_appearance() ? get_owner()->get_appearance()->get_final_pose_mat_MS_ref() : Meshes::PoseMatBonesRef());
	}
	if (_againstCollision == NONE || _againstCollision == AgainstCollision::Precise)
	{
		if (_againstCollision == AgainstCollision::Precise)
		{
			if (!get_precise_collision().get_bounding_box().is_empty())
			{
				debug_draw_box(true, false, Colour::white, 0.0f, Box(get_precise_collision().get_bounding_box()));
			}
		}
		get_precise_collision().debug_draw(true, false, Colour::cyan, 0.2f, get_owner()->get_appearance() ? get_owner()->get_appearance()->get_final_pose_mat_MS_ref() : Meshes::PoseMatBonesRef());
	}
	todo_future(TXT("draw radius for gradient"));
	debug_pop_transform();
	debug_no_subject();
	debug_no_context();
}

void ModuleCollision::should_consider_for_collision(DoorInRoom const * _door, Transform const & _placedInRoomWithDoor, float _distanceSoFar, REF_ bool & _collision, REF_ bool & _collisionGradient, REF_ bool & _collisionDetection) const
{
	if (!_collision && !_collisionGradient && !_collisionDetection)
	{
		return;
	}

	float const offHoleDistMultiplier = 3.0f;	// to prefer distances within hole - if object is outside hole, it's distance on door plane will be extended
												// this helps to disallow objects going to another room when they should stay within room
	
	float useRadius = collisionPrimitiveRadius - _distanceSoFar;
	float useRadiusForGradient = get_radius_for_gradient() - _distanceSoFar;
	float useRadiusForDetection = get_radius_for_detection() - _distanceSoFar;
	
	if (movementQueryPoint)
	{
		float maxDist = max(useRadius, max(useRadiusForGradient, useRadiusForDetection));
#ifdef INVESTIGATE__SHOULD_CONSIDER_FOR_COLLISION
		debug_context(_door->get_in_room());
		debug_subject(get_owner());
#ifdef AN_DEBUG_RENDERER
		Vector3 useCentre = movementQueryPoint->transform_to_world_of(_placedInRoomWithDoor).get_centre();
		Vector3 atDoor = _door->get_plane().get_dropped(useCentre);
		atDoor = _door->find_inside_hole(atDoor);
#endif
		debug_draw_sphere(true, false, Colour::cyan, 0.1f, Sphere(useCentre, maxDist));
		debug_draw_arrow(true, Colour::cyan, useCentre, _door->get_plane().get_dropped(useCentre));
#endif
		float dist = maxDist;
		if (_door->is_in_radius(false, movementQueryPoint->transform_to_world_of(_placedInRoomWithDoor), maxDist, offHoleDistMultiplier, NP, &dist))
		{
			_collision &= dist <= useRadius;
			_collisionGradient &= dist <= useRadiusForGradient;
			_collisionDetection &= dist <= useRadiusForDetection;
#ifdef INVESTIGATE__SHOULD_CONSIDER_FOR_COLLISION
			debug_draw_sphere(true, false, Colour::green, 0.3f, Sphere(atDoor, 0.01f));
			debug_no_subject();
			debug_no_context();
#endif
			return;
		}
#ifdef INVESTIGATE__SHOULD_CONSIDER_FOR_COLLISION
		debug_draw_sphere(true, false, Colour::red, 0.3f, Sphere(atDoor, 0.01f));
		debug_no_subject(); 
		debug_no_context();
#endif
	}
	else if (movementQuerySegment)
	{
		float maxDist = max(useRadius, max(useRadiusForGradient, useRadiusForDetection));
		float dist = maxDist;
		if (_door->is_in_radius(false, movementQuerySegment->transform_to_world_of(_placedInRoomWithDoor), maxDist, offHoleDistMultiplier, NP, &dist))
		{
			_collision &= dist <= useRadius;
			_collisionGradient &= dist <= useRadiusForGradient;
			_collisionDetection &= dist <= useRadiusForDetection;
			return;
		}
	}

	// if no query, keep what there was, we should keep the values of _collision, _collisionGradient and _collisionDetection
	// we just use whatever we had for presence (this method is used when building presence links)
}

bool ModuleCollision::is_collision_in_radius(DoorInRoom const * _door, Transform const & _placedInRoomWithDoor, float _radius) const
{
	float const offHoleDistMultiplier = 3.0f;	// to prefer distances within hole - if object is outside hole, it's distance on door plane will be extended
												// this helps to disallow objects going to another room when they should stay within room
	if (movementQueryPoint)
	{
		return _door->is_in_radius(false, movementQueryPoint->transform_to_world_of(_placedInRoomWithDoor), collisionPrimitiveRadius + _radius, offHoleDistMultiplier);
	}
	else if (movementQuerySegment)
	{
		return _door->is_in_radius(false, movementQuerySegment->transform_to_world_of(_placedInRoomWithDoor), collisionPrimitiveRadius + _radius, offHoleDistMultiplier);
	}
	else
	{
		return false;
	}
}

float ModuleCollision::get_distance_between(ModuleCollision const * _a, Optional<Transform> _aPlacement, ModuleCollision const * _b, Optional<Transform> _bPlacement)
{
	an_assert(_aPlacement.is_set() || _bPlacement.is_set() ||
		_a->get_owner()->get_presence()->get_in_room() == _b->get_owner()->get_presence()->get_in_room());

	if (!_aPlacement.is_set())
	{
		_aPlacement = _a->get_owner()->get_presence()->get_placement();
	}

	if (!_bPlacement.is_set())
	{
		_bPlacement = _b->get_owner()->get_presence()->get_placement();
	}

	if (_a->movementQueryPoint)
	{
		auto aQP = _a->movementQueryPoint->transform_to_world_of(_aPlacement.get());

		if (_b->movementQueryPoint)
		{
			auto bQP = _b->movementQueryPoint->transform_to_world_of(_bPlacement.get());

			return (aQP.location - bQP.location).length() - _a->collisionPrimitiveRadius - _b->collisionPrimitiveRadius;
		}
		if (_b->movementQuerySegment)
		{
			auto bQS = _b->movementQuerySegment->transform_to_world_of(_bPlacement.get());

			return Segment(bQS.locationA, bQS.locationB).get_distance(aQP.location) - _a->collisionPrimitiveRadius - _b->collisionPrimitiveRadius;
		}
	}
	if (_a->movementQuerySegment)
	{
		auto aQS = _a->movementQuerySegment->transform_to_world_of(_aPlacement.get());

		if (_b->movementQueryPoint)
		{
			auto bQP = _b->movementQueryPoint->transform_to_world_of(_bPlacement.get());

			return Segment(aQS.locationA, aQS.locationB).get_distance(bQP.location) - _a->collisionPrimitiveRadius - _b->collisionPrimitiveRadius;
		}
		if (_b->movementQuerySegment)
		{
			auto bQS = _b->movementQuerySegment->transform_to_world_of(_bPlacement.get());

			return Segment(aQS.locationA, aQS.locationB).get_distance(Segment(bQS.locationA, bQS.locationB)) - _a->collisionPrimitiveRadius - _b->collisionPrimitiveRadius;
		}
	}

	return 99999.0f;
}

void ModuleCollision::reset()
{
	while (collidesWithFlagsStack.get_size() > 1)
	{
		pop_collides_with_flags();
	}
	while (detectsFlagsStack.get_size() > 1)
	{
		pop_collides_with_flags();
	}
	while (collisionFlagsStack.get_size() > 1)
	{
		pop_collision_flags();
	}
	dontCollideWith.clear();
	dontCollideWithUntilNoCollision.clear();
	dontCollideWithIfAttachedTo.clear();
	base::reset();
}

void ModuleCollision::DontCollideWith::add_to(Array<DontCollideWith> & _array, Collision::ICollidableObject const * _object, Optional<float> const & _forTime)
{
	if (_object)
	{
		for_every(dont, _array)
		{
			if (dont->object == _object)
			{
				if (!dont->timeLeft.is_set())
				{
					dont->timeLeft = _forTime;
				}
				else if (_forTime.is_set() && _forTime.get() > dont->timeLeft.get())
				{
					dont->timeLeft = _forTime;
				}
				return;
			}
		}
		_array.push_back(DontCollideWith(_object, _forTime));
	}
}

void ModuleCollision::dont_collide_with(Collision::ICollidableObject const * _object, Optional<float> const & _forTime)
{
	MODULE_OWNER_LOCK(TXT("ModuleCollision::dont_collide_with"));
	DontCollideWith::add_to(dontCollideWith, _object, _forTime);
}

void ModuleCollision::dont_collide_with_until_no_collision(Collision::ICollidableObject const * _object, Optional<float> const & _forTime)
{
	MODULE_OWNER_LOCK(TXT("ModuleCollision::dont_collide_with_until_no_collision"));
	DontCollideWith::add_to(dontCollideWithUntilNoCollision, _object, _forTime);
}

void ModuleCollision::dont_collide_with_up_to_top_instigator(IModulesOwner const * _object, Optional<float> const & _forTime)
{
	MODULE_OWNER_LOCK(TXT("ModuleCollision::dont_collide_with_up_to_top_instigator"));
	while (_object)
	{
		dont_collide_with(fast_cast<Collision::ICollidableObject>(_object), _forTime);
		_object = _object->get_instigator(true);
	}
}

void ModuleCollision::dont_collide_with_up_to_top_attached_to(IModulesOwner const * _object, Optional<float> const & _forTime)
{
	MODULE_OWNER_LOCK(TXT("ModuleCollision::dont_collide_with_up_to_top_attached_to"));
	while (_object)
	{
		dont_collide_with(fast_cast<Collision::ICollidableObject>(_object), _forTime);
		_object = _object->get_presence()->get_attached_to();
	}
}

void ModuleCollision::dont_collide_with_until_no_collision_up_to_top_instigator(IModulesOwner const * _object, Optional<float> const & _forTime)
{
	MODULE_OWNER_LOCK(TXT("ModuleCollision::dont_collide_with_until_no_collision_up_to_top_instigator"));
	while (_object)
	{
		dont_collide_with_until_no_collision(fast_cast<Collision::ICollidableObject>(_object), _forTime);
		_object = _object->get_instigator(true);
	}
}

void ModuleCollision::dont_collide_with_if_attached_to(IModulesOwner const * _object, Optional<float> const & _forTime)
{
	MODULE_OWNER_LOCK(TXT("ModuleCollision::dont_collide_with_if_attached_to"));
	DontCollideWith::add_to(dontCollideWithIfAttachedTo, fast_cast<Collision::ICollidableObject>(_object), _forTime);
}

void ModuleCollision::dont_collide_with_if_attached_to_top(IModulesOwner const * _object, Optional<float> const & _forTime)
{
	MODULE_OWNER_LOCK(TXT("ModuleCollision::dont_collide_with_if_attached_to_top"));
	while (_object)
	{
		auto* next = _object->get_presence()->get_attached_to();
		if (! next)
		{
			dont_collide_with_if_attached_to(_object, _forTime);
		}
		_object = next;

	}
}

void ModuleCollision::update_bounding_box()
{
	if (auto const * appearance = get_owner()->get_appearance())
	{
		update_on_final_pose(appearance->get_final_pose_MS());
	}
	else
	{
		update_on_final_pose(nullptr);
	}
}

void ModuleCollision::update_on_final_pose(Meshes::Pose const * _poseMS)
{
	// check poses as if nothing has changed we should not update bounding boxes - useful for bigger more complex objects
	bool forceUpdate = false;
	if (_poseMS)
	{
		if (collisionUpdatedForPoseMS && collisionUpdatedForPoseMS->get_skeleton() != _poseMS->get_skeleton())
		{
			collisionUpdatedForPoseMS->release();
			collisionUpdatedForPoseMS = nullptr;
		}
		if (!collisionUpdatedForPoseMS)
		{
			collisionUpdatedForPoseMS = Meshes::Pose::get_one(_poseMS->get_skeleton());
			forceUpdate = true;
		}
	}

	if (collisionUpdatedForPoseMS && _poseMS &&
		! forceUpdate)
	{
		if (*collisionUpdatedForPoseMS == *_poseMS)
		{
			// no need to full update
			movementCollision.update_bounding_box_placement_only();
			fallbackPreciseCollision.update_bounding_box_placement_only();
			preciseCollision.update_bounding_box_placement_only();
			return;
		}
	}

	bool requestLazyUpdatePresenceLinks = false;

	{
		MEASURE_PERFORMANCE(calculateFinalPose_skeletal_collisions_movement);
		requestLazyUpdatePresenceLinks |= movementCollision.update_bounding_box(_poseMS);
	}
	{
		MEASURE_PERFORMANCE(calculateFinalPose_skeletal_collisions_precise_fallback);
		requestLazyUpdatePresenceLinks |= fallbackPreciseCollision.update_bounding_box(_poseMS);
	}
	{
		MEASURE_PERFORMANCE(calculateFinalPose_skeletal_collisions_precise);
		requestLazyUpdatePresenceLinks |= preciseCollision.update_bounding_box(_poseMS);
	}

	if (updateCollisionPrimitiveBasingOnBoundingBox || updateCollisionPrimitiveBasingOnBoundingBoxAsSphere)
	{
		update_collision_primitive_basing_on_bounding_box(updateCollisionPrimitiveBasingOnBoundingBoxAsSphere);
	}

	if (requestLazyUpdatePresenceLinks)
	{
		if (auto* p = get_owner()->get_presence())
		{
			p->request_lazy_update_presence_links();
		}
	}
}

void ModuleCollision::update_collision_primitive_basing_on_bounding_box(bool _asSphere)
{
	Vector3 centre = movementCollision.get_bounding_box().centre();
	Vector3 size = movementCollision.get_bounding_box().length();

	Vector3 new_collisionPrimitiveCentreOffset = centre;
	Vector3 new_collisionPrimitiveCentreDistance = collisionPrimitiveCentreDistance;
	float new_collisionPrimitiveRadius = collisionPrimitiveRadius;

	float const useLonger = 0.5f;
	if (_asSphere)
	{
		new_collisionPrimitiveCentreDistance = Vector3::zero;
		new_collisionPrimitiveRadius = (size.length() * useLonger + (1.0f - useLonger) * max(size.x, max(size.y, size.z))) * 0.5f;
	}
	else
	{
		Vector3 axis = Vector3::zero;
		if (size.x >= size.y && size.x >= size.z)
		{
			new_collisionPrimitiveRadius = ((size * Vector3(0.0f, 1.0f, 1.0f)).length() * useLonger + (1.0f - useLonger) * max(size.y, size.z)) * 0.5f;
			axis.x = size.x - collisionPrimitiveRadius * 2.0f;
		}
		else if (size.y >= size.x && size.y >= size.z)
		{
			new_collisionPrimitiveRadius = ((size * Vector3(1.0f, 0.0f, 1.0f)).length() * useLonger + (1.0f - useLonger) * max(size.x, size.z)) * 0.5f;
			axis.y = size.y - collisionPrimitiveRadius * 2.0f;
		}
		else
		{
			new_collisionPrimitiveRadius = ((size * Vector3(1.0f, 1.0f, 0.0f)).length() * useLonger + (1.0f - useLonger) * max(size.x, size.y)) * 0.5f;
			axis.z = size.z - collisionPrimitiveRadius * 2.0f;
		}
		new_collisionPrimitiveCentreDistance = axis;
	}

	// store only if differ
	if ((new_collisionPrimitiveCentreOffset - collisionPrimitiveCentreOffset).length() > 0.01f ||
		(new_collisionPrimitiveCentreDistance - collisionPrimitiveCentreDistance).length() > 0.01f ||
		abs(new_collisionPrimitiveRadius - collisionPrimitiveRadius) > 0.01f)
	{
		collisionPrimitiveCentreOffset = new_collisionPrimitiveCentreOffset;
		collisionPrimitiveCentreDistance = new_collisionPrimitiveCentreDistance;
		collisionPrimitiveRadius = new_collisionPrimitiveRadius;

		radiusForGradient = collisionPrimitiveRadius;

		update_collision_model_and_query();
	}
}

void ModuleCollision::store_collided_with(CollisionInfo const & _result)
{
	for_every(cw, collidedWith)
	{
		if (cw->collidedWithShape == _result.collidedWithShape &&
			cw->collidedWithObject == _result.collidedWithObject)
		{
			// already stored
			return;
		}
	}
	collidedWith.push_back(_result);
}

void ModuleCollision::store_detected_collision_with(Collision::ICollidableObject const * _ico)
{
	detectedCollisionWith.push_back_unique(_ico);
}

void ModuleCollision::store_detected(DetectedCollidable const & _dc)
{
	detected.push_back_unique(_dc);
}

void ModuleCollision::add_not_colliding_with_to(CheckCollisionContext & _ccc) const
{
	for_every(c, dontCollideWith)
	{
		_ccc.avoid(c->object);
	}
}

bool ModuleCollision::does_collide_with(Collision::ICollidableObject const * _collidable) const
{
	if (!_collidable)
	{
		return false;
	}
	for_every(c, collidedWith)
	{
		if (c->collidedWithObject == _collidable)
		{
			return true;
		}
	}

	return false;
}

float ModuleCollision::calc_distance_from_primitive_os(Vector3 const & _locOS, bool _subtractRadius) const
{
	float subtractRadius = _subtractRadius ? collisionPrimitiveRadius : 0.0f;
	if (movementQueryPoint)
	{
		return max(0.0f, (_locOS - movementQueryPoint->location).length() - subtractRadius);
	}
	else if (movementQuerySegment)
	{
		return max(0.0f, (Segment(movementQuerySegment->locationA, movementQuerySegment->locationB).get_distance(_locOS) - subtractRadius));
	}
	else
	{
		return max(0.0f, (_locOS - collisionPrimitiveCentreOffset).length() - subtractRadius);
	}
}

bool ModuleCollision::calc_closest_point_on_primitive_os(REF_ Vector3 & _locOS) const
{
	if (movementQueryPoint)
	{
		_locOS = movementQueryPoint->location;
		return true;
	}
	if (movementQuerySegment)
	{
		Segment s(movementQuerySegment->locationA, movementQuerySegment->locationB);
		float t = s.get_t(_locOS);
		float tClamped = clamp(t, 0.0f, 1.0f);
		_locOS = s.get_at_t(tClamped);
		return true;
	}
	return false;
}

Name const& ModuleCollision::find_closest_bone(Vector3 const& _locOS, AgainstCollision::Type _againstCollision) const
{
	if (auto* imo = get_owner())
	{
		auto poseRef = imo->get_appearance() ? imo->get_appearance()->get_final_pose_mat_MS_ref() : Meshes::PoseMatBonesRef();
		Collision::CheckCollisionContext ccContext;
		ccContext.use_pose_mat_bones_ref(poseRef);
		Optional<float> closestDist;
		Collision::ICollidableShape const * closestShape = nullptr;
		for_every(mci, get_against_collision(_againstCollision).get_instances())
		{
			mci->get_closest_shape(ccContext, _locOS, closestDist, closestShape);
		}
		if (closestShape)
		{
			return closestShape->get_collidable_shape_bone();
		}
	}
	return Name::invalid();
}

void ModuleCollision::prevent_future_current_pushing_through_collision(Optional<float> const& _forTime)
{
	if (cantBePushedThroughCollision)
	{
		for_every(c, collidedWith)
		{
			if (auto* cimo = fast_cast<IModulesOwner>(c->collidedWithObject.get()))
			{
				if (auto* cimoc = cimo->get_collision())
				{
					if (cimoc->throughCollisionPusher)
					{
						dont_collide_with_until_no_collision(c->collidedWithObject.get(), _forTime);
					}
				}
			}
		}
	}
}

bool ModuleCollision::should_prevent_pushing_through_collision() const
{
	if (cantBePushedThroughCollision)
	{
		for_every(c, collidedWith)
		{
			if (auto* cimo = fast_cast<IModulesOwner>(c->collidedWithObject.get()))
			{
				while (cimo)
				{
					if (auto* cimoc = cimo->get_collision())
					{
						if (cimoc->throughCollisionPusher)
						{
							return true;
						}
					}
					cimo = cimo->get_presence()->get_attached_to();
				}
			}
		}
	}

	return false;
}
