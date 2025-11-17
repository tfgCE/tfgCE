#pragma once

#include "..\math\math.h"

#include "gradientQueryResult.h"

namespace Collision
{
	struct GradientQueryResult;

	struct QueryPrimitivePoint
	{
		Vector3 location;

		QueryPrimitivePoint() {}
		QueryPrimitivePoint(Vector3 const & _location);

		QueryPrimitivePoint transform_to_world_of(Matrix44 const & _matrix) const;
		QueryPrimitivePoint transform_to_local_of(Matrix44 const & _matrix) const;

		QueryPrimitivePoint transform_to_world_of(Transform const & _transform) const;
		QueryPrimitivePoint transform_to_local_of(Transform const & _transform) const;

		Vector3 const & get_centre() const { return location; }

		Range3 calculate_bounding_box(float _radius) const;

		template <typename ShapeObject>
		GradientQueryResult get_gradient(ShapeObject const & _shape, float _maxDistance, bool _square = false) const;
	};

	#include "queryPrimitivePoint.inl"
};
