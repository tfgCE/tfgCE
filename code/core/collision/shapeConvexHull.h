#pragma once

#include "shape.h"

namespace Collision
{
	struct QueryPrimitivePoint;
	struct QueryPrimitiveSegment;
	struct GradientQueryResult;

	struct ConvexHull
	: public Shape<::ConvexHull>
	{
	public:
		struct QueryContext
		{
			QueryContext();

		private: friend struct ConvexHull;
			Triangle const * triangle;
		};
	
	public:
		ConvexHull& set(::ConvexHull const & _other) { ::ConvexHull::operator=(_other); return *this; }

	public: // shape methods
		ShapeAgainstPlanes::Type check_against_planes(PlaneSet const & _clipPlanes, float _threshold) const;

		GradientQueryResult get_gradient(QueryPrimitivePoint const & _point, REF_ QueryContext & _queryContext, float _maxDistance, bool _square = false) const;

	protected:
		ShapeAgainstPlanes::Type check_against_plane(Plane const & _clipPlane, float _threshold) const;

		static GradientQueryResult get_gradient_for_triangle(REF_ Triangle const & _triangle, Vector3 const & _point, REF_ QueryContext & _queryContext);
	};
};
