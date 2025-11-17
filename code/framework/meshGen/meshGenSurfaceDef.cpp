#include "meshGenSurfaceDef.h"

#include "meshGenGenerationContext.h"
#include "meshGenUtils.h"
#include "meshGenElement.h"

#include "..\..\core\other\simpleVariableStorage.h"

using namespace Framework;
using namespace MeshGeneration;

bool SurfaceDef::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	for_every(node, _node->all_children())
	{
		result &= sub_load_from_xml(node);
	}

	return result;
}

bool SurfaceDef::sub_load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	if (_node->get_name() == TXT("subStep"))
	{
		subStep = SubStepDef();
		result &= subStep.access().load_from_xml(_node);
	}
	else if (_node->get_name() == TXT("maxEdgeLength"))
	{
		maxEdgeLength = _node->get_float();
	}
	else if (_node->get_name() == TXT("addInnerGrid"))
	{
		addInnerGrid = true;
	}
	else if (_node->get_name() == TXT("dontAddInnerGrid"))
	{
		addInnerGrid = false;
	}
	else if (_node->get_name() == TXT("allowSimpleFlat"))
	{
		useSimpleFlat = Allowance::Allow;
	}
	else if (_node->get_name() == TXT("disallowSimpleFlat") ||
			 _node->get_name() == TXT("noSimpleFlat"))
	{
		useSimpleFlat = Allowance::Disallow;
	}
	else if (_node->get_name() == TXT("forceSimpleFlat"))
	{
		useSimpleFlat = Allowance::Force;
	}

	return result;
}
