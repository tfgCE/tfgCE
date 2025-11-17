#include "loaderArmActivator.h"

#include "..\..\framework\display\display.h"
#include "..\..\framework\display\displayUtils.h"
#include "..\..\framework\display\displayDrawCommands.h"
#include "..\library\library.h"

#include "..\..\core\system\input.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\vr\iVR.h"

#include "..\..\framework\library\usedLibraryStored.inl"

using namespace Loader;

ArmActivator::ArmActivator()
{
	displayObject = new Framework::Display();
	displayObject->ignore_game_frames();

	DEFINE_STATIC_NAME_STR(mansionOfGeorge, TXT("mansion of george"));
	DEFINE_STATIC_NAME(display);
	Framework::DisplayType * displayType = ::Framework::Library::get_current()->get_display_types().find(::Framework::LibraryName(NAME(mansionOfGeorge), NAME(display)));
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

	displayObject->always_draw_commands_immediately();

	show_wait();

	// play background sample
	if (Framework::Sample * sample = displayType->get_background_sound_sample())
	{
		loadingSamplePlayback = sample->get_sample()->play(0.2f);
	}
}

ArmActivator::~ArmActivator()
{
	delete displayObject;
}

int ArmActivator::show_start()
{
	VectorInt2 charRes = displayObject->get_char_resolution();
	int y = charRes.y * 3 / 4;
	int length = charRes.x;
	Framework::DisplayUtils::clear_all(displayObject);
	displayObject->add((new Framework::DisplayDrawCommands::TextAt())->text(TXT("TEA FOR GOD"))->at(0, y)->length(length)->h_align_centre_fine());
	y -= 1;
	displayObject->add((new Framework::DisplayDrawCommands::TextAt())->text(TXT("demo"))->at(0, y)->length(length)->h_align_centre_fine());
	y -= 6;
	return y;
}

int ArmActivator::show_wait()
{
	int y = show_start();
	VectorInt2 charRes = displayObject->get_char_resolution();
	int length = charRes.x;
	displayObject->add((new Framework::DisplayDrawCommands::TextAt())->text(TXT("Wait"))->at(0, y)->length(length)->h_align_centre_fine());
	y -= 3;
	displayObject->add((new Framework::DisplayDrawCommands::TextAt())->text(TXT("Poczekaj"))->at(0, y)->length(length)->h_align_centre_fine());
	return y;
}

int ArmActivator::show_point_at_screen()
{
	int y = show_start();
	VectorInt2 charRes = displayObject->get_char_resolution();
	int length = charRes.x;
	displayObject->add((new Framework::DisplayDrawCommands::TextAt())->text(TXT("Point at screen"))->at(0, y)->length(length)->h_align_centre_fine());
	y -= 3;
	displayObject->add((new Framework::DisplayDrawCommands::TextAt())->text(TXT("Wyceluj w ekran"))->at(0, y)->length(length)->h_align_centre_fine());
	return y;
}

int ArmActivator::show_pull_trigger()
{
	int y = show_start();
	VectorInt2 charRes = displayObject->get_char_resolution();
	int length = charRes.x;
	displayObject->add((new Framework::DisplayDrawCommands::TextAt())->text(TXT("Pull trigger"))->at(0, y)->length(length)->h_align_centre_fine());
	y -= 3;
	displayObject->add((new Framework::DisplayDrawCommands::TextAt())->text(TXT("Pociagnij za spust"))->at(0, y)->length(length)->h_align_centre_fine());
	return y;
}

void ArmActivator::deactivate()
{
	// do not actually deactivate, just let the player know that we can now deactivate
	if (!allowDeactivating)
	{
		show_point_at_screen();
	}
	allowDeactivating = true;
}

void ArmActivator::update(float _deltaTime)
{
	displayObject->advance(_deltaTime);
	if (allowDeactivating)
	{
		bool exit = false;
#ifdef AN_DEVELOPMENT
		exit = true;
#endif
		if (VR::IVR::can_be_used())
		{
			bool newPointingAtScreen = false;
			Transform const & deviceMesh3DPlacement = displayObject->get_vr_mesh_placement();
			bool pointing[2];
			for_count(int, handIdx, 2)
			{
				pointing[handIdx] = false;
				if (VR::IVR::get()->get_controls_pose_set().hands[handIdx].placement.is_set())
				{
					todo_implement(TXT("changed from removed pointerPlacement to placement"));
					Transform handPose = VR::IVR::get()->get_controls_pose_set().hands[handIdx].placement.get();
					Vector3 doDir = (deviceMesh3DPlacement.get_translation() * Vector3(1.0f, 1.0f, 0.0f)).normal();
#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
					Vector3 handDir = handPose.vector_to_world(Vector3(0.0f, 1.0f, 0.33f));
#endif
#endif
					float atDO = Vector3::dot(handPose.vector_to_world(Vector3(0.0f, 1.0f, -0.33f)), doDir);
					if (atDO > 0.8f)
					{
						pointing[handIdx] = true;
						newPointingAtScreen = true;
					}
				}
			}

			if (pointingAtScreen ^ newPointingAtScreen)
			{
				pointingAtScreen = newPointingAtScreen;
				if (pointingAtScreen)
				{
					show_pull_trigger();
				}
				else
				{
					show_point_at_screen();
				}
			}
			if (pointingAtScreen)
			{
				for_count(int, handIdx, 2)
				{
					if (pointing[handIdx])
					{
						{
							Optional<bool> trigger = VR::IVR::get()->get_controls().hands[handIdx].is_button_pressed(VR::Input::Button::Trigger);
							if (trigger.is_set())
							{
								if (trigger.get())
								{
									exit = true;
								}
							}
						}
						{
							Optional<bool> pointerPinch = VR::IVR::get()->get_controls().hands[handIdx].is_button_pressed(VR::Input::Button::PointerPinch);
							if (pointerPinch.is_set())
							{
								if (pointerPinch.get())
								{
									exit = true;
								}
							}
						}
					}
				}
			}
		}
#ifdef AN_STANDARD_INPUT
		exit |= System::Input::get()->is_key_pressed(System::Key::Space);
#endif
		if (exit)
		{
			loadingSamplePlayback.fade_out(0.1f);
			base::deactivate();
		}
	}
}

void ArmActivator::display(System::Video3D * _v3d, bool _vr)
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
