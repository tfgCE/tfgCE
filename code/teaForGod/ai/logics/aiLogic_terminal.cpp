#include "aiLogic_terminal.h"

#include "..\..\game\game.h"
#include "..\..\game\modes\gameMode_Pilgrimage.h"
#include "..\..\modules\modulePilgrim.h"
#include "..\..\modules\modulePilgrimHand.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\pilgrimage\pilgrimageInstance.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\room.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(bayRadius);
DEFINE_STATIC_NAME(bayRadiusDetect);

// sockets
DEFINE_STATIC_NAME(bay);

// tags
DEFINE_STATIC_NAME(hand);

// emissive layers
DEFINE_STATIC_NAME(active);
DEFINE_STATIC_NAME(empty);

//

REGISTER_FOR_FAST_CAST(Terminal);

Terminal::Terminal(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	terminalData = fast_cast<TerminalData>(_logicData);
}

Terminal::~Terminal()
{
}

void Terminal::advance(float _deltaTime)
{
	base::advance(_deltaTime);
}

void Terminal::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

void Terminal::update_object_in_bay(::Framework::IModulesOwner* imo, float _deltaTime)
{
	auto* imoAp = imo->get_appearance();
	if (!imoAp)
	{
		return;
	}

	bayActive = true;
	/* should be always active
	if (auto* game = Game::get_as<Game>())
	{
		if (auto* pilgrimage = fast_cast<GameModes::Pilgrimage>(game->get_mode()))
		{
			if (pilgrimage->has_pilgrimage_instance())
			{
				bayActive = !pilgrimage->access_pilgrimage_instance().is_current_open();
			}
		}
	}
	*/

	if (bayActive)
	{
		debug_context(imo->get_presence()->get_in_room());
		debug_filter(ai_terminal);
		float const bayRadiusSq = sqr(bayRadius);
		Vector3 bayLoc = imoAp->get_os_to_ws_transform().location_to_world(imoAp->calculate_socket_os(baySocket.get_index()).get_translation());

#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (!objectInBay.is_active())
		{
			debug_draw_text(true, Colour::red, bayLoc + Vector3(0.0f, 0.0f, 0.05f), Vector2::half, true, NP, true, TXT("?!"));
		}
#endif

		if (auto* ib = objectInBay.get_target())
		{
			if (ib->get_presence()->get_in_room() != imo->get_presence()->get_in_room())
			{
				debug_draw_text(true, Colour::red, bayLoc + Vector3(0.0f, 0.0f, 0.05f), Vector2::half, true, NP, true, TXT("diff room"));
				clear_object_in_bay();
			}
			else
			{
				float inBayDist = (bayLoc - ib->get_presence()->get_centre_of_presence_WS()).length();
				float threshold = bayRadius * hardcoded magic_number 1.2f;
				if (inBayDist > threshold)
				{
					clear_object_in_bay();
					debug_draw_text(true, Colour::red, bayLoc + Vector3(0.0f, 0.0f, 0.05f), Vector2::half, true, NP, true, TXT("too far %.3f > %.3f"), inBayDist, threshold);
				}
			}
		}

		if (!objectInBay.is_active())
		{
			objectInBayTime = 0.0f;
		}

		float bestDistSq;
		Framework::IModulesOwner* bestImo = nullptr;
		Optional<int> bestKindPriority;

		// we can check just our room and just objects, don't check presence links
		{
			//FOR_EVERY_OBJECT_IN_ROOM(o, imo->get_presence()->get_in_room())
			for_every_ptr(o, imo->get_presence()->get_in_room()->get_objects())
			{
				if (o->get_gameplay_as<ModulePilgrimHand>())
				{
					if (auto* op = o->get_presence())
					{
						Vector3 oCentre = op->get_centre_of_presence_WS();
						float distSq = (bayLoc - oCentre).length_squared();
						debug_draw_time_based(0.5f, debug_draw_arrow(true, distSq < bayRadiusSq ? Colour::green : Colour::red, bayLoc, oCentre));
						if (distSq < bayRadiusSq &&
							(!bestImo || distSq < bestDistSq))
						{
							bestImo = o;
							bestDistSq = distSq;
						}
					}
				}
			}
		}

		if (bestImo != objectInBay.get_target())
		{
			clear_object_in_bay();
			if (bestImo)
			{
				objectInBay.find_path(imo, bestImo);
				objectInBayTime = 0.0f;
				debug_draw_text(true, Colour::magenta, bayLoc + Vector3(0.0f, 0.0f, 0.1f), Vector2::half, true, NP, true, TXT("changed"));
			}
		}

		if (objectInBay.is_active())
		{
			objectInBayTime += _deltaTime;
		}

		if (objectInBay.is_active() && objectInBayTime > 0.1f)
		{
			if (auto* o = objectInBay.get_target())
			{
				if (auto* t = o->get_top_instigator())
				{
					if (auto* mp = t->get_gameplay_as<ModulePilgrim>())
					{
						if (!requiresEmpty)
						{
							mp->request_terminal_connection(imo);
							requiresEmpty = true;
						}
					}
				}
			}
		}

#ifdef AN_DEVELOPMENT
		if (objectInBay.is_active())
		{
			debug_draw_sphere(true, true, Colour::green, 0.4f, Sphere(bayLoc, bayRadius));
		}
		else
		{
			debug_draw_sphere(true, true, Colour::blue, 0.25f, Sphere(bayLoc, bayRadius));
		}
#endif

		debug_draw_text(true, Colour::green, bayLoc, Vector2::half, true, NP, true, TXT("oibt:%.3f"), objectInBayTime);
		debug_no_context();
		debug_no_filter();
	}
	else
	{
		clear_object_in_bay();
	}

}

void Terminal::clear_object_in_bay()
{
	requiresEmpty = false;
	if (objectInBay.is_active())
	{
		if (auto* o = objectInBay.get_target())
		{
			if (auto* t = o->get_top_instigator())
			{
				if (auto* mp = t->get_gameplay_as<ModulePilgrim>())
				{
					mp->request_terminal_connection(nullptr);
				}
			}
		}
	}
	objectInBay.clear_target();
}

void Terminal::update_bay(::Framework::IModulesOwner* imo, bool _force)
{
	if (auto* e = imo->get_custom<CustomModules::EmissiveControl>())
	{
		bool newBayOccupiedEmissive = objectInBay.is_active();

		// if anything has changed
		if ((bayOccupiedEmissive ^ newBayOccupiedEmissive) ||
			(bayActive ^ bayActiveEmissive) ||
			_force)
		{
			if (bayActive)
			{
				if (newBayOccupiedEmissive)
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
			else
			{
				e->emissive_deactivate(NAME(active));
				e->emissive_deactivate(NAME(empty));
				bayActiveEmissive = bayActive;
			}
		}
		bayActiveEmissive = bayActive;
		bayOccupiedEmissive = newBayOccupiedEmissive;
	}
}


LATENT_FUNCTION(Terminal::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai terminal] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(float, accumulatedDeltaTime);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Terminal>(logic);

	accumulatedDeltaTime += LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	accumulatedDeltaTime = 0.0f;

	self->bayRadius = imo->get_variables().get_value(NAME(bayRadius), self->bayRadius);
	self->bayRadius = imo->get_variables().get_value(NAME(bayRadiusDetect), self->bayRadius);

	self->baySocket.set_name(NAME(bay));
	self->baySocket.look_up(imo->get_appearance()->get_mesh());

	self->objectInBay.set_owner(imo);

	self->update_bay(imo, true);
	while (true)
	{
		// update in-bay object
		{
			// check if we have moved too far
			self->update_object_in_bay(imo, accumulatedDeltaTime);
		}

		self->update_bay(imo);

		if (self->objectInBay.is_active())
		{
			accumulatedDeltaTime = 0.0f;
			LATENT_YIELD();
		}
		else
		{
			accumulatedDeltaTime = 0.0f;
			LATENT_WAIT(Random::get_float(0.05f, 0.1f));
		}
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(TerminalData);

bool TerminalData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool TerminalData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
