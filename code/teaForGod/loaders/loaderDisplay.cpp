#include "loaderDisplay.h"

#include "..\..\framework\display\display.h"
#include "..\..\framework\display\displayDrawCommands.h"
#include "..\library\library.h"

#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\vr\iVR.h"

#include "..\..\framework\library\usedLibraryStored.inl"

using namespace Loader;

Display::Display()
: timeToDeactivation(0.0f)
{
	DEFINE_STATIC_NAME_STR(lsLoading, TXT("Loading loading"));
	DEFINE_STATIC_NAME_STR(lsGameTitle, TXT("Loading game title"));
	DEFINE_STATIC_NAME_STR(lsWhoWhen, TXT("Loading who when"));

	displayObject = new Framework::Display();
	displayObject->ignore_game_frames();

	DEFINE_STATIC_NAME(displays);
	DEFINE_STATIC_NAME(loading);
	Framework::DisplayType * displayType = ::Framework::Library::get_current()->get_display_types().find(::Framework::LibraryName(NAME(displays), NAME(loading)));
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

	DEFINE_STATIC_NAME(title);

	VectorInt2 charRes = displayObject->get_char_resolution();
	int y = charRes.y * 3 / 4;
	int length = charRes.x;
	displayObject->add((new Framework::DisplayDrawCommands::TextAt())->text(LOC_STR(NAME(lsLoading)))->at(0, y)->length(length)->h_align_centre());
	y -= 2;
	displayObject->add((new Framework::DisplayDrawCommands::TextAt())->text(LOC_STR(NAME(lsGameTitle)))->at(0, y)->length(length)->h_align_centre()->use_colour_pair(NAME(title)));
	y -= 4;
	displayObject->add((new Framework::DisplayDrawCommands::TextAt())->text(LOC_STR(NAME(lsWhoWhen)))->at(0, y)->length(length)->h_align_centre());

	// play background sample
	if (Framework::Sample * sample = displayType->get_background_sound_sample())
	{
		loadingSamplePlayback = sample->get_sample()->play(0.2f);
	}
}

Display::~Display()
{
	delete displayObject;
}

void Display::deactivate()
{
	timeToDeactivation = 1.0f;
	displayObject->turn_off();
	loadingSamplePlayback.fade_out(0.1f);
}

void Display::update(float _deltaTime)
{
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

void Display::display(System::Video3D * _v3d, bool _vr)
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
			displayObject->render_vr_scene(_v3d, &displayVRSceneContext);
			::System::RenderTarget::bind_none();
			VR::IVR::get()->copy_render_to_output(_v3d);
		}
	}
}
