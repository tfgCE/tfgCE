#include "editorGame.h"

#include "modes\gameMode_editor.h"

#include "..\game\gameMode.h"
#include "..\game\gameSceneLayer.h"
#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\loaders\loaderVoidRoom.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\debug\debugVisualiser.h"
#include "..\..\core\system\video\renderTarget.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(EditorGame);

EditorGame::EditorGame()
{
}

void EditorGame::create_library()
{
	if (create_library_func)
	{
		create_library_func();
	}
	else
	{
		Library::initialise_static<Library>();
	}
}

void EditorGame::close_library(Framework::LibraryLoadLevel::Type _upToLibraryLoadLevel, Loader::ILoader* _loader)
{
	base::close_library(_upToLibraryLoadLevel, _loader);
}

void EditorGame::setup_library()
{
	base::setup_library();
}

bool EditorGame::load_editor_library(LibraryLoadLevel::Type _startAtLibraryLoadLevel)
{
	DebugVisualiser::block(true);
	bool result = true;
	if (_startAtLibraryLoadLevel <= LibraryLoadLevel::System)
	{
		if (!base::load_library(get_system_library_directory(), LibraryLoadLevel::System, &Loader::VoidRoom(Loader::VoidRoomLoaderType::PreviewGame, Colour::blackWarm, Colour::cyan).for_init()))
		{
			result = false;
			error(TXT("error loading system library"));
		}
	}
	if (_startAtLibraryLoadLevel <= LibraryLoadLevel::Main)
	{
		if (!base::load_library(get_library_directory(), LibraryLoadLevel::Main, &Loader::VoidRoom(Loader::VoidRoomLoaderType::PreviewGame, Colour::blackWarm, Colour::whiteWarm).for_init()))
		{
			result = false;
			error(TXT("error loading library"));
		}
	}
	DebugVisualiser::block(false);
	return result;
}

void EditorGame::close_editor_library(LibraryLoadLevel::Type _closeAtLibraryLoadLevel)
{
	DebugVisualiser::block(true);
	base::close_library(_closeAtLibraryLoadLevel, &Loader::VoidRoom(Loader::VoidRoomLoaderType::PreviewGame, Colour::blackWarm, Colour::orange).for_shutdown());
	DebugVisualiser::block(false);
}

void EditorGame::post_start()
{
	base::post_start();
}

void EditorGame::pre_close()
{
	base::pre_close();
}

void EditorGame::before_game_starts()
{
	base::before_game_starts();

	output(TXT("setup renderer mode"));
	debug_renderer_mode(DebugRendererMode::OnlyActiveFilters);
}

void EditorGame::after_game_ended()
{
	base::after_game_ended();
}

EditorGame::~EditorGame()
{
	// just in any case
	worldManager.allow_immediate_sync_jobs();
	perform_sync_world_job(TXT("destroy world"), [this](){ sync_destroy_world(); });

	an_assert(! sceneLayerStack.is_set());
	an_assert(mode == nullptr);
}

void EditorGame::initialise_system()
{
	base::initialise_system();
}

void EditorGame::create_render_buffers()
{
	base::create_render_buffers();
}

void EditorGame::close_system()
{
	::System::Core::on_quick_exit_no_longer(TXT("save player profile"));

	base::close_system();
}

void EditorGame::create_mode_on_no_mode()
{
	start_mode(new GameModes::EditorSetup(this));
}

void EditorGame::pre_advance()
{
	scoped_call_stack_info(TXT("tfg::game::pre_advance"));

#ifdef AN_RENDERER_STATS
	Framework::Render::Stats::get().clear();
#endif

	// do game's pre advance after semaphore
	base::pre_advance();

	Framework::GameAdvanceContext gameAdvanceContext;

	// pre mode update lock for sync jobs
	{
		// not used yet WorldManagerImmediateSyncJobsScopedLock lockImmediateSyncJobs(this);

		setup_game_advance_context(REF_ gameAdvanceContext);

		if (mode.is_set())
		{
			{
				mode->pre_advance(gameAdvanceContext);
			}

			if (mode->should_end() || endModeRequested)
			{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
				output(TXT("mode says it should end  or  end mode requested"));
#endif
				end_mode(true);
			}
		}

	}

	// mode management
	{
		if (!mode.is_set())
		{
			scoped_call_stack_info(TXT("no game mode"));

			create_mode_on_no_mode();

			ready_for_continuous_advancement();
		}
	}

	// post mode lock for sync jobs
	{
		// not used yet WorldManagerImmediateSyncJobsScopedLock lockImmediateSyncJobs(this);

		if (sceneLayerStack.is_set())
		{
			sceneLayerStack->remove_all_to_end(); // better to do this post mode as we check "should_end"
		}

		if (sceneLayerStack.is_set())
		{
			sceneLayerStack->pre_advance(gameAdvanceContext);
		}
	}
}

void EditorGame::pre_game_loop()
{
	base::pre_game_loop();

	Framework::GameAdvanceContext gameAdvanceContext;
	setup_game_advance_context(REF_ gameAdvanceContext);

	if (mode.is_set())
	{
		mode->pre_game_loop(gameAdvanceContext);
	}
}


void EditorGame::sound()
{
	// update sound system through base class
	// base will allow soundGate
	base::sound();
}

void EditorGame::advance_controls()
{
	scoped_call_stack_info(TXT("EditorGame::advance_controls"));

	base::advance_controls();

	Framework::GameAdvanceContext gameAdvanceContext;
	setup_game_advance_context(REF_ gameAdvanceContext);

	// process controls
	{
		{
			if (sceneLayerStack.is_set())
			{
				if (!::System::Core::is_paused() &&
					(::System::Input::get()->is_grabbed()))
				{
					sceneLayerStack->process_controls(gameAdvanceContext);
				}

				sceneLayerStack->process_all_time_controls(gameAdvanceContext);
			}
		}

		if (sceneLayerStack.is_set())
		{
			if (!::System::Core::is_paused())
			{
				// to allow to debug vr controls from outside
				sceneLayerStack->process_vr_controls(gameAdvanceContext);
			}
		}
	}
}

void EditorGame::render()
{
	assert_rendering_thread();

	// we haven't rendered yet
	debug_renderer_not_yet_rendered();
	
	{
		if (! ::System::Core::is_rendering_paused() &&
			sceneLayerStack.is_set())
		{
			sceneLayerStack->prepare_to_render(nullptr);
		}
	}

	{
		render_block_gate_allow_one();
	}

	bool renderingAllowed = true;

	{	// display buffer
		System::Video3D * v3d = System::Video3D::get();

		// update
		{
			v3d->update();
		}

		{
			// display buffer now, this way we gain on time when we ready scene to be rendered
			if (v3d->is_buffer_ready_to_display())
			{
				v3d->display_buffer_if_ready();
			}
		}
	}

	if (renderingAllowed)
	{
		{
			if (sceneLayerStack.is_set())
			{
				sceneLayerStack->render_on_render(nullptr);
			}
		}

		::System::RenderTarget::unbind_to_none();

		// do post process if required, instead of copying
		{
			System::RenderTarget* rtSrc = get_main_render_buffer();
			System::RenderTarget* rtDest = get_main_render_output_render_buffer();
			if (rtSrc && rtSrc != rtDest)
			{
				System::Video3D* v3d = System::Video3D::get();
				rtSrc->resolve_forced_unbind();
				if (rtDest)
				{
					rtDest->bind();
				}
				else
				{
					System::RenderTarget::bind_none();
				}
				setup_to_render_to_render_target(v3d, rtDest);

				rtSrc->render(0, v3d, Vector2::zero, v3d->get_viewport_size().to_vector2());

				::System::RenderTarget::unbind_to_none();
			}
		}

		{
			if (sceneLayerStack.is_set())
			{
				sceneLayerStack->render_after_post_process(nullptr);
			}
		}

		System::RenderTarget::unbind_to_none();

		{
			game_icon_render_ui_on_main_render_output();
		}

		if (!VR::IVR::get() || (!VR::IVR::get()->does_use_direct_to_vr_rendering()/* && VR::IVR::get()->does_use_separate_output_render_target()*/))
		{
			render_main_render_output_to_output();
		}
	}

	debug_renderer_undefine_contexts();
	debug_renderer_already_rendered();

	// post rendering, load one mesh3d part
	{
		// load one part
		Meshes::Mesh3DPart::load_one_part_to_gpu();
	}
}

void EditorGame::game_loop()
{
	base::game_loop();

	{
		Framework::GameAdvanceContext gameAdvanceContext;
		setup_game_advance_context(REF_ gameAdvanceContext);

		// advance scene stack, if any scene requests to not advance scenes below, stop updating right there
		// at the bottom there should be "AdvanceWorld" scene
		if (sceneLayerStack.is_set())
		{
			sceneLayerStack->advance(gameAdvanceContext);
		}
		// don't advance world on its own
	}
}

String EditorGame::get_game_name() const
{
	return String(::System::Core::get_app_name());
}

void EditorGame::start_mode(Framework::GameModeSetup* _modeSetup)
{
	base::start_mode(_modeSetup);

	// reset for continuous advancement
	ready_for_continuous_advancement();
}

void EditorGame::end_mode(bool _abort)
{
	base::end_mode(_abort);
}

bool EditorGame::should_input_be_grabbed() const
{
	return base::should_input_be_grabbed();
}

void EditorGame::pre_update_loader()
{
}

void EditorGame::post_render_loader()
{
	// loaders output to actual output, we need to render everything there
	::System::RenderTargetPtr keepRT;
	keepRT = mainRenderOutputRenderBuffer;
	mainRenderOutputRenderBuffer.clear();

	game_icon_render_ui_on_main_render_output();

	// let's go back to normal
	mainRenderOutputRenderBuffer = keepRT;
}

void EditorGame::prepare_render_buffer_resolutions(REF_ VectorInt2& _mainResolution, REF_ VectorInt2& _secondaryViewResolution)
{
	VectorInt2 mainResolutionProvided = _mainResolution;
	VectorInt2 useEyeResolution = mainResolutionProvided;
	if (does_use_vr())
	{
		if (auto* vr = VR::IVR::get())
		{
			useEyeResolution = vr->get_render_full_size_for_aspect_ratio_coef(VR::Eye::Right);
		}
	}

	_secondaryViewResolution = VectorInt2::zero;
}

void EditorGame::render_main_render_output_to_output()
{
	System::RenderTarget* eyeRt;
	eyeRt = get_main_render_output_render_buffer();

	if (eyeRt)
	{
		eyeRt->resolve_forced_unbind();
	}

	if (!eyeRt)
	{
		// either already rendered or nothing to render
		return;
	}

	auto* v3d = ::System::Video3D::get();

	System::RenderTarget::bind_none();

	Colour backgroundColour = Colour::black;

	v3d->setup_for_2d_display();
	v3d->set_default_viewport();
	v3d->set_near_far_plane(0.02f, 100.0f);
	v3d->clear_colour(backgroundColour);

	// restore projection matrix
	v3d->set_2d_projection_matrix_ortho();
	v3d->access_model_view_matrix_stack().clear();

	VectorInt2 targetSize = MainConfig::global().get_video().resolution;

	System::RenderTarget* singleRt = eyeRt;
	if (singleRt)
	{
		singleRt->render_fit(0, ::System::Video3D::get(), Vector2::zero, targetSize.to_vector2());
	}

	System::RenderTarget::unbind_to_none();
}

void EditorGame::on_advance_and_display_loader(Loader::ILoader* _loader)
{
	base::on_advance_and_display_loader(_loader);
}

//

