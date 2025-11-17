#pragma once

#include "..\..\globalDefinitions.h"

#include "..\..\math\math.h"
#include "..\..\memory\pooledObject.h"

#define VIEW_PLANES_COUNT 6

namespace System
{
	// similar to ViewFrustum but instead of just being against POV it is an arbitrary set of planes used to build scene (cut stuff behind the planes)
	// contrary to ViewFrustum this should not change room-to-room
	class ViewPlanes
	{
	public:
		ViewPlanes();
		~ViewPlanes();

		void clear();

		// add clockwise
		void add_plane(Plane const & _plane);

		inline bool is_empty() const { return planes.is_empty(); }

		inline bool is_inside(Vector3 const & _point) const;
		inline bool is_visible(Vector3 const & _point, float _radius, Vector3 const & _centreDistance = Vector3::zero) const; // centre distance should be already transformed by _placement
		inline bool is_visible(Matrix44 const & _placement, Range3 const & _boundingBox) const; // bounding box is in _placement space

		ArrayStatic<Plane, VIEW_PLANES_COUNT> const & get_planes() const { return planes; }

	private:
		ArrayStatic<Plane, VIEW_PLANES_COUNT> planes;

	};

	#include "viewPlanes.inl"

};

