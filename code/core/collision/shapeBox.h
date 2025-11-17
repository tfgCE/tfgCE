#pragma once

#include "shape.h"

namespace Collision
{
	struct QueryPrimitivePoint;
	struct QueryPrimitiveSegment;
	struct GradientQueryResult;

	struct Box
	: public Shape<::Box>
	{
	public:
		struct QueryContext
		{
			int32 temp; // empty
		};

	public:
		Box();

		Box& set(::Box const & _other) { ::Box::operator=(_other); return *this; }

	public: // shape methods
		ShapeAgainstPlanes::Type check_against_planes(PlaneSet const & _clipPlanes, float _threshold) const;

		GradientQueryResult get_gradient(QueryPrimitivePoint const & _point, REF_ QueryContext & _queryContext, float _maxDistance, bool _square = false) const;

	protected:
		inline static void query_distance_axis(Vector3 const & _diff, Vector3 const & _axis, float const _halfSize, OUT_ float & _offsetOutsideBox);

		ShapeAgainstPlanes::Type check_against_plane(Plane const & _clipPlane, float _threshold) const;
	};
};
