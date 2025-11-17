#pragma once

#include "..\..\framework\module\moduleMovement.h"

namespace TeaForGodEmperor
{
	class ModuleMovementRam
	: public Framework::ModuleMovement
	{
		FAST_CAST_DECLARE(ModuleMovementRam);
		FAST_CAST_BASE(Framework::ModuleMovement);
		FAST_CAST_END();

		typedef Framework::ModuleMovement base;
	public:
		static Framework::RegisteredModule<Framework::ModuleMovement> & register_itself();

		ModuleMovementRam(Framework::IModulesOwner* _owner);
	
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementRam, find_ram_inclination_of_gait, find_ram_inclination_of_current_gait, ramInclination, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementRam, find_ram_inclination_blend_time_of_gait, find_ram_inclination_blend_time_of_current_gait, ramInclinationBlendTime, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHover, find_max_collision_speed_of_gait, find_max_collision_speed_of_current_gait, maxCollisionVelocity, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovementHover, find_collision_affects_velocity_factor_of_gait, find_collision_affects_velocity_factor_of_current_gait, collisionAffectsVelocityFactor, float);

	protected:	// ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context);
		override_ Vector3 calculate_requested_velocity(float _deltaTime, bool _adjustForStoppingEtc, PrepareMoveContext& _context) const;
	};

	class ModuleMovementRamData
	: public Framework::ModuleMovementData
	{
		FAST_CAST_DECLARE(ModuleMovementRamData);
		FAST_CAST_BASE(Framework::ModuleMovementData);
		FAST_CAST_END();
		typedef Framework::ModuleMovementData base;
	public:
		ModuleMovementRamData(Framework::LibraryStored* _inLibraryStored);

	protected: // ModuleMovementData
		override_ bool load_gait_param_from_xml(Framework::ModuleMovementData::Gait & _gait, IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
	};
};

