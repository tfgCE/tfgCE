#include "mggc_systemTags.h"

#include "..\..\..\core\system\core.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace GenerationConditions;

//

bool SystemTags::check(ElementInstance & _instance) const
{
	return condition.check(::System::Core::get_system_tags());

	return false;
}

bool SystemTags::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	condition.load_from_string(_node->get_text());

	return result;
}
