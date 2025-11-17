#include "optional.h"

#include "..\math\math.h"

template <>
bool Optional<bool>::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		element = false;
		set_flag(flags, Flag::HasAnyValueStored);
	}
	if (IO::XML::Attribute const * valueAttr = _node->get_attribute(TXT("value")))
	{
		return load_from_xml(valueAttr);
	}
	else
	{
		set(_node->get_bool(element));
	}
	return true;
}

template <>
bool Optional<int>::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		element = 0;
		set_flag(flags, Flag::HasAnyValueStored);
	}
	if (IO::XML::Attribute const * valueAttr = _node->get_attribute(TXT("value")))
	{
		return load_from_xml(valueAttr);
	}
	else
	{
		set(_node->get_int(element));
	}
	return true;
}

template <>
bool Optional<float>::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		element = 0.0f;
		set_flag(flags, Flag::HasAnyValueStored);
	}
	if (IO::XML::Attribute const * valueAttr = _node->get_attribute(TXT("value")))
	{
		return load_from_xml(valueAttr);
	}
	else
	{
		set(_node->get_float(element));
	}
	return true;
}

template <>
bool Optional<Name>::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		element = Name::invalid();
		set_flag(flags, Flag::HasAnyValueStored);
	}
	if (IO::XML::Attribute const * valueAttr = _node->get_attribute(TXT("value")))
	{
		return load_from_xml(valueAttr);
	}
	else
	{
		set(_node->get_name(element));
	}
	return true;
}

template <>
bool Optional<VectorInt2>::load_from_xml(IO::XML::Node const* _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		element = VectorInt2::zero;
		set_flag(flags, Flag::HasAnyValueStored);
	}
	if (IO::XML::Attribute const* valueAttr = _node->get_attribute(TXT("value")))
	{
		return load_from_xml(valueAttr);
	}
	else
	{
		VectorInt2 value = VectorInt2::zero; // always overwrite
		value.load_from_xml(_node);
		set(value);
	}
	return true;
}

template <>
bool Optional<Vector2>::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		element = Vector2::zero;
		set_flag(flags, Flag::HasAnyValueStored);
	}
	if (IO::XML::Attribute const * valueAttr = _node->get_attribute(TXT("value")))
	{
		return load_from_xml(valueAttr);
	}
	else
	{
		Vector2 value = Vector2::zero; // always overwrite
		value.load_from_xml(_node);
		set(value);
	}
	return true;
}

template <>
bool Optional<Vector3>::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		element = Vector3::zero;
		set_flag(flags, Flag::HasAnyValueStored);
	}
	if (IO::XML::Attribute const * valueAttr = _node->get_attribute(TXT("value")))
	{
		return load_from_xml(valueAttr);
	}
	else
	{
		Vector3 value = Vector3::zero; // always overwrite
		value.load_from_xml(_node);
		set(value);
	}
	return true;
}

template <>
bool Optional<Rotator3>::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		element = Rotator3::zero;
		set_flag(flags, Flag::HasAnyValueStored);
	}
	if (IO::XML::Attribute const * valueAttr = _node->get_attribute(TXT("value")))
	{
		return load_from_xml(valueAttr);
	}
	else
	{
		Rotator3 value = Rotator3::zero; // always overwrite
		value.load_from_xml(_node);
		set(value);
	}
	return true;
}

template <>
bool Optional<Range>::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		element = Range::empty;
		set_flag(flags, Flag::HasAnyValueStored);
	}
	if (IO::XML::Attribute const * valueAttr = _node->get_attribute(TXT("value")))
	{
		return load_from_xml(valueAttr);
	}
	else
	{
		Range value = Range::empty; // always overwrite
		value.load_from_xml(_node);
		value.load_from_string(_node->get_internal_text());
		set(value);
	}
	return true;
}

template <>
bool Optional<Range2>::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		element = Range2::empty;
		set_flag(flags, Flag::HasAnyValueStored);
	}
	if (IO::XML::Attribute const * valueAttr = _node->get_attribute(TXT("value")))
	{
		return load_from_xml(valueAttr);
	}
	else
	{
		Range2 value = Range2::empty; // always overwrite
		value.load_from_xml(_node);
		value.load_from_string(_node->get_internal_text());
		set(value);
	}
	return true;
}

template <>
bool Optional<Range3>::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		element = Range3::empty;
		set_flag(flags, Flag::HasAnyValueStored);
	}
	if (IO::XML::Attribute const * valueAttr = _node->get_attribute(TXT("value")))
	{
		return load_from_xml(valueAttr);
	}
	else
	{
		Range3 value = Range3::empty; // always overwrite
		value.load_from_xml(_node);
		value.load_from_string(_node->get_internal_text());
		set(value);
	}
	return true;
}

template <>
bool Optional<RangeInt>::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		element = RangeInt::empty;
		set_flag(flags, Flag::HasAnyValueStored);
	}
	if (IO::XML::Attribute const * valueAttr = _node->get_attribute(TXT("value")))
	{
		return load_from_xml(valueAttr);
	}
	else
	{
		RangeInt value = RangeInt::empty; // always overwrite
		value.load_from_xml(_node);
		value.load_from_string(_node->get_internal_text());
		set(value);
	}
	return true;
}

template <>
bool Optional<Transform>::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		element = Transform::identity;
		set_flag(flags, Flag::HasAnyValueStored);
	}
	if (IO::XML::Attribute const * valueAttr = _node->get_attribute(TXT("value")))
	{
		return load_from_xml(valueAttr);
	}
	else
	{
		Transform value = Transform::identity; // always overwrite
		value.load_from_xml(_node);
		set(value);
	}
	return true;
}

template <>
bool Optional<Sphere>::load_from_xml(IO::XML::Node const* _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		element = Sphere::zero;
		set_flag(flags, Flag::HasAnyValueStored);
	}
	if (IO::XML::Attribute const* valueAttr = _node->get_attribute(TXT("value")))
	{
		return load_from_xml(valueAttr);
	}
	else
	{
		Sphere value = Sphere::zero; // always overwrite
		value.load_from_xml(_node);
		set(value);
	}
	return true;
}

template <>
bool Optional<bool>::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return false;
	}
	set(_attr->get_as_bool());
	return true;
}

template <>
bool Optional<int>::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return false;
	}
	set(_attr->get_as_int());
	return true;
}

template <>
bool Optional<float>::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return false;
	}
	set(_attr->get_as_float());
	return true;
}

template <>
bool Optional<Name>::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return false;
	}
	set(_attr->get_as_name());
	return true;
}

template <>
bool Optional<VectorInt2>::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return false;
	}

	VectorInt2 value = VectorInt2::zero; // always overwrite
	value.load_from_string(_attr->get_as_string());
	set(value);
	return true;
}

template <>
bool Optional<Vector2>::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return false;
	}

	Vector2 value = Vector2::zero; // always overwrite
	value.load_from_string(_attr->get_as_string());
	set(value);
	return true;
}

template <>
bool Optional<Vector3>::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return false;
	}
	Vector3 value = Vector3::zero; // always overwrite
	value.load_from_string(_attr->get_as_string());
	set(value);
	return true;
}

template <>
bool Optional<Rotator3>::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return false;
	}
	Rotator3 value = Rotator3::zero; // always overwrite
	value.load_from_string(_attr->get_as_string());
	set(value);
	return true;
}

template <>
bool Optional<Range>::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return false;
	}
	Range value = Range::empty; // always overwrite
	value.load_from_string(_attr->get_as_string());
	set(value);
	return true;
}

template <>
bool Optional<Range2>::load_from_xml(IO::XML::Attribute const* _attr)
{
	if (!_attr)
	{
		return false;
	}

	Range2 value = Range2::zero; // always overwrite
	value.load_from_string(_attr->get_as_string());
	set(value);
	return true;
}

template <>
bool Optional<Range3>::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return false;
	}
	Range3 value = Range3::empty; // always overwrite
	value.load_from_string(_attr->get_as_string());
	set(value);
	return true;
}

template <>
bool Optional<RangeInt>::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return false;
	}
	RangeInt value = RangeInt::empty; // always overwrite
	value.load_from_string(_attr->get_as_string());
	set(value);
	return true;
}

template <>
bool Optional<Transform>::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return false;
	}
	error_loading_xml(_attr->get_parent_node(), TXT("can't load transform from a string"));
	return false;
}

template <>
bool Optional<Sphere>::load_from_xml(IO::XML::Attribute const* _attr)
{
	if (!_attr)
	{
		return false;
	}
	error_loading_xml(_attr->get_parent_node(), TXT("can't load sphere from a string"));
	return false;
}
