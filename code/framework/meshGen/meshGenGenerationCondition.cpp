#include "meshGenGenerationCondition.h"

#include "generationConditions\mggc_and.h"
#include "generationConditions\mggc_allowMoreDetails.h"
#include "generationConditions\mggc_bigEnoughForLOD.h"
#include "generationConditions\mggc_bullshotSystem.h"
#include "generationConditions\mggc_dev.h"
#include "generationConditions\mggc_gameTags.h"
#include "generationConditions\mggc_lod.h"
#include "generationConditions\mggc_not.h"
#include "generationConditions\mggc_or.h"
#include "generationConditions\mggc_parameter.h"
#include "generationConditions\mggc_previewGame.h"
#include "generationConditions\mggc_slidingLocomotion.h"
#include "generationConditions\mggc_systemTags.h"
#include "generationConditions\mggc_videoOptions.h"
#include "generationConditions\mggc_wheresMyPoint.h"

#include "..\..\core\io\xml.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;

//

ICondition * ICondition::create_condition(String const & _name)
{
	if (_name == TXT("and"))				{ return new GenerationConditions::And(); }
	if (_name == TXT("allowMoreDetails"))	{ return new GenerationConditions::AllowMoreDetails(); }
	if (_name == TXT("bigEnoughForLOD"))	{ return new GenerationConditions::BigEnoughForLOD(); }
	if (_name == TXT("bullshots"))			{ return new GenerationConditions::BullshotSystem(); }
	if (_name == TXT("bullshotSystem"))		{ return new GenerationConditions::BullshotSystem(); }
	if (_name == TXT("dev"))				{ return new GenerationConditions::Dev(); }
	if (_name == TXT("gameTags"))			{ return new GenerationConditions::GameTags(); }
	if (_name == TXT("lod"))				{ return new GenerationConditions::LOD(); }
	if (_name == TXT("not"))				{ return new GenerationConditions::Not(); }
	if (_name == TXT("or"))					{ return new GenerationConditions::Or(); }
	if (_name == TXT("parameter"))			{ return new GenerationConditions::Parameter(); }
	if (_name == TXT("parameterGlobal"))	{ return new GenerationConditions::ParameterGlobal(); }
	if (_name == TXT("previewGame"))		{ return new GenerationConditions::PreviewGame(); }
	if (_name == TXT("slidingLocomotion"))	{ return new GenerationConditions::SlidingLocomotion(); }
	if (_name == TXT("systemTags"))			{ return new GenerationConditions::SystemTags(); }
	if (_name == TXT("videoOptions"))		{ return new GenerationConditions::VideoOptions(); }
	if (_name == TXT("wheresMyPoint"))		{ return new GenerationConditions::WheresMyPoint(); }

	return nullptr;
}

ICondition * ICondition::load_condition_from_xml(IO::XML::Node const * _node)
{
	if (ICondition* condition = ICondition::create_condition(_node->get_name()))
	{
		if (condition->load_from_xml(_node))
		{
			return condition;
		}
		else
		{
			error_loading_xml(_node, TXT("could not load generation condition"));
			return nullptr;
		}
	}
	else
	{
		error_loading_xml(_node, TXT("could not create generation condition \"%S\""), _node->get_name().to_char());
		return nullptr;
	}
}

//

GenerationCondition::~GenerationCondition()
{
	for_every(condition, conditions)
	{
		delete_and_clear(*condition);
	}
}

bool GenerationCondition::check(ElementInstance & _instance) const
{
	bool result = true;

	for_every_ptr(condition, conditions)
	{
		result &= condition->check(_instance);
	}

	return result;
}

bool GenerationCondition::load_from_xml(IO::XML::Node const * _node, tchar const * _childName)
{
	if (!_node)
	{
		return true;
	}

	bool result = true;
	if (IO::XML::Node const * node = _childName ? _node->first_child_named(_childName) : _node)
	{
		for_every(child, node->all_children())
		{
			if (ICondition* condition = ICondition::load_condition_from_xml(child))
			{
				conditions.push_back(condition);
			}
			else
			{
				delete condition;
				result = false;
			}
		}
	}

	return result;
}
