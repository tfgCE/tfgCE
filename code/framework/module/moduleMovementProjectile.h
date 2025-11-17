#pragma once

#include "moduleMovement.h"
#include "..\collision\againstCollision.h"
#include "..\..\core\system\timeStamp.h"

#ifndef BUILD_PUBLIC_RELEASE
#define CATCH_HANGING_PROJECTILE
#endif

namespace Collision
{
	interface_class ICollidableShape;
};

namespace Framework
{
	class ModuleMovementProjectile
	: public ModuleMovement
	{
		FAST_CAST_DECLARE(ModuleMovementProjectile);
		FAST_CAST_BASE(ModuleMovement);
		FAST_CAST_END();

		typedef ModuleMovement base;
	public:
		static RegisteredModule<ModuleMovement> & register_itself();

		ModuleMovementProjectile(IModulesOwner* _owner);

		bool can_ricochet() const { return canRicochet; }
		bool can_penetrate() const { return canPenetrate; }

		void request_one_back_check(Vector3 const & _offset) { oneBackCheckOffset = _offset; }

	protected:	// Module
		override_ void reset(); // go back to default state
		override_ void setup_with(ModuleData const * _moduleData);

	protected:	// ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context);

	protected:
		void alter_velocity(float _deltaTime, VelocityAndDisplacementContext& _context);

	private:
		AgainstCollision::Type againstCollision = AgainstCollision::Precise;

		Optional<Vector3> oneBackCheckOffset; // if non zero, on the first frame of the movement we will be doing check from the back to see if maybe we're trying to shoot through a shield or something
		
		RUNTIME_ int framesAlive = 0;
		RUNTIME_ bool justRicocheted = false;
		RUNTIME_ bool justPenetrated = false;

		bool ignoreCollision = false; // we may use this for particles only, ignore collision then
		bool canRicochet = false; // this is an information if can ricochet in general (not only by itself but also if is forced, some projectiles can't ricochet, beams, sticky ones)
		bool canPenetrate = false; // this is an information if can penetrate in general
		bool alignToVelocity = true;
		Rotator3 autoRotation = Rotator3::zero;
		float maintainSpeed = 1.0f;
		float acceleration = 0.0f;
		float deceleration = 0.0f;
		float minSpeed = 0.0f; // can't get lower than this
		float maxSpeed = 0.0f; // can't get higher than this (unless 0)

		struct Contacted
		{
			Collision::ICollidableShape const * shape;
			Collision::ICollidableObject const * object;
			::System::TimeStamp collidedTS;
			Vector3 contactLocWS = Vector3::zero;
		};
		ArrayStatic<Contacted, 8> contacted;


#ifdef CATCH_HANGING_PROJECTILE
		Framework::Room* inRoom = nullptr;
		Optional<Transform> placement;
#endif
	};
};

