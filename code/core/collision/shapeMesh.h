#pragma once

#include "shape.h"

namespace Collision
{
	struct QueryPrimitivePoint;
	struct QueryPrimitiveSegment;
	struct GradientQueryResult;
	struct ConvexHull;

	/**
	 *	Meshes should be avoided as they may not give good gradient (won't catch internal corners).
	 *	Therefore it is advised to break meshes into convex hulls.
	 */
	struct Mesh
	: public Shape<::SimpleMesh>
	{
	public:
		struct QueryContext
		{
			int32 temp; // empty
		};
	
	public:
		Mesh& set(::SimpleMesh const & _other) { ::SimpleMesh::operator=(_other); return *this; }

		void break_into_convex_hulls(Array<Collision::ConvexHull> & _hulls, Array<Collision::Mesh> & _meshes) const;
		bool break_into_convex_meshes(Array<Collision::Mesh> & _meshes) const; // returns true if broken into meshes, if is convex, does nothing and returns false

	public: // shape methods
		ShapeAgainstPlanes::Type check_against_planes(PlaneSet const & _clipPlanes, float _threshold) const;

		GradientQueryResult get_gradient(QueryPrimitivePoint const & _point, REF_ QueryContext & _queryContext, float _maxDistance, bool _square = false) const;

	protected:
		ShapeAgainstPlanes::Type check_against_plane(Plane const & _clipPlane, float _threshold) const;

		static GradientQueryResult get_gradient_for_triangle(REF_ Triangle const & _triangle, Vector3 const & _point, REF_ QueryContext & _queryContext, float _maxDistance);
	};
};
