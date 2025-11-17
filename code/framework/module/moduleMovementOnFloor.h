#pragma once

#include "moduleMovement.h"

namespace Framework
{
	class ModuleMovementOnFloor
	: public ModuleMovement
	{
		FAST_CAST_DECLARE(ModuleMovementOnFloor);
		FAST_CAST_BASE(ModuleMovement);
		FAST_CAST_END();

		typedef ModuleMovement base;
	public:
		static RegisteredModule<ModuleMovement> & register_itself();

		ModuleMovementOnFloor(IModulesOwner* _owner);
	
	protected:	// Module
		override_ void setup_with(ModuleData const * _moduleData);

	protected:	// ModuleMovement
		override_ void apply_gravity_to_velocity(float _deltaTime, REF_ Vector3 & _accelerationInFrame, REF_ Vector3 & _locationDisplacement);

	private:
		float floorLevelMatchTime = 0.1f; // how long does it take to match requested floor level
		bool forceMatchFloorLevel = false; // if forced, will always match, won't switch to falling/dropping - the other code should do that

	};
};

