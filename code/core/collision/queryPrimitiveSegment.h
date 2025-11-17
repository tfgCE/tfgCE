#pragma once

#include "..\math\math.h"

#include "gradientQueryResult.h"

namespace Collision
{
	struct QueryPrimitiveSegment
	{
		Vector3 locationA;
		Vector3 locationB;

		QueryPrimitiveSegment() {}
		QueryPrimitiveSegment(Vector3 const & _locationA, Vector3 const & _locationB);

		QueryPrimitiveSegment transform_to_world_of(Matrix44 const & _matrix) const;
		QueryPrimitiveSegment transform_to_local_of(Matrix44 const & _matrix) const;

		QueryPrimitiveSegment transform_to_world_of(Transform const & _transform) const;
		QueryPrimitiveSegment transform_to_local_of(Transform const & _transform) const;

		Vector3 get_centre() const { return (locationA + locationB) * 0.5f; }

		Range3 calculate_bounding_box(float _radius) const;

		template <typename ShapeObject>
		GradientQueryResult get_gradient(ShapeObject const & _shape, float _maxDistance) const;
	};
};

#include "queryPrimitiveSegment.inl"
