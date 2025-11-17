#include "gse_goToRandomLabel.h"

#include "gse_label.h"

#include "..\gameScript.h"
#include "..\registeredGameScriptConditions.h"

#include "..\..\..\core\io\xml.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

bool GoToRandomLabel::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	randomInt.load_from_xml(_node, TXT("randomValue"));
	for_every(n, _node->children_named(TXT("goTo")))
	{
		Choose ch;
		ch.value.load_from_xml(n, TXT("forValue"));
		ch.changeValueOnFailedCondition.load_from_xml(n, TXT("changeValueOnFailedCondition"));
		ch.id = n->get_name_attribute_or_from_child(TXT("labelId"), ch.id);
		ch.id = n->get_name_attribute_or_from_child(TXT("name"), ch.id);
		error_loading_xml_on_assert(ch.id.is_valid(), result, n, TXT("\"labelId\" for label required"));
		if (auto* node = n->first_child_named(TXT("if")))
		{
			if (auto* condNode = node->first_child())
			{
				ch.ifCondition = RegisteredScriptConditions::create(condNode->get_name().to_char());
				if (ch.ifCondition.is_set())
				{
					result &= ch.ifCondition->load_from_xml(condNode, _lc);
				}
			}
		}
		choose.push_back(ch);
	}

	return result;
}

bool GoToRandomLabel::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(ch, choose)
	{
		if (ch->ifCondition.is_set())
		{
			result &= ch->ifCondition->prepare_for_game(_library, _pfgContext);
		}
	}

	return result;
}

ScriptExecutionResult::Type GoToRandomLabel::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	int value = randomInt.get();
	Name goToLabel;
	bool tryAgain = true;
	int triesLeft = 100;
	while (tryAgain && triesLeft > 0)
	{
		--triesLeft;
		tryAgain = false;
		for_every(ch, choose)
		{
			if (!ch->value.is_set() ||
				(ch->value.is_set() && ch->value.get() == value))
			{
				bool ok = true;
				if (ch->ifCondition.is_set())
				{
					if (! ch->ifCondition->check(_execution))
					{
						ok = false;
						if (ch->changeValueOnFailedCondition.is_set())
						{
							value = ch->changeValueOnFailedCondition.get();
							tryAgain = true;
						}
					}
				}
				if (ok)
				{
					goToLabel = ch->id;
					tryAgain = false;
					break;
				}
			}
		}
	}

	if (goToLabel.is_valid())
	{
		int labelAt = NONE;
		if (auto* script = _execution.get_script())
		{
			for_every_ref(e, script->get_elements())
			{
				if (auto* label = fast_cast<Label>(e))
				{
					if (label->get_id() == goToLabel)
					{
						labelAt = for_everys_index(e);
						break;
					}
				}
			}
		}

		if (labelAt != NONE)
		{
			_execution.set_at(labelAt);
		}
		else
		{
			error(TXT("did not find label \"%S\""), goToLabel.to_char());
		}
		return ScriptExecutionResult::SetNextInstruction;
	}

	return ScriptExecutionResult::Continue;
}
