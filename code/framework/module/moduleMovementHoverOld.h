#pragma once

#include "moduleMovement.h"

namespace Framework
{
	class ModuleMovementHoverOld
	: public ModuleMovement
	{
		FAST_CAST_DECLARE(ModuleMovementHoverOld);
		FAST_CAST_BASE(ModuleMovement);
		FAST_CAST_END();

		typedef ModuleMovement base;
	public:
		static RegisteredModule<ModuleMovement> & register_itself();

		ModuleMovementHoverOld(IModulesOwner* _owner);
	
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHoverOld, find_inclination_limit_of_gait, find_inclination_limit_of_current_gait, inclinationLimit, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHoverOld, find_leveled_xy_thrust_coef_of_gait, find_leveled_xy_thrust_coef_of_current_gait, leveledXYThrustCoef, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHoverOld, find_stop_inclination_coef_of_gait, find_stop_inclination_coef_of_current_gait, stopInclinationCoef, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHoverOld, find_change_speed_inclination_coef_of_gait, find_change_speed_inclination_coef_of_current_gait, changeSpeedInclinationCoef, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHoverOld, find_air_resitance_coef_of_gait, find_air_resitance_coef_of_current_gait, airResistanceCoef, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHoverOld, find_max_collision_speed_of_gait, find_max_collision_speed_of_current_gait, maxCollisionVelocity, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHoverOld, find_collision_affects_velocity_factor_of_gait, find_collision_affects_velocity_factor_of_current_gait, collisionAffectsVelocityFactor, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHoverOld, find_level_collision_effect_factor_of_gait, find_level_collision_effect_factor_of_current_gait, levelCollisionEffectFactor, float);

	protected:	// Module
		override_ void setup_with(ModuleData const * _moduleData);

	protected:	// ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context);
		override_ Vector3 calculate_requested_velocity(float _deltaTime, bool _adjustForStoppingEtc, PrepareMoveContext& _context) const;
		override_ void on_activate_movement(ModuleMovement* _prevMovement);

	private:
		float calculate_inclination_angle(float _forVelocity, float _gravityForce, float const & leveledXYThrustCoef, float const & airResistanceCoef) const;

		float applyResultForce = 0.0f;
		float reorientatePt = 0.0f;
		float reorientationTime = 1.0f;
		float currentReorientationTime = 1.0f;
	};

	class ModuleMovementHoverOldData
	: public ModuleMovementData
	{
		FAST_CAST_DECLARE(ModuleMovementHoverOldData);
		FAST_CAST_BASE(ModuleMovementData);
		FAST_CAST_END();
		typedef ModuleMovementData base;
	public:
		ModuleMovementHoverOldData(LibraryStored* _inLibraryStored);

	protected: // ModuleMovementData
		override_ bool load_gait_param_from_xml(ModuleMovementData::Gait & _gait, IO::XML::Node const * _node, LibraryLoadingContext & _lc);
	};
};

