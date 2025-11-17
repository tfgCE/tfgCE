#include "aiLogicTask_deathSequenceSimple.h"

#include "..\..\..\modules\custom\health\mc_health.h"
#include "..\..\..\modules\custom\mc_emissiveControl.h"

#include "..\..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\..\framework\ai\aiLogic.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\module\moduleController.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\object\temporaryObject.h"

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

// temporary objects
DEFINE_STATIC_NAME(dying);
DEFINE_STATIC_NAME(explode);

//

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::death_sequence_simple)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("death sequence simple"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM_OPTIONAL(SafePtr<::Framework::IModulesOwner>, killedBy, SafePtr<::Framework::IModulesOwner>());

	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::IModulesOwner*, imo);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, dying);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, mostLikelyDeathInstigator);

	LATENT_VAR(Random::Generator, rg);

	LATENT_BEGIN_CODE();

	imo = mind->get_owner_as_modules_owner();

	ai_log(mind->get_logic(), TXT("death sequence"));

	if (auto* h = imo->get_custom<CustomModules::Health>())
	{
		mostLikelyDeathInstigator = h->get_last_damage_instigator();
	}

	if (killedBy.is_pointing_at_something()) // means it was provided
	{
		mostLikelyDeathInstigator = killedBy;
	}

	if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
	{
		em->emissive_deactivate_all();
	}

	if (auto* tos = imo->get_temporary_objects())
	{
		dying = tos->spawn_attached(NAME(dying));
	}

	{
		auto& locomotion = mind->access_locomotion();
		locomotion.stop();
		locomotion.dont_control();
		if (auto* c = imo->get_controller())
		{
			c->set_requested_relative_velocity_orientation(Rotator3(0.0f, rg.get_float(0.6f, 1.6f) /* a bit faster than 100% */ * (rg.get_bool()? -1.0f : 1.0f), 0.0f));
		}
	}

	LATENT_WAIT(0.5f);

	if (auto* tos = imo->get_temporary_objects())
	{
		Vector3 explosionLocOS = imo->get_presence()->get_random_point_within_presence_os(0.8f);
		Transform explosionWS = imo->get_presence()->get_placement().to_world(Transform(explosionLocOS, Quat::identity));
		Framework::Room* inRoom;
		imo->get_presence()->move_through_doors(explosionWS, OUT_ inRoom);
		tos->spawn_in_room(NAME(explode), inRoom, explosionWS);
	}

	ai_log(mind->get_logic(), TXT("perform death"));
	if (auto* h = imo->get_custom<CustomModules::Health>())
	{
		h->override_death_effects(false, 0.05f);
		Framework::IModulesOwner* deathInstigator = mostLikelyDeathInstigator.get();
		h->setup_death_params(false, false, deathInstigator);
		h->perform_death(deathInstigator);
	}

	LATENT_ON_BREAK();
	//
	if (mind)
	{
		::Framework::AI::Locomotion& locomotion = mind->access_locomotion();
		locomotion.stop();
	}

	LATENT_ON_END();
	//
	if (dying.is_set())
	{
		if (auto* to = fast_cast<Framework::TemporaryObject>(dying.get()))
		{
			to->desire_to_deactivate();
		}
		dying.clear();
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

