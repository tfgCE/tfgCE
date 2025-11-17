#include "aiLogic_puzzlePanel.h"

#include "..\..\game\playerSetup.h"
#include "..\..\modules\custom\mc_emissiveControl.h"

#include "..\..\..\core\vr\iVR.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
// instead of doing whole sequence, for non vr builds, do just one button press
// also error one is placed to test mistake
#define SHORTCUT_FOR_DEV_NON_VR
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(interval);
DEFINE_STATIC_NAME(intervalIncrease);
DEFINE_STATIC_NAME(addActive);
DEFINE_STATIC_NAME(addError);
DEFINE_STATIC_NAME(addErrorIncreasTryInterval);
DEFINE_STATIC_NAME(triggerGameScriptExecutionTrapOnGoodButton);
DEFINE_STATIC_NAME(triggerGameScriptExecutionTrapOnBadButton);
DEFINE_STATIC_NAME(triggerGameScriptExecutionTrapOnMistake);
DEFINE_STATIC_NAME(triggerGameScriptExecutionTrapOnAboutToDone);
DEFINE_STATIC_NAME(triggerGameScriptExecutionTrapOnDone);
DEFINE_STATIC_NAME(crashOnDone);

// variables
DEFINE_STATIC_NAME(interactiveDeviceId);
DEFINE_STATIC_NAME(puzzleButtonIdx);

// emissive layers for buttons
DEFINE_STATIC_NAME(ready);
DEFINE_STATIC_NAME(bad);
DEFINE_STATIC_NAME(busy);

// sounds
DEFINE_STATIC_NAME(mistake);
DEFINE_STATIC_NAME(crash);
DEFINE_STATIC_NAME(done);
DEFINE_STATIC_NAME_STR(goodButton, TXT("good button"));

// temporary objects
DEFINE_STATIC_NAME_STR(damagedSparkles, TXT("damaged sparkles"));

// ai message
DEFINE_STATIC_NAME_STR(aboutToDone, TXT("about to done"));
DEFINE_STATIC_NAME_STR(turnOff, TXT("turn off"));

//

REGISTER_FOR_FAST_CAST(PuzzlePanel);

PuzzlePanel::PuzzlePanel(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	puzzlePanelData = fast_cast<PuzzlePanelData>(_logicData);
}

PuzzlePanel::~PuzzlePanel()
{
}

void PuzzlePanel::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	for_every(b, buttons)
	{
		if (state == State::Active)
		{
			if (b->imo.get())
			{
				if (auto* mms = fast_cast<Framework::ModuleMovementSwitch>(b->imo->get_movement()))
				{
					if (mms->is_active_at(1))
					{
						b->pressed = true;
					}
				}
			}
		}
		else
		{
			b->pressed = false;
			b->prevPressed = false;
		}
	}
}

void PuzzlePanel::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	interval = _parameters.get_value<float>(NAME(interval), interval);
	intervalIncrease = _parameters.get_value<float>(NAME(intervalIncrease), intervalIncrease);

	addActive = _parameters.get_value<RangeInt>(NAME(addActive), addActive);
	if (auto* a = _parameters.get_existing<int>(NAME(addActive)))
	{
		addActive = RangeInt(*a);
	}

	startingAddError = _parameters.get_value<int>(NAME(addError), startingAddError);
	addErrorIncreaseTryInterval = _parameters.get_value<int>(NAME(addErrorIncreasTryInterval), addErrorIncreaseTryInterval);

	triggerGameScriptExecutionTrapOnGoodButton = _parameters.get_value<Name>(NAME(triggerGameScriptExecutionTrapOnGoodButton), triggerGameScriptExecutionTrapOnGoodButton);
	triggerGameScriptExecutionTrapOnBadButton = _parameters.get_value<Name>(NAME(triggerGameScriptExecutionTrapOnBadButton), triggerGameScriptExecutionTrapOnBadButton);
	triggerGameScriptExecutionTrapOnMistake = _parameters.get_value<Name>(NAME(triggerGameScriptExecutionTrapOnMistake), triggerGameScriptExecutionTrapOnMistake);
	triggerGameScriptExecutionTrapOnAboutToDone = _parameters.get_value<Name>(NAME(triggerGameScriptExecutionTrapOnAboutToDone), triggerGameScriptExecutionTrapOnAboutToDone);
	triggerGameScriptExecutionTrapOnDone = _parameters.get_value<Name>(NAME(triggerGameScriptExecutionTrapOnDone), triggerGameScriptExecutionTrapOnDone);

	crashOnDone = _parameters.get_value<bool>(NAME(crashOnDone), crashOnDone);
}

void PuzzlePanel::switch_state(State _state)
{
	if (crashOnDone && (_state == State::Done))
	{
		_state = State::Crash;
	}
	state = _state;
	stateSubIdx = NONE;
	stateSubTime = 0.0f;
	if (state == State::Mistake)
	{
		Framework::GameScript::ScriptExecution::trigger_execution_trap(triggerGameScriptExecutionTrapOnMistake);
	}
	if (state == State::AboutToDone)
	{
		Framework::GameScript::ScriptExecution::trigger_execution_trap(triggerGameScriptExecutionTrapOnAboutToDone);
	}
	if (state == State::Done)
	{
		Framework::GameScript::ScriptExecution::trigger_execution_trap(triggerGameScriptExecutionTrapOnDone);
	}
}

void PuzzlePanel::next_state_sub_idx()
{
	++ stateSubIdx;
	stateSubTime = 0.0f;
}

void PuzzlePanel::play_sound(Framework::IModulesOwner* imo, Name const& _sound)
{
	if (auto* s = imo->get_sound())
	{
		s->play_sound(_sound);
	}
}

LATENT_FUNCTION(PuzzlePanel::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai puzzle panel] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

#ifdef AUTO_TEST
	LATENT_VAR(::System::TimeStamp, autoTestTS);
#endif

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<PuzzlePanel>(logic);

	self->stateSubTime += LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("puzzle panel, hello!"));

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aboutToDone), [self](Framework::AI::Message const& _message)
			{
				self->switch_state(State::AboutToDone);
			}
		);
		messageHandler.set(NAME(turnOff), [self](Framework::AI::Message const& _message)
			{
				self->switch_state(State::Off);
			}
		);
		messageHandler.set(NAME(crash), [self](Framework::AI::Message const& _message)
			{
				self->switch_state(State::Crash);
			}
		);
	}

	while (self->buttons.is_empty())
	{
		if (auto* r = imo->get_presence()->get_in_room())
		{
			if (auto* id = imo->get_variables().get_existing<int>(NAME(interactiveDeviceId)))
			{
				auto* world = imo->get_in_world();
				an_assert(world);

				world->for_every_object_with_id(NAME(interactiveDeviceId), *id,
					[self, imo](Framework::Object* _object)
					{
						if (_object != imo)
						{
							if (auto* idx = _object->get_variables().get_existing<int>(NAME(puzzleButtonIdx)))
							{
								self->buttons.set_size(max(self->buttons.get_size(), *idx + 1));
								self->buttons[*idx].imo = _object;
							}
							else
							{
								error(TXT("no [int] puzzleButtonIdx variable provided for \"%S\""), _object->ai_get_name().to_char());
							}
						}
						return false; // keep going on - we want to get all
					});
			}
			else
			{
				error(TXT("no \"interactiveDeviceID\" for \"%S\""), imo->ai_get_name().to_char());
			}
		}
		LATENT_WAIT(self->rg.get_float(0.1f, 0.2f));
	}

	self->switch_state(State::Reset);

	while (true)
	{
		if (self->state == State::Reset)
		{
			if (self->stateSubIdx == NONE)
			{
				for_every(b, self->buttons)
				{
					b->state = Button::State::Off;
					b->update_emissive();
				}
				self->next_state_sub_idx();
			}
			else
			{
				if (self->stateSubTime > 1.0f)
				{
					++self->tryIdx;
					self->currentInterval = max(self->currentInterval, self->interval);
					self->currentAddError = max(self->currentAddError, self->startingAddError);
					if (self->tryIdx != 0)
					{
						self->currentInterval += self->intervalIncrease;
					}
					if (self->tryIdx != 0 && (self->tryIdx % self->addErrorIncreaseTryInterval) == 0)
					{
						++self->currentAddError;
					}
					self->switch_state(State::Active);
				}
			}
		}
		else if (self->state == State::Active)
		{
			if (self->stateSubIdx == NONE)
			{
				self->next_state_sub_idx();
			}
			// sub idx %2
			// 0 setup
			// 1 wait for input
			if ((self->stateSubIdx % 2) == 0)
			{
				int buttonsToActivate = self->stateSubIdx == 0 ? 1 : self->rg.get_int_from_range(self->addActive.min, self->addActive.max);
				int buttonsToError = self->stateSubIdx == 0 ? 0 : self->currentAddError;
#ifdef SHORTCUT_FOR_DEV_NON_VR
				if (!VR::IVR::get())
				{
					buttonsToError = max(1, buttonsToError);
				}
#endif
				if (GameSettings::get().difficulty.simplerPuzzles)
				{
					buttonsToError = max(1, buttonsToError);
				}
				ARRAY_STACK(int, indices, self->buttons.get_size());
				{
					int activeSoFar = 0;
					for_every(b, self->buttons)
					{
						if (b->state == Button::State::Off)
						{
							indices.push_back(for_everys_index(b));
						}
						if (b->state == Button::State::Active)
						{
							++activeSoFar;
						}
					}
					if (activeSoFar == 0)
					{
						buttonsToActivate = max(1, buttonsToActivate);
					}
				}
				{
					while (buttonsToError > 0 && !indices.is_empty())
					{
						int i = self->rg.get_int(indices.get_size());
						int idx = indices[i];
						indices.remove_fast_at(i);
						self->buttons[idx].state = Button::State::Error;
						self->buttons[idx].update_emissive();
						--buttonsToError;
					}
					while (buttonsToActivate > 0 && ! indices.is_empty())
					{
						int i = self->rg.get_int(indices.get_size());
						int idx = indices[i];
						indices.remove_fast_at(i);
						self->buttons[idx].state = Button::State::Active;
						self->buttons[idx].update_emissive();
						--buttonsToActivate;
					}
				}
				int stillActiveButtons = 0;
				for_every(b, self->buttons)
				{
					if (b->state == Button::State::Active)
					{
						++stillActiveButtons;
					}
				}
#ifdef SHORTCUT_FOR_DEV_NON_VR
				if (!VR::IVR::get() && self->stateSubIdx > 0)
				{
					self->switch_state(State::AboutToDone);
				}
				else
#endif
				if (GameSettings::get().difficulty.simplerPuzzles && self->stateSubIdx > 0)
				{
					self->switch_state(State::AboutToDone);
				}
				else if (stillActiveButtons == 0)
				{
					self->switch_state(State::AboutToDone);
				}
				else
				{
					self->next_state_sub_idx();
				}
			}
			else if ((self->stateSubIdx % 2) == 1)
			{
				for_every(b, self->buttons)
				{
					if (b->state == Button::State::Active ||
						b->state == Button::State::Error)
					{
						if (b->pressed && ! b->prevPressed)
						{
							if (b->state == Button::State::Active)
							{
								b->state = Button::State::Off;
								b->update_emissive();
								self->next_state_sub_idx();
								Framework::GameScript::ScriptExecution::trigger_execution_trap(self->triggerGameScriptExecutionTrapOnGoodButton);
								self->play_sound(imo, NAME(goodButton));
							}
							else
							{
								b->mistake = true;
								Framework::GameScript::ScriptExecution::trigger_execution_trap(self->triggerGameScriptExecutionTrapOnBadButton);
								self->switch_state(State::Mistake);
							}
						}
					}
				}
				if (self->stateSubIdx > 1 && self->stateSubTime > self->currentInterval && self->currentInterval > 0.0f)
				{
					self->switch_state(State::Mistake);
				}
			}
			for_every(b, self->buttons)
			{
				b->prevPressed = b->pressed;
				b->pressed = false;
			}
		}
		else if (self->state == State::Mistake)
		{
			bool forceNow = false;
			if (self->stateSubIdx == NONE)
			{
				forceNow = true;
			}

			if (forceNow || self->stateSubTime > 0.25f)
			{
				self->play_sound(imo, NAME(mistake));
				self->next_state_sub_idx();
				bool beOn = true;
				if (!forceNow &&
					(self->stateSubIdx % 2) == 1 &&
					PlayerPreferences::are_currently_flickering_lights_allowed())
				{
					beOn = false;
				}
				for_every(b, self->buttons)
				{
					b->state = beOn != b->mistake? Button::State::Error : Button::State::Off;
					b->update_emissive();
				}
			}
			if (self->stateSubIdx > 8)
			{
				for_every(b, self->buttons)
				{
					b->mistake = false;
				}
				self->switch_state(State::Reset);
			}
		}
		else if (self->state == State::AboutToDone)
		{
			if (self->stateSubIdx == NONE || self->stateSubTime > 0.1f)
			{
				self->next_state_sub_idx();
				if (self->buttons.is_index_valid(self->stateSubIdx))
				{
					auto& b = self->buttons[self->stateSubIdx];
					b.state = Button::State::OK;
					b.altState = b.state;
					b.update_emissive();
				}
				else
				{
					self->switch_state(State::Done);
				}
			}
		}
		else if (self->state == State::Done)
		{
			if (self->stateSubIdx == NONE)
			{
				self->play_sound(imo, NAME(done));
			}
			if (self->stateSubIdx == NONE || self->stateSubTime > 0.5f)
			{
				self->next_state_sub_idx();
				for_every(b, self->buttons)
				{
					if (b->state != b->altState)
					{
						b->update_emissive((self->stateSubIdx % 2) == 0);
					}
				}
			}
		}	
		else if (self->state == State::Crash)
		{
			if (self->stateSubIdx == NONE)
			{
				self->play_sound(imo, NAME(crash));
				for_every(b, self->buttons)
				{
					b->state = (Button::State)self->rg.get_int(Button::State::MAX);
					b->altState = (Button::State)self->rg.get_int(Button::State::MAX);
					// will update emissive right after this
				}
				if (auto* to = imo->get_temporary_objects())
				{
					for_count(int, i, 3)
					{
						Transform relativePlacement = Transform::identity;
						relativePlacement.set_translation(Vector3(self->rg.get_float(-0.15f, 0.15f), 0.05f, self->rg.get_float(-0.10f, 0.10f)));
						to->spawn(NAME(damagedSparkles), Framework::ModuleTemporaryObjects::SpawnParams().at_relative_placement(relativePlacement));
					}
				}
			}
			if (self->stateSubIdx == NONE || self->stateSubTime > 0.5f)
			{
				self->next_state_sub_idx();
				for_every(b, self->buttons)
				{
					if (b->state != b->altState)
					{
						b->update_emissive((self->stateSubIdx % 2) == 0);
					}
				}
			}
		}
		else if (self->state == State::Off)
		{
			if (self->stateSubIdx == NONE)
			{
				self->next_state_sub_idx();
				for_every(b, self->buttons)
				{
					b->state = Button::State::Off;
					b->update_emissive();
				}
			}
		}
			
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

void PuzzlePanel::Button::update_emissive(bool _alt)
{
	if (auto* i = imo.get())
	{
		if (auto* ec = i->get_custom<CustomModules::EmissiveControl>())
		{
			auto& useState = _alt ? altState : state;
			ec->emissive_set_active(NAME(ready), useState == State::Active);
			ec->emissive_set_active(NAME(bad), useState == State::Error);
			ec->emissive_set_active(NAME(busy), useState == State::OK);
		}
	}
}

//

REGISTER_FOR_FAST_CAST(PuzzlePanelData);

bool PuzzlePanelData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool PuzzlePanelData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
