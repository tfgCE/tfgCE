#include "wmp_transformOps.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool TransformOp::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= transformToolSet.load_from_xml(_node);

	return result;
}

bool TransformOp::update(REF_ Value & _value, Context & _context) const
{
	if (! can_handle(_value))
	{
		error_processing_wmp(TXT("can't transform %S"), RegisteredType::get_name_of(_value.get_type()));
		return false;
	}

	bool result = true;

	Value resultValue = _value;
	Value transformValue;
	if (transformToolSet.update(transformValue, _context))
	{
		if (transformValue.get_type() == type_id<Transform>() ||
			transformValue.get_type() == type_id<Rotator3>() ||
			transformValue.get_type() == type_id<Quat>())
		{
			handle_operation(resultValue, transformValue);
		}
		else
		{
			error_processing_wmp(TXT("\"transformOp\" expected \"transform\", \"rotator\" or \"quat\" value"));
			result = false;
		}
	}
	else
	{
		result = false;
	}

	_value = resultValue;

	return result;
}

//

bool TransformOpOnVector::can_handle(Value const & _value) const
{
	return _value.get_type() == type_id<Vector3>();
}

//

bool TransformOpOnTransform::can_handle(Value const & _value) const
{
	return _value.get_type() == type_id<Transform>();
}

//

bool TransformOpOnQuat::can_handle(Value const & _value) const
{
	return _value.get_type() == type_id<Quat>();
}

//

void VectorToWorldOf::handle_operation(REF_ Value & _vector, Value const & _transform) const
{
	if (_transform.get_type() == type_id<Transform>())
	{
		_vector.set(_transform.get_as<Transform>().vector_to_world(_vector.get_as<Vector3>()));
	}
	else if (_transform.get_type() == type_id<Rotator3>())
	{
		_vector.set(Transform(Vector3::zero, _transform.get_as<Rotator3>().to_quat()).vector_to_world(_vector.get_as<Vector3>()));
	}
	else if (_transform.get_type() == type_id<Quat>())
	{
		_vector.set(Transform(Vector3::zero, _transform.get_as<Quat>()).vector_to_world(_vector.get_as<Vector3>()));
	}
}

//

void VectorToLocalOf::handle_operation(REF_ Value & _vector, Value const & _transform) const
{
	if (_transform.get_type() == type_id<Transform>())
	{
		_vector.set(_transform.get_as<Transform>().vector_to_local(_vector.get_as<Vector3>()));
	}
	else if (_transform.get_type() == type_id<Rotator3>())
	{
		_vector.set(Transform(Vector3::zero, _transform.get_as<Rotator3>().to_quat()).vector_to_local(_vector.get_as<Vector3>()));
	}
	else if (_transform.get_type() == type_id<Quat>())
	{
		_vector.set(Transform(Vector3::zero, _transform.get_as<Quat>()).vector_to_local(_vector.get_as<Vector3>()));
	}
}

//

void LocationToWorldOf::handle_operation(REF_ Value & _vector, Value const & _transform) const
{
	if (_transform.get_type() == type_id<Transform>())
	{
		_vector.set(_transform.get_as<Transform>().location_to_world(_vector.get_as<Vector3>()));
	}
	else
	{
		if (_transform.get_type() == type_id<Rotator3>())
		{
			_vector.set(Transform(Vector3::zero, _transform.get_as<Rotator3>().to_quat()).location_to_world(_vector.get_as<Vector3>()));
		}
		else if (_transform.get_type() == type_id<Quat>())
		{
			_vector.set(Transform(Vector3::zero, _transform.get_as<Quat>()).location_to_world(_vector.get_as<Vector3>()));
		}
	}
}

//

void LocationToLocalOf::handle_operation(REF_ Value & _vector, Value const & _transform) const
{
	if (_transform.get_type() == type_id<Transform>())
	{
		_vector.set(_transform.get_as<Transform>().location_to_local(_vector.get_as<Vector3>()));
	}
	else
	{
		if (_transform.get_type() == type_id<Rotator3>())
		{
			_vector.set(Transform(Vector3::zero, _transform.get_as<Rotator3>().to_quat()).location_to_local(_vector.get_as<Vector3>()));
		}
		else if (_transform.get_type() == type_id<Quat>())
		{
			_vector.set(Transform(Vector3::zero, _transform.get_as<Quat>()).location_to_local(_vector.get_as<Vector3>()));
		}
	}
}

//

void ToWorldOf::handle_operation(REF_ Value & _value, Value const & _transform) const
{
	if (_transform.get_type() == type_id<Transform>())
	{
		_value.set(_transform.get_as<Transform>().to_world(_value.get_as<Transform>()));
	}
	else if (_transform.get_type() == type_id<Rotator3>())
	{
		_value.set(Transform(Vector3::zero, _transform.get_as<Rotator3>().to_quat()).to_world(_value.get_as<Transform>()));
	}
	else if (_transform.get_type() == type_id<Quat>())
	{
		_value.set(Transform(Vector3::zero, _transform.get_as<Quat>()).to_world(_value.get_as<Transform>()));
	}
}

//

void ToLocalOf::handle_operation(REF_ Value & _value, Value const & _transform) const
{
	if (_transform.get_type() == type_id<Transform>())
	{
		_value.set(_transform.get_as<Transform>().to_local(_value.get_as<Transform>()));
	}
	else if (_transform.get_type() == type_id<Rotator3>())
	{
		_value.set(Transform(Vector3::zero, _transform.get_as<Rotator3>().to_quat()).to_local(_value.get_as<Transform>()));
	}
	else if (_transform.get_type() == type_id<Quat>())
	{
		_value.set(Transform(Vector3::zero, _transform.get_as<Quat>()).to_local(_value.get_as<Transform>()));
	}
}

//

void QuatToWorldOf::handle_operation(REF_ Value & _value, Value const & _transform) const
{
	if (_transform.get_type() == type_id<Transform>())
	{
		_value.set(_transform.get_as<Transform>().get_orientation().to_world(_value.get_as<Quat>()));
	}
	else if (_transform.get_type() == type_id<Rotator3>())
	{
		_value.set(_transform.get_as<Rotator3>().to_quat().to_world(_value.get_as<Quat>()));
	}
	else if (_transform.get_type() == type_id<Quat>())
	{
		_value.set(_transform.get_as<Quat>().to_world(_value.get_as<Quat>()));
	}
}

//

void QuatToLocalOf::handle_operation(REF_ Value & _value, Value const & _transform) const
{
	if (_transform.get_type() == type_id<Transform>())
	{
		_value.set(_transform.get_as<Transform>().get_orientation().to_local(_value.get_as<Quat>()));
	}
	else if (_transform.get_type() == type_id<Rotator3>())
	{
		_value.set(_transform.get_as<Rotator3>().to_quat().to_local(_value.get_as<Quat>()));
	}
	else if (_transform.get_type() == type_id<Quat>())
	{
		_value.set(_transform.get_as<Quat>().to_local(_value.get_as<Quat>()));
	}
}
