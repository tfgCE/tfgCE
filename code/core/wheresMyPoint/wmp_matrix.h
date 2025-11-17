#pragma once

#include "iWMPTool.h"
#include "wmp_rotator3.h"
#include "wmp_vector3.h"

namespace WheresMyPoint
{
	class RawMatrix44
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		::Matrix44 matrix = ::Matrix44::identity; // if nothing else is set, this will be used
	};

};
