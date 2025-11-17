#include "mggc_not.h"

#include "..\meshGenGenerationContext.h"

#include "..\..\..\core\io\xml.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace GenerationConditions;

//

Not::~Not()
{
	delete_and_clear(condition);
}

bool Not::check(ElementInstance & _instance) const
{
	if (! condition)
	{
		return true;
	}
	
	return !condition->check(_instance);
}

bool Not::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	if (IO::XML::Node const * node = _node->first_child())
	{
		delete_and_clear(condition);
		condition = ICondition::load_condition_from_xml(node);
		if (! condition)
		{
			result = false;
		}
	}

	return result;
}

