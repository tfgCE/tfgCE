#include "lhs_comfortMessageAndBasicOptions.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\widgets\lhw_text.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\playerSetup.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// screen id
DEFINE_STATIC_NAME(screen_comfortMessageAndBasicOptions);

// widget ids
DEFINE_STATIC_NAME(button_subtitles);
DEFINE_STATIC_NAME(button_flickering);
DEFINE_STATIC_NAME(button_cameraRumble);
DEFINE_STATIC_NAME(button_crouch);
DEFINE_STATIC_NAME(button_rotatingElevators);
DEFINE_STATIC_NAME(button_vignetteForMovement);
DEFINE_STATIC_NAME(button_immobileVR);
DEFINE_STATIC_NAME(button_continue);

// localised strings
DEFINE_STATIC_NAME_STR(lsComfortMessageAndBasicOptionsTop, TXT("hub; cmf.msg+bsc.opt; top"));
DEFINE_STATIC_NAME_STR(lsComfortMessageAndBasicOptionsBottom, TXT("hub; cmf.msg+bsc.opt; bottom"));
DEFINE_STATIC_NAME_STR(lsContinue, TXT("hub; cmf.msg+bsc.opt; continue"));
DEFINE_STATIC_NAME_STR(lsContinueGameIsLoading, TXT("hub; cmf.msg+bsc.opt; continue; game is loading"));
DEFINE_STATIC_NAME_STR(lsSubtitlesOn, TXT("hub; cmf.msg+bsc.opt; subtitles; on"));
DEFINE_STATIC_NAME_STR(lsSubtitlesOff, TXT("hub; cmf.msg+bsc.opt; subtitles; off"));
DEFINE_STATIC_NAME_STR(lsFlickeringLightsOn, TXT("hub; cmf.msg+bsc.opt; flickering lights; on"));
DEFINE_STATIC_NAME_STR(lsFlickeringLightsOff, TXT("hub; cmf.msg+bsc.opt; flickering lights; off"));
DEFINE_STATIC_NAME_STR(lsCameraRumbleOn, TXT("hub; cmf.msg+bsc.opt; camera rumble; on"));
DEFINE_STATIC_NAME_STR(lsCameraRumbleOff, TXT("hub; cmf.msg+bsc.opt; camera rumble; off"));
DEFINE_STATIC_NAME_STR(lsCrouchOn, TXT("hub; cmf.msg+bsc.opt; crouch; on"));
DEFINE_STATIC_NAME_STR(lsCrouchOff, TXT("hub; cmf.msg+bsc.opt; crouch; off"));
DEFINE_STATIC_NAME_STR(lsRotatingElevatorsOn, TXT("hub; cmf.msg+bsc.opt; rotating elevators; on"));
DEFINE_STATIC_NAME_STR(lsRotatingElevatorsOff, TXT("hub; cmf.msg+bsc.opt; rotating elevators; off"));
DEFINE_STATIC_NAME_STR(lsVignetteForMovementOn, TXT("hub; cmf.msg+bsc.opt; vignette; on"));
DEFINE_STATIC_NAME_STR(lsVignetteForMovementOff, TXT("hub; cmf.msg+bsc.opt; vignette; off"));
DEFINE_STATIC_NAME_STR(lsImmobileVROn, TXT("hub; cmf.msg+bsc.opt; im.vr; on"));
DEFINE_STATIC_NAME_STR(lsImmobileVROff, TXT("hub; cmf.msg+bsc.opt; im.vr; off"));
DEFINE_STATIC_NAME_STR(lsImmobileVRAuto, TXT("hub; cmf.msg+bsc.opt; im.vr; auto"));
DEFINE_STATIC_NAME_STR(lsImmobileVRAutoOn, TXT("hub; cmf.msg+bsc.opt; im.vr; auto; on"));
DEFINE_STATIC_NAME_STR(lsImmobileVRAutoOff, TXT("hub; cmf.msg+bsc.opt; im.vr; auto; off"));

//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(ComfortMessageAndBasicOptions);

ComfortMessageAndBasicOptions::ComfortMessageAndBasicOptions(float _delay)
: delay(_delay)
{
}

void ComfortMessageAndBasicOptions::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	okToEnd = false;
	timeToShow = delay;

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	get_hub()->deactivate_all_screens();

	if (timeToShow <= 0.0f)
	{
		show();
	}
}

void ComfortMessageAndBasicOptions::show()
{
	get_hub()->deactivate_all_screens();

	get_hub()->force_reset_and_update_forward();

	Vector2 fs = HubScreen::s_fontSizeInPixels;
	float fsxy = max(fs.x, fs.y);

	Vector2 size(35.0f, 50.0f);
	Vector2 ppa(22.0f, 22.0f);
	Rotator3 atDir = get_hub()->get_current_forward();
	Rotator3 verticalOffset(0.0f, 0.0f, 0.0f);

	auto* screen = new HubScreen(get_hub(), NAME(screen_comfortMessageAndBasicOptions), size, atDir, get_hub()->get_radius() * 0.5f, NP, NP, ppa);
	screen->activate();
	get_hub()->add_screen(screen);

	Vector2 spacing = Vector2::one * fsxy;
	Range2 at = screen->mainResolutionInPixels.expanded_by(-spacing);

	// show screen with a message
	{
		Range2 eat = at;
		auto* e = new HubWidgets::Text(Name::invalid(), eat, LOC_STR(NAME(lsComfortMessageAndBasicOptionsTop)));
		e->alignPt.x = 0.0f;
		e->alignPt.y = 1.0f;
		screen->add_widget(e);
		e->at.y.min = e->at.y.max - e->calculate_vertical_size();

		at.y.max = e->at.y.min - spacing.y;
	}

	// add buttons to change settings
	if (TeaForGodEmperor::PlayerSetup::access_current_if_exists())
	{
		Array<HubScreenButtonInfo> buttons;
		{
			/*
			// this is handled in the arrival sequence
			buttons.push_back(HubScreenButtonInfo(NAME(button_subtitles), NAME(lsSubtitlesOn), [this]()
				{
					auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
					auto& pp = ps.access_preferences();
					pp.subtitles = !pp.subtitles;
					update_buttons();
				}));
			*/
			/*
			// separate step
			buttons.push_back(HubScreenButtonInfo(NAME(button_immobileVR), NAME(lsImmobileVROn), [this]()
				{
					auto& mc = MainConfig::access_global();
					mc.set_immobile_vr(mc.get_immobile_vr() == Option::Auto ? Option::True : (mc.get_immobile_vr() == Option::True ? Option::False : Option::Auto));
					if (auto* vr = VR::IVR::get())
					{
						vr->reset_immobile_origin_in_vr_space();
					}
					get_hub()->deactivate_all_screens();
					timeToShow = 0.1f; // give it a bit to update vr poses, so we're at the right spot
				}));
			*/
			buttons.push_back(HubScreenButtonInfo(NAME(button_flickering), NAME(lsFlickeringLightsOn), [this]()
				{
					auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
					auto& pp = ps.access_preferences();
					pp.flickeringLights = pp.flickeringLights == Option::Allow? Option::Disallow : Option::Allow;
					update_buttons();
				}));
			/*
			buttons.push_back(HubScreenButtonInfo(NAME(button_cameraRumble), NAME(lsCameraRumbleOn), [this]()
				{
					auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
					auto& pp = ps.access_preferences();
					pp.cameraRumble = pp.cameraRumble == Option::Allow? Option::Disallow : Option::Allow;
					update_buttons();
				}));
			*/
			buttons.push_back(HubScreenButtonInfo(NAME(button_crouch), NAME(lsCrouchOn), [this]()
				{
					auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
					auto& pp = ps.access_preferences();
					if (pp.allowCrouch == Option::Disallow) { pp.allowCrouch = Option::Auto; } else
															{ pp.allowCrouch = Option::Disallow; }
					update_buttons();
				}));
			buttons.push_back(HubScreenButtonInfo(NAME(button_rotatingElevators), NAME(lsRotatingElevatorsOn), [this]()
				{
					auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
					auto& pp = ps.access_preferences();
					pp.rotatingElevators = pp.rotatingElevators == Option::Allow ? Option::Disallow : Option::Allow;
					pp.apply_to_game_tags();
					update_buttons();
				}));
			buttons.push_back(HubScreenButtonInfo(NAME(button_vignetteForMovement), NAME(lsVignetteForMovementOff), [this]()
				{
					auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
					auto& pp = ps.access_preferences();
					pp.useVignetteForMovement = pp.useVignetteForMovement == 0.0f ? pp.vignetteForMovementStrength : 0.0f;
					update_buttons();
				}));
			Range2 bat = at;
			bat.y.min = bat.y.max - ((fs.y * 3.0f) * (float)buttons.get_size() + spacing.y * (float)(buttons.get_size() - 1));
			screen->add_button_widgets_grid(buttons, VectorInt2(1, buttons.get_size()), bat, spacing);

			at.y.max = bat.y.min - spacing.y;
		}

		update_buttons();
	}

	{
		Range2 eat = at;
		auto* e = new HubWidgets::Text(Name::invalid(), eat, LOC_STR(NAME(lsComfortMessageAndBasicOptionsBottom)));
		e->alignPt.x = 1.0f;
		e->alignPt.y = 1.0f;
		screen->add_widget(e);
		e->at.y.min = e->at.y.max - e->calculate_vertical_size();

		at.y.max = e->at.y.min - spacing.y;
	}

	// confirm and continue button
	{
		Array<HubScreenButtonInfo> buttons;
		{
			buttons.push_back(HubScreenButtonInfo(NAME(button_continue), showGameIsLoading? NAME(lsContinueGameIsLoading) : NAME(lsContinue), [this]() { okToEnd = true; get_hub()->deactivate_all_screens(); }).activate_on_trigger_hold());

			Range2 bat = at;
			bat.y.min = bat.y.max - ((fs.y * 5.0f) * (float)buttons.get_size() + spacing.y * (float)(buttons.get_size() - 1));
			screen->add_button_widgets_grid(buttons, VectorInt2(1, buttons.get_size()), bat, spacing);

			at.y.max = bat.y.min - spacing.y;
		}
	}

	screen->compress_vertically();
}

void ComfortMessageAndBasicOptions::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);
	if (timeToShow > 0.0f && !okToEnd)
	{
		timeToShow -= _deltaTime;
		if (timeToShow <= 0.0f)
		{
			show();
		}
	}

	if (showGameIsLoading)
	{
		if (auto* g = TeaForGodEmperor::Game::get())
		{
			if (!g->is_loading_library())
			{
				showGameIsLoading = false;
				if (auto* screen = get_hub()->get_screen(NAME(screen_comfortMessageAndBasicOptions)))
				{
					if (auto* t = fast_cast<HubWidgets::Text>(screen->get_widget(NAME(button_continue))))
					{
						t->set(NAME(lsContinue));
					}
				}
			}
		}
	}
}

void ComfortMessageAndBasicOptions::on_deactivate(HubScene* _next)
{
	if (okToEnd)
	{
		base::on_deactivate(_next);
	}
}

void ComfortMessageAndBasicOptions::on_loader_deactivate()
{
	base::on_loader_deactivate();
}

void ComfortMessageAndBasicOptions::update_buttons()
{
	if (auto* screen = get_hub()->get_screen(NAME(screen_comfortMessageAndBasicOptions)))
	{
		if (TeaForGodEmperor::PlayerSetup::access_current_if_exists())
		{
			auto& mc = MainConfig::global();
			auto& ps = TeaForGodEmperor::PlayerSetup::get_current();
			auto& pp = ps.get_preferences();
			if (auto* t = fast_cast<HubWidgets::Text>(screen->get_widget(NAME(button_subtitles))))
			{
				t->set(pp.subtitles ? NAME(lsSubtitlesOn) : NAME(lsSubtitlesOff));
			}
			if (auto* t = fast_cast<HubWidgets::Text>(screen->get_widget(NAME(button_flickering))))
			{
				t->set(pp.flickeringLights == Option::Allow ? NAME(lsFlickeringLightsOn) : NAME(lsFlickeringLightsOff));
			}
			if (auto* t = fast_cast<HubWidgets::Text>(screen->get_widget(NAME(button_cameraRumble))))
			{
				t->set(pp.flickeringLights == Option::Allow ? NAME(lsCameraRumbleOn) : NAME(lsCameraRumbleOff));
			}
			if (auto* t = fast_cast<HubWidgets::Text>(screen->get_widget(NAME(button_crouch))))
			{
				t->set(pp.allowCrouch == Option::Disallow ? NAME(lsCrouchOff) : NAME(lsCrouchOn));
			}
			if (auto* t = fast_cast<HubWidgets::Text>(screen->get_widget(NAME(button_rotatingElevators))))
			{
				t->set(pp.rotatingElevators == Option::Disallow ? NAME(lsRotatingElevatorsOff) : NAME(lsRotatingElevatorsOn));
			}
			if (auto* t = fast_cast<HubWidgets::Text>(screen->get_widget(NAME(button_vignetteForMovement))))
			{
				t->set(pp.useVignetteForMovement == 0.0f ? NAME(lsVignetteForMovementOff) : NAME(lsVignetteForMovementOn));
			}
			if (auto* t = fast_cast<HubWidgets::Text>(screen->get_widget(NAME(button_immobileVR))))
			{
				t->set(mc.get_immobile_vr() == Option::True ? NAME(lsImmobileVROn) : (mc.get_immobile_vr() == Option::False ? NAME(lsImmobileVROff) : (mc.get_immobile_vr_auto()? NAME(lsImmobileVRAutoOn) : NAME(lsImmobileVRAutoOff))));
			}
		}
	}
}
