#include "meshGenConfig.h"

#include "..\..\core\math\math.h"

//

using namespace Framework;
using namespace MeshGeneration;

//

Config* Config::s_config = nullptr;

void Config::initialise_static()
{
	an_assert(!s_config);
	s_config = new Config();
}

void Config::close_static()
{
	an_assert(s_config);
	delete_and_clear(s_config);
}

Config::Config()
{
	SET_EXTRA_DEBUG_INFO(subStepDefModifyDividers, TXT("MeshGeneration::Config.subStepDefModifyDividers"));
	SET_EXTRA_DEBUG_INFO(minSizes, TXT("MeshGeneration::Config.minSizes"));

	subStepDefModifyDividers.push_back(1.0f);
	subStepDefModifyDividers.push_back(2.0f);
	subStepDefModifyDividers.push_back(4.0f);
	subStepDefModifyDividers.push_back(8.0f);
	subStepDefModifyDividers.push_back(16.0f);
	subStepDefModifyDividers.push_back(32.0f);

	minSizes.push_back(0.0f);
	minSizes.push_back(0.05f);
	minSizes.push_back(0.1f);
	minSizes.push_back(0.2f);

	smoothEverythingAtLOD = 3;
	lodIndexMultiplier = 1;
}

float Config::get_sub_step_def_modify_divider(int _lod, float _useFully, float _use1AsBase) const
{
	if (subStepDefModifyDividers.is_empty())
	{
		return 1.0f;
	}
	else
	{
		float firstModifier = subStepDefModifyDividers[0];
		float modifier = subStepDefModifyDividers[clamp(_lod, 0, subStepDefModifyDividers.get_size() - 1)];
		float result = firstModifier + (modifier - firstModifier) * _useFully;
		if (_use1AsBase != 0.0f)
		{
			float properFirstModifier = firstModifier * (1.0f - _use1AsBase) + 1.0f * _use1AsBase;
			result *= properFirstModifier / firstModifier;
		}
		an_assert(_lod == 0 || get_sub_step_def_modify_divider(_lod - 1, _useFully, _use1AsBase) < result);
		return result;
	}
}

float Config::get_min_size(int _lod, float _useFully) const
{
	if (minSizes.is_empty())
	{
		return 0.0f;
	}
	else
	{
		float minSize = minSizes[clamp(_lod, 0, minSizes.get_size() - 1)];
		return minSize * _useFully;
	}
}

bool Config::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	for_every(meshGenNode, _node->children_named(TXT("meshGen")))
	{
		smoothEverythingAtLOD = meshGenNode->get_int_attribute_or_from_child(TXT("smoothEverythingAtLOD"), smoothEverythingAtLOD);
		lodIndexMultiplier = meshGenNode->get_int_attribute_or_from_child(TXT("lodIndexMultiplier"), lodIndexMultiplier);
		for_every(containerNode, meshGenNode->children_named(TXT("subStepDef")))
		{
			subStepDefModifyDividers.clear();
			for_every(valueNode, containerNode->children_named(TXT("modifyDivider")))
			{
				if (valueNode->has_attribute(TXT("value")))
				{
					subStepDefModifyDividers.push_back(valueNode->get_float_attribute(TXT("value"), 1.0f));
				}
			}
		}
		for_every(containerNode, meshGenNode->children_named(TXT("minSizes")))
		{
			minSizes.clear();
			for_every(valueNode, containerNode->children_named(TXT("minSize")))
			{
				if (valueNode->has_attribute(TXT("value")))
				{
					minSizes.push_back(valueNode->get_float_attribute(TXT("value"), 1.0f));
				}
			}
		}
	}

	return result;
}

void Config::save_to_xml(IO::XML::Node* _node, bool _userSpecific) const
{
	if (!_userSpecific)
	{
		if (auto * meshGenNode = _node->add_node(TXT("meshGen")))
		{
			meshGenNode->set_int_to_child(TXT("smoothEverythingAtLOD"), smoothEverythingAtLOD);
			meshGenNode->set_int_to_child(TXT("lodIndexMultiplier"), lodIndexMultiplier);
			if (auto * containerNode = meshGenNode->add_node(TXT("subStepDef")))
			{
				for_every(modifyDivider, subStepDefModifyDividers)
				{
					if (auto * valueNode = containerNode->add_node(TXT("modifyDivider")))
					{
						valueNode->set_float_attribute(TXT("value"), *modifyDivider);
					}
				}
			}
			if (auto * containerNode = meshGenNode->add_node(TXT("minSizes")))
			{
				for_every(minSize, minSizes)
				{
					if (auto * valueNode = containerNode->add_node(TXT("minSize")))
					{
						valueNode->set_float_attribute(TXT("value"), *minSize);
					}
				}
			}
		}
	}
}
