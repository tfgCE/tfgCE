#include "aiLogic_a_blob.h"

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

#include "..\..\..\core\mesh\mesh3dInstanceImpl.inl"

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai messages
DEFINE_STATIC_NAME(dealtDamage);

// variables
DEFINE_STATIC_NAME(blobTime);

// sounds / temporary objects
DEFINE_STATIC_NAME(hurt);

//

REGISTER_FOR_FAST_CAST(A_Blob);

A_Blob::A_Blob(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

A_Blob::~A_Blob()
{
}

void A_Blob::advance(float _deltaTime)
{
	set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));

	base::advance(_deltaTime);
}

LATENT_FUNCTION(A_Blob::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	
	LATENT_VAR(float, blobTimeSpeed);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<A_Blob>(logic);

	{
		float deltaTime = LATENT_DELTA_TIME;

		self->blobTime += (double)(deltaTime * blobTimeSpeed);

		blobTimeSpeed = blend_to_using_time(blobTimeSpeed, 1.0f, 0.5f, deltaTime);
	}
 
	LATENT_BEGIN_CODE();

	blobTimeSpeed = 1.0f;

	messageHandler.use_with(mind);
	{
		auto* framePtr = &_frame;
		messageHandler.set(NAME(dealtDamage), [framePtr, &blobTimeSpeed, imo](Framework::AI::Message const& _message)
			{
				blobTimeSpeed = 2.0f;
				framePtr->end_waiting();
				if (auto* s = imo->get_sound())
				{
					s->play_sound(NAME(hurt));
				}
			}
		);
	}

	while (true)
	{
		if (auto* a = imo->get_appearance())
		{
			a->access_mesh_instance().set_shader_param<float>(NAME(blobTime), (float)self->blobTime);
		}

		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

