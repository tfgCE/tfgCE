#include "layoutSocket.h"

//

using namespace Framework;

//

bool LayoutSocket::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;
			
	name = _node->get_name_attribute(TXT("name"), name);

	placement.load_from_xml_child_node(_node, TXT("placement"));

	return result;
}
