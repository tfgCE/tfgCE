#include "wmp_getPt.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

//

using namespace WheresMyPoint;

//

bool GetPt::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	if (_node && !_node->get_text().is_empty() && toolSet.is_empty())
	{
		value.load_from_xml(_node);
	}

	return result;
}

template <typename Class>
bool GetPt::update_value(REF_ Value& _value, Context& _context) const
{
	bool result = true;

	if (!toolSet.is_empty())
	{
		if (toolSet.update(_value, _context) && _value.is_set())
		{
			if (_value.get_type() == type_id<Class>())
			{
				// ok
			}
			else
			{
				error_processing_wmp_tool(this, TXT("expecting %S"), RegisteredType::get_name_of(type_id<Class>()));
			}
		}
		else
		{
			error_processing_wmp_tool(this, TXT("could not get value for %S"), RegisteredType::get_name_of(type_id<Class>()));
			result = false;
		}
	}
	else if (value.is_set())
	{
		if (type_id<Class>() == type_id<float>())
		{
			_value.set(value.get());
		}
		else
		{
			error_processing_wmp_tool(this, TXT("expecting float"));
		}
	}
	else if (_value.get_type() != type_id<Class>())
	{
		_value.set(zero<Class>());
	}

	return result;
}

bool GetPt::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (_value.get_type() == type_id<Range>())
	{
		Value ptValue;
		ptValue.set(0.0f);
		if(update_value<float>(ptValue, _context))
		{
			if (_value.get_type() == type_id<Range>())
			{
				if (ptValue.get_type() == type_id<float>())
				{
					_value.set(_value.get_as<Range>().get_at(ptValue.get_as<float>()));
				}
				else
				{
					error_processing_wmp(TXT("expecting float pt value"));
					result = false;
				}
			}
			else
			{
				todo_implement;
			}
		}
		else
		{
			error_processing_wmp(TXT("problem processing pt value"));
			result = false;
		}
	}
	else if (_value.get_type() == type_id<Range2>())
	{
		Value ptValue;
		ptValue.set(Vector2::zero);
		if(update_value<Vector2>(ptValue, _context))
		{
			if (_value.get_type() == type_id<Range2>())
			{
				if (ptValue.get_type() == type_id<Vector2>())
				{
					_value.set(_value.get_as<Range2>().get_at(ptValue.get_as<Vector2>()));
				}
				else
				{
					error_processing_wmp(TXT("expecting Vector2 pt value"));
					result = false;
				}
			}
			else
			{
				todo_implement;
			}
		}
		else
		{
			error_processing_wmp(TXT("problem processing pt value"));
			result = false;
		}
	}
	else if (_value.get_type() == type_id<Range3>())
	{
		Value ptValue;
		ptValue.set(Vector3::zero);
		if(update_value<Vector3>(ptValue, _context))
		{
			if (_value.get_type() == type_id<Range3>())
			{
				if (ptValue.get_type() == type_id<Vector3>())
				{
					_value.set(_value.get_as<Range3>().get_at(ptValue.get_as<Vector3>()));
				}
				else
				{
					error_processing_wmp(TXT("expecting Vector3 pt value"));
					result = false;
				}
			}
			else
			{
				todo_implement;
			}
		}
		else
		{
			error_processing_wmp(TXT("problem processing pt value"));
			result = false;
		}
	}
	else
	{
		error_processing_wmp(TXT("unable to handle type \"%S\" by getPt"), RegisteredType::get_name_of(_value.get_type()));
		result = false;
	}

	return result;
}
