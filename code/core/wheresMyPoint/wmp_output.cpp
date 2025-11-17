#include "wmp_output.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\debug\logInfoContext.h"
#include "..\io\outputWriter.h"
#include "..\io\xml.h"

//

#define set_output_colour() output_colour(0, 1, 1, 1);

//

using namespace WheresMyPoint;

//

bool Output::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	info = _node->get_text();

	name = _node->get_name_attribute(TXT("from"), name);
	name = _node->get_name_attribute(TXT("var"), name);

	debug = _node->get_name_attribute(TXT("debug"), debug);

	asXML = _node->get_bool_attribute_or_from_child_presence(TXT("asXML"), asXML);

	return result;
}

bool Output::update(REF_ Value& _value, Context& _context) const
{
	bool result = true;
	LogInfoContext log;
	if (debug.is_valid())
	{
		if (debug == TXT("randomGenerator"))
		{
			ScopedOutputLock outputLock;
			set_output_colour();
			output(TXT("randomGenerator: %S"), _context.get_random_generator().get_seed_string().to_char());
			output_colour();
		}
	}
	else if (name.is_valid())
	{
		Value tempValue;
		if (_context.get_owner()->restore_value_for_wheres_my_point(name, tempValue))
		{
			if (asXML)
			{
				IO::XML::Document doc;
				auto* node = doc.get_root()->add_node(RegisteredType::get_name_of(tempValue.get_type()));
				node->set_attribute(TXT("name"), info.is_empty() ? name.to_char() : info.to_char());
				RegisteredType::save_to_xml(node, tempValue.get_type(), tempValue.get_raw());

				IO::OutputWriter ow;
				doc.save_xml(&ow, false, NP);
			}
			else
			{
				RegisteredType::log_value(log, tempValue.get_type(), tempValue.get_raw(), info.is_empty() ? name.to_char() : info.to_char());
			}
		}
		else
		{
			error_processing_wmp(TXT("could not restore owner variable \"%S\""), name.to_char());
			result = false;
		}
	}
	else
	{
		RegisteredType::log_value(log, _value.get_type(), _value.get_raw(), info.to_char());
	}
	{
		ScopedOutputLock outputLock;
		set_output_colour();
		log.output_to_output(false);
		output_colour();
	}
	return result;
}

void Output::log_variable(Context& _context) const
{
	if (name.is_valid())
	{
		Value tempValue;
		if (_context.get_owner()->restore_value_for_wheres_my_point(name, tempValue))
		{
			LogInfoContext log;
			log.log(TXT("var \"%S\""), name.to_char());
			RegisteredType::log_value(log, tempValue.get_type(), tempValue.get_raw(), name.to_char());
			{
				ScopedOutputLock outputLock;
				set_output_colour();
				log.output_to_output();
				output_colour();
			}
		}
	}
}

//

bool Log::update(REF_ Value& _value, Context& _context) const
{
	{
		ScopedOutputLock outputLock;
		set_output_colour();
		output(TXT("%S"), info.to_char());
		output_colour();
	}
	return true;
}

//

bool Warning::update(REF_ Value& _value, Context& _context) const
{
	warn_processing_wmp_tool(this, TXT("%S"), info.to_char());
	log_variable(_context);
	return true; // it's just a warning
}

//

bool Error::update(REF_ Value& _value, Context& _context) const
{
	error_processing_wmp_tool(this, TXT("%S"), info.to_char());
	log_variable(_context);
	return false;
}

