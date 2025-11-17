#pragma once

#include "..\..\framework\appearance\socketID.h"
#include "..\..\framework\module\moduleMovement.h"

namespace TeaForGodEmperor
{
	class ModuleMovementH_Craft
	: public Framework::ModuleMovement
	{
		FAST_CAST_DECLARE(ModuleMovementH_Craft);
		FAST_CAST_BASE(Framework::ModuleMovement);
		FAST_CAST_END();

		typedef ModuleMovement base;
	public:
		static Framework::RegisteredModule<Framework::ModuleMovement> & register_itself();

		ModuleMovementH_Craft(Framework::IModulesOwner* _owner);
	
		bool is_blending_in() const { return blendInTime > 0.0f && timeActive < blendInTime; }

		void blend_in(float _time) { timeActive = 0.0f; blendInTime = _time; }

	protected:	// Module
		override_ void setup_with(Framework::ModuleData const* _moduleData);

	protected:	// ModuleMovement
		override_ void on_activate_movement(Framework::ModuleMovement* _prevMovement);
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context);

	protected:
		float timeActive = 0.0f;
		float blendInTime = 0.0f;
	};
};

