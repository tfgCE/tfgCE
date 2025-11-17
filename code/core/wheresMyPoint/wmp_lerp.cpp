#include "wmp_lerp.h"

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

bool Lerp::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	at = _node->get_float_attribute(TXT("at"), at);

	result &= value0ToolSet.load_from_xml(_node->first_child_named(TXT("value0")));
	result &= value1ToolSet.load_from_xml(_node->first_child_named(TXT("value1")));
	result &= atToolSet.load_from_xml(_node->first_child_named(TXT("at")));

	error_loading_xml_on_assert(!value0ToolSet.is_empty(), result, _node, TXT("value0 empty or missing"));
	error_loading_xml_on_assert(!value1ToolSet.is_empty(), result, _node, TXT("value1 empty or missing"));

	return result;
}

bool Lerp::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value resVal;

	Value v0, v1;
	v0 = _value;
	v1 = _value;
	if (!value0ToolSet.update(v0, _context))
	{
		error_processing_wmp_tool(this, TXT("invalid value"));
		result = false;
	}
	if (!value1ToolSet.update(v1, _context))
	{
		error_processing_wmp_tool(this, TXT("invalid value"));
		result = false;

	}
	Value useAt;
	useAt.set(at);
	if (!atToolSet.update(useAt, _context))
	{
		error_processing_wmp_tool(this, TXT("invalid value"));
		result = false;
	}

	if (result)
	{
		if (useAt.get_type() != type_id<float>())
		{
			error_processing_wmp_tool(this, TXT("expecting float (at)"));
			result = false;
		}
		if (v0.get_type() != v1.get_type())
		{
			error_processing_wmp_tool(this, TXT("v0 and v1 types mismatch"));
			result = false;
		}
	}
	if (result)
	{
		if (v0.get_type() == type_id<float>())
		{
			resVal.set(lerp(useAt.get_as<float>(), v0.get_as<float>(), v1.get_as<float>()));
		}
		else if (v0.get_type() == type_id<Vector3>())
		{
			resVal.set(lerp(useAt.get_as<float>(), v0.get_as<Vector3>(), v1.get_as<Vector3>()));
		}
		else if (v0.get_type() == type_id<Transform>())
		{
			resVal.set(Transform::lerp(useAt.get_as<float>(), v0.get_as<Transform>(), v1.get_as<Transform>()));
		}
		else
		{
			todo_implement;
		}
	}

	_value = resVal;

	return result;
}

