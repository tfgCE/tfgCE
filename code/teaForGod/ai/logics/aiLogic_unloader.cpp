#include "aiLogic_unloader.h"

#include "..\..\game\game.h"
#include "..\..\game\gameLog.h"

#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\gameplay\equipment\me_gun.h"
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

// variables
DEFINE_STATIC_NAME(bayRadius);
DEFINE_STATIC_NAME(bayRadiusDetect);
DEFINE_STATIC_NAME(unloaderOpen);
DEFINE_STATIC_NAME(unloaderEnabled);

// sockets
DEFINE_STATIC_NAME(bay);

// emissive layers
DEFINE_STATIC_NAME(active);
DEFINE_STATIC_NAME(empty);

// ai messages
DEFINE_STATIC_NAME_STR(aim_enableUnloader, TXT("unloader; enable"));
DEFINE_STATIC_NAME_STR(aim_disableUnloader, TXT("unloader; disable"));

//

REGISTER_FOR_FAST_CAST(Unloader);

Unloader::Unloader(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	unloaderData = fast_cast<UnloaderData>(_logicData);
}

Unloader::~Unloader()
{
}

void Unloader::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (allowAutoOpen)
	{
		if (!objectInBay.is_active())
		{
			unloaderOpenTarget = 1.0f;
		}
	}
	if (! unloaderEnabled)
	{
		unloaderOpenTarget = 0.0f;
	}

	unloaderOpen = blend_to_using_speed_based_on_time(unloaderOpen, unloaderOpenTarget, unloaderOpen < unloaderOpenTarget ? 0.2f : 0.3f, _deltaTime);
	
	unloaderOpenVar.access<float>() = unloaderOpen;
}

void Unloader::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	unloaderOpenVar.set_name(NAME(unloaderOpen));
	unloaderOpenVar.look_up<float>(_parameters);

	unloaderEnabled = _parameters.get_value<bool>(NAME(unloaderEnabled), unloaderEnabled);
}

void Unloader::update_object_in_bay(::Framework::IModulesOwner* imo)
{
	auto* imoAp = imo->get_appearance();
	if (!imoAp)
	{
		return;
	}

	debug_context(imo->get_presence()->get_in_room());
	debug_filter(ai_energyKiosk);

	float const bayRadiusSq = sqr(bayRadius);
	Vector3 bayLoc = imoAp->get_os_to_ws_transform().location_to_world(imoAp->calculate_socket_os(baySocket.get_index()).get_translation());

	todo_multiplayer_issue(TXT("not checking which player has seen it, any does?"));
	// allow for object in bay only if is in a room that has been recently seen by a player

	if (auto* ib = objectInBay.get_target())
	{
		if (ib->get_presence()->get_in_room() != imo->get_presence()->get_in_room() ||
			!imo->get_presence()->get_in_room() ||
			!imo->get_presence()->get_in_room()->was_recently_seen_by_player())
		{
			objectInBay.clear_target();
		}
		else
		{
			float inBayDist = (bayLoc - ib->get_presence()->get_centre_of_presence_WS()).length();
			if (inBayDist > bayRadius * hardcoded magic_number 1.2f)
			{
				objectInBay.clear_target();
			}
		}
	}

	float bestDistSq;
	Framework::IModulesOwner* bestImo = nullptr;

	// we can check just our room and just objects, don't check presence links
	if (unloaderOpen > 0.0f &&
		(imo->get_presence()->get_in_room() && imo->get_presence()->get_in_room()->was_recently_seen_by_player()))
	{
		Framework::IModulesOwner* inBay = objectInBay.get_target();
		int objectsInBayCount = inBay? 1 : 0;
		//FOR_EVERY_OBJECT_IN_ROOM(o, imo->get_presence()->get_in_room())
		for_every_ptr(o, imo->get_presence()->get_in_room()->get_objects())
		{
			if (o != inBay &&
				o != imo) // it doesn't make much sense to include itself
			{
				if (auto* op = o->get_presence())
				{
					Vector3 oCentre = op->get_centre_of_presence_WS();
					float distSq = (bayLoc - oCentre).length_squared();
					debug_draw_arrow(true, distSq < bayRadiusSq ? Colour::green : Colour::red, bayLoc, oCentre);
					if (distSq < bayRadiusSq)
					{
						if (inBay)
						{
							if (op->is_attached_at_all_to(inBay))
							{
								// skip objects attached to object in bay, these might be buttons etc
								continue;
							}
						}
						++objectsInBayCount;
						if (auto* p = o->get_custom<CustomModules::Pickup>())
						{
							if (p->is_in_holder(imo))
							{
								if (!bestImo ||
									bestDistSq > distSq)
								{
									bestImo = o;
									bestDistSq = distSq;
								}
							}
						}
					}
				}
			}
		}
		objectInBayIsSole = objectsInBayCount == 1;
	}

	if (objectInBay.get_target() != bestImo)
	{
		objectInBay.find_path(imo, bestImo);
	}

#ifdef AN_DEVELOPMENT
	if (objectInBay.is_active())
	{
		debug_draw_sphere(true, true, Colour::blue, 0.5f, Sphere(bayLoc, 0.05f));
	}
#endif
	objectInBayWasSet = objectInBay.is_active();

	debug_no_context();
	debug_no_filter();
}

void Unloader::update_bay(::Framework::IModulesOwner* imo, bool _force)
{
	if (auto* e = imo->get_custom<CustomModules::EmissiveControl>())
	{
		int newBayOccupiedEmissive = 0;
		if (objectInBay.is_active())
		{
			newBayOccupiedEmissive = 1;
		}
		if (! unloaderEnabled)
		{
			newBayOccupiedEmissive = -1;
		}

		if ((bayOccupiedEmissive != newBayOccupiedEmissive) || _force)
		{
			if (newBayOccupiedEmissive < 0)
			{
				e->emissive_deactivate(NAME(active));
				e->emissive_deactivate(NAME(empty));
			}
			else if (newBayOccupiedEmissive)
			{
				e->emissive_activate(NAME(active));
				e->emissive_deactivate(NAME(empty));
			}
			else
			{
				e->emissive_deactivate(NAME(active));
				e->emissive_activate(NAME(empty));
			}
		}
		bayOccupiedEmissive = newBayOccupiedEmissive;
	}
}

LATENT_FUNCTION(Unloader::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai unloader] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Unloader>(logic);

	LATENT_BEGIN_CODE();

	self->bayRadius = imo->get_variables().get_value(NAME(bayRadius), self->bayRadius);
	self->bayRadius = imo->get_variables().get_value(NAME(bayRadiusDetect), self->bayRadius);

	self->baySocket.set_name(NAME(bay));
	self->baySocket.look_up(imo->get_appearance()->get_mesh());

	self->objectInBay.set_owner(imo);

	self->update_bay(imo, true);

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aim_enableUnloader), [self](Framework::AI::Message const& _message)
			{
				self->unloaderEnabled = true;
			}
		);
		messageHandler.set(NAME(aim_disableUnloader), [self](Framework::AI::Message const& _message)
			{
				self->unloaderEnabled = false;
			}
		);
	}

	while (true)
	{
		// update in-bay object
		{
			// check if we have moved too far
			self->update_object_in_bay(imo);
		}

		self->update_bay(imo);

		if (self->objectInBay.is_active())
		{
			LATENT_WAIT(0.5f);
			if (!self->objectInBay.is_active())
			{
				goto NO_OBJECT;
			}
			while (self->objectInBay.is_active() && self->unloaderOpen > 0.0f)
			{
				// close only if there are no other objects, update object in bay to check if something else popped up
				self->update_object_in_bay(imo);
				self->unloaderOpenTarget = self->objectInBayIsSole? 0.0f : 1.0f;
				LATENT_YIELD();
			}
			// closed - unless we have no object
			if (!self->objectInBay.is_active())
			{
				goto NO_OBJECT;
			}
			self->allowAutoOpen = false;
			{
				if (auto* t = self->objectInBay.get_target())
				{
					// if we discard a weapon, move it to the end of the armoury queue
					// if it has "at" set, it will be kept. If not, we should give it a lesser priority when choosing "at"
					if (auto* w = t->get_gameplay_as<ModuleEquipments::Gun>())
					{
						auto& p = Persistence::access_current();
						p.move_weapon_in_armoury_to_end(w->get_weapon_setup());
					}
					// no need to process brought item here by mission state, the mission has already ended and we base on last result whether we need it or not
					t->cease_to_exist(false);
				}
				self->objectInBay.clear_target();
			}
			LATENT_WAIT_NO_RARE_ADVANCE(0.3f);
			self->allowAutoOpen = true; // no need to set unloaderOpenTarget
		}

	NO_OBJECT:
		LATENT_WAIT_NO_RARE_ADVANCE(Random::get_float(0.1f, 0.2f));

	}
	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(UnloaderData);

bool UnloaderData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool UnloaderData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
