#include "aiLogic_hackBox.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_hitIndicator.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
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

// variables
DEFINE_STATIC_NAME(interactiveDeviceId);
DEFINE_STATIC_NAME(hackBoxButtonIdx);
DEFINE_STATIC_NAME(wallBoxDoorOpen);

// emissive layers
DEFINE_STATIC_NAME(ready);
DEFINE_STATIC_NAME(operating);

// sounds
DEFINE_STATIC_NAME(done);
DEFINE_STATIC_NAME(transferred);

//

REGISTER_FOR_FAST_CAST(HackBox);

HackBox::HackBox(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	hackBoxData = fast_cast<HackBoxData>(_logicData);

	openVar.set_name(NAME(wallBoxDoorOpen));
}

HackBox::~HackBox()
{
}

void HackBox::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (depletingTime.is_set())
	{
		depletingTime = depletingTime.get() + _deltaTime;
	}

	for_every(b, buttons)
	{
		if (! depleted)
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

	timeToCheckPlayer -= _deltaTime;

	if (timeToCheckPlayer < 0.0f)
	{
		timeToCheckPlayer = Random::get_float(0.1f, 0.3f);

		bool isPlayerInNow = false;

		{
			auto* imo = get_mind()->get_owner_as_modules_owner();
			for_every_ptr(object, imo->get_presence()->get_in_room()->get_objects())
			{
				if (object->get_gameplay_as<ModulePilgrim>())
				{
					isPlayerInNow = true;
					break;
				}
			}
		}

		if (isPlayerInNow ^ playerIsIn)
		{
			playerIsIn = isPlayerInNow;
		}
	}
}

void HackBox::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(HackBox::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai hackBox] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(bool, madeKnown);
	LATENT_VAR(bool, madeVisited);
	LATENT_VAR(bool, somethingChanged);
	LATENT_VAR(Optional<VectorInt2>, cellAt);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<HackBox>(logic);

	LATENT_BEGIN_CODE();

	self->depleted = false;

	ai_log(self, TXT("hack box, hello!"));

	self->openVar.look_up<float>(imo->access_variables());

	if (imo)
	{
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			cellAt = piow->find_cell_at(imo);
			if (cellAt.is_set())
			{
				self->depleted = piow->is_pilgrim_device_state_depleted(cellAt.get(), imo);
				ai_log(self, self->depleted? TXT("depleted") : TXT("available"));
			}
		}

		ai_log(self, TXT("get button"));

#ifdef AN_USE_AI_LOG
		if (auto* id = imo->get_variables().get_existing<int>(NAME(interactiveDeviceId)))
		{
			ai_log(self, TXT("id = %i"), *id);
		}
#endif

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
								if (auto* idx = _object->get_variables().get_existing<VectorInt2>(NAME(hackBoxButtonIdx)))
								{
									self->buttons.push_back();
									self->buttons.get_last().at = *idx;
									self->buttons.get_last().imo = _object;
								}
								else
								{
									error(TXT("no [VectorInt2] hackBoxButtonIdx variable provided for \"%S\""), _object->ai_get_name().to_char());
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

		ai_log(self, TXT("buttons accessed"));
	}

	// reset puzzle
	{
		if (self->depleted)
		{
			for_every(b, self->buttons)
			{
				b->on = false;
			}
		}
		else
		{
			bool defaultOn = self->rg.get_bool();
			for_every(b, self->buttons)
			{
				b->on = defaultOn;
			}
			// randomise puzzle
			while (self->do_all_buttons_have_same_state())
			{
				for_count(int, i, self->rg.get_int_from_range(30, 100))
				{
					self->act_on_button(self->buttons[self->rg.get_int(self->buttons.get_size())].at);
				}
			}
		}
	}

	self->depletingTime.clear();

	madeKnown = false;
	madeVisited = false;
	somethingChanged = true;

	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		madeVisited = piow->is_pilgrim_device_state_visited(cellAt.get(), imo);
	}

	while (true)
	{
		if (self->playerIsIn && !madeVisited)
		{
			madeVisited = true;
			if (auto* piow = PilgrimageInstanceOpenWorld::get())
			{
				piow->mark_pilgrim_device_state_visited(cellAt.get(), imo);
			}
			if (auto* ms = MissionState::get_current())
			{
				ms->visited_interface_box();
			}
		}

		self->openVar.access<float>() = (!self->depleted && self->playerIsIn) ? 1.0f : 0.0f;

		if (self->depletingTime.is_set())
		{
			if (self->depletingTime.get() < 5.0f)
			{
				float const speed = 6.0f;
				float const spacing = 3.0f;
				float const beOn = 1.0f;
				for_every(b, self->buttons)
				{
					b->on = mod((float)b->at.y + round(self->depletingTime.get() * speed), spacing) <= beOn;
				}
			}
			else
			{
				if (auto* s = imo->get_sound())
				{
					s->play_sound(NAME(transferred));
				}

				{
					todo_multiplayer_issue(TXT("we just get player here"));
					if (auto* g = Game::get_as<Game>())
					{
						if (auto* pa = g->access_player().get_actor())
						{
							if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
							{
								mp->access_overlay_info().speak(PilgrimOverlayInfoSpeakLine::TransferComplete, PilgrimOverlayInfo::SpeakParams().allow_auto_clear());
							}
						}
					}
				}

				self->depleted = true;
				self->depletingTime.clear();
				for_every(b, self->buttons)
				{
					b->on = false;
				}
			}

			somethingChanged = true;
		}
		else if (! self->depleted)
		{
			for_every(b, self->buttons)
			{
				if (b->pressed && !b->prevPressed)
				{
					somethingChanged = true;
					self->act_on_button(b->at);

					if (!madeKnown)
					{
						if (auto* piow = PilgrimageInstanceOpenWorld::get())
						{
							madeKnown = true;
							piow->mark_pilgrimage_device_direction_known(imo);
						}
					}
				}
			}
			if (self->do_all_buttons_have_same_state())
			{
				somethingChanged = true;
				self->depletingTime = 0.0f;

				if (auto* s = imo->get_sound())
				{
					s->play_sound(NAME(done));
				}

				{
					todo_multiplayer_issue(TXT("we just get player here"));
					if (auto* g = Game::get_as<Game>())
					{
						if (auto* pa = g->access_player().get_actor())
						{
							if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
							{
								mp->access_overlay_info().speak(PilgrimOverlayInfoSpeakLine::Success, PilgrimOverlayInfo::SpeakParams().allow_auto_clear());
								mp->access_overlay_info().speak(PilgrimOverlayInfoSpeakLine::ReceivingData, PilgrimOverlayInfo::SpeakParams().allow_auto_clear().speak_delay(1.5f));
							}
						}
					}
				}

				if (auto* ms = MissionState::get_current())
				{
					ms->hacked_box();
				}
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					if (cellAt.is_set())
					{
						piow->mark_pilgrim_device_state_depleted(cellAt.get(), imo);
						piow->mark_pilgrimage_device_direction_known(imo);
					}
				}
			}
		}

		// update buttons
		{
			for_every(b, self->buttons)
			{
				b->prevPressed = b->pressed;
				b->pressed = false;
			}
		}

		if (somethingChanged)
		{
			for_every(b, self->buttons)
			{
				b->update_emissive(self->depletingTime.is_set() || self->depleted);
			}
		}

		if (somethingChanged)
		{
			somethingChanged = false;
			LATENT_YIELD();
		}
		else
		{
			LATENT_WAIT(self->rg.get_float(0.05f, 0.2f));
		}
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

void HackBox::act_on_button(VectorInt2 const& _at)
{
	for_every(b, buttons)
	{
		int dist = abs(b->at.x - _at.x) + abs(b->at.y - _at.y);
		if (dist <= 1)
		{
			b->on = !b->on;
		}
	}
}

bool HackBox::do_all_buttons_have_same_state() const
{
	if (buttons.is_empty())
	{
		return false;
	}
	bool state = buttons.get_first().on;
	for_every(b, buttons)
	{
		if (b->on != state)
		{
			return false;
		}
	}
	return true;
}

//

void HackBox::Button::update_emissive(bool _depleted)
{
	if (imo.get())
	{
		if (auto* ec = imo->get_custom<CustomModules::EmissiveControl>())
		{
			if (_depleted)
			{
				if (on)
				{
					ec->emissive_activate(NAME(operating));
				}
				else
				{
					ec->emissive_deactivate(NAME(operating));
				}
				ec->emissive_deactivate(NAME(ready));
			}
			else
			{
				if (on)
				{
					ec->emissive_activate(NAME(ready));
				}
				else
				{
					ec->emissive_deactivate(NAME(ready));
				}
				ec->emissive_deactivate(NAME(operating));
			}
		}
	}
}

//

REGISTER_FOR_FAST_CAST(HackBoxData);

bool HackBoxData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool HackBoxData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
