#pragma once

#include "iWMPTool.h"
#include "wmp_vector3.h"

namespace WheresMyPoint
{
	class Plane
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		::Plane plane = ::Plane::identity; // if normal and location not set, this will be used
		WheresMyPoint::Vector3 normal;
		WheresMyPoint::Vector3 location;
		int loadedPoints = 0;
		WheresMyPoint::Vector3 points[3];
	};
};
