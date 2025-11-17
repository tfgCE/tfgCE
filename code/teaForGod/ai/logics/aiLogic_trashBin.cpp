#include "aiLogic_trashBin.h"

#include "..\..\teaForGodTest.h"

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

#ifdef AN_DEVELOPMENT_OR_PROFILER
#include "..\..\..\core\system\input.h"
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
DEFINE_STATIC_NAME(trashBinOpen);

DEFINE_STATIC_NAME(trashBinDeviceId);

// sockets
DEFINE_STATIC_NAME(bay);

// emissive layers
DEFINE_STATIC_NAME(active);
DEFINE_STATIC_NAME(empty);

// ai messages
DEFINE_STATIC_NAME_STR(aim_tb_objectToDestroy, TXT("trash bin; object to destroy"));
DEFINE_STATIC_NAME(object);
DEFINE_STATIC_NAME_STR(aim_tb_destroyObject, TXT("trash bin; destroy object"));

// sound
DEFINE_STATIC_NAME(activated);

//

REGISTER_FOR_FAST_CAST(TrashBin);

TrashBin::TrashBin(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	trashBinData = fast_cast<TrashBinData>(_logicData);
}

TrashBin::~TrashBin()
{
}

void TrashBin::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	destroyHandle.advance(_deltaTime);

	if (bayShouldRemainClosed)
	{
		trashBinOpenTarget = 0.0f;
	}
	else
	{
		trashBinOpenTarget = 1.0f;
		if (allowClosingBay)
		{
			if (destroyHandle.get_switch_at() >= 1.0f
#ifdef TEST_TRASH_BIN
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
				|| (::System::Input::get()->is_key_pressed(::System::Key::LeftAlt) &&
					::System::Input::get()->is_key_pressed(::System::Key::T)
					)
#endif
#endif
#endif
				)
			{
				trashBinOpenTarget = 0.0f;
			}
		}
	}
	
	trashBinOpen = blend_to_using_speed_based_on_time(trashBinOpen, trashBinOpenTarget, trashBinOpen < trashBinOpenTarget ? 0.2f : 0.3f, _deltaTime);
	
	if (trashBinOpenVar.is_valid())
	{
		trashBinOpenVar.access<float>() = trashBinOpen;
	}
}

void TrashBin::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
}

void TrashBin::update_object_in_bay(::Framework::IModulesOwner* imo)
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
	if (trashBinOpen > 0.0f &&
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

void TrashBin::update_bay(::Framework::IModulesOwner* imo, bool _force)
{
	if (auto* e = imo->get_custom<CustomModules::EmissiveControl>())
	{
		int newBayOccupiedEmissive = 0;
		if (objectInBay.is_active())
		{
			newBayOccupiedEmissive = 1;
		}

		if ((bayOccupiedEmissive != newBayOccupiedEmissive) || _force)
		{
			// as both are not pulsing, keep "empty" activated all the time
			e->emissive_activate(NAME(empty));
			if (newBayOccupiedEmissive < 0)
			{
				e->emissive_deactivate(NAME(active));
			}
			else if (newBayOccupiedEmissive)
			{
				e->emissive_activate(NAME(active));
			}
			else
			{
				e->emissive_deactivate(NAME(active));
			}
		}
		bayOccupiedEmissive = newBayOccupiedEmissive;
	}
}

LATENT_FUNCTION(TrashBin::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai trashBin] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	
	LATENT_VAR(bool, objectDestroyConfirmed);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<TrashBin>(logic);

	LATENT_BEGIN_CODE();

	self->trashBinOpenVar.set_name(NAME(trashBinOpen));
	self->trashBinOpenVar.look_up<float>(imo->access_variables());

	self->bayRadius = imo->get_variables().get_value(NAME(bayRadius), self->bayRadius);
	self->bayRadius = imo->get_variables().get_value(NAME(bayRadiusDetect), self->bayRadius);

	self->baySocket.set_name(NAME(bay));
	self->baySocket.look_up(imo->get_appearance()->get_mesh());

	self->objectInBay.set_owner(imo);

	self->update_bay(imo, true);

	self->destroyHandle.initialise(imo, NAME(trashBinDeviceId));

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aim_tb_destroyObject), [self, &objectDestroyConfirmed](Framework::AI::Message const& _message)
			{
				if (auto* param = _message.get_param(NAME(object)))
				{
					auto* objToDestroy = param->get_as<SafePtr<Framework::IModulesOwner>>().get();
					if (objToDestroy && objToDestroy == self->objectInBay.get_target())
					{
						objectDestroyConfirmed = true;
					}
				}
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

		self->allowClosingBay = false;

		if (self->objectInBay.is_active())
		{
			LATENT_WAIT(0.5f);
			if (!self->objectInBay.is_active())
			{
				goto NO_OBJECT;
			}
			while (self->objectInBay.is_active() && self->trashBinOpen > 0.0f)
			{
				// close only if there are no other objects, update object in bay to check if something else popped up
				self->update_object_in_bay(imo);
				self->allowClosingBay = self->objectInBayIsSole; // to actually close, will check the handle
				LATENT_YIELD();
			}
			// closed - unless we have no object
			if (!self->objectInBay.is_active())
			{
				goto NO_OBJECT;
			}
			self->bayShouldRemainClosed = true;
			// confirmation process (if required)
			{
				ai_log(self, TXT("destroy object o%p ordered"), self->objectInBay.get_target());
				objectDestroyConfirmed = true;
				if (auto* t = self->objectInBay.get_target())
				{
					if (t->get_gameplay_as<ModuleEquipments::Gun>())
					{
						objectDestroyConfirmed = false; // wait for confirmation
					}
				}
				if (!objectDestroyConfirmed)
				{
					ai_log(self, TXT("ask for confirmation to destroy o%p"), self->objectInBay.get_target());
					if (auto* message = imo->get_in_world()->create_ai_message(NAME(aim_tb_objectToDestroy)))
					{
						message->to_sub_world(imo->get_in_sub_world());
						message->access_param(NAME(object)).access_as<SafePtr<Framework::IModulesOwner>>() = self->objectInBay.get_target();
					}
				}
				if (auto* s = imo->get_sound())
				{
					s->play_sound(NAME(activated));
				}
				// wait for confirmation (if required)
				while (!objectDestroyConfirmed && self->objectInBay.get_target())
				{
					LATENT_YIELD();
				}
				if (auto* t = self->objectInBay.get_target())
				{
					ai_log(self, TXT("confirmed, destroy object o%p"), t);
					t->cease_to_exist(false);
				}
				self->objectInBay.clear_target();
			}
			LATENT_WAIT_NO_RARE_ADVANCE(0.3f);
			self->bayShouldRemainClosed = false; // no need to set trashBinOpenTarget
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

REGISTER_FOR_FAST_CAST(TrashBinData);

bool TrashBinData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool TrashBinData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
