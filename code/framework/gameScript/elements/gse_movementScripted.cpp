#include "gse_movementScripted.h"

#include "..\gameScript.h"

#include "..\..\game\game.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\..\core\io\xml.h"
#include "..\..\..\core\system\core.h"

#include "..\..\module\moduleController.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

// variable
DEFINE_STATIC_NAME(movementScriptedWait);

//

bool MovementScripted::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	wait = _node->get_bool_attribute_or_from_child_presence(TXT("wait"), wait);
	
	linearSpeedVar = _node->get_name_attribute(TXT("linearSpeedVar"), linearSpeedVar);

	stop = _node->get_bool_attribute_or_from_child_presence(TXT("stop"), stop);
	stopImmediately = _node->get_bool_attribute_or_from_child_presence(TXT("stopImmediately"), stopImmediately);

	movementMayBeInvalid = linearSpeedVar.is_valid();
	if (!stop && !stopImmediately)
	{
		movement.load_from_xml(_node, wait || movementMayBeInvalid);
	}

	return result;
}

bool MovementScripted::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type MovementScripted::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	ScriptExecutionResult::Type onRepeatResult = ScriptExecutionResult::Repeat;

	if (stop || stopImmediately)
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
		{
			if (auto* imo = exPtr->get())
			{
				Concurrency::ScopedSpinLock lock(imo->access_lock());
				if (auto* m = fast_cast<ModuleMovementScripted>(imo->get_movement()))
				{
					an_assert(stop || stopImmediately);
					m->stop(stopImmediately);
				}
			}
		}
	}
	else
	{
		if (movement.is_time_based())
		{
			float& waitVar = _execution.access_variables().access<float>(NAME(movementScriptedWait));

			if (is_flag_set(_flags, ScriptExecution::Entered))
			{
				waitVar = movement.time.get().max + 0.1f;
			}
			else
			{
				float deltaTime = ::System::Core::get_delta_time();
				waitVar -= deltaTime;
				if (waitVar <= 0.0f)
				{
					onRepeatResult = ScriptExecutionResult::Continue;

#ifndef BUILD_PUBLIC_RELEASE
					{
						static bool sentOnce = false;
						if (!sentOnce)
						{
							error_dev_investigate(TXT("movement scripted should end by now but it hasn't? force we ended"));
							sentOnce = true;
							String shortText = String::printf(TXT("movement scripted should end by now"));
							String moreInfo;
							{
								moreInfo += TXT("movement scripted should end by now\n\n");
								moreInfo += TXT("but it hasn't?\n");
								moreInfo += TXT("force we ended\n");
								if (_execution.get_script())
								{
									moreInfo += String::printf(TXT("%S [@i]\n"), _execution.get_script()->get_name().to_string().to_char(), _execution.get_at());
								}
							}
							if (auto* g = Game::get())
							{
								g->send_full_log_info_in_background(shortText, moreInfo);
							}
						}
					}
#endif
				}
			}
		}

		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
		{
			if (auto* imo = exPtr->get())
			{
				Concurrency::ScopedSpinLock lock(imo->access_lock());
				if (auto* m = fast_cast<ModuleMovementScripted>(imo->get_movement()))
				{
					if (!movement.is_valid() && !movementMayBeInvalid)
					{
						if (m->is_at_destination())
						{
							return ScriptExecutionResult::Continue;
						}
						return wait ? onRepeatResult : ScriptExecutionResult::Continue;
					}
					else
					{
						if (is_flag_set(_flags, ScriptExecution::Flag::Entered))
						{
							ScriptedMovement movementCopy = movement;
							if (linearSpeedVar.is_valid())
							{
								if (auto* lsVar = _execution.get_variables().get_existing<float>(linearSpeedVar))
								{
									movementCopy.linearSpeed = *lsVar;
								}
							}
							m->do_movement(movementCopy);
						}
						else
						{
							if (m->is_at_destination())
							{
								return ScriptExecutionResult::Continue;
							}
						}
						return wait? onRepeatResult : ScriptExecutionResult::Continue;
					}
				}
			}
		}
	}

	return ScriptExecutionResult::Continue;
}
