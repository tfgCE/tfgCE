#include "aiLogic_findableBox.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

REGISTER_FOR_FAST_CAST(FindableBox);

FindableBox::FindableBox(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	findableBoxData = fast_cast<FindableBoxData>(_logicData);
}

FindableBox::~FindableBox()
{
}

void FindableBox::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	timeToCheckPlayer -= _deltaTime;

	if (timeToCheckPlayer < 0.0f)
	{
		timeToCheckPlayer = Random::get_float(0.1f, 0.3f);

		bool isPlayerInNow = false;

		{
			auto* imo = get_mind()->get_owner_as_modules_owner();
			for_every_ptr(object, imo->get_presence()->get_in_room()->get_objects())
			{
				if (object->get_gameplay_as<ModulePilgrim>())
				{
					isPlayerInNow = true;
					break;
				}
			}
		}

		if (isPlayerInNow ^ playerIsIn)
		{
			playerIsIn = isPlayerInNow;
		}
	}
}

void FindableBox::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(FindableBox::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai findableBox] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(bool, madeKnown);
	LATENT_VAR(bool, madeVisited);
	LATENT_VAR(Optional<VectorInt2>, cellAt);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<FindableBox>(logic);

	LATENT_BEGIN_CODE();

	self->depleted = false;

	ai_log(self, TXT("findable box, hello!"));

	if (imo)
	{
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			cellAt = piow->find_cell_at(imo);
			if (cellAt.is_set())
			{
				self->depleted = piow->is_pilgrim_device_state_depleted(cellAt.get(), imo);
				ai_log(self, self->depleted? TXT("depleted") : TXT("available"));
			}
		}
	}

	madeKnown = false;
	madeVisited = false;

	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		madeVisited = piow->is_pilgrim_device_state_visited(cellAt.get(), imo);
	}

	while (true)
	{
		if (self->playerIsIn)
		{
			if (!madeVisited)
			{
				madeVisited = true;
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					piow->mark_pilgrim_device_state_visited(cellAt.get(), imo);
				}
				if (auto* ms = MissionState::get_current())
				{
					ms->visited_interface_box();
				}
			}

			if (!madeKnown)
			{
				madeKnown = true;
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					piow->mark_pilgrimage_device_direction_known(imo);
				}
			}

			if (!self->depleted)
			{
				self->depleted = true;
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					if (cellAt.is_set())
					{
						piow->mark_pilgrim_device_state_depleted(cellAt.get(), imo);
						piow->mark_pilgrimage_device_direction_known(imo);
					}
				}
			}
		}

		LATENT_WAIT(Random::get_float(0.05f, 0.2f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(FindableBoxData);

bool FindableBoxData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool FindableBoxData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
