#include "aiMindInstance.h"

#include "..\library\usedLibraryStored.inl"

#include "..\module\moduleController.h"
#include "..\modulesOwner\modulesOwner.h"

#include "aiLogic.h"
#include "aiLogics.h"

#include "aiSocial.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AI;

//

MindInstance::MindInstance(IAIMindOwner* _owner)
: owner(_owner)
, ownerAsModulesOwner(fast_cast<IModulesOwner>(owner))
, execution(this)
, locomotion(this)
, perception(this)
, navigation(this)
, social(nullptr)
{
	social = new Social(this); // to have anything
}

MindInstance::~MindInstance()
{
	use_mind(nullptr);
	delete_and_clear(social);
}

void MindInstance::use_mind(Mind * _mind)
{
	if (mind.get() == _mind)
	{
		return;
	}

	if (mind.get())
	{
		todo_note(TXT("remove currently used mind"));
		end_logic();
	}
	delete_and_clear(social);
	execution.reset();
	locomotion.reset();
	mind.set(_mind);
	social = new Social(this);
	if (Mind* usedMind = mind.get())
	{
		navFlags = usedMind->get_nav_flags();
		Name const & logicName = usedMind->get_logic_name();
		if (logicName.is_valid())
		{
			use_logic(Logics::create(logicName, this, usedMind->get_logic_data()));
			if (auto* imo = get_owner_as_modules_owner())
			{
				if (imo->get_presence()) // naive and simple check to see if all modules exist, if not, learn_from will be called at the end of initialise_modules
				{
					learn_from(imo->access_variables());
				}
			}
		}
	}
}

void MindInstance::use_logic(Logic* _logic)
{
	if (logic == _logic)
	{
		return;
	}

	end_logic();

	logic = _logic;
}

void MindInstance::end_logic()
{
	if (logic)
	{
		endingLogic = true;
		logic->end();
		delete_and_clear(logic);
		endingLogic = false;
	}
}

void MindInstance::advance_logic(float _deltaTime)
{
	if (!mindEnabled)
	{
		return;
	}
	MEASURE_PERFORMANCE_CONTEXT(mind.get() ? mind->get_name().get_name().to_char() : TXT("--"));
	if (logic)
	{
		MEASURE_PERFORMANCE(mindInstance_logicAdvance);
		logic->advance(_deltaTime);
	}
	{
		MEASURE_PERFORMANCE(mindInstance_execution);
		execution.advance(_deltaTime);
	}
}

void MindInstance::set_enabled(bool _enabled)
{
	mindEnabled = _enabled;
	if (mind.get() && mind->does_use_locomotion())
	{
		if (mindEnabled)
		{
			// restore to "no control"
			locomotion.reset();
		}
		else
		{
			auto* imo = get_owner_as_modules_owner();
			LocomotionState::apply(mind->get_locomotion_when_mind_disabled(), locomotion, imo ? imo->get_controller() : nullptr);
		}
	}
}

void MindInstance::advance_locomotion(float _deltaTime)
{
	if (! mind.get() || mind->does_use_locomotion())
	{
		locomotion.advance(_deltaTime);
	}
}

void MindInstance::advance_perception()
{
	if (!mindEnabled)
	{
		return;
	}
	perception.advance();
}

void MindInstance::learn_from(SimpleVariableStorage & _parameters)
{
	if (logic)
	{
		logic->learn_from(_parameters);
	}
}

void MindInstance::ready_to_activate()
{
	if (logic)
	{
		logic->ready_to_activate();
	}
}
