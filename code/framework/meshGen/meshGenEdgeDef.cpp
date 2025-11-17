#include "meshGenEdgeDef.h"

#include "meshGenGenerationContext.h"
#include "meshGenUtils.h"
#include "meshGenElement.h"

#include "..\..\core\other\simpleVariableStorage.h"

using namespace Framework;
using namespace MeshGeneration;

bool EdgeDef::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	for_every(node, _node->all_children())
	{
		result &= sub_load_from_xml(node);
	}

	return result;
}

bool EdgeDef::sub_load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	if (_node->get_name() == TXT("subStep"))
	{
		subStep = SubStepDef();
		result &= subStep.load_from_xml(_node);
	}
	else if (_node->get_name() == TXT("allowEvenSeparation"))
	{
		warn_loading_xml(_node, TXT("use allowRoundSeparation instead of allowEvenSeparation"));
		useRoundSeparation = Allowance::Allow;
	}
	else if (_node->get_name() == TXT("allowRoundSeparation"))
	{
		useRoundSeparation = Allowance::Allow;
	}
	else if (_node->get_name() == TXT("disallowEvenSeparation"))
	{
		warn_loading_xml(_node, TXT("use disallowRoundSeparation instead of disallowEvenSeparation"));
		useRoundSeparation = Allowance::Disallow;
	}
	else if (_node->get_name() == TXT("disallowRoundSeparation"))
	{
		useRoundSeparation = Allowance::Disallow;
	}
	else if (_node->get_name() == TXT("forceEvenSeparation"))
	{
		warn_loading_xml(_node, TXT("use forceRoundSeparation instead of forceEvenSeparation"));
		useRoundSeparation = Allowance::Force;
	}
	else if (_node->get_name() == TXT("forceRoundSeparation"))
	{
		useRoundSeparation = Allowance::Force;
	}
	else if (_node->get_name() == TXT("autoSmoothNormalDotLimit"))
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
	else if (_node->get_name() == TXT("autoSmooth"))
	{
		if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("surfaceGroup")))
		{
			autoSmoothSurfaceGroup = attr->get_as_name();
		}
		if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("normalDotLimit")))
		{
			autoSmoothNormalDotLimit = attr->get_as_float();
		}
		if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("normalAngleDiffLimit")))
		{
			autoSmoothNormalDotLimit = cos_deg(attr->get_as_float());
		}
	}

	return result;
}
