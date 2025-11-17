#include "aiLogicComponent_confussion.h"

#include "..\..\..\game\gameDirector.h"

#include "..\..\..\modules\custom\mc_emissiveControl.h"

#include "..\..\..\..\framework\ai\aiLogic.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_USE_AI_LOG
#define INVESTIGATE_ALERT
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai message names
DEFINE_STATIC_NAME(dealtDamage);
DEFINE_STATIC_NAME(confussion);

// ai hunch/message params
DEFINE_STATIC_NAME(confussionDuration);

// (object) variables
DEFINE_STATIC_NAME(confused);

//

Confussion::Confussion()
{
}

Confussion::~Confussion()
{
}

void Confussion::setup(Framework::AI::MindInstance* _mind)
{
	an_assert(_mind);
	Framework::AI::MindInstance* mind = _mind;
	auto* imo = mind? mind->get_owner_as_modules_owner() : nullptr;
	owner = imo;
	confusedVar = imo->access_variables().find<bool>(NAME(confused));

	messageHandler.use_with(mind);

	messageHandler.set(NAME(dealtDamage), [this](Framework::AI::Message const& _message)
		{
			be_not_confused(TXT("[perception] no longer confused due to damage"));
		}
	);
	messageHandler.set(NAME(confussion), [this](Framework::AI::Message const& _message)
		{
			if (auto* confussionDurationParam = _message.get_param(NAME(confussionDuration)))
			{
#ifdef AN_USE_AI_LOG
				bool wasConfused = confussionTimeLeft > 0.0f;
#endif
				confused = true;
				confussionTimeLeft = max(confussionTimeLeft, confussionDurationParam->get_as<float>());
				update_emissive_and_var();
#ifdef AN_USE_AI_LOG
				if (!wasConfused)
				{
					Framework::AI::MindInstance* mind = nullptr;
					if (auto* imo = owner.get())
					{
						if (auto* ai = imo->get_ai())
						{
							mind = ai->get_mind();
						}
					}
					if (mind)
					{
						ai_log_colour(mind->get_logic(), Colour::cyan);
						ai_log(mind->get_logic(), TXT("[perception] confused for %.1fs"), confussionTimeLeft);
						ai_log_no_colour(mind->get_logic());
					}
				}
#endif
			}
		}
	);

}

void Confussion::update_emissive_and_var()
{
	auto* imo = owner.get();

	if (emissive_confused == confused || !imo)
	{
		return;
	}

	emissive_confused = confused;
	if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
	{
		if (emissive_confused)
		{
			em->emissive_activate(NAME(confused));
		}
		else
		{
			em->emissive_deactivate(NAME(confused));
		}
	}

	if (confusedVar.is_valid())
	{
		confusedVar.access<bool>() = confused;
	}
}

void Confussion::advance(float _deltaTime)
{
	if (GameDirector::is_violence_disallowed())
	{
		if (auto* gd = GameDirector::get())
		{
			if (gd->game_is_no_violence_forced())
			{
#ifdef AN_USE_AI_LOG
				bool wasConfused = confussionTimeLeft > 0.0f;
#endif
				confused = true;
				confussionTimeLeft = 1.0f;
				update_emissive_and_var();
#ifdef AN_USE_AI_LOG
				if (!wasConfused)
				{
					Framework::AI::MindInstance* mind = nullptr;
					if (auto* imo = owner.get())
					{
						if (auto* ai = imo->get_ai())
						{
							mind = ai->get_mind();
						}
					}
					if (mind)
					{
						ai_log_colour(mind->get_logic(), Colour::cyan);
						ai_log(mind->get_logic(), TXT("[perception] confused due to game forcing no violence"));
						ai_log_no_colour(mind->get_logic());
					}
				}
#endif
			}
		}
	}

	if (confussionTimeLeft > 0.0f)
	{
		if (!confused)
		{
			confused = true;
			update_emissive_and_var();
		}
		confussionTimeLeft -= _deltaTime;
		if (confussionTimeLeft <= 0.0f)
		{
			confused = false;
			confussionTimeLeft = 0.0f;
			update_emissive_and_var();
#ifdef AN_USE_AI_LOG
			{
				Framework::AI::MindInstance* mind = nullptr;
				if (auto* imo = owner.get())
				{
					if (auto* ai = imo->get_ai())
					{
						mind = ai->get_mind();
					}
				}
				if (mind)
				{
					ai_log_colour(mind->get_logic(), Colour::cyan);
					ai_log(mind->get_logic(), TXT("[perception] no longer confused"));
					ai_log_no_colour(mind->get_logic());
				}
			}
#endif
		}
	}
}

void Confussion::be_not_confused(tchar const * _reason)
{
#ifdef AN_USE_AI_LOG
	if (confused || confussionTimeLeft > 0.0f)
	{
		Framework::AI::MindInstance* mind = nullptr;
		if (auto* imo = owner.get())
		{
			if (auto* ai = imo->get_ai())
			{
				mind = ai->get_mind();
			}
		}
		if (mind)
		{
			ai_log_colour(mind->get_logic(), Colour::cyan);
			ai_log(mind->get_logic(), _reason);
			ai_log_no_colour(mind->get_logic());
		}
	}
#endif
	confused = false;
	confussionTimeLeft = 0.0f;
	update_emissive_and_var();

}