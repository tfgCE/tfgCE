#include "math.h"
#include "..\io\xml.h"
#include "..\serialisation\serialiser.h"

bool Transform::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}

	bool result = true;
	
	translation.load_from_xml_child_node(_node, TXT("location"));
	translation.load_from_xml_child_node(_node, TXT("translation"));

	orientation.load_from_xml_child_node_as_rotator(_node, TXT("rotation"));
	
	if (_node->first_child_named(TXT("forward")) ||
		_node->first_child_named(TXT("up")) ||
		_node->first_child_named(TXT("right")))
	{
		Vector3 forwardAxis = Vector3::zero;
		Vector3 rightAxis = Vector3::zero;
		Vector3 upAxis = Vector3::zero;

		forwardAxis.load_from_xml(_node->first_child_named(TXT("forward")));
		upAxis.load_from_xml(_node->first_child_named(TXT("up")));
		rightAxis.load_from_xml(_node->first_child_named(TXT("right")));

		Axis::Type firstAxis = Axis::None;
		Axis::Type secondAxis = Axis::None;

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

		if (firstAxis != Axis::None &&
			secondAxis == Axis::None)
		{
			error_loading_xml(_node, TXT("provide two axes or none"));
			result = false;
		}
		else
		{
			forwardAxis.normalise();
			rightAxis.normalise();
			upAxis.normalise();
			build_from_axes(firstAxis, secondAxis, forwardAxis, rightAxis, upAxis, translation);
			if (scale == 0.0f)
			{
				// case for no valid values provided, most likely wmp tool translation
				scale = 1.0f;
			}
		}
	}

	scale = _node->get_float_attribute_or_from_child(TXT("scale"), scale);

	todo_note(TXT("try with axes or points"));

	return result;
}

bool Transform::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode)
{
	if (!_node)
	{
		return false;
	}
	if (IO::XML::Node const * child = _node->first_child_named(_childNode))
	{
		return load_from_xml(child);
	}
	return false;
}

bool Transform::save_to_xml(IO::XML::Node * _node) const
{
	bool result = true;

	result &= translation.save_to_xml_child_node(_node, TXT("translation"));
	result &= orientation.save_to_xml_child_node_as_rotator(_node, TXT("rotation"));

	return result;
}

bool Transform::save_to_xml_child_node(IO::XML::Node * _node, tchar const * _childNode) const
{
	return save_to_xml(_node->add_node(_childNode));
}

bool Transform::serialise(Serialiser & _serialiser)
{
	bool result = true;
	result &= serialise_data(_serialiser, translation);
	result &= serialise_data(_serialiser, orientation);
	result &= serialise_data(_serialiser, scale);
	assert_slow(scale >= 0.0f);
	return result;
}

bool Transform::build_from_axes(Axis::Type _firstAxis, Axis::Type _secondAxis, Vector3 const & _forward, Vector3 const & _right, Vector3 const & _up, Vector3 const & _location)
{
	bool result = true;

	Matrix33 builtOrientation;
	if (builtOrientation.build_orthogonal_from_axes(_firstAxis, _secondAxis, _forward, _right, _up))
	{
		orientation = builtOrientation.to_quat();
	}
	else
	{
		result = false;
	}

	translation = _location;

	scale = _firstAxis == Axis::X ? _right.length() :
		   (_firstAxis == Axis::Y ? _forward.length() :
									_up.length());

	assert_slow(scale >= 0.0f);

	return result;
}

bool Transform::build_from_two_axes(Axis::Type _firstAxis, Axis::Type _secondAxis, Vector3 const & _first, Vector3 const & _second, Vector3 const & _location)
{
	bool result = true;

	Matrix33 builtOrientation;
	if (builtOrientation.build_orthogonal_from_two_axes(_firstAxis, _secondAxis, _first, _second))
	{
		orientation = builtOrientation.to_quat();
	}
	else
	{
		result = false;
	}

	translation = _location;

	scale = _first.length();

	assert_slow(scale >= 0.0f);

	return result;
}

void Transform::log(LogInfoContext & _log) const
{
	_log.log(TXT("translation : %S"), translation.to_string().to_char());
	_log.log(TXT("orientation : %S"), orientation.to_rotator().to_string().to_char());
	if (scale != 1.0f)
	{
		_log.log(TXT("scale : %.8f"), scale);
	}
}
