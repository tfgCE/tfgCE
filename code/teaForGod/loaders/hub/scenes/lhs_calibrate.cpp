#include "lhs_calibrate.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_customDraw.h"
#include "..\widgets\lhw_image.h"
#include "..\widgets\lhw_text.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\playerSetup.h"
#include "..\..\..\roomGenerators\roomGenerationInfo.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// localised strings
DEFINE_STATIC_NAME_STR(lsCalibrateInfo, TXT("hub; calibrate; info"));
DEFINE_STATIC_NAME_STR(lsCalibrateHeight, TXT("hub; calibrate; height"));
DEFINE_STATIC_NAME_STR(lsCalibrateDoorHeight, TXT("hub; calibrate; door height"));
DEFINE_STATIC_NAME_STR(lsCalibrateCurrentDoorHeight, TXT("hub; calibrate; current door height"));
DEFINE_STATIC_NAME_STR(lsCalibrate, TXT("hub; calibrate; calibrate"));
DEFINE_STATIC_NAME_STR(lsGoBack, TXT("hub; common; back"));

// screens
DEFINE_STATIC_NAME(calibrate);
DEFINE_STATIC_NAME(hubBackground);

// ids
DEFINE_STATIC_NAME(idHeight);
DEFINE_STATIC_NAME(idDoorHeight);
DEFINE_STATIC_NAME(idCurrentDoorHeight);

//

using namespace Loader;
using namespace HubScenes;

//

float const Calibrate::max_distance = 0.15f;

REGISTER_FOR_FAST_CAST(Calibrate);

Calibrate::Calibrate()
{
}

Calibrate::~Calibrate()
{
}

void Calibrate::go_back()
{
	Array<Name> screensToKeep;
	screensToKeep.push_back(NAME(hubBackground));
	//
	get_hub()->keep_only_screens(screensToKeep);

	get_hub()->set_scene(prevScene.get());
}

void Calibrate::on_activate(HubScene* _prev)
{
	if (!prevScene.is_set())
	{
		prevScene = _prev;
	}

	base::on_activate(_prev);

	if (auto* vr = VR::IVR::get())
	{
		output(TXT("play area info on calibrate"));
		vr->output_play_area();
	}

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	Array<Name> screensToKeep;
	screensToKeep.push_back(NAME(hubBackground));
	screensToKeep.push_back(NAME(calibrate));
	//
	get_hub()->keep_only_screens(screensToKeep);

	show_screen();
}

void Calibrate::show_screen()
{
	Vector2 ppa(24.0f, 24.0f);
	float sizeCoef = 1.0f * (12.0f / 10.0f);

	bool fill = false;
	{
		auto* scr = get_hub()->get_screen(NAME(calibrate));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(calibrate), Vector2(35.0f * sizeCoef, 80.0f * sizeCoef), get_hub()->get_current_forward(), get_hub()->get_radius() * 0.65f, true, NP, ppa);
			scr->followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER;
			get_hub()->add_screen(scr);
			fill = true;
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			Array<HubScreenOption> options;
			options.make_space_for(20);

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsCalibrateInfo), nullptr, HubScreenOption::Header));

			options.push_back(HubScreenOption());

			options.push_back(HubScreenOption(NAME(idHeight), NAME(lsCalibrateHeight), nullptr, HubScreenOption::Text));
			options.push_back(HubScreenOption(NAME(idDoorHeight), NAME(lsCalibrateDoorHeight), nullptr, HubScreenOption::Text));
			options.push_back(HubScreenOption(NAME(idCurrentDoorHeight), NAME(lsCalibrateCurrentDoorHeight), nullptr, HubScreenOption::Text));

			scr->add_option_widgets(options, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f, scr->mainResolutionInPixels.y.max - max(fs.x, fs.y) * 1.0f)));

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsCalibrate), [this]() { store_calibration(); go_back(); }).activate_on_trigger_hold());
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsGoBack), [this]() { go_back(); }));

			scr->add_button_widgets(buttons, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y) * 6.0f), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 1.0f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 3.0f)), max(fs.x, fs.y));

			scr->compress_vertically();
		}
	}
}

void Calibrate::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);

	calibrate();
}

void Calibrate::calibrate()
{
	doorHeight = TeaForGodEmperor::PlayerPreferences::get_door_height_from_eye_level(&height);
	float currentDoorHeight = TeaForGodEmperor::PlayerSetup::get_current().get_preferences().doorHeight;

	if (auto* scr = get_hub()->get_screen(NAME(calibrate)))
	{
		MeasurementSystem::Type ms = TeaForGodEmperor::PlayerSetup::get_current().get_preferences().get_measurement_system();
		if (auto* w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idHeight))))
		{
			if (ms == MeasurementSystem::Imperial)
			{
				w->set(MeasurementSystem::as_feet_inches(height));
			}
			else
			{
				w->set(MeasurementSystem::as_meters(height));
			}
		}
		if (auto* w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idDoorHeight))))
		{
			if (ms == MeasurementSystem::Imperial)
			{
				w->set(MeasurementSystem::as_feet_inches(doorHeight));
			}
			else
			{
				w->set(MeasurementSystem::as_meters(doorHeight));
			}
		}
		if (auto* w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idCurrentDoorHeight))))
		{
			if (ms == MeasurementSystem::Imperial)
			{
				w->set(MeasurementSystem::as_feet_inches(currentDoorHeight));
			}
			else
			{
				w->set(MeasurementSystem::as_meters(currentDoorHeight));
			}
		}
	}
}

void Calibrate::store_calibration()
{
	if (auto* scr = get_hub()->get_screen(NAME(calibrate)))
	{
		if (scr->is_active())
		{
			auto& psp = TeaForGodEmperor::PlayerSetup::access_current().access_preferences();
			psp.doorHeight = doorHeight;
			psp.apply_to_vr_and_room_generation_info(); // will set vr scaling too
			if (auto* vr = VR::IVR::get())
			{
				output(TXT("play area info calibrated"));
				vr->output_play_area();
			}
			if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
			{
				game->update_hub_scene_meshes();
			}
			else
			{
				get_hub()->update_scene_mesh();
			}
		}
	}
}
