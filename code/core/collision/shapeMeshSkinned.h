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
	struct MeshSkinned
	: public Shape<::SkinnedMesh>
	{
	public:
		struct QueryContext
		{
			int32 temp; // empty
		};
	
	public:
		MeshSkinned& set(::SkinnedMesh const & _other) { ::SkinnedMesh::operator=(_other); return *this; }

		ShapeAgainstPlanes::Type check_against_planes(PlaneSet const & _clipPlanes, float _threshold) const;

		GradientQueryResult get_gradient(QueryPrimitivePoint const & _point, REF_ QueryContext & _queryContext, float _maxDistance, bool _square = false) const;

	protected: friend class Model;
		bool cache_for(Meshes::Skeleton const * _skeleton);
		bool check_segment(REF_ Segment & _segment, REF_ CheckSegmentResult & _result, CheckCollisionContext & _context, float _increaseSize = 0.0f) const;

	public: // not so safe that they are public
		void debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Meshes::PoseMatBonesRef const & _poseMatBonesRef = Meshes::PoseMatBonesRef()) const;
		Range3 calculate_bounding_box(Transform const & _usingPlacement, Meshes::Pose const * _poseMS, bool _quick) const;
	};
};
