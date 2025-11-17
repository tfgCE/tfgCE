#include "aiLogic_catcher.h"

#include "..\..\game\game.h"
#include "..\..\game\gameLog.h"

#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_pickup.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleGameplay.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\framework\world\room.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"
#include "..\..\..\core\system\video\video3dPrimitives.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai messages
DEFINE_STATIC_NAME_STR(aim_addRoom, TXT("catcher; add room"));
DEFINE_STATIC_NAME_STR(aimParam_room, TXT("room"));
DEFINE_STATIC_NAME_STR(aim_trackWeapons, TXT("catcher; track weapons"));

//

REGISTER_FOR_FAST_CAST(Catcher);

Catcher::Catcher(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	catcherData = fast_cast<CatcherData>(_logicData);
}

Catcher::~Catcher()
{
}

void Catcher::advance(float _deltaTime)
{
	base::advance(_deltaTime);
}

void Catcher::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(Catcher::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai catcher] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * self = fast_cast<Catcher>(logic);

	LATENT_BEGIN_CODE();

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aim_addRoom), [self](Framework::AI::Message const& _message)
			{
				if (auto* roomPtr = _message.get_param(NAME(aimParam_room)))
				{
					if (auto* room = roomPtr->get_as< SafePtr<Framework::Room> >().get())
					{
						self->add_room_to_track(room);
					}
				}
			}
		);
		messageHandler.set(NAME(aim_trackWeapons), [self](Framework::AI::Message const& _message)
			{
				self->track_weapons();
			}
		);
	}

	while (true)
	{
		LATENT_WAIT(0.1f);
	}
	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

void Catcher::add_room_to_track(Framework::Room* _room)
{
	roomsToTrack.push_back();
	roomsToTrack.get_last() = _room;
}

void Catcher::track_weapons()
{
	weapons.clear();
	for_every_ref(room, roomsToTrack)
	{
		for_every_ptr(imo, room->get_objects())
		{
			if (auto* g = imo->get_gameplay_as< ModuleEquipments::Gun>())
			{
				weapons.push_back();
				auto& w = weapons.get_last();
				w.imo = imo;
				todo_implement //w.setup.copy_from(g->get_weapon_setup());
			}
		}
	}
}


void Catcher::on_pilgrimage_instance_switch(PilgrimageInstance const* _from, PilgrimageInstance const* _to)
{
	// determine if the catcher is in the _from's main region, act only if so

	bool process = false;

	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (auto* r = _from->get_main_region())
		{
			if (auto* inRoom = imo->get_presence()->get_in_room())
			{
				if (inRoom->is_in_region(r))
				{
					process = true;
				}
			}
		}
	}

	if (!process)
	{
		return;
	}

	transfer_weapons_to_persistence(true); // we're moving somewhere else, weapons that are on the pilgrim should be handled by something else

	pio_remove();
}

void Catcher::on_pilgrimage_instance_end(PilgrimageInstance const* _pilgrimage, PilgrimageResult::Type _pilgrimageResult)
{
	// determine if the catcher is in the _pilgrimage's main region, act only if so
	// note that this is exclusive with switch, we either end or switch

	bool process = false;

	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (auto* r = _pilgrimage->get_main_region())
		{
			if (auto* inRoom = imo->get_presence()->get_in_room())
			{
				if (inRoom->is_in_region(r))
				{
					process = true;
				}
			}
		}
	}

	if (!process)
	{
		return;
	}

	transfer_weapons_to_persistence(false); // we ended/interrupted, even if we died, the weapons we have on the pilgrim should remain in the armoury

	pio_remove();
}

void Catcher::transfer_weapons_to_persistence(bool _weaponsInTrackedRoomsOnly)
{
	// check where each weapon is
	//	if player took it - remove it from the persistence
	//	if doesn't exist anymore - we don't know if it existed or not, keep it as it is in the persistence
	//	check where it was put and store that

	//auto& p = Persistence::access_current();

	for_every(w, weapons)
	{
		auto* imo = w->imo.get();

		bool store = false;
		if (! _weaponsInTrackedRoomsOnly)
		{
			// store all
			store = true;
		}
		else if (!imo)
		{
			store = true;
			// it must have been destroyed with the pilgrimage or via unloader, store it in the presence
			continue;
		}
		else
		{
			bool inRoom = false;
			for_every_ref(room, roomsToTrack)
			{
				if (room && imo->get_presence()->get_in_room() == room)
				{
					inRoom = true;
					break;
				}
			}
			if (inRoom)
			{
				store = true;
				// no need to check the pilgrim as pilgrim is gone to another pilgrimage
			}
		}

		if (store)
		{
			todo_implement //p.add_weapon_to_armoury(w->setup, false);
		}
	}

	weapons.clear();
}

//

REGISTER_FOR_FAST_CAST(CatcherData);

bool CatcherData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool CatcherData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
