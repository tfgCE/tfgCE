#include "wmp_todo.h"

#include "..\debug\logInfoContext.h"
#include "..\io\xml.h"

using namespace WheresMyPoint;

bool ToDo::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	info = _node->get_text();

	return result;
}

bool ToDo::update(REF_ Value & _value, Context & _context) const
{
	LogInfoContext log;
	RegisteredType::log_value(log, _value.get_type(), _value.get_raw(), info.to_char());
	log.output_to_output();
	todo_important(TXT("wheresMyPoint todo: %S"), info.to_char());
	return true;
}
