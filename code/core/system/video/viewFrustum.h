#pragma once

#include "..\..\globalDefinitions.h"

#include "..\..\math\math.h"
#include "..\..\memory\pooledObject.h"

#define VIEW_FRUSTUM_EDGE_COUNT 24

namespace System
{
	struct ViewFrustumEdge
	{
		// all edges share point (0,0,0)
		// "point" is anywhere, dir along edge of frustum
		// "normal" can be calculated as result of two consequetives points (thanks to shared 0,0,0 point)
		Vector3 LOCATION_ point;
		Vector3 CACHED_ NORMAL_ normal;

		inline ViewFrustumEdge() {}
		inline ViewFrustumEdge(Vector3 const & LOCATION_ _point) : point( _point ) {}
	};

	class ViewFrustum
	: public PooledObject<ViewFrustum>
	{
	public:
		ViewFrustum();
		~ViewFrustum();

		void clear();

		// add clockwise
		void add_point(Vector3 const & LOCATION_ _point);
		void build();

		inline bool is_empty() const { return edges.is_empty(); }

		bool clip(ViewFrustum const * _subject, ViewFrustum const * _clipper); // returns if non empty

		inline bool is_inside(Vector3 const & _point) const;
		inline bool is_visible(Vector3 const & _point, float _radius, Vector3 const & _centreDistance = Vector3::zero) const; // centre distance should be already transformed by _placement
		inline bool is_visible(Matrix44 const & _placement, Range3 const & _boundingBox) const; // bounding box is in _placement space

		ArrayStatic<ViewFrustumEdge, VIEW_FRUSTUM_EDGE_COUNT> const & get_edges() const { return edges; }

	private:
#ifdef AN_ASSERT
		bool normalsValid;
#endif
		ArrayStatic<ViewFrustumEdge, VIEW_FRUSTUM_EDGE_COUNT> edges;

	};

	#include "viewFrustum.inl"

};

