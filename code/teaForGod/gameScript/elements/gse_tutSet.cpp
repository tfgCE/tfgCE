#include "gse_tutSet.h"

#include "..\..\tutorials\tutorialSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool TutSet::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	if (auto* attr = _node->get_attribute(TXT("handDisplaysState")))
	{
		handDisplaysStateProvided = true;
		if (attr->get_as_string() == TXT("open"))
		{
			handDisplaysState = true;
		}
		else if (attr->get_as_string() == TXT("close"))
		{
			handDisplaysState = false;
		}
		else if (attr->get_as_string() == TXT("clear"))
		{
			handDisplaysState.clear();
		}
		else
		{
			warn_loading_xml(_node, TXT("\"handDisplaysState\" state not recognised, clearing"));
			handDisplaysState.clear();
		}
	}

	if (auto* attr = _node->get_attribute(TXT("energyTransfer")))
	{
		energyTransfer = Option::parse(attr->get_as_string().to_char(), Option::Allow, Option::Allow | Option::Block, &result);
	}

	if (auto* attr = _node->get_attribute(TXT("physicalViolence")))
	{
		physicalViolence = Option::parse(attr->get_as_string().to_char(), Option::Allow, Option::Allow | Option::Block, &result);
	}

	if (auto* attr = _node->get_attribute(TXT("pickupOrbs")))
	{
		pickupOrbs = Option::parse(attr->get_as_string().to_char(), Option::Allow, Option::Allow | Option::Block, &result);
	}

	if (auto* attr = _node->get_attribute(TXT("navigationTargetVar")))
	{
		navigationTargetVar = attr->get_as_name();
	}

	return result;
}

#define APPLY_PREFERENCE(what) if (what.is_set()) pp.what = what.get();

Framework::GameScript::ScriptExecutionResult::Type TutSet::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (handDisplaysStateProvided)
	{
		TutorialSystem::get()->access_forced_hand_displays_state() = handDisplaysState;
	}
	if (energyTransfer.is_set())
	{
		TutorialSystem::get()->set_allow_energy_transfer(energyTransfer.get() == Option::Allow);
	}
	if (physicalViolence.is_set())
	{
		TutorialSystem::get()->set_allow_physical_violence(physicalViolence.get() == Option::Allow);
	}
	if (pickupOrbs.is_set())
	{
		TutorialSystem::get()->set_allow_pickup_orbs(pickupOrbs.get() == Option::Allow);
	}
	if (navigationTargetVar.is_set())
	{
		Framework::IModulesOwner* navTarget = nullptr;
		if (navigationTargetVar.get().is_valid())
		{
			if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(navigationTargetVar.get()))
			{
				navTarget = exPtr->get();
			}
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
