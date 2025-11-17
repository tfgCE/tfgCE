#include "aiMessageTemplate.h"

#include "..\..\core\io\xml.h"

using namespace Framework;
using namespace AI;

bool MessageTemplate::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	name = _node->get_name_attribute_or_from_child(TXT("name"), name);
	delay = _node->get_float_attribute_or_from_child(TXT("delay"), delay);
	cooldown.load_from_xml(_node, TXT("cooldown")); // don't get result as it means it was loaded or not
	playerOnly = _node->get_bool_attribute_or_from_child_presence(TXT("playerOnly"), playerOnly);
	
	return result;
}

//

void MessageTemplateCooldowns::add(MessageTemplate const & _message)
{
	if (! _message.cooldown.is_set() || _message.cooldown.get() <= 0.0f)
	{
		return;
	}
	base::add(_message.name, _message.cooldown.get());
}
