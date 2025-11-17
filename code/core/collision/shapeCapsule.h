#pragma once

#include "shape.h"

namespace Collision
{
	struct QueryPrimitivePoint;
	struct QueryPrimitiveSegment;
	struct GradientQueryResult;

	struct Capsule
	: public Shape<::Capsule>
	{
	public:
		struct QueryContext
		{
			int32 temp; // empty
		};

	public:
		Capsule();

		Capsule& set(::Capsule const & _other) { ::Capsule::operator=(_other); return *this; }

	public: // shape methods
		ShapeAgainstPlanes::Type check_against_planes(PlaneSet const & _clipPlanes, float _threshold) const;

		GradientQueryResult get_gradient(QueryPrimitivePoint const & _point, REF_ QueryContext & _queryContext, float _maxDistance, bool _square = false) const;

	protected:
		ShapeAgainstPlanes::Type check_against_plane(Plane const & _clipPlane, float _threshold) const;
	};
};
