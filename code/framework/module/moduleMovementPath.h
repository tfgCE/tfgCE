#pragma once

#include "moduleMovement.h"

namespace Framework
{
	class ModuleMovementPath
	: public ModuleMovement
	{
		FAST_CAST_DECLARE(ModuleMovementPath);
		FAST_CAST_BASE(ModuleMovement);
		FAST_CAST_END();

		typedef ModuleMovement base;
	public:
		struct CurveSinglePathExtraParams
		{
			Optional<float> moveInwards;
			Optional<float> moveInwardsT; // how t translates to moving inwards (at 0 or 1 is fully away
		};

	public:
		static RegisteredModule<ModuleMovement> & register_itself();

		ModuleMovementPath(IModulesOwner* _owner);
	
		float get_acceleration() const { return acceleration; }
		void set_acceleration(float _acceleration) { acceleration = _acceleration; }

		float get_speed() const { return speed; }
		void set_speed(float _speed) { speed = _speed; }
		
		void be_initial_placement() { initialPlacement = true; } // best to be used with direct t mode (set_t), velocity will be zero

		void set_t(float _t) { t = _t; targetT.clear(); } // will calculate velocity but use T as movement
		void set_target_t(float _t) { targetT = _t; }
		void clear_target_t() { targetT.clear(); } // order to stop wherever you are
		float get_t() const { return t; }
		Optional<float> const & get_target_t() const { return targetT; }

		bool is_using_any_path() const { return path != P_None; }

		void use_linear_path(Transform const & _a, Transform const & _b, float _t);
		bool is_using_linear_path() const { return path == P_Linear; }

		void use_curve_single_path(Transform const & _a, Transform const & _b, BezierCurve<Vector3> const& _curve, float _t, Optional<CurveSinglePathExtraParams> const & _extraParams = NP);
		bool is_using_curve_single_path() const { return path == P_CurveSingle; }
		void on_curve_single_path_reorient_after(Optional<float> const & _reorientAfter) { pathCurveSingle.reorientAfter = _reorientAfter; }
		void on_curve_single_path_align_axis_along_t(Optional<Axis::Type> const & _alignAxisAlongT, bool _alignAxisAlongTInvert = false) { pathCurveSingle.alignAxisAlongT = _alignAxisAlongT; pathCurveSingle.alignAxisAlongTInvert = _alignAxisAlongTInvert; }
		BezierCurve<Vector3> const & get_curve_single_path() const { return pathCurveSingle.curve; }

	protected:	// Module
		override_ void setup_with(ModuleData const * _moduleData);

	protected:	// ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context);
		override_ bool does_use_controller_for_movement() const { return false; }

	public:
		float speed = 5.0f;
		float acceleration = 10.0f;

		enum Path
		{
			P_None,
			P_Linear,
			P_CurveSingle
		};
		Path path = P_None;

		float t = 0.0f; // this is distance t!
		Optional<float> targetT;
		bool initialPlacement = true; // always initial for first time

		// linear path
		struct PathLinear
		{
			Transform a = Transform::identity;
			Transform b = Transform::identity;
		} pathLinear;

		struct PathCurveSingle
		{
			Optional<float> reorientAfter;
			Optional<Axis::Type> alignAxisAlongT;
			bool alignAxisAlongTInvert = false;
			// a and b is to get orientation
			Transform a = Transform::identity;
			Transform b = Transform::identity; 
			BezierCurve<Vector3> curve;
			CurveSinglePathExtraParams extraParams;
			CACHED_ float perpInwardsDirSign = 0.0f;
			CACHED_ Vector3 axis;
		} pathCurveSingle;

		float currentSpeed = 0.0f;

	};
};

