#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

#include "..\containers\arrayStatic.h"

#define PLANE_SET_PLANE_LIMIT 6

struct PlaneSet
{
public:
	static PlaneSet const& empty() { return s_empty; }

	ArrayStatic<Plane, PLANE_SET_PLANE_LIMIT> planes; // max plane count defined

	PlaneSet() { SET_EXTRA_DEBUG_INFO(planes, TXT("PlaneSet.planes")); }
	inline bool is_empty() const { return planes.is_empty(); }

	inline void clear();
	inline void add(Plane const & _plane);
	inline void add(PlaneSet const & _planes);

	void add_smartly_more_relevant(Plane const& _plane); // adds a more relevant plane, if there are other planes that are coplanar, they are removed

	inline void transform_to_local_of(PlaneSet const & _source, Matrix44 const & _transformToNewSpace);
	inline void transform_to_local_of(PlaneSet const & _source, Transform const & _transform);

	inline void transform_to_local_of(Matrix44 const & _transformToNewSpace);
	inline void transform_to_local_of(Transform const & _transform);

	bool load_from_xml_child_nodes(IO::XML::Node const * _node, tchar const * _childName = TXT("clipPlane"));

private:
	static PlaneSet s_empty;
};
