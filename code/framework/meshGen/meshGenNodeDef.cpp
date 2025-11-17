#include "meshGenNodeDef.h"

#include "meshGenGenerationContext.h"
#include "meshGenUtils.h"
#include "meshGenElement.h"

#include "..\..\core\other\simpleVariableStorage.h"

using namespace Framework;
using namespace MeshGeneration;

bool NodeDef::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	for_every(node, _node->all_children())
	{
		result &= sub_load_from_xml(node);
	}

	return result;
}

bool NodeDef::sub_load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	if (_node->get_name() == TXT("autoSmoothNormalDotLimit"))
	{
		autoSmoothNormalDotLimit = _node->get_float();
	}
	else if (_node->get_name() == TXT("autoSmoothNormalAngleDiffLimit"))
	{
		autoSmoothNormalDotLimit = cos_deg(_node->get_float());
	}
	else if (_node->get_name() == TXT("alwaysSmoothNormal"))
	{
		autoSmoothNormalDotLimit = -2.0f;
	}
	else if (_node->get_name() == TXT("dontSmoothNormal"))
	{
		autoSmoothNormalDotLimit = 2.0f;
	}

	return result;
}
