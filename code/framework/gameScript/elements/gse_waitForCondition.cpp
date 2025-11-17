#include "gse_waitForCondition.h"

#include "..\..\module\modulePresence.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\..\core\system\core.h"

#include "gse_label.h"

#include "..\gameScript.h"
#include "..\registeredGameScriptConditions.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(checkIntervalLeft);
DEFINE_STATIC_NAME(timeWaitingForCondition);
DEFINE_STATIC_NAME(waitForCondition);

//

bool WaitForCondition::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	if (auto* condNode = _node->first_child())
	{
		ifCondition = RegisteredScriptConditions::create(condNode->get_name().to_char());
		if (ifCondition.is_set())
		{
			result &= ifCondition->load_from_xml(condNode, _lc);
		}
	}

	timeLimit.load_from_xml(_node, TXT("timeLimit"));
	goToLabelOnTimeLimit = _node->get_name_attribute_or_from_child(TXT("goToLabelOnTimeLimit"), goToLabelOnTimeLimit);

	checkInterval.load_from_xml(_node, TXT("interval"));
	if (_node->has_attribute(TXT("checkInterval")))
	{
		checkInterval.load_from_xml(_node, TXT("checkInterval"));
		warn_loading_xml_dev_ignore(_node, TXT("use \"interval\" not \"checkInterval\""));
	}

	return result;
}

bool WaitForCondition::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (ifCondition.is_set())
	{
		result &= ifCondition->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

ScriptExecutionResult::Type WaitForCondition::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (is_flag_set(_flags, ScriptExecution::Entered))
	{
		_execution.access_variables().access<float>(NAME(checkIntervalLeft)) = 0.0f;
		_execution.access_variables().access<float>(NAME(timeWaitingForCondition)) = 0.0f;
		_execution.access_variables().access<float>(NAME(waitForCondition)) = 0.0f;
	}

	float deltaTime = ::System::Core::get_delta_time();

	_execution.access_variables().access<float>(NAME(checkIntervalLeft)) -= deltaTime;
	_execution.access_variables().access<float>(NAME(timeWaitingForCondition)) += deltaTime;

	if (checkInterval.is_set())
	{
		bool doNow = false;

		if (_execution.get_variables().get_value<float>(NAME(checkIntervalLeft), 0.0f) < 0.0f)
		{
			_execution.access_variables().access<float>(NAME(checkIntervalLeft)) = Random::get(checkInterval.get());
			doNow = true;
		}
		if (! doNow && timeLimit.is_set())
		{
			if (_execution.get_variables().get_value<float>(NAME(timeWaitingForCondition), 0.0f) >= timeLimit.get())
			{
				doNow = true;
			}
		}
		if (!doNow)
		{
			return ScriptExecutionResult::Repeat;
		}
	}

	if (ifCondition.is_set())
	{
		if (ifCondition->check(_execution))
		{
			return ScriptExecutionResult::Continue;
		}
	}

	if (timeLimit.is_set())
	{
		if (_execution.get_variables().get_value<float>(NAME(timeWaitingForCondition), 0.0f) >= timeLimit.get())
		{
			if (goToLabelOnTimeLimit.is_valid())
			{
				int labelAt = NONE;
				if (auto* script = _execution.get_script())
				{
					for_every_ref(e, script->get_elements())
					{
						if (auto* label = fast_cast<Label>(e))
						{
							if (label->get_id() == goToLabelOnTimeLimit)
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
					error(TXT("did not find label \"%S\""), goToLabelOnTimeLimit.to_char());
				}

				return ScriptExecutionResult::SetNextInstruction;
			}
			else
			{
				return ScriptExecutionResult::Continue;
			}
		}
	}

	return ScriptExecutionResult::Repeat;
}

void WaitForCondition::interrupted(ScriptExecution& _execution) const
{
	base::interrupted(_execution);
}

