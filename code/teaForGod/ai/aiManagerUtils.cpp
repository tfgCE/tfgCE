#include "aiManagerUtils.h"

#include "..\game\game.h"
#include "..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\framework\ai\aiMindInstance.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\module\moduleAI.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\object\scenery.h"
#include "..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;

//

// library global referencer
DEFINE_STATIC_NAME_STR(gr_aiManager, TXT("ai manager"));

// parameters
DEFINE_STATIC_NAME(inRoom);
DEFINE_STATIC_NAME(inRegion);

//

bool ManagerUtils::queue_spawn_ai_manager(REF_ RefCountObjectPtr<Framework::DelayedObjectCreation>& _doc, SpawnAiManagerParams const& _params)
{
	an_assert(_params.mind);
	an_assert(_params.room);
	if (auto* aiManagerSceneryType = Framework::Library::get_current()->get_global_references().get<Framework::SceneryType>(NAME(gr_aiManager), false))
	{
		auto* useAIMind = _params.mind;

		String location;
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			Optional<VectorInt2> at;
			if (_params.region)
			{
				at = piow->find_cell_at(_params.region);
			}
			else if (_params.room)
			{
				at = piow->find_cell_at(_params.room);
			}
			else if (_params.imoOwner)
			{
				at = piow->find_cell_at(_params.imoOwner->get_presence()->get_in_room());
			}
			if (at.is_set())
			{
				location = String::printf(TXT("%ix%i"), at.get().x, at.get().y);
			}
		}

		Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
		doc->activateImmediatelyEvenIfRoomVisible = true;
		doc->wmpOwnerObject = _params.imoOwner;
		doc->inRoom = _params.room;
		doc->name = String::printf(TXT("[%S] \"%S\" via ai manager utils"), location.to_char(), _params.mind->get_name().to_string().to_char());
		doc->objectType = aiManagerSceneryType;
		doc->randomGenerator = _params.imoOwner->get_individual_random_generator();
		doc->priority = 1000;
		doc->checkSpaceBlockers = false;

		_params.room->collect_variables(doc->variables);

		if (_params.region)
		{
			doc->variables.access<SafePtr<Framework::Region>>(NAME(inRegion)) = _params.region;
		}
		else
		{
			doc->variables.access<SafePtr<Framework::Room>>(NAME(inRoom)) = _params.room;
		}

		doc->post_initialise_modules_function = [useAIMind, aiManagerSceneryType](Framework::Object* spawnedObject)
		{
			if (auto* ai = spawnedObject->get_ai())
			{
				if (!ai->get_mind()->get_mind())
				{
					ai->get_mind()->use_mind(useAIMind);
				}
				else
				{
					error(TXT("ai manager scenery type \"%S\" should not have mind set"), aiManagerSceneryType->get_name().to_string().to_char());
				}
			}
			else
			{
				error(TXT("ai manager scenery type \"%S\" does not have an ai module"), aiManagerSceneryType->get_name().to_string().to_char());
			}
		};

		Game::get()->queue_delayed_object_creation(doc);

		_doc = doc;

		return true;
	}

	return false;
}

