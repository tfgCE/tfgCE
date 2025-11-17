#include "aiLogic_a_tentacle.h"

#include "..\..\modules\custom\mc_carrier.h"

#include "..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleMovementPath.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\room.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai messages
DEFINE_STATIC_NAME_STR(aim_tentacleGetAngry, TXT("a_tentacle; get angry"));
DEFINE_STATIC_NAME_STR(aim_tentacleReadyToUnfold, TXT("a_tentacle; ready to unfold"));
DEFINE_STATIC_NAME_STR(aim_tentacleUnfold, TXT("a_tentacle; unfold")); // +length
DEFINE_STATIC_NAME_STR(aim_tentacleReadyToBurst, TXT("a_tentacle; ready to burst"));
DEFINE_STATIC_NAME_STR(aim_tentacleBurst, TXT("a_tentacle; burst")); // +length
DEFINE_STATIC_NAME_STR(aim_tentacleState, TXT("a_tentacle; state"));

// ai message parameters
DEFINE_STATIC_NAME(chance); // float
DEFINE_STATIC_NAME(length); // Range or float
DEFINE_STATIC_NAME(state); // Name

// variables
DEFINE_STATIC_NAME(tentacleState); // check a_tentacle appearance controller to learn more about this
DEFINE_STATIC_NAME(tentacleStateIdx);
DEFINE_STATIC_NAME(tentacleStateLength);

// tentacle states
DEFINE_STATIC_NAME(readyToUnfold);
DEFINE_STATIC_NAME(unfold); // requires tentacleStateLength
DEFINE_STATIC_NAME(readyToBurst);
DEFINE_STATIC_NAME(burst); // requires tentacleStateLength
DEFINE_STATIC_NAME(angry); // requires tentacleStateLength

//

REGISTER_FOR_FAST_CAST(A_Tentacle);

A_Tentacle::A_Tentacle(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

A_Tentacle::~A_Tentacle()
{
}

void A_Tentacle::advance(float _deltaTime)
{
	set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));

	base::advance(_deltaTime);
}

LATENT_FUNCTION(A_Tentacle::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	
	LATENT_VAR(Random::Generator, rg);

	LATENT_BEGIN_CODE();

	messageHandler.use_with(mind);
	{
		struct MessageProcessor
		{
			static bool get_length(OUT_ float& out_len, Random::Generator & rg, Framework::AI::Param const* _param)
			{
				if (_param)
				{
					if (_param->get_type() == type_id<Range>())
					{
						out_len = rg.get(_param->get_as<Range>());
						return true;
					}
					else if (_param->get_type() == type_id<float>())
					{
						out_len = _param->get_as<float>();
						return true;
					}
					else
					{
						todo_implement;
					}
				}
				return false;
			}
		};
			
		messageHandler.set(NAME(aim_tentacleGetAngry), [mind, &rg](Framework::AI::Message const& _message)
			{
				bool beAngryNow = true;
				if (auto* chance = _message.get_param(NAME(chance)))
				{
					if (!rg.get_chance(chance->get_as<float>()))
					{
						beAngryNow = false;
					}
				}
				if (beAngryNow)
				{
					float len = 1000.0f;
					MessageProcessor::get_length(OUT_ len, rg, _message.get_param(NAME(length)));

					if (auto* imo = mind->get_owner_as_modules_owner())
					{
						MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("message to params processing"));
						++imo->access_variables().access<int>(NAME(tentacleStateIdx));
						// hardcoded name, check a_tentacle appearance controller
						imo->access_variables().access<Name>(NAME(tentacleState)) = NAME(angry);
						imo->access_variables().access<float>(NAME(tentacleStateLength)) = len;
					}
				}
			});
		messageHandler.set(NAME(aim_tentacleReadyToUnfold), [mind](Framework::AI::Message const& _message)
			{
				if (auto* imo = mind->get_owner_as_modules_owner())
				{
					MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("message to params processing"));
					++imo->access_variables().access<int>(NAME(tentacleStateIdx));
					// hardcoded name, check a_tentacle appearance controller
					imo->access_variables().access<Name>(NAME(tentacleState)) = NAME(readyToUnfold);
				}
			});
		messageHandler.set(NAME(aim_tentacleUnfold), [mind, &rg](Framework::AI::Message const& _message)
			{
				if (auto* imo = mind->get_owner_as_modules_owner())
				{
					float len = 4.0f;
					MessageProcessor::get_length(OUT_ len, rg, _message.get_param(NAME(length)));

					MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("message to params processing"));
					++imo->access_variables().access<int>(NAME(tentacleStateIdx));
					// hardcoded name, check a_tentacle appearance controller
					imo->access_variables().access<Name>(NAME(tentacleState)) = NAME(unfold);
					imo->access_variables().access<float>(NAME(tentacleStateLength)) = len;
				}
			});
		messageHandler.set(NAME(aim_tentacleReadyToBurst), [mind](Framework::AI::Message const& _message)
			{
				if (auto* imo = mind->get_owner_as_modules_owner())
				{
					MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("message to params processing"));
					++imo->access_variables().access<int>(NAME(tentacleStateIdx));
					// hardcoded name, check a_tentacle appearance controller
					imo->access_variables().access<Name>(NAME(tentacleState)) = NAME(readyToBurst);
				}
			});
		messageHandler.set(NAME(aim_tentacleBurst), [mind, &rg](Framework::AI::Message const& _message)
			{
				if (auto* imo = mind->get_owner_as_modules_owner())
				{
					float len = 2.0f;
					MessageProcessor::get_length(OUT_ len, rg, _message.get_param(NAME(length)));

					MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("message to params processing"));
					++imo->access_variables().access<int>(NAME(tentacleStateIdx));
					// hardcoded name, check a_tentacle appearance controller
					imo->access_variables().access<Name>(NAME(tentacleState)) = NAME(burst);
					imo->access_variables().access<float>(NAME(tentacleStateLength)) = len;
				}
			});
		// generic message
		messageHandler.set(NAME(aim_tentacleState), [mind, &rg](Framework::AI::Message const& _message)
			{
				if (auto* imo = mind->get_owner_as_modules_owner())
				{
					MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("message to params processing"));
					++imo->access_variables().access<int>(NAME(tentacleStateIdx));
					if (auto* state = _message.get_param(NAME(state)))
					{
						// hardcoded name, check a_tentacle appearance controller
						imo->access_variables().access<Name>(NAME(tentacleState)) = state->get_as<Name>();
					}
					float len = 1000.0f;
					if (MessageProcessor::get_length(OUT_ len, rg, _message.get_param(NAME(length))))
					{
						// hardcoded name, check a_tentacle appearance controller
						imo->access_variables().access<float>(NAME(tentacleStateLength)) = len;
					}
				}
			});
	}

	while (true)
	{
		LATENT_WAIT(Random::get_float(0.1f, 0.3f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

