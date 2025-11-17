#include "loaderPlatformerGame.h"

#include "..\..\framework\display\display.h"
#include "..\..\framework\display\displayDrawCommands.h"
#include "..\library\library.h"

#include "..\..\core\mainConfig.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\vr\iVR.h"

#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\minigames\platformer\platformerGame.h"
#include "..\..\framework\minigames\platformer\platformerInfo.h"

using namespace Loader;

PlatformerGame::PlatformerGame()
{
	displayObject = new Framework::Display();
	displayObject->ignore_game_frames();

	DEFINE_STATIC_NAME_STR(mansionOfGeorge, TXT("mansion of george"));
	DEFINE_STATIC_NAME(display);
	Framework::DisplayType* displayType = ::Framework::Library::get_current()->get_display_types().find(::Framework::LibraryName(NAME(mansionOfGeorge), NAME(display)));
	if (displayType)
	{
		displayObject->setup_with(displayType);
	}
	else
	{
		Framework::DisplaySetup displaySetup;
		displaySetup.setup_resolution_with_borders(VectorInt2(24 * 8, 16 * 8), VectorInt2(6, 5));
		displaySetup.refreshFrequency = 5.0f;
		displaySetup.retraceVisibleFrequency = 1.0f;
		displaySetup.drawCyclesPerSecond = 10000; // to draw everything in first pass
		displaySetup.postProcessParameters.access<float>(Name(String(TXT("retracePower")))) = 0.015f;
		displaySetup.postProcessParameters.access<float>(Name(String(TXT("alphaRetrace")))) = 0.05f;
		displayObject->use_setup(displaySetup);
	}

	if (auto* vr = VR::IVR::get())
	{
		displayObject->setup_output_size_max(vr->get_render_render_target(VR::Eye::Left)->get_size(), 1.0f);
	}
	else
	{
		displayObject->setup_output_size_max(::System::Video3D::get()->get_screen_size(), 1.0f);
	}
	displayObject->setup_output_size_limit(VectorInt2(1200, 1200));
	displayObject->init(::Framework::Library::get_current());

	::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(displayObject->get_setup().wholeDisplayResolution,
		MainConfig::global().get_video().get_msaa_samples());
	DEFINE_STATIC_NAME(colour);
	// we use ordinary rgb 8bit textures
	rtSetup.add_output_texture(::System::OutputTextureDefinition(NAME(colour),
		::System::VideoFormat::rgba8,
		::System::BaseVideoFormat::rgba));

	mainRT = new ::System::RenderTarget(rtSetup);

	// output from game is presented as background
	displayObject->use_background(mainRT.get(), 0);
	displayObject->set_background_fill_mode(Framework::DisplayBackgroundFillMode::Whole);

	// clear to nothing
	clear_display();
	displayObject->draw_all_commands_immediately();

	// play background sample
	if (Framework::Sample * sample = displayType->get_background_sound_sample())
	{
		loadingSamplePlayback = sample->get_sample()->play(0.2f);
	}

	DEFINE_STATIC_NAME(loadingMiniGame);
	loadingMiniGameInput.use(NAME(loadingMiniGame));
	loadingMiniGameInput.use(Framework::GameInputIncludeVR::All);

#ifdef AN_MINIGAME_PLATFORMER
	// load game
	{
		DEFINE_STATIC_NAME_STR(mansionOfGeorge, TXT("mansion of george"));
		DEFINE_STATIC_NAME(info);
		if (Platformer::Info* info = ::Framework::Library::get_current()->get_minigame_platformer_infos().find(::Framework::LibraryName(NAME(mansionOfGeorge), NAME(info))))
		{
			game = new Platformer::Game(info, displayObject->get_setup().wholeDisplayResolution);
			
			DEFINE_STATIC_NAME_STR(lsMinigameStarted, TXT("Platformer minigame loader; minigame started"));
			display_text(LOC_STR(NAME(lsMinigameStarted)));
			timeToClear = 3.0f;
		}
	}
#endif
}

PlatformerGame::~PlatformerGame()
{
#ifdef AN_MINIGAME_PLATFORMER
	delete_and_clear(game);
#endif
	mainRT = nullptr;
	delete_and_clear(displayObject);
}

void PlatformerGame::clear_display()
{
	displayObject->add((new Framework::DisplayDrawCommands::CLS(Colour::none)));
	displayObject->add((new Framework::DisplayDrawCommands::Border(Colour::none)));
}

void PlatformerGame::display_text(String const & _text)
{
	clear_display();
	displayObject->add((new Framework::DisplayDrawCommands::TextAtMultiline())
		->in_region(Framework::RangeCharCoord2(Framework::RangeCharCoord(5, displayObject->get_char_resolution().x - 5), Framework::RangeCharCoord(0, 5)))
		->v_align_bottom()
		->compact()
		->h_align_centre()
		->text(_text));
}

void PlatformerGame::update_game_loaded_info()
{
	displayingGamepadWasActive = gamepadWasActive;
	if (VR::IVR::can_be_used())
	{
		DEFINE_STATIC_NAME_STR(lsGameLoaded, TXT("Platformer minigame loader; game loaded, vr"));
		display_text(LOC_STR(NAME(lsGameLoaded)));
	}
	else if (displayingGamepadWasActive)
	{
		DEFINE_STATIC_NAME_STR(lsGameLoaded, TXT("Platformer minigame loader; game loaded, gamepad"));
		display_text(LOC_STR(NAME(lsGameLoaded)));
	}
	else
	{
		DEFINE_STATIC_NAME_STR(lsGameLoaded, TXT("Platformer minigame loader; game loaded, keyboard"));
		display_text(LOC_STR(NAME(lsGameLoaded)));
	}
}

void PlatformerGame::deactivate()
{
	timeToClear = 0.0f;
	gameLoaded = true;
#ifdef AN_MINIGAME_PLATFORMER
	if (game && ! game->does_allow_to_auto_exit())
	{
		update_game_loaded_info();
	}
	else
#endif
	{
		switch_off();
	}
}

void PlatformerGame::switch_off()
{
	todo_future(TXT("add saving mini game"));
	if (switchedOff)
	{
		return;
	}
	switchedOff = true;
	timeToDeactivation = 0.5f;
	displayObject->turn_off();
	loadingSamplePlayback.fade_out(0.1f);
}

void PlatformerGame::update(float _deltaTime)
{
	if (timeToClear > 0.0f)
	{
		timeToClear -= _deltaTime;
		if (timeToClear <= 0.0f)
		{
			clear_display();
		}
	}
#ifdef AN_MINIGAME_PLATFORMER
	if (game)
	{
		game->advance(_deltaTime, mainRT.get());
	}
#endif

	if (::System::Input::get()->is_keyboard_active() ||
		::System::Input::get()->is_gamepad_active())
	{
		if (::System::Input::get()->is_gamepad_active())
		{
			gamepadWasActive = true;
		}
		if (::System::Input::get()->is_keyboard_active())
		{
			gamepadWasActive = false;
		}
	}

	if (gameLoaded)
	{
		if (displayingGamepadWasActive != gamepadWasActive)
		{
			update_game_loaded_info();
		}
		DEFINE_STATIC_NAME(end);
		if (loadingMiniGameInput.is_button_pressed(NAME(end)))
		{
			switch_off();
		}
#ifdef AN_MINIGAME_PLATFORMER
		else if (game && game->does_allow_to_auto_exit())
		{
			switch_off();
		}
#endif
	}

	displayObject->advance(_deltaTime);
	if (timeToDeactivation > 0.0f)
	{
		timeToDeactivation -= _deltaTime;
		if (timeToDeactivation <= 0.0f || displayObject->get_visible_power_on() < 0.1f)
		{
			base::deactivate();
		}
	}
}

void PlatformerGame::display(System::Video3D * _v3d, bool _vr)
{
	displayObject->update_display();

	System::RenderTarget::bind_none();
	_v3d->set_default_viewport();
	_v3d->set_near_far_plane(0.02f, 100.0f);
	_v3d->setup_for_2d_display();
	_v3d->clear_colour(Colour::black);

	// restore projection matrix
	_v3d->set_2d_projection_matrix_ortho();
	_v3d->access_model_view_matrix_stack().clear();

	if (auto* displayRT = displayObject->get_output_render_target())
	{
		_v3d->clear_colour(Colour::black);
		displayObject->render_2d(_v3d, _v3d->get_viewport_size().to_vector2() * Vector2::half, NP, 0.0f, System::BlendOp::One, System::BlendOp::OneMinusSrcAlpha);

		// render display as vr scene
		if (_vr)
		{
			displayObject->render_vr_scene(_v3d, &displayVRSceneContext, System::BlendOp::One, System::BlendOp::OneMinusSrcAlpha);
			VR::IVR::get()->copy_render_to_output(_v3d);
		}
	}
}
