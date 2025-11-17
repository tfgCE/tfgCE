#include "aiLogic_bigTrainEngineCylinder.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\custom\mc_lcdLights.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define AUTO_TEST

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai messages
DEFINE_STATIC_NAME_STR(aim_goBad, TXT("engine cylinder; go bad"));
DEFINE_STATIC_NAME_STR(aim_goGood, TXT("engine cylinder; go good"));
DEFINE_STATIC_NAME_STR(aim_goMaintenance, TXT("engine cylinder; go maintenace"));

// parameters
DEFINE_STATIC_NAME(cylinderLightCount);

//

REGISTER_FOR_FAST_CAST(BigTrainEngineCylinder);

BigTrainEngineCylinder::BigTrainEngineCylinder(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	bigTrainEngineCylinderData = fast_cast<BigTrainEngineCylinderData>(_logicData);
}

BigTrainEngineCylinder::~BigTrainEngineCylinder()
{
}

void BigTrainEngineCylinder::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	bool updateLights = false;

	if (!lights.is_empty())
	{
		if (state == State::Maintenance)
		{
			for_every(l, lights)
			{
				l->timeToChange -= _deltaTime;
				if (l->timeToChange < 0.0f)
				{
					l->timeToChange = (float)rg.get_int_from_range(1, 5) * 1.0f;

					if (for_everys_index(l) == 0)
					{
						l->colour = Colour::black;
						l->power = 0.0f;
					}
					else
					{
						l->colour = l->power == 0.0f ? (rg.get_chance(0.25f) ? bigTrainEngineCylinderData->colourBad : bigTrainEngineCylinderData->colourGood) : Colour::black.with_alpha(0.0f);
						l->power = l->power == 0.0f ? 1.0f : 0.0f;
					}

					updateLights = true;
				}
			}
		}
		else if (state == State::GoingBad)
		{
			timeToChange -= _deltaTime;
			while (timeToChange <= 0.0f && _deltaTime > 0.0f)
			{
				int changeLightIdx = stateSubIdx + 1;
				if (lights.is_index_valid(changeLightIdx))
				{
					auto& l = lights[changeLightIdx];
					l.colour = bigTrainEngineCylinderData->colourBad;
					l.power = 1.0f;

					updateLights = true;

					++stateSubIdx;
					timeToChange += 0.013f;
				}
				else
				{
					auto& l = lights[0];
					l.colour = Colour::black.with_alpha(0.0f);
					l.power = 0.0f;

					updateLights = true;

					switch_state(State::Bad);
					break;
				}
			}
		}
		else if (state == State::Bad)
		{
			timeToChange -= _deltaTime;
			if (timeToChange <= 0.0f)
			{
				++stateSubIdx;
				Colour turnToColour = ((stateSubIdx % 2) == 1) ? bigTrainEngineCylinderData->colourBad : Colour::black.with_alpha(0.0f);
				float turnToPower = ((stateSubIdx % 2) == 1) ? 1.0f : 0.0f;

				for_every(l, lights)
				{
					l->colour = turnToColour;
					l->power = turnToPower;
				}

				updateLights = true;

				timeToChange = 1.0f;
			}
		}
		else if (state == State::Good)
		{
			timeToChange -= _deltaTime;
			if (timeToChange <= 0.0f)
			{
				Colour turnToColour = bigTrainEngineCylinderData->colourGood;
				float turnToPower = 1.0f;

				for_every(l, lights)
				{
					l->colour = turnToColour;
					l->power = turnToPower;
				}

				updateLights = true;

				timeToChange = 10.0f;
			}
		}
	}

	if (updateLights)
	{
		if (auto* imo = get_mind()->get_owner_as_modules_owner())
		{
			if (auto* lcdLights = imo->get_custom<Framework::CustomModules::LCDLights>())
			{
				if (lcdLights)
				{
					for_every(l, lights)
					{
						lcdLights->set_light(for_everys_index(l), l->colour, l->power, 0.05f, true);
					}
					lcdLights->on_update();
				}
			}
		}
	}
}

void BigTrainEngineCylinder::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	int lightCount = 0;
	lightCount = _parameters.get_value<int>(NAME(cylinderLightCount), lightCount);
	
	lights.set_size(lightCount);
	for_every(l, lights)
	{
		l->timeToChange = 0.0f;
	}
}

void BigTrainEngineCylinder::switch_state(State _state)
{
	state = _state;
	timeToChange = 0.0f;
	stateSubIdx = 0;
}

LATENT_FUNCTION(BigTrainEngineCylinder::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai big train engine cylinder] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
#ifdef AUTO_TEST
	LATENT_VAR(::System::TimeStamp, autoTestTS);
#endif

	auto * self = fast_cast<BigTrainEngineCylinder>(logic);

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("big train engine cylinder, hello!"));

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aim_goBad), [self](Framework::AI::Message const& _message) { self->switch_state(State::GoingBad); });
		messageHandler.set(NAME(aim_goGood), [self](Framework::AI::Message const& _message) { self->switch_state(State::Good); });
		messageHandler.set(NAME(aim_goMaintenance), [self](Framework::AI::Message const& _message) { self->switch_state(State::Maintenance); });
	}

#ifdef AUTO_TEST
	autoTestTS.reset();
#endif

	while (true)
	{
#ifdef AUTO_TEST
		{
			float timeSince = autoTestTS.get_time_since();
			if (self->state == State::Maintenance)
			{
				if (timeSince > 10.0f)
				{
					self->switch_state(State::GoingBad);
					autoTestTS.reset();
				}
			}
			else if (self->state == State::Bad)
			{
				if (timeSince > 10.0f)
				{
					self->switch_state(State::Good);
					autoTestTS.reset();
				}
			}
			else if (self->state == State::Good)
			{
				if (timeSince > 10.0f)
				{
					self->switch_state(State::Maintenance);
					autoTestTS.reset();
				}
			}
		}
#endif
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(BigTrainEngineCylinderData);

bool BigTrainEngineCylinderData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	colourGood.load_from_xml_child_or_attr(_node, TXT("colourGood"));
	colourBad.load_from_xml_child_or_attr(_node, TXT("colourBad"));

	return result;
}

bool BigTrainEngineCylinderData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
