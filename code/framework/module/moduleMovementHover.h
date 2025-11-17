#pragma once

#include "moduleMovement.h"

namespace Framework
{
	class ModuleMovementHover
	: public ModuleMovement
	{
		FAST_CAST_DECLARE(ModuleMovementHover);
		FAST_CAST_BASE(ModuleMovement);
		FAST_CAST_END();

		typedef ModuleMovement base;
	public:
		static RegisteredModule<ModuleMovement> & register_itself();

		ModuleMovementHover(IModulesOwner* _owner);
	
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHover, find_rotation_blend_time_of_gait, find_rotation_blend_time_of_current_gait, rotationBlendTime, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHover, find_rotation_from_speed_limit_of_gait, find_rotation_from_speed_limit_of_current_gait, rotationFromSpeedLimit, Rotator3);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHover, find_rotation_from_speed_coef_of_gait, find_rotation_from_speed_coef_of_current_gait, rotationFromSpeedCoef, Rotator3);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHover, find_rotation_from_turning_limit_of_gait, find_rotation_from_turning_limit_of_current_gait, rotationFromTurninglimit, Rotator3);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHover, find_rotation_from_turning_coef_of_gait, find_rotation_from_turning_coef_of_current_gait, rotationFromTurningCoef, Rotator3);

	protected:	// Module
		override_ void setup_with(ModuleData const * _moduleData);

	protected:	// ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context);
		override_ Vector3 calculate_requested_velocity(float _deltaTime, bool _adjustForStoppingEtc, PrepareMoveContext& _context) const;
		override_ void on_activate_movement(ModuleMovement* _prevMovement);

	private:
	};

	class ModuleMovementHoverData
	: public ModuleMovementData
	{
		FAST_CAST_DECLARE(ModuleMovementHoverData);
		FAST_CAST_BASE(ModuleMovementData);
		FAST_CAST_END();
		typedef ModuleMovementData base;
	public:
		ModuleMovementHoverData(LibraryStored* _inLibraryStored);

	protected: // ModuleMovementData
		override_ bool load_gait_param_from_xml(ModuleMovementData::Gait & _gait, IO::XML::Node const * _node, LibraryLoadingContext & _lc);
	};
};

