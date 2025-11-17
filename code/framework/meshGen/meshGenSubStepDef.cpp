#include "meshGenSubStepDef.h"

#include "meshGenConfig.h"
#include "meshGenGenerationContext.h"
#include "meshGenUtils.h"
#include "meshGenElement.h"
#include "meshGenValueDefImpl.inl"

#include "..\game\game.h"

#include "..\..\core\other\simpleVariableStorage.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;

//

DEFINE_STATIC_NAME(subStepIgnoreLinear);
DEFINE_STATIC_NAME(subStepCount);
DEFINE_STATIC_NAME(subStepMinCount);
DEFINE_STATIC_NAME(subStepMaxCount);
DEFINE_STATIC_NAME(subStepDivider);
DEFINE_STATIC_NAME(subStepLength);
DEFINE_STATIC_NAME(subStepDividerLessDetails);
DEFINE_STATIC_NAME(subStepLengthLessDetails);
DEFINE_STATIC_NAME(maxSubStepDivider);
DEFINE_STATIC_NAME(maxSubStepLength);
DEFINE_STATIC_NAME(evenNumberOfSubSteps);

//

#define FOR_PARAM_FROM_STORAGE(storage, type, name) \
	if (type const * name = storage.get_existing<type>(NAME(name)))

int SubStepDef::calculate_sub_step_count_for(float _length, GenerationContext const & _context, Optional<float> const& _details, Optional<bool> const& _ignoreAutoModifiers) const
{
	float details = _details.get(1.0f);
	bool ignoreAutoModifiers = _ignoreAutoModifiers.get(false);
	bool moreDetails = true;
	if (_context.does_force_more_details())
	{
		moreDetails = true;
		ignoreAutoModifiers = true;
	}
	else if (auto * g = Game::get())
	{
		moreDetails = g->should_generate_more_details();
	}
	if (! moreDetails)
	{
		if (details > 1.0f && !ignoreAutoModifiers)
		{
			details = 1.0f + 0.25f * (details - 1.0f); // less details
		}
	}
	Optional<int> result;
	if (count.is_set())
	{
		result = count.get(_context);
		if (details != 1.0f)
		{
			result = (int)((float)result.get() * details);
		}
	}

	ValueDef<float> const& useDivider = moreDetails || !dividerLessDetails.is_set() ? divider : dividerLessDetails;

	if (!result.is_set() && useDivider.is_set())
	{
		bool ignoreLODDetailsNow = ignoreLODDetails.get_with_default(_context, false);
		float dividerValue = useDivider.get(_context);
		if (!moreDetails && dividerCoefLessDetails.is_set() && !ignoreLODDetailsNow)
		{
			dividerValue *= dividerCoefLessDetails.get(_context);
		}
		if (dividerValue > 0.0f)
		{
			if (!ignoreAutoModifiers && !ignoreLODDetailsNow)
			{
				if (auto * g = Game::get())
				{
					float modifyDividerValue = Config::get().get_sub_step_def_modify_divider(max(g->get_generate_at_lod(), _context.get_for_lod()), _context.get_use_lod_fully(), _context.get_1_as_sub_step_base());
					dividerValue *= modifyDividerValue;
				}
			}
			dividerValue /= details;
#ifdef AN_ALLOW_BULLSHOTS
			if (BullshotSystem::is_active() && !ignoreLODDetailsNow)
			{
				dividerValue /= BullshotSystem::get_sub_step_details();
			}
#endif
			if (maxDivider.is_set())
			{
				float maxDividerValue = maxDivider.get(_context);
				if (maxDividerValue > 0.0f)
				{
					dividerValue = min(dividerValue, maxDividerValue);
				}
			}
			result = max(1, (int)round(_length / dividerValue));
		}
	}

	if (!result.is_set())
	{
		todo_note(TXT("this could be extended by type of object etc, here _context would be used"));
		result = max(1, (int)(_length / 0.1f));
	}

	if (minCount.is_set())
	{
		result = max(minCount.get(_context), result.get());
	}
	if (maxCount.is_set())
	{
		result = min(maxCount.get(_context), result.get());
	}
	if (evenNumber.is_set() && evenNumber.get(_context))
	{
		if ((result.get() % 2) == 1)
		{
			result = result.get() + 1;
		}
	}
	return max(1, result.get());
}

bool SubStepDef::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	if (_node)
	{
		subDivideLinear.load_from_xml(_node, TXT("subDivideLinear"));
		evenNumber.load_from_xml(_node, TXT("evenNumber"));
		ignoreLODDetails.load_from_xml(_node, TXT("ignoreLOD"));
		ignoreLODDetails.load_from_xml(_node, TXT("ignoreDetails"));
		ignoreLODDetails.load_from_xml(_node, TXT("ignoreLODDetails"));
		count.load_from_xml(_node, TXT("count"));
		minCount.load_from_xml(_node, TXT("minCount"));
		maxCount.load_from_xml(_node, TXT("maxCount"));
		divider.load_from_xml(_node, TXT("divider"));
		divider.load_from_xml(_node, TXT("length"));
		divider.load_from_xml(_node, TXT("step"));
		divider.load_from_xml(_node, TXT("stepLength"));
		maxDivider.load_from_xml(_node, TXT("maxDivider"));
		maxDivider.load_from_xml(_node, TXT("maxLength"));
		maxDivider.load_from_xml(_node, TXT("maxStep"));
		maxDivider.load_from_xml(_node, TXT("maxStepLength"));
		dividerLessDetails.load_from_xml(_node, TXT("dividerLessDetails"));
		dividerLessDetails.load_from_xml(_node, TXT("lengthLessDetails"));
		dividerLessDetails.load_from_xml(_node, TXT("stepLessDetails"));
		dividerLessDetails.load_from_xml(_node, TXT("stepLengthLessDetails"));
		dividerCoefLessDetails.load_from_xml(_node, TXT("dividerCoefLessDetails"));
		dividerCoefLessDetails.load_from_xml(_node, TXT("lengthCoefLessDetails"));
		dividerCoefLessDetails.load_from_xml(_node, TXT("stepCoefLessDetails"));
		dividerCoefLessDetails.load_from_xml(_node, TXT("stepLengthCoefLessDetails"));
	}

	return result;
}

bool SubStepDef::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * const _subStepChildName)
{
	return load_from_xml(_node->first_child_named(_subStepChildName));
}

void SubStepDef::load_from_variable_storage(SimpleVariableStorage const & _storage)
{
	FOR_PARAM_FROM_STORAGE(_storage, bool, subStepIgnoreLinear)
	{
		subDivideLinear = *subStepIgnoreLinear;
	}
	FOR_PARAM_FROM_STORAGE(_storage, int, subStepCount)
	{
		count = *subStepCount;
	}
	FOR_PARAM_FROM_STORAGE(_storage, bool, evenNumberOfSubSteps)
	{
		evenNumber = *evenNumberOfSubSteps;
	}
	FOR_PARAM_FROM_STORAGE(_storage, int, subStepMinCount)
	{
		minCount = *subStepMinCount;
	}
	FOR_PARAM_FROM_STORAGE(_storage, int, subStepMaxCount)
	{
		maxCount = *subStepMaxCount;
	}
	FOR_PARAM_FROM_STORAGE(_storage, float, subStepDivider)
	{
		divider = *subStepDivider;
	}
	FOR_PARAM_FROM_STORAGE(_storage, float, maxSubStepDivider)
	{
		maxDivider = *maxSubStepDivider;
	}
}

void SubStepDef::load_from_context(GenerationContext const & _context)
{
	FOR_PARAM(_context, bool, subStepIgnoreLinear)
	{
		subDivideLinear = *subStepIgnoreLinear;
	}
	FOR_PARAM(_context, int, subStepCount)
	{
		count = *subStepCount;
	}
	FOR_PARAM(_context, bool, evenNumberOfSubSteps)
	{
		evenNumber = *evenNumberOfSubSteps;
	}
	FOR_PARAM(_context, int, subStepMinCount)
	{
		minCount = *subStepMinCount;
	}
	FOR_PARAM(_context, int, subStepMaxCount)
	{
		maxCount = *subStepMaxCount;
	}
	FOR_PARAM(_context, float, subStepDivider)
	{
		divider = *subStepDivider;
	}
	FOR_PARAM(_context, float, subStepLength)
	{
		divider = *subStepLength;
	}
	FOR_PARAM(_context, float, maxSubStepDivider)
	{
		maxDivider = *maxSubStepDivider;
	}
	FOR_PARAM(_context, float, maxSubStepLength)
	{
		maxDivider = *maxSubStepLength;
	}
	bool moreDetails = true;
	if (_context.does_force_more_details())
	{
		moreDetails = true;
	}
	else if (auto * g = Game::get())
	{
		moreDetails = g->should_generate_more_details();
	}
	if (!moreDetails)
	{
		FOR_PARAM(_context, float, subStepDividerLessDetails)
		{
			divider = *subStepDividerLessDetails;
		}
		FOR_PARAM(_context, float, subStepLengthLessDetails)
		{
			divider = *subStepLengthLessDetails;
		}
	}
}

