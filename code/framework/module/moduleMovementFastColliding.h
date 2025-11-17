#pragma once

#include "moduleMovement.h"
#include "..\collision\againstCollision.h"

namespace Framework
{
	class ModuleMovementFastColliding
	: public ModuleMovement
	{
		FAST_CAST_DECLARE(ModuleMovementFastColliding);
		FAST_CAST_BASE(ModuleMovement);
		FAST_CAST_END();

		typedef ModuleMovement base;
	public:
		static RegisteredModule<ModuleMovement> & register_itself();

		ModuleMovementFastColliding(IModulesOwner* _owner);
	
	public: // Module
		override_ void setup_with(ModuleData const * _moduleData);

	protected:	// ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context);

	private:
		float stopAtVelocity = 0.0f;
		float getVelocityFromCollidedWith = 0.2f;
	};
};

