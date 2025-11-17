#include "wmp_transform.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace WheresMyPoint;

//

bool WheresMyPoint::Transform::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	transform.load_from_xml(_node);

	result &= forwardAxis.load_from_xml(_node->first_child_named(TXT("forward")));
	result &= upAxis.load_from_xml(_node->first_child_named(TXT("up")));
	result &= rightAxis.load_from_xml(_node->first_child_named(TXT("right")));
	result &= location.load_from_xml(_node->first_child_named(TXT("location")));
	result &= rotation.load_from_xml(_node->first_child_named(TXT("rotation")));
	result &= scale.load_from_xml(_node->first_child_named(TXT("scale")));

	Axis::Type* axis = &firstAxis;
	for_every(node, _node->all_children())
	{
		bool nextAxis = false;
		if (node->get_name() == TXT("forward"))
		{
			*axis = Axis::Forward;
			nextAxis = true;
		}
		if (node->get_name() == TXT("right"))
		{
			*axis = Axis::Right;
			nextAxis = true;
		}
		if (node->get_name() == TXT("up"))
		{
			*axis = Axis::Up;
			nextAxis = true;
		}
		if (nextAxis)
		{
			if (axis == &firstAxis)
			{
				axis = &secondAxis;
			}
			else
			{
				break;
			}
		}
	}

	if (rotation.is_empty() &&
		firstAxis != Axis::None &&
		secondAxis == Axis::None)
	{
		error_loading_xml(_node, TXT("provide two axes or none or rotation"));
		result = false;
	}

	return result;
}

bool WheresMyPoint::Transform::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	_value.set(transform);

	Value locationValue;
	result &= location.update(locationValue, _context);
	::Vector3 locationVector3 = transform.get_translation();
	if (locationValue.get_type() == type_id<::Vector3>())
	{
		locationVector3 = locationValue.get_as<::Vector3>();
	}

	::Rotator3 rotationRotator3 = transform.get_orientation().to_rotator();
	if (!rotation.is_empty())
	{
		Value rotationValue;
		result &= rotation.update(rotationValue, _context);
		if (rotationValue.get_type() == type_id<::Rotator3>())
		{
			rotationRotator3 = rotationValue.get_as<::Rotator3>();
		}
	}

	float useScale = 1.0f;
	if (!scale.is_empty())
	{
		Value scaleValue;
		result &= scale.update(scaleValue, _context);
		if (scaleValue.get_type() == type_id<float>())
		{
			useScale = scaleValue.get_as<float>();
		}
	}

	if (firstAxis != Axis::None &&
		secondAxis != Axis::None)
	{
		Value forwardAxisValue;
		Value rightAxisValue;
		Value upAxisValue;
		result &= forwardAxis.update(forwardAxisValue, _context);
		result &= rightAxis.update(rightAxisValue, _context);
		result &= upAxis.update(upAxisValue, _context);

		::Vector3 forwardAxisVector3 = ::Vector3::zero;
		::Vector3 rightAxisVector3 = ::Vector3::zero;
		::Vector3 upAxisVector3 = ::Vector3::zero;

		if (forwardAxisValue.get_type() == type_id<::Vector3>())
		{
			forwardAxisVector3 = forwardAxisValue.get_as<::Vector3>().normal();
		}
		if (rightAxisValue.get_type() == type_id<::Vector3>())
		{
			rightAxisVector3 = rightAxisValue.get_as<::Vector3>().normal();
		}
		if (upAxisValue.get_type() == type_id<::Vector3>())
		{
			upAxisVector3 = upAxisValue.get_as<::Vector3>().normal();
		}

		::Transform resultTransform;
		if (result)
		{
			resultTransform.build_from_axes(firstAxis, secondAxis, forwardAxisVector3, rightAxisVector3, upAxisVector3, locationVector3);
		}

		an_assert(resultTransform.get_orientation().is_normalised());

		resultTransform.set_scale(useScale);

		_value.set(resultTransform);
	}
	else
	{
		::Transform resultTransform = ::Transform::identity;
		resultTransform.set_translation(locationVector3);
		if (!rotation.is_empty())
		{
			resultTransform.set_orientation(rotationRotator3.to_quat());
		}

		an_assert(resultTransform.get_orientation().is_normalised());

		resultTransform.set_scale(useScale);

		_value.set(resultTransform);
	}

	return result;
}

//

bool ToTransform::update(REF_ Value& _value, Context& _context) const
{
	bool result = true;

	if (_value.get_type() == type_id<::Matrix44>())
	{
		_value.set(_value.get_as<::Matrix44>().to_transform());
	}
	else if (_value.get_type() == type_id<::Rotator3>())
	{
		_value.set(::Transform(::Vector3::zero, _value.get_as<::Rotator3>().to_quat()));
	}
	else
	{
		error_processing_wmp(TXT("can't convert \"%S\" to Transform"), RegisteredType::get_name_of(_value.get_type()));
		result = false;
	}

	return result;
}
