#include "wmp_customTool.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"

//

using namespace WheresMyPoint;

//

bool CustomTool::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);
	for_every(node, _node->children_named(TXT("inputParameter")))
	{
		Parameter parameter;
		if (auto * attr = node->get_attribute(TXT("from")))
		{
			parameter.store = Parameter::Store::Normal;
			parameter.valueNameInStore = attr->get_as_name();
		}
		if (auto * attr = node->get_attribute(TXT("fromGlobal")))
		{
			parameter.store = Parameter::Store::Global;
			parameter.valueNameInStore = attr->get_as_name();
		}
		if (auto * attr = node->get_attribute(TXT("fromTemp")))
		{
			parameter.store = Parameter::Store::Temp;
			parameter.valueNameInStore = attr->get_as_name();
		}
		parameter.parameterName = node->get_name_attribute(TXT("name"));
		if (!parameter.parameterName.is_valid())
		{
			error_loading_xml(node, TXT("no name for paramter"));
			result = false;
			continue;
		}
		if (!parameter.valueNameInStore.is_valid())
		{
			error_loading_xml(node, TXT("no value defined form which to restore for paramter"));
			result = false;
			continue;
		}
		inputParameters.push_back(parameter);
	}
	for_every(node, _node->children_named(TXT("outputParameter")))
	{
		Parameter parameter;
		if (auto * attr = node->get_attribute(TXT("as")))
		{
			parameter.store = Parameter::Store::Normal;
			parameter.valueNameInStore = attr->get_as_name();
		}
		if (auto * attr = node->get_attribute(TXT("asGlobal")))
		{
			parameter.store = Parameter::Store::Global;
			parameter.valueNameInStore = attr->get_as_name();
		}
		if (auto * attr = node->get_attribute(TXT("asTemp")))
		{
			parameter.store = Parameter::Store::Temp;
			parameter.valueNameInStore = attr->get_as_name();
		}
		parameter.parameterName = node->get_name_attribute(TXT("name"));
		if (!parameter.parameterName.is_valid())
		{
			error_loading_xml(node, TXT("no name for paramter"));
			result = false;
			continue;
		}
		if (!parameter.valueNameInStore.is_valid())
		{
			error_loading_xml(node, TXT("no value defined form which to restore for paramter"));
			result = false;
			continue;
		}
		outputParameters.push_back(parameter);
	}
	customToolName = Name(_node->get_name()); // has to be valid, it's node name, isn't it?
	return true;
}

bool CustomTool::update(REF_ Value& _value, Context& _context) const
{
	if (!customTool)
	{
		customTool = RegisteredTools::find_custom(customToolName);
	}
	if (!customTool)
	{
		error_processing_wmp_tool(this, TXT("custom tool \"%S\" missing"), customToolName.to_char());
		return false;
	}
	return update_using(_value, _context, customTool);
}

bool CustomTool::update_using(REF_ Value& _value, Context& _context, RegisteredCustomTool const* _customTool) const
{
	_context.push_temp_stack();
	for_every(requiredInputParameter, _customTool->inputParameters)
	{
		Parameter const * inputParameter = nullptr;
		for_every(findInputParameter, inputParameters)
		{
			if (findInputParameter->parameterName == requiredInputParameter->name)
			{
				inputParameter = findInputParameter;
			}
		}

		if (!inputParameter)
		{
			error_processing_wmp(TXT("input paramter \"%S\" for \"%S\" missing"), requiredInputParameter->name.to_char(), _customTool->name.to_char());
			return false;
		}

		{
			Value value;
			if (inputParameter->store == Parameter::Store::Global)
			{
				if (!_context.get_owner()->restore_global_value_for_wheres_my_point(inputParameter->valueNameInStore, value))
				{
					error_processing_wmp(TXT("could not restore input parameter global variable \"%S\""), inputParameter->valueNameInStore.to_char());
					return false;
				}
			}
			if (inputParameter->store == Parameter::Store::Normal)
			{
				if (!_context.get_owner()->restore_value_for_wheres_my_point(inputParameter->valueNameInStore, value))
				{
					error_processing_wmp(TXT("could not restore input parameter variable \"%S\""), inputParameter->valueNameInStore.to_char());
					return false;
				}
			}
			if (inputParameter->store == Parameter::Store::Temp)
			{
				if (auto const * val = _context.get_temp_value(inputParameter->valueNameInStore))
				{
					value = *val;
				}
				else
				{
					error_processing_wmp(TXT("could not restore input parameter temp variable \"%S\""), inputParameter->valueNameInStore.to_char());
					return false;
				}
			}
			if (value.get_type() != requiredInputParameter->type)
			{
				error_processing_wmp(TXT("type mismatch for input parameter \"%S\": %S v %S"), requiredInputParameter->name.to_char(), RegisteredType::get_name_of(requiredInputParameter->type), RegisteredType::get_name_of(value.get_type()));
				return false;
			}
			_context.set_temp_value(requiredInputParameter->name, value);
		}
	}
	bool result = _customTool->tools.update(_value, _context);
	for_every(requiredOutputParameter, _customTool->outputParameters)
	{
		Parameter const * outputParameter = nullptr;
		for_every(findOutputParameter, outputParameters)
		{
			if (findOutputParameter->parameterName == requiredOutputParameter->name)
			{
				outputParameter = findOutputParameter;
			}
		}

		if (outputParameter)
		{
			Value value;
			if (auto const * val = _context.get_temp_value(requiredOutputParameter->name))
			{
				value = *val;
			}
			else
			{
				error_processing_wmp(TXT("could not restore output parameter temp variable \"%S\""), requiredOutputParameter->name.to_char());
				return false;
			}

			if (value.get_type() != requiredOutputParameter->type)
			{
				error_processing_wmp(TXT("type mismatch for output parameter \"%S\": %S v %S"), requiredOutputParameter->name.to_char(), RegisteredType::get_name_of(requiredOutputParameter->type), RegisteredType::get_name_of(value.get_type()));
				return false;
			}

			if (outputParameter->store == Parameter::Store::Global)
			{
				_context.get_owner()->store_global_value_for_wheres_my_point(outputParameter->valueNameInStore, value);
			}
			if (outputParameter->store == Parameter::Store::Normal)
			{
				_context.get_owner()->store_value_for_wheres_my_point(outputParameter->valueNameInStore, value);
			}
			if (outputParameter->store == Parameter::Store::Temp)
			{
				_context.keep_temp_before_popping(outputParameter->valueNameInStore);
			}
		}
	}
	_context.pop_temp_stack();
	return result;
}

//

bool CallCustomTool::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	customToolName = _node->get_name_attribute(TXT("name"), Name::invalid());
	
	return result;
}

bool CallCustomTool::update(REF_ Value& _value, Context& _context) const
{
	customTool = nullptr;

	Name useCustomToolName = customToolName;

	Value valueToOp = _value;
	if (toolSet.update(valueToOp, _context))
	{
		// all good (or nothing happened)
	}
	else
	{
		return false;
	}

	if (valueToOp.is_set())
	{
		if (valueToOp.get_type() == type_id<Name>())
		{
			useCustomToolName = valueToOp.get_as<Name>();
		}
		else
		{
			error_processing_wmp_tool(this, TXT("expected Name value"));
			return false;
		}
	}

	auto* foundCustomTool = RegisteredTools::find_custom(useCustomToolName);
	if (!foundCustomTool)
	{
		error_processing_wmp_tool(this, TXT("custom tool \"%S\" missing"), useCustomToolName.to_char());
		return false;
	}

	return update_using(_value, _context, foundCustomTool);
}


