#include "aiLogic_subtitleDevice.h"

#include "..\..\game\playerSetup.h"

#include "..\..\sound\subtitleSystem.h"
#include "..\..\sound\voiceoverSystem.h"

#include "..\..\modules\custom\mc_emissiveControl.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
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
DEFINE_STATIC_NAME(pilgrimLock);

// emissive layers
DEFINE_STATIC_NAME(ready);
DEFINE_STATIC_NAME(active);

//

REGISTER_FOR_FAST_CAST(SubtitleDevice);

SubtitleDevice::SubtitleDevice(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	subtitleDeviceData = fast_cast<SubtitleDeviceData>(_logicData);
}

SubtitleDevice::~SubtitleDevice()
{
	if (display)
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
}

void SubtitleDevice::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	{
		bool newVoiceoverActive = voiceoverActive;
		if (auto* vos = VoiceoverSystem::get())
		{
			newVoiceoverActive = vos->is_any_subtitle_active();
		}
		else
		{
			newVoiceoverActive = false;
		}
		if (voiceoverActive != newVoiceoverActive)
		{
			voiceoverActive = newVoiceoverActive;
			redrawNow = true;
		}
	}

	{
		bool newSubtitlesOn = SubtitleSystem::withSubtitles;
		if (subtitlesOn != newSubtitlesOn)
		{
			subtitlesOn = newSubtitlesOn;
			redrawNow = true;
		}
	}

	if (redrawNow)
	{
		updateEmissiveOnce = true;
	}
	if (updateEmissiveOnce)
	{
		updateEmissiveOnce = false;
		update_emissive(subtitlesOn);
	}

	if (! display)
	{
		if (auto* mind = get_mind())
		{
			if (auto* imo = mind->get_owner_as_modules_owner())
			{
				if (auto* displayModule = imo->get_custom<::Framework::CustomModules::Display>())
				{
					display = displayModule->get_display();

					if (display)
					{
						display->always_draw_commands_immediately();
						display->set_on_update_display(this,
						[this](Framework::Display* _display)
						{
							if (!redrawNow)
							{
								return;
							}
							redrawNow = false;

							_display->drop_all_draw_commands();

							Colour drawColour = subtitlesOn? subtitleDeviceData->colourOn : subtitleDeviceData->colourOff;
							bool inverse = subtitlesOn ? subtitleDeviceData->inverseOn : subtitleDeviceData->inverseOff;
							Colour backgroundColour = Colour::black;

							if (!voiceoverActive)
							{
								inverse = false;
							}

							_display->add((new Framework::DisplayDrawCommands::Border(inverse? drawColour : backgroundColour)));
							_display->add((new Framework::DisplayDrawCommands::CLS(inverse? drawColour : backgroundColour)));

							if (voiceoverActive)
							{
								_display->set_current_ink(inverse ? backgroundColour : drawColour);
								_display->set_current_paper(inverse ? drawColour : backgroundColour);
								_display->add((new Framework::DisplayDrawCommands::TextAtMultiline())
									->at(Framework::CharCoords(0, _display->get_char_resolution().y - 1))
									->limit(_display->get_char_resolution().x)
									->in_region(Framework::RangeCharCoord2(RangeInt(0, _display->get_char_resolution().x - 1), RangeInt(0, _display->get_char_resolution().y - 1)))
									->text(TXT("SUBTITLES"))
									);
							}

							_display->always_draw_commands_immediately();
						});
					}
				}
			}
		}
	}
}

void SubtitleDevice::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

void SubtitleDevice::update_emissive(bool _subtitlesOn)
{
	ai_log(this, TXT("update emissive [%S]"), _subtitlesOn ? TXT("on") : TXT("off"));

	if (button.get())
	{
		if (auto* ec = button->get_custom<CustomModules::EmissiveControl>())
		{
			if (voiceoverActive)
			{
				if (_subtitlesOn)
				{
					ec->emissive_activate(NAME(active));
					ec->emissive_deactivate(NAME(ready));
				}
				else
				{
					ec->emissive_deactivate(NAME(active));
					ec->emissive_activate(NAME(ready));
				}
			}
			else
			{
				ec->emissive_deactivate(NAME(active));
				ec->emissive_deactivate(NAME(ready));
			}
		}
	}
}

void SubtitleDevice::switch_subtitles(Optional<bool> _subtitlesOn)
{
	if (!_subtitlesOn.is_set())
	{
		_subtitlesOn = !subtitlesOn;
	}
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		auto& p = ps->access_preferences();
		p.subtitles = _subtitlesOn.get();
		p.apply();
	}
	redrawNow = true;
}

LATENT_FUNCTION(SubtitleDevice::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai subtitle device] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(bool, buttonPressed);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<SubtitleDevice>(logic);

	LATENT_BEGIN_CODE();

	buttonPressed = false;

	ai_log(self, TXT("subtitle device, hello!"));

	if (imo)
	{
		ai_log(self, TXT("get button"));

#ifdef AN_USE_AI_LOG
		if (auto* id = imo->get_variables().get_existing<int>(NAME(interactiveDeviceId)))
		{
			ai_log(self, TXT("id = %i"), *id);
		}
#endif

		while (!self->button.get())
		{
			if (auto* id = imo->get_variables().get_existing<int>(NAME(interactiveDeviceId)))
			{
				ai_log(self, TXT("id = %i"), *id);
				auto* world = imo->get_in_world();
				an_assert(world);

				world->for_every_object_with_id(NAME(interactiveDeviceId), *id,
					[self, imo](Framework::Object* _object)
					{
						if (_object != imo)
						{
							self->button = _object;
							return true; // one is enough
						}
						return false; // keep going on
					});
			}
			LATENT_WAIT(Random::get_float(0.2f, 0.5f));
		}

		ai_log(self, TXT("button in room accessed"));

		if (self->button.get())
		{
			if (auto* ec = self->button->get_custom<CustomModules::EmissiveControl>())
			{
				{
					auto& layer = ec->emissive_access(NAME(active));
					layer.set_colour(self->subtitleDeviceData->colourOn);
				}
				{
					auto& layer = ec->emissive_access(NAME(ready));
					layer.set_colour(self->subtitleDeviceData->colourOff);
				}
			}
		}

		self->update_emissive(self->subtitlesOn);
	}

	while (true)
	{
		if (self->voiceoverActive)
		{
			bool newButtonPressed = false;
			if (self->button.get())
			{
				if (auto* mms = fast_cast<Framework::ModuleMovementSwitch>(self->button->get_movement()))
				{
					if (mms->is_active_at(1))
					{
						newButtonPressed = true;
					}
				}
			
				if (newButtonPressed && !buttonPressed)
				{
					self->switch_subtitles();
				}
			}

			buttonPressed = newButtonPressed;
		}
		LATENT_WAIT(Random::get_float(0.1f, 0.2f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(SubtitleDeviceData);

bool SubtitleDeviceData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	colourOff.load_from_xml_child_node(_node, TXT("colourOff"));
	colourOn.load_from_xml_child_node(_node, TXT("colourOn"));
	inverseOff = _node->get_bool_attribute_or_from_child_presence(TXT("inverseOff"), inverseOff);
	inverseOn = _node->get_bool_attribute_or_from_child_presence(TXT("inverseOn"), inverseOn);

	return result;
}

bool SubtitleDeviceData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
