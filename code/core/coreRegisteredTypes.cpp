#include "coreRegisteredTypes.h"

#define AN_REGISTER_TYPES_OFFSET 1000

#include "other\registeredTypeRegistering.h"

#include "io\xml.h"
#include "math\math.h"
#include "mesh\boneID.h"
#include "other\simpleVariableStorage.h"
#include "other\packedBytes.h"
#include "random\random.h"
#include "random\randomNumber.h"
#include "tags\tag.h"
#include "tags\tagCondition.h"
#include "types\colour.h"
#include "types\name.h"
#include "types\string.h"

//

// some defaults
Optional<bool> optionalBoolDefault;
Optional<float> optionalFloatDefault;
Optional<Vector3> optionalVector3Default;
Random::Number<float> randomNumberFloat;
Random::Number<int> randomNumberInt;
Random::Generator randomGenerator;

//

// should be mirrored in headers with DECLARE_REGISTERED_TYPE
DEFINE_TYPE_LOOK_UP(bool);
DEFINE_TYPE_LOOK_UP(float);
DEFINE_TYPE_LOOK_UP(int);
DEFINE_TYPE_LOOK_UP(long int);
DEFINE_TYPE_LOOK_UP_OBJECT(Random::Number<float>, &randomNumberFloat);
DEFINE_TYPE_LOOK_UP_OBJECT(Random::Number<int>, &randomNumberInt);
DEFINE_TYPE_LOOK_UP_OBJECT(String, nullptr);
DEFINE_TYPE_LOOK_UP_OBJECT(Tags, nullptr);
DEFINE_TYPE_LOOK_UP_OBJECT(TagCondition, nullptr);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Optional<bool>, &optionalBoolDefault);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Optional<float>, &optionalFloatDefault);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Optional<Vector3>, &optionalVector3Default);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Name, &Name::invalid());
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Transform, &Transform::identity);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Plane, &Plane::identity);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Rotator3, &Rotator3::zero);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Vector2, &Vector2::zero);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Vector3, &Vector3::zero);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Vector4, &Vector4::zero);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(VectorInt2, &VectorInt2::zero);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(VectorInt3, &VectorInt3::zero);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(VectorInt4, &VectorInt4::zero);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Range, &Range::empty);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Range2, &Range2::empty);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Range3, &Range3::empty);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(RangeInt, &RangeInt::empty);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(RangeInt2, &RangeInt2::empty);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Quat, &Quat::identity);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Matrix33, &Matrix33::identity);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Matrix44, &Matrix44::identity);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Sphere, &Sphere::zero);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Colour, &Colour::black);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(Meshes::BoneID, &Meshes::BoneID::invalid());
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(SimpleVariableInfo, &SimpleVariableInfo::invalid()); // we actually use typeID to set it inside SimpleVariableInfo
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(PackedBytes, &PackedBytes::empty());
DEFINE_TYPE_LOOK_UP_PLAIN_DATA_NAMED(Random::Generator, randomGenerator, &randomGenerator);
DEFINE_TYPE_LOOK_UP_PTR(Vector3);

//

void CoreRegisteredTypes::initialise_static()
{
	ADD_SUB_FIELD(Transform, Vector3, translation);
	ADD_SUB_FIELD(Transform, float, scale);
	ADD_SUB_FIELD(Transform, Quat, orientation);

	ADD_SUB_FIELD(Plane, Vector3, normal);
	ADD_SUB_FIELD(Plane, Vector3, anchor);

	ADD_SUB_FIELD(Rotator3, float, pitch);
	ADD_SUB_FIELD(Rotator3, float, yaw);
	ADD_SUB_FIELD(Rotator3, float, roll);

	ADD_SUB_FIELD(Vector2, float, x);
	ADD_SUB_FIELD(Vector2, float, y);

	ADD_SUB_FIELD(Vector3, float, x);
	ADD_SUB_FIELD(Vector3, float, y);
	ADD_SUB_FIELD(Vector3, float, z);

	ADD_SUB_FIELD(Vector4, float, x);
	ADD_SUB_FIELD(Vector4, float, y);
	ADD_SUB_FIELD(Vector4, float, z);
	ADD_SUB_FIELD(Vector4, float, w);

	ADD_SUB_FIELD(VectorInt2, int, x);
	ADD_SUB_FIELD(VectorInt2, int, y);

	ADD_SUB_FIELD(VectorInt3, int, x);
	ADD_SUB_FIELD(VectorInt3, int, y);
	ADD_SUB_FIELD(VectorInt3, int, z);

	ADD_SUB_FIELD(VectorInt4, int, x);
	ADD_SUB_FIELD(VectorInt4, int, y);
	ADD_SUB_FIELD(VectorInt4, int, z);
	ADD_SUB_FIELD(VectorInt4, int, w);

	ADD_SUB_FIELD(Range, float, min);
	ADD_SUB_FIELD(Range, float, max);

	ADD_SUB_FIELD(Range2, Range, x);
	ADD_SUB_FIELD(Range2, Range, y);

	ADD_SUB_FIELD(Range3, Range, x);
	ADD_SUB_FIELD(Range3, Range, y);
	ADD_SUB_FIELD(Range3, Range, z);

	ADD_SUB_FIELD(RangeInt, int, min);
	ADD_SUB_FIELD(RangeInt, int, max);

	ADD_SUB_FIELD(RangeInt2, RangeInt, x);
	ADD_SUB_FIELD(RangeInt2, RangeInt, y);

	ADD_SUB_FIELD(Quat, float, x);
	ADD_SUB_FIELD(Quat, float, y);
	ADD_SUB_FIELD(Quat, float, z);
	ADD_SUB_FIELD(Quat, float, w);

	ADD_SUB_FIELD(Sphere, Vector3, location);
	ADD_SUB_FIELD(Sphere, float, radius);
}

void CoreRegisteredTypes::close_static()
{
}

//
//
// parsing

void parse_value__int(String const & _from, void * _into)
{
	ParserUtils::parse_int_ref(_from, REF_ CAST_TO_VALUE(int, _into));
}
ADD_SPECIALISED_PARSE_VALUE(int, parse_value__int);

void parse_value__bool(String const & _from, void * _into)
{
	ParserUtils::parse_bool_ref(_from, REF_ CAST_TO_VALUE(bool, _into));
}
ADD_SPECIALISED_PARSE_VALUE(bool, parse_value__bool);

void parse_value__float(String const & _from, void * _into)
{
	ParserUtils::parse_float_ref(_from, REF_ CAST_TO_VALUE(float, _into));
}
ADD_SPECIALISED_PARSE_VALUE(float, parse_value__float);

void parse_value__string(String const & _from, void * _into)
{
	CAST_TO_VALUE(String, _into) = _from;
}
ADD_SPECIALISED_PARSE_VALUE(String, parse_value__string);

void parse_value__name(String const & _from, void * _into)
{
	CAST_TO_VALUE(Name, _into) = Name(_from);
}
ADD_SPECIALISED_PARSE_VALUE(Name, parse_value__name);

ADD_SPECIALISED_PARSE_VALUE_USING(Tags, load_from_string);
ADD_SPECIALISED_PARSE_VALUE_USING(TagCondition, load_from_string);
ADD_SPECIALISED_PARSE_VALUE_USING(Range, load_from_string);
ADD_SPECIALISED_PARSE_VALUE_USING(Range2, load_from_string);
ADD_SPECIALISED_PARSE_VALUE_USING(Range3, load_from_string);
ADD_SPECIALISED_PARSE_VALUE_USING(RangeInt, load_from_string);
ADD_SPECIALISED_PARSE_VALUE_USING(Rotator3, load_from_string);
ADD_SPECIALISED_PARSE_VALUE_USING(Vector2, load_from_string);
ADD_SPECIALISED_PARSE_VALUE_USING(Vector3, load_from_string);
ADD_SPECIALISED_PARSE_VALUE_USING(Vector4, load_from_string);
ADD_SPECIALISED_PARSE_VALUE_USING(VectorInt2, load_from_string);
ADD_SPECIALISED_PARSE_VALUE_USING(VectorInt3, load_from_string);
ADD_SPECIALISED_PARSE_VALUE_USING(VectorInt4, load_from_string);
ADD_SPECIALISED_PARSE_VALUE_USING(Colour, load_from_string);
ADD_SPECIALISED_PARSE_VALUE_USING(PackedBytes, load_from_string);

//
//
// saving to xml

bool save_to_xml__int(IO::XML::Node* _node, void const* _value)
{
	_node->add_text(String::printf(TXT("%i"), CAST_TO_VALUE(int, _value)));
	return true;
}
ADD_SPECIALISED_SAVE_TO_XML(int, save_to_xml__int);

bool save_to_xml__bool(IO::XML::Node* _node, void const* _value)
{
	_node->add_text(CAST_TO_VALUE(bool, _value)? TXT("true") : TXT("false"));
	return true;
}
ADD_SPECIALISED_SAVE_TO_XML(bool, save_to_xml__bool);

bool save_to_xml__float(IO::XML::Node* _node, void const* _value)
{
	_node->add_text(String::printf(TXT("%.8f"), CAST_TO_VALUE(float, _value)));
	return true;
}
ADD_SPECIALISED_SAVE_TO_XML(float, save_to_xml__float);

bool save_to_xml__tags(IO::XML::Node* _node, void const* _value)
{
	_node->add_text(String::printf(TXT("%S"), CAST_TO_VALUE(Tags, _value).to_string().to_char()));
	return true;
}
ADD_SPECIALISED_SAVE_TO_XML(Tags, save_to_xml__tags);

bool save_to_xml__tag_condition(IO::XML::Node* _node, void const* _value)
{
	_node->add_text(String::printf(TXT("%S"), CAST_TO_VALUE(TagCondition, _value).to_string().to_char()));
	return true;
}
ADD_SPECIALISED_SAVE_TO_XML(TagCondition, save_to_xml__tag_condition);

ADD_SPECIALISED_SAVE_TO_XML_USING(Name, save_to_xml);
ADD_SPECIALISED_SAVE_TO_XML_USING(String, save_to_xml);
ADD_SPECIALISED_SAVE_TO_XML_USING(Rotator3, save_to_xml);
ADD_SPECIALISED_SAVE_TO_XML_USING(Vector2, save_to_xml);
ADD_SPECIALISED_SAVE_TO_XML_USING(Vector3, save_to_xml);
ADD_SPECIALISED_SAVE_TO_XML_USING(RangeInt, save_to_xml);
ADD_SPECIALISED_SAVE_TO_XML_USING(RangeInt2, save_to_xml);
ADD_SPECIALISED_SAVE_TO_XML_USING(VectorInt2, save_to_xml);
ADD_SPECIALISED_SAVE_TO_XML_USING(VectorInt3, save_to_xml);
ADD_SPECIALISED_SAVE_TO_XML_USING(Transform, save_to_xml);
ADD_SPECIALISED_SAVE_TO_XML_USING(Quat, save_to_xml_as_rotator);
ADD_SPECIALISED_SAVE_TO_XML_USING_NAMED(Random::Generator, randomGenerator, save_to_xml);
ADD_SPECIALISED_SAVE_TO_XML_USING(PackedBytes, save_to_xml);

//
//
// loading from xml

bool load_from_xml__int(IO::XML::Node const * _node, void * _into)
{
	CAST_TO_VALUE(int, _into) = _node->get_int(CAST_TO_VALUE(int, _into));
	return true;
}
ADD_SPECIALISED_LOAD_FROM_XML(int, load_from_xml__int);

bool load_from_xml__bool(IO::XML::Node const * _node, void * _into)
{
	CAST_TO_VALUE(bool, _into) = _node->get_bool(CAST_TO_VALUE(bool, _into));
	return true;
}
ADD_SPECIALISED_LOAD_FROM_XML(bool, load_from_xml__bool);

bool load_from_xml__float(IO::XML::Node const * _node, void * _into)
{
	CAST_TO_VALUE(float, _into) = _node->get_float(CAST_TO_VALUE(float, _into));
	return true;
}
ADD_SPECIALISED_LOAD_FROM_XML(float, load_from_xml__float);

bool load_from_xml__string(IO::XML::Node const * _node, void * _into)
{
	CAST_TO_VALUE(String, _into) = _node->get_text();
	return true;
}
ADD_SPECIALISED_LOAD_FROM_XML(String, load_from_xml__string);

bool load_from_xml__name(IO::XML::Node const * _node, void * _into)
{
	CAST_TO_VALUE(Name, _into) = Name(_node->get_text());
	return true;
}
ADD_SPECIALISED_LOAD_FROM_XML(Name, load_from_xml__name);

ADD_SPECIALISED_LOAD_FROM_XML_USING(Tags, load_from_xml_attribute_or_node);
ADD_SPECIALISED_LOAD_FROM_XML_USING(TagCondition, load_from_xml_attribute_or_node);
ADD_SPECIALISED_LOAD_FROM_XML_USING(Transform, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(Plane, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(Rotator3, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(Vector2, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(Vector3, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(Vector4, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(VectorInt2, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(VectorInt3, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(VectorInt4, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(Quat, load_from_xml_as_rotator);
ADD_SPECIALISED_LOAD_FROM_XML_USING(Range, load_from_xml_or_text);
ADD_SPECIALISED_LOAD_FROM_XML_USING(Range2, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(Range3, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(RangeInt, load_from_xml_or_text);
ADD_SPECIALISED_LOAD_FROM_XML_USING(RangeInt2, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(Matrix44, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(Matrix33, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(Sphere, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(Colour, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING_NAMED(Random::Generator, randomGenerator, load_from_xml);
ADD_SPECIALISED_LOAD_FROM_XML_USING(PackedBytes, load_from_xml);

//
//
// logging values

void log_value__int(LogInfoContext & _log, void const * _value, tchar const * _name)
{
	if (_name)
	{
		_log.log(TXT("%S [int] : %i"), _name, CAST_TO_VALUE(int, _value));
	}
	else
	{
		_log.log(TXT("[int] : %i"), CAST_TO_VALUE(int, _value));
	}
}
ADD_SPECIALISED_LOG_VALUE(int, log_value__int);

void log_value__bool(LogInfoContext & _log, void const * _value, tchar const * _name)
{
	if (_name)
	{
		_log.log(TXT("%S [bool] : %S"), _name, CAST_TO_VALUE(bool, _value)? TXT("true") : TXT("false"));
	}
	else
	{
		_log.log(TXT("[bool] : %S"), CAST_TO_VALUE(bool, _value) ? TXT("true") : TXT("false"));
	}
}
ADD_SPECIALISED_LOG_VALUE(bool, log_value__bool);

void log_value__float(LogInfoContext & _log, void const * _value, tchar const * _name)
{
	if (_name)
	{
		_log.log(TXT("%S [float] : %.8f"), _name, CAST_TO_VALUE(float, _value));
	}
	else
	{
		_log.log(TXT("[float] : %.8f"), CAST_TO_VALUE(float, _value));
	}
}
ADD_SPECIALISED_LOG_VALUE(float, log_value__float);

void log_value__string(LogInfoContext & _log, void const * _value, tchar const * _name)
{
	if (_name)
	{
		_log.log(TXT("%S [String] : %S"), _name, CAST_TO_VALUE(String, _value).to_char());
	}
	else
	{
		_log.log(TXT("[String] : %S"), CAST_TO_VALUE(String, _value).to_char());
	}
}
ADD_SPECIALISED_LOG_VALUE(String, log_value__string);

void log_value__name(LogInfoContext & _log, void const * _value, tchar const * _name)
{
	if (_name)
	{
		_log.log(TXT("%S [Name] : %S"), _name, CAST_TO_VALUE(Name, _value).to_char());
	}
	else
	{
		_log.log(TXT("[Name] : %S"), CAST_TO_VALUE(Name, _value).to_char());
	}
}
ADD_SPECIALISED_LOG_VALUE(Name, log_value__name);

void log_value__optional_bool(LogInfoContext & _log, void const * _value, tchar const * _name)
{
	if (CAST_TO_VALUE(Optional<bool>, _value).is_set())
	{
		if (_name)
		{
			_log.log(TXT("%S [Optional<bool>] : %S"), _name, CAST_TO_VALUE(Optional<bool>, _value).get()? TXT("true") : TXT("false"));
		}
		else
		{
			_log.log(TXT("[Optional<bool>] : %.8f"), CAST_TO_VALUE(Optional<bool>, _value).get() ? TXT("true") : TXT("false"));
		}
	}
	else
	{
		if (_name)
		{
			_log.log(TXT("%S [Optional<bool>] : not set"), _name);
		}
		else
		{
			_log.log(TXT("[Optional<bool>] : not set"));
		}
	}
}
ADD_SPECIALISED_LOG_VALUE(Optional<bool>, log_value__optional_bool);

void log_value__optional_float(LogInfoContext & _log, void const * _value, tchar const * _name)
{
	if (CAST_TO_VALUE(Optional<float>, _value).is_set())
	{
		if (_name)
		{
			_log.log(TXT("%S [Optional<float>] : %.8f"), _name, CAST_TO_VALUE(Optional<float>, _value).get());
		}
		else
		{
			_log.log(TXT("[Optional<float>] : %.8f"), CAST_TO_VALUE(Optional<float>, _value).get());
		}
	}
	else
	{
		if (_name)
		{
			_log.log(TXT("%S [Optional<float>] : not set"), _name);
		}
		else
		{
			_log.log(TXT("[Optional<float>] : not set"));
		}
	}
}
ADD_SPECIALISED_LOG_VALUE(Optional<float>, log_value__optional_float);

ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(Tags);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(TagCondition);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(Plane);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(Rotator3);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(Vector2);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(Vector3);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(Vector4);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(VectorInt2);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(VectorInt3);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(VectorInt4);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(Quat);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(Range);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(Range2);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(Range3);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(RangeInt);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(RangeInt2);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(Colour);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(Sphere);
ADD_SPECIALISED_LOG_VALUE_USING_TO_STRING(PackedBytes);

ADD_SPECIALISED_LOG_VALUE_USING_LOG(Transform);
ADD_SPECIALISED_LOG_VALUE_USING_LOG(Matrix44);
ADD_SPECIALISED_LOG_VALUE_USING_LOG(Matrix33);

