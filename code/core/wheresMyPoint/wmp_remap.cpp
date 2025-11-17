#include "wmp_remap.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\other\parserUtils.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool WheresMyPoint::Remap::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= load_into(_node, TXT("from"), REF_ from0, REF_ from1);
	result &= load_into(_node, TXT("to"), REF_ to0, REF_ to1);

	result &= load_into(_node, TXT("from0"), REF_ from0, REF_ from0ToolSet);
	result &= load_into(_node, TXT("from1"), REF_ from1, REF_ from1ToolSet);
	result &= load_into(_node, TXT("to0"), REF_ to0, REF_ to0ToolSet);
	result &= load_into(_node, TXT("to1"), REF_ to1, REF_ to1ToolSet);

	clampValue = _node->get_bool_attribute_or_from_child_presence(TXT("clamp"), clampValue);
	clampValue = ! _node->get_bool_attribute_or_from_child_presence(TXT("dontClamp"), !clampValue);

	return result;
}

bool WheresMyPoint::Remap::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	float _from0 = from0;
	float _from1 = from1;
	float _to0 = to0;
	float _to1 = to1;

	result &= run_for(_from0, from0ToolSet, _context);
	result &= run_for(_from1, from1ToolSet, _context);
	result &= run_for(_to0, to0ToolSet, _context);
	result &= run_for(_to1, to1ToolSet, _context);

	if (_value.get_type() == type_id<float>())
	{
		_value.set<float>(remap_value(_value.get_as<float>(), _from0, _from1, _to0, _to1, clampValue));
	}
	else
	{
		error_processing_wmp_tool(this, TXT("expected float input value"));
		result = false;
	}

	return result;
}

bool WheresMyPoint::Remap::load_into(IO::XML::Node const* _node, tchar const* _name, REF_ float& _into0, REF_ float& _into1)
{
	if (auto* attr = _node->get_attribute(_name))
	{
		List<String> tokens;
		attr->get_as_string().split(String::comma(), tokens);
		if (tokens.get_size() == 1)
		{
			tokens.clear();
			attr->get_as_string().split(String(TXT("to")), tokens);
			if (tokens.get_size() == 1)
			{
				error_loading_xml(_node, TXT("could not load remap \"%S\""), _name);
				return false;
			}
		}
		if (tokens.get_size() == 2)
		{
			List<String>::Iterator iToken0 = tokens.begin();
			List<String>::Iterator iToken1 = iToken0; ++iToken1;
			_into0 = ParserUtils::parse_float(*iToken0, _into0);
			_into1 = ParserUtils::parse_float(*iToken1, _into1);
			return true;
		}
		else
		{
			error_loading_xml(_node, TXT("could not load remap \"%S\""), _name);
			return false;
		}
	}
	return true;
}

bool WheresMyPoint::Remap::load_into(IO::XML::Node const* _node, tchar const* _name, REF_ float& _into, REF_ ToolSet& _intoToolset)
{
	bool result = true;

	_into = _node->get_float_attribute(_name, _into);
	if (auto* node = _node->first_child_named(_name))
	{
		result &= _intoToolset.load_from_xml(node);
	}

	return result;
}

bool WheresMyPoint::Remap::run_for(REF_ float& _val, REF_ ToolSet const& _toolSet, Context& _context) const
{
	bool result = true;
	if (!_toolSet.is_empty())
	{
		Value valueToOp;
		if (_toolSet.update(valueToOp, _context))
		{
			if (valueToOp.is_set())
			{
				if (valueToOp.get_type() == type_id<float>())
				{
					_val = valueToOp.get_as<float>();
				}
				else
				{
					error_processing_wmp_tool(this, TXT("expected float value"));
					result = false;
				}
			}
			else
			{
				error_processing_wmp_tool(this, TXT("expected a value"));
				result = false;
			}
		}
		else
		{
			result = false;
		}
	}
	return result;
}