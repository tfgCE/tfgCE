#pragma once

#include "iWMPTool.h"
#include "wmp_float.h"
#include "wmp_rotator3.h"
#include "wmp_vector3.h"

namespace WheresMyPoint
{
	class Transform
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		::Transform transform = ::Transform::identity; // if nothing else is set, this will be used
		WheresMyPoint::Vector3 forwardAxis;
		WheresMyPoint::Vector3 rightAxis;
		WheresMyPoint::Vector3 upAxis;
		WheresMyPoint::Vector3 location;
		WheresMyPoint::Rotator3 rotation;
		WheresMyPoint::Float scale;

		Axis::Type firstAxis = Axis::None;
		Axis::Type secondAxis = Axis::None;
	};

	class ToTransform
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value& _value, Context& _context) const;
	};
};
