#include "aiLogicTask_hideDisappear.h"

#include "..\..\aiCommonVariables.h"
#include "..\..\..\game\gameDirector.h"
#include "..\..\..\modules\moduleAI.h"

#include "..\..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\..\framework\ai\aiLogic.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\object\object.h"
#include "..\..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Tasks;

//

// collision push/pop flags
DEFINE_STATIC_NAME(hideDisappear);

// tags
DEFINE_STATIC_NAME(hostileCharacter);

//

bool Logics::should_hide_disappear(::Framework::AI::MindInstance* mind, bool _evenIfVisible)
{
	if (auto* gd = GameDirector::get_active())
	{
		if (gd->should_hide_immobile_hostiles())
		{
			if (auto* imo = mind->get_owner_as_modules_owner())
			{
				if (auto* a = imo->get_appearance())
				{
					if (!a->is_visible())
					{
						// stay hidden
						return true;
					}
				}
				if (auto* o = imo->get_as_object())
				{
					if (!o->get_tags().get_tag_as_int(NAME(hostileCharacter)))
					{
						return false;
					}
				}
				if (_evenIfVisible)
				{
					return true;
				}
				{
					int rd = imo->get_room_distance_to_recently_seen_by_player();
					if (rd > 1)
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

LATENT_FUNCTION(Tasks::hide_disappear)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("hide / disappear"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(PerceptionPause, perceptionPaused);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(bool, remainHidden);

	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("hide, disappear"));
	ai_log_no_colour(mind->get_logic());

	perceptionPaused.pause(NAME(hideDisappear));

	if (imo)
	{
		if (auto* a = imo->get_appearance())
		{
			a->be_visible(false);
		}
		if (auto* c = imo->get_collision())
		{
			c->push_collision_flags(NAME(hideDisappear), Collision::Flags::none());
			c->push_collides_with_flags(NAME(hideDisappear), Collision::Flags::none());
			c->push_detects_flags(NAME(hideDisappear), Collision::Flags::none());
			c->update_collidable_object();
		}
		if (auto* ai = fast_cast<ModuleAI>(imo->get_ai()))
		{
			ai->mark_visible_for_game_director(false);
		}
	}

	remainHidden = true;
	while (remainHidden)
	{
		{
			bool gdToAppear = true;
			if (auto* gd = GameDirector::get_active())
			{
				if (gd->should_hide_immobile_hostiles())
				{
					gdToAppear = false;
				}
			}
			if (gdToAppear)
			{
				if (auto* imo = mind->get_owner_as_modules_owner())
				{
					if (auto* r = imo->get_presence()->get_in_room())
					{
						if (r->get_distance_to_recently_seen_by_player() > 1)
						{
							remainHidden = false;
						}
					}
				}
			}
		}
		LATENT_WAIT(Random::get_float(0.2f, 0.5f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("show, appear"));
	ai_log_no_colour(mind->get_logic());

	if (auto* ai = fast_cast<ModuleAI>(imo->get_ai()))
	{
		ai->mark_visible_for_game_director(true);
	}

	if (imo)
	{
		if (auto* a = imo->get_appearance())
		{
			a->be_visible(true);
		}
		if (auto* c = imo->get_collision())
		{
			c->pop_collision_flags(NAME(hideDisappear));
			c->pop_collides_with_flags(NAME(hideDisappear));
			c->pop_detects_flags(NAME(hideDisappear));
			c->update_collidable_object();
		}
	}

	perceptionPaused.unpause(NAME(hideDisappear));

	LATENT_END_CODE();
	LATENT_RETURN();
}
