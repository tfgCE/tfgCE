#include "gsc_pilgrimage.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\pilgrimage\pilgrimage.h"
#include "..\..\pilgrimage\pilgrimageInstance.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\world\door.h"
#include "..\..\..\framework\world\region.h"
#include "..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

// "holding" values
DEFINE_STATIC_NAME(upgradeCanister);

//

bool Conditions::Pilgrimage::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	isCurrent.load_from_xml(_node, TXT("isCurrent"), _lc);
	isNext.load_from_xml(_node, TXT("isNext"), _lc);
	isThereNext.load_from_xml(_node, TXT("isThereNext"));
	isTransitionRoomToNextPilgrimageAvailable.load_from_xml(_node, TXT("isTransitionRoomToNextPilgrimageAvailable"));
	
	tagged.load_from_xml_attribute_or_child_node(_node, TXT("tagged"));

	startingRoomExitDoorOpen.load_from_xml(_node, TXT("startingRoomExitDoorOpen"));
	startingRoomExitDoorAdvisedToBeOpen.load_from_xml(_node, TXT("startingRoomExitDoorAdvisedToBeOpen"));
	
	currentContainsObjectVar.load_from_xml(_node, TXT("currentContainsObjectVar"));
	currentContainsRoomVar.load_from_xml(_node, TXT("currentContainsRoomVar"));

	return result;
}

bool Conditions::Pilgrimage::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= isCurrent.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= isNext.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

bool Conditions::Pilgrimage::check(Framework::GameScript::ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	if (auto* pi = PilgrimageInstance::get())
	{
		if (auto* ic = isCurrent.get())
		{
			bool matches = ic == pi->get_pilgrimage();
			anyOk |= matches;
			anyFailed |= !matches;
		}
		if (auto* ic = isNext.get())
		{
			auto* np = pi->get_next_pilgrimage();
			bool matches = ic == np;
			anyOk |= matches;
			anyFailed |= !matches;
		}
		if (isThereNext.is_set())
		{
			auto* np = pi->get_next_pilgrimage();
			bool matches = (np != nullptr) == isThereNext.get();
			anyOk |= matches;
			anyFailed |= !matches;
		}
		if (!tagged.is_empty())
		{
			if (auto* p = pi->get_pilgrimage())
			{
				bool matches = tagged.check(p->get_tags());
				anyOk |= matches;
				anyFailed |= !matches;
			}
		}
	}
	if (auto* pi = PilgrimageInstanceOpenWorld::get())
	{
		if (startingRoomExitDoorOpen.is_set())
		{
			if (auto* d = pi->get_starting_room_exit_door())
			{
				if (startingRoomExitDoorOpen.get())
				{
					bool isOpen = abs(d->get_open_factor()) >= 1.0f;
					anyOk |= isOpen;
					anyFailed |= !isOpen;
				}
				else
				{
					bool isClosed = abs(d->get_open_factor()) <= 1.0f;
					anyOk |= isClosed;
					anyFailed |= !isClosed;
				}
			}
		}
		if (startingRoomExitDoorAdvisedToBeOpen.is_set())
		{
			bool advised = pi->is_starting_door_advised_to_be_open();
			if (startingRoomExitDoorAdvisedToBeOpen.get())
			{
				anyOk |= advised;
				anyFailed |= !advised;
			}
			else
			{
				anyOk |= !advised;
				anyFailed |= advised;
			}
		}
		if (isTransitionRoomToNextPilgrimageAvailable.is_set())
		{
			bool thereIsNextPilgrimage = false;
			if (auto* npi = pi->get_next_pilgrimage_instance())
			{
				if (auto* owpi = fast_cast<PilgrimageInstanceOpenWorld>(pi))
				{
					thereIsNextPilgrimage = owpi->get_any_transition_room();
				}
				else
				{
					thereIsNextPilgrimage = true;
				}
			}
			bool conditionOK = isTransitionRoomToNextPilgrimageAvailable.get() == thereIsNextPilgrimage;
			anyOk |= conditionOK;
			anyFailed |= !conditionOK;
		}
		if (currentContainsObjectVar.is_set())
		{
			bool contains = false;
			if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(currentContainsObjectVar.get()))
			{
				if (auto* imo = exPtr->get())
				{
					if (auto* p = imo->get_presence())
					{
						if (auto* r = p->get_in_room())
						{
							contains = r->is_in_region(pi->get_main_region());
						}
					}
				}
			}
			anyOk |= contains;
			anyFailed |= !contains;
		}
		if (currentContainsRoomVar.is_set())
		{
			bool contains = false;
			if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(currentContainsRoomVar.get()))
			{
				if (auto* r = exPtr->get())
				{
					contains = r->is_in_region(pi->get_main_region());
				}
			}
			anyOk |= contains;
			anyFailed |= !contains;
		}
	}

	return anyOk && !anyFailed;
}
