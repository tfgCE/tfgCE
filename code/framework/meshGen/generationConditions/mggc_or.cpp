#include "mggc_or.h"

#include "..\meshGenGenerationContext.h"

#include "..\..\..\core\io\xml.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace GenerationConditions;

//

Or::~Or()
{
	for_every(condition, conditions)
	{
		delete_and_clear(*condition);
	}
}

bool Or::check(ElementInstance & _instance) const
{
	if (conditions.is_empty())
	{
		return true;
	}
	
	for_every_ptr(condition, conditions)
	{
		if (condition->check(_instance))
		{
			return true;
		}
	}

	return false;
}

bool Or::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	for_every(child, _node->all_children())
	{
		if (ICondition* condition = ICondition::load_condition_from_xml(child))
		{
			conditions.push_back(condition);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

