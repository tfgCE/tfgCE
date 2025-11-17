#include "aiLogic_pilgrimTeaBox.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_hitIndicator.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\custom\mc_lcdLights.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// sounds
DEFINE_STATIC_NAME(arming);

// variables
DEFINE_STATIC_NAME_STR(aim_arm, TXT("pilgrim tea box; arm"));
	DEFINE_STATIC_NAME(time);

//

REGISTER_FOR_FAST_CAST(PilgrimTeaBox);

PilgrimTeaBox::PilgrimTeaBox(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	initialUpdateRequired = true;

	if (auto* data = fast_cast<PilgrimTeaBoxData>(_logicData))
	{
		notArmedColour = data->notArmedColour;
		armingColour = data->armingColour;
		armedColour = data->armedColour;
		notArmedPower = data->notArmedPower;
		armingLightPower = data->armingLightPower;
		armedLightPower = data->armedLightPower;
	}
}

PilgrimTeaBox::~PilgrimTeaBox()
{
}

void PilgrimTeaBox::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (lights.is_empty())
	{
		lights.set_size(32);
		for_every(l, lights)
		{
			l->arming = rg.get_bool();
		}
	}

	auto* imo = get_mind()->get_owner_as_modules_owner();

	if (initialUpdateRequired)
	{
		initialUpdateRequired = false;
		if (auto* l = imo->get_custom<Framework::CustomModules::LCDLights>())
		{
			for_count(int, i, lights.get_size())
			{
				l->set_light(i, Colour::black, 0.0f, true);
			}
			l->on_update();
		}
	}

	if (timeActive > 0.0f)
	{
		timeLeft -= _deltaTime;
		if (auto* lcd = imo->get_custom<Framework::CustomModules::LCDLights>())
		{
			bool changedSomething = false;
			for_every(l, lights)
			{
				l->timeToUpdate -= _deltaTime;
				if (l->timeToUpdate <= 0.0f)
				{
					l->timeToUpdate = max(0.05f, min(timeLeft - 0.3f, rg.get_float(0.1f, 0.4f)));
					if (!l->armed)
					{
						float armedChance = (timeActive - timeLeft) / (timeActive - 0.2f);
						armedChance = clamp(armedChance, 0.0f, 1.0f);
 						armedChance = sqr(armedChance);

						l->armed = rg.get_chance(armedChance);
						if (l->armed)
						{
							lcd->set_light(for_everys_index(l), armedColour, armedLightPower, 0.05f, true);
							changedSomething = true;
						}
						else
						{
							l->arming = !l->arming;
							lcd->set_light(for_everys_index(l), l->arming ? armingColour : notArmedColour, l->arming? armingLightPower : notArmedPower, 0.02f, true);
							changedSomething = true;
						}
					}

				}
			}
			if (changedSomething)
			{
				lcd->on_update();
			}
		}
	}
}

void PilgrimTeaBox::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(PilgrimTeaBox::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[tea box] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * self = fast_cast<PilgrimTeaBox>(logic);

	LATENT_BEGIN_CODE();

	{
		messageHandler.use_with(mind);
		{
			messageHandler.set(NAME(aim_arm), [self](Framework::AI::Message const& _message)
				{
					self->timeActive = 1.0f;
					if (auto* p = _message.get_param(NAME(time)))
					{
						self->timeActive = p->get_as<float>();
					}

					if (auto* imo = self->get_mind()->get_owner_as_modules_owner())
					{
						if (auto* s = imo->get_sound())
						{
							s->play_sound(NAME(arming));
						}
					}

					self->timeLeft = self->timeActive;
				}
			);
		}
	}

	while (true)
	{
		if (self->timeActive > 0.0f)
		{
			LATENT_WAIT_NO_RARE_ADVANCE(Random::get_float(0.1f, 0.2f));
		}
		else
		{
			LATENT_WAIT(Random::get_float(0.1f, 0.2f));
		}
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//--

REGISTER_FOR_FAST_CAST(PilgrimTeaBoxData);

PilgrimTeaBoxData::PilgrimTeaBoxData()
{

}

bool PilgrimTeaBoxData::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	notArmedColour.load_from_xml_child_or_attr(_node, TXT("notArmedColour"));
	armingColour.load_from_xml_child_or_attr(_node, TXT("armingColour"));
	armedColour.load_from_xml_child_or_attr(_node, TXT("armedColour"));

	armingLightPower = _node->get_float_attribute_or_from_child(TXT("armingLightPower"), armingLightPower);
	armedLightPower = _node->get_float_attribute_or_from_child(TXT("armedLightPower"), armedLightPower);

	return result;
}

bool PilgrimTeaBoxData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
