#pragma once

#include "..\..\framework\appearance\socketID.h"
#include "..\..\framework\module\moduleMovement.h"

namespace TeaForGodEmperor
{
	class ModuleMovementAirshipProxy
	: public Framework::ModuleMovement
	{
		FAST_CAST_DECLARE(ModuleMovementAirshipProxy);
		FAST_CAST_BASE(Framework::ModuleMovement);
		FAST_CAST_END();

		typedef ModuleMovement base;
	public:
		static Framework::RegisteredModule<Framework::ModuleMovement> & register_itself();

		ModuleMovementAirshipProxy(Framework::IModulesOwner* _owner);
	
		void set_movement_source(Framework::IModulesOwner* _source);

	protected:	// Module
		override_ void setup_with(Framework::ModuleData const * _moduleData);

	protected:	// ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context);
		override_ bool does_use_controller_for_movement() const { return false; }

	private:
		SafePtr<Framework::IModulesOwner> anchorOwner;
		Optional<Transform> anchorRelativePlacement; // if owner present will be used to update anchor placement
		Optional<Transform> anchorPlacement;
		Optional<VectorInt2> inMapCell;
		float cellSize = 0.0f;

		SafePtr<Framework::IModulesOwner> movementSource;

		Optional<bool> rareAdvance;
	};
};

