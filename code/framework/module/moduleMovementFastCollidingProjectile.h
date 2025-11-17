#pragma once

#include "moduleMovementProjectile.h"
#include "..\collision\againstCollision.h"

namespace Framework
{
	class ModuleMovementFastCollidingProjectile
	: public ModuleMovementProjectile
	{
		FAST_CAST_DECLARE(ModuleMovementFastCollidingProjectile);
		FAST_CAST_BASE(ModuleMovementProjectile);
		FAST_CAST_END();

		typedef ModuleMovementProjectile base;
	public:
		static RegisteredModule<ModuleMovement> & register_itself();

		ModuleMovementFastCollidingProjectile(IModulesOwner* _owner);
	
	public: // Module
		override_ void setup_with(ModuleData const * _moduleData);

	protected:	// ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context); // overrides ModuleMovementProjectile's implementation
	};
};

