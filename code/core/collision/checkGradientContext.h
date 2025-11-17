#pragma once

#include "checkGradientPoints.h"
#include "checkGradientDistances.h"

// .inl
#include "..\debug\debugRenderer.h"
#include "..\performance\performanceUtils.h"
#include "..\..\core\mesh\pose.h"

struct Matrix44;

namespace Collision
{
	class ModelInstance;
	class ModelInstanceSet;

	// context for checking distance against single shape - used within room to update distances
	template <typename CollisionQueryPrimitive>
	struct CheckGradientContext
	{
		CheckGradientPoints<CollisionQueryPrimitive> points;
		CheckGradientDistances distances; // this value is taken back to CheckGradientContext
		Optional<float> maxDistanceToUpdateGradient; // max distance to update gradient
		Optional<float> maxDistanceToCheck; // max distance for this update
		Optional<float> minDistanceFound; // minimal distance during update that we found
		PlaneSet const clipPlanes; // clip planes are taken from presence link for which we are checking

		CheckGradientContext(CheckGradientPoints<CollisionQueryPrimitive> const & _points, CheckGradientDistances const & _distances, PlaneSet const & _clipPlanes = PlaneSet::empty());

		bool update(Matrix44 const & _placementMatrix, ICollidableObject const * _object, Meshes::PoseMatBonesRef const & _poseMatBonesRef, ModelInstanceSet const & _set, float _massDifferenceCoef);
		bool update(Matrix44 const & _placementMatrix, ICollidableObject const * _object, Meshes::PoseMatBonesRef const & _poseMatBonesRef, ModelInstance const & _instance, float _massDifferenceCoef);

		CheckGradientDistances const & get_distances() const { return distances; }

		inline Flags const & get_collision_flags() const { return flags; }
		inline void use_collision_flags(Flags const & _flags) { flags = _flags; }

		inline bool should_only_check() const { return onlyCheck; }
		inline void only_check(bool _check) { onlyCheck = _check; }

		inline bool should_focus_on_gradient_only() const { return shoudlFocusOnGradientOnly; }
		inline void focus_on_gradient_only(bool _gradient) { shoudlFocusOnGradientOnly = _gradient; }

		inline bool should_only_check_first_collision() const { return firstCollisionCheckOnly; }
		inline void first_collision_check_only(bool _check) { firstCollisionCheckOnly = _check; }

		inline float get_actual_distance_limit() const;

	private:
		Flags flags;
		bool onlyCheck = false; // if this is true, some calculations are skipped and we only check if we collided, we do not modify distances
		bool shoudlFocusOnGradientOnly = false; // if this is true, we use lowest current max distance
		bool firstCollisionCheckOnly = false; // if this is true, we only get first collision
	};

	// worker function to udpate check gradient context for single shape
	template <typename CollisionQueryPrimitive, typename CollisionShape>
	bool update_check_gradient_for_shape(CheckGradientContext<CollisionQueryPrimitive> & _cgc, ICollidableObject const * _object, Meshes::PoseMatBonesRef const & _poseMatBonesRef, Optional<Collision::Flags> const & _mainMaterialCollisionFlags, CollisionShape const & _shape, PlaneSet const & _clipPlanes, float _massDifferenceCoef);

	// worker function to udpate check gradient context for array of shapes
	template <typename CollisionQueryPrimitive, typename CollisionShape>
	bool update_check_gradient_for_shape_array(CheckGradientContext<CollisionQueryPrimitive> & _cgc, ICollidableObject const * _object, Meshes::PoseMatBonesRef const & _poseMatBonesRef, Optional<Collision::Flags> const & _mainMaterialCollisionFlags, Array<CollisionShape> const & _shapeArray, PlaneSet const & _clipPlanes, float _massDifferenceCoef);

	#include "checkGradientContext.inl"

};
