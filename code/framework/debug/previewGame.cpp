#include "previewGame.h"

#include "previewAILogic.h"

#include "testConfig.h"

#include "..\..\core\debugConfig.h"
#include "..\..\core\mainConfig.h"
#include "..\..\core\mainSettings.h"
#include "..\..\core\renderDoc.h"
#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\concurrency\asynchronousJobList.h"
#include "..\..\core\types\names.h"
#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\debug\debugVisualiser.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\system\core.h"
#include "..\..\core\system\input.h"
#include "..\..\core\system\sound\soundSystem.h"
#include "..\..\core\system\video\video3d.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\video3dPrimitives.h"

#include "..\render\renderContext.h"

#include "..\ai\aiMindInstance.h"
#include "..\appearance\controllers\ac_particlesUtils.h"
#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\advance\advanceContext.h"
#include "..\jobSystem\jobSystem.h"
#include "..\module\moduleAI.h"
#include "..\module\moduleAppearance.h"
#include "..\module\moduleController.h"
#include "..\module\moduleGameplayPreview.h"
#include "..\module\moduleMovement.h"
#include "..\module\modulePresence.h"
#include "..\module\moduleTemporaryObjects.h"
#include "..\object\actor.h"
#include "..\object\item.h"
#include "..\object\object.h"
#include "..\object\objectType.h"
#include "..\object\scenery.h"
#include "..\object\temporaryObject.h"
#include "..\world\environment.h"
#include "..\world\presenceLink.h"
#include "..\world\room.h"
#include "..\world\subWorld.h"
#include "..\world\world.h"

#include "..\render\renderScene.h"
#include "..\sound\soundScene.h"

#include "..\postProcess\postProcessInstance.h"
#include "..\postProcess\chain\postProcessChain.h"
#include "..\postProcess\chain\postProcessChainProcessElement.h"

#include "..\loaders\loaderVoidRoom.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#define AN_ALLOW_QUICK

//

using namespace Framework;

float previewGrandScale = 1.0f;

Range z_range_ortho() { return Range(1.0f, 10000.0f); }
Range z_range_persp() { return Range(0.02f, 30000.0f); }

REGISTER_FOR_FAST_CAST(PreviewGame);

PreviewGame::PreviewGame(CreateLibraryFunc _createLibraryFunc)
{
#ifdef AN_DEVELOPMENT
	ignore_debug_on_error_warn();
#endif

	wantsToQuit = false;

	createLibraryFunc = _createLibraryFunc;

	runningSingleThreaded = false;
	mouseGrabbed = false;

	pivot = Vector3::zero;
	pivotFollow = Vector3::zero;
	distance = 5.0f;
	rotation = Rotator3::zero;

	zRange = z_range_persp();
	fov = 90.0f;

	showObject.object = nullptr;
	showObject.name = LibraryName::invalid();

	debug_renderer_mode(DebugRendererMode::All);

#ifdef AN_DEVELOPMENT
	MeshGenerator::show_debug_data();
	MeshGenerator::gather_debug_data();
	MeshGenerator::show_generation_info();
	MeshGenerator::gather_generation_info();
#endif

#ifdef AN_PERFORMANCE_MEASURE
	doesAllowPerformanceOutput = false;
#endif

#ifdef AN_DEVELOPMENT
	MeshGeneratorRequest::commonRandomGenerator = mainRandomGenerator;
#endif
}

PreviewGame::~PreviewGame()
{
	delete_and_clear(soundScene);
}

bool PreviewGame::reload_preview_library()
{
	debug_clear();
	debug_renderer_next_frame_clear();
	debug_clear();

	close_preview_library(LibraryLoadLevel::Main);
	output_colour(1, 0, 1, 1);
	output(TXT("reloading library..."));
	output_colour();
	bool result = load_preview_library(LibraryLoadLevel::Main); // never reload system!

	update_default_assets();

	// rebuild dir entries
	update_interesting_dirs();

	debug_clear();
	debug_renderer_next_frame_clear();
	debug_clear();

	// clear to log new info to display (and to output/log file)
	prevShowObject = nullptr;

	post_reload();

	return result;
}

void PreviewGame::create_library()
{
	createLibraryFunc();
#ifdef AN_DEVELOPMENT
	::Framework::Library::get_current()->ignore_preparing_problems();
#endif
}

bool PreviewGame::load_preview_library(LibraryLoadLevel::Type _startAtLibraryLoadLevel)
{
	DebugVisualiser::block(true);
	reloadNeeded = false;
	quickReloadNeeded.clear();
	reloadStarting = 0;
	bool result = true;
	if (_startAtLibraryLoadLevel <= LibraryLoadLevel::System)
	{
		if (! base::load_library(get_system_library_directory(), LibraryLoadLevel::System, &Loader::VoidRoom(Loader::VoidRoomLoaderType::PreviewGame, Colour::blackWarm, Colour::cyan).for_init()))
		{
			result = false;
			error(TXT("error loading system library"));
		}
	}
	if (_startAtLibraryLoadLevel <= LibraryLoadLevel::Main && ! noLibrary)
	{
		if (useLibraryDirectories.is_empty())
		{
			if (! base::load_library(get_library_directory(), LibraryLoadLevel::Main, &Loader::VoidRoom(Loader::VoidRoomLoaderType::PreviewGame, Colour::blackWarm, Colour::whiteWarm).for_init()))
			{
				result = false;
				error(TXT("error loading library"));
			}
		}
		else
		{
			if (!base::load_library(useLibraryDirectories, useLibraryFiles, LibraryLoadLevel::Main, &Loader::VoidRoom(Loader::VoidRoomLoaderType::PreviewGame, Colour::blackWarm, Colour::whiteWarm).for_init()))
			{
				result = false;
				error(TXT("error loading library"));
			}
		}
	}
	checkReload.allow();
	checkReloadTS.reset();
	create_post_process_chain();

	DebugVisualiser::block(false);
	return result;
}

void PreviewGame::close_preview_library(LibraryLoadLevel::Type _closeAtLibraryLoadLevel)
{
	destroy_test();

	if (soundScene)
	{
		soundScene->clear(); // otherwise we may be still accessing non existent samples
	}

	if (_closeAtLibraryLoadLevel == LibraryLoadLevel::Close)
	{
		// drop sky box mesh to allow to unload library
		environments.clear();
	}
	close_post_process_chain();
	showObject.object = nullptr;
	update_interesting_dirs();
	DebugVisualiser::block(true);
	base::close_library(_closeAtLibraryLoadLevel, &Loader::VoidRoom(Loader::VoidRoomLoaderType::PreviewGame, Colour::blackWarm, Colour::orange).for_shutdown());
	DebugVisualiser::block(false);
}

void prepare_library_in_background(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	Framework::Library::get_current()->prepare_for_game();
}

bool PreviewGame::reload_as_needed(Optional<bool> _quick)
{
	destroy_test();

	output_colour(1, 0, 1, 0);
	output(TXT("reload as needed"));
	output_colour();

	reloadInProgress = true;

	reloadNeeded = false;
	quickReloadNeeded.clear();
	reloadStarting = 0;

	debug_clear();
	debug_renderer_next_frame_clear();
	debug_clear();

	bool result = true;

	if (reloadOnlyEnvironments)
	{
		_quick = true;
	}
	else if (!_quick.is_set())
	{
		if (mode == PreviewGameMode::Normal)
		{
			_quick = false;
		}
		else
		{
			_quick = true;
		}
	}

	if (! _quick.get())
	{
		result = reload_preview_library();
	}
	else
	{
#ifdef AN_DEVELOPMENT
		quickReloading = true;
		output_colour(1, 0, 1, 1);
		output(TXT("quick reloading..."));
		output_colour();
		Array<String> filePaths;
		for_every(dirEntry, (reloadOnlyEnvironments? environmentInterestingDirEntries : quickInterestingDirEntries))
		{
			filePaths.push_back_unique(dirEntry->path);
		}
		for_every(filePath, filePaths)
		{
			Framework::Library::get_current()->reload_from_file(*filePath, LibrarySource::Assets, [this](LibraryStored* _object) {
				return true; // reload all from file
				return  prevShowObject == _object ||
						fast_cast<MeshGenerator>(_object) != nullptr ||
						fast_cast<CustomLibraryStoredData>(_object) != nullptr ||
						(reloadOnlyEnvironments && (fast_cast<EnvironmentType>(_object) != nullptr || fast_cast<EnvironmentOverlay>(_object) != nullptr));
				/*
					return (fast_cast<Mesh>(_object) != nullptr && fast_cast<Mesh>(_object)->get_mesh_generator()) ||
						fast_cast<MeshGenerator>(_object) != nullptr ||
					   (fast_cast<Skeleton>(_object) != nullptr && fast_cast<Skeleton>(_object)->get_mesh_generator()) ||
						fast_cast<MeshGenerator>(_object) != nullptr ||
						fast_cast<CollisionModel>(_object) != nullptr ||
						fast_cast<CustomLibraryStoredData>(_object) != nullptr;
				 */
			});
		}
		get_job_system()->do_asynchronous_job(background_jobs(), prepare_library_in_background, (void*)nullptr);
		auto backgroundJobsList = get_job_system()->get_asynchronous_list(background_jobs());
		auto systemJobsList = get_job_system()->get_asynchronous_list(system_jobs());
		while (!backgroundJobsList->has_finished() || !systemJobsList->has_finished())
		{
			// advance system
			::System::Core::advance();

			// advance vr, if can be used
			if (does_use_vr())
			{
				VR::IVR::get()->advance();
				
				handle_vr_all_time_controls_input();
			}

			DebugVisualiserPtr dv(DebugVisualiser::get_active());
			if (dv.is_set())
			{
				if (MainSettings::global().should_render_fence_be_at_end_of_frame())
				{
					if (does_use_vr())
					{
						VR::IVR::get()->end_render(::System::Video3D::get());
					}
				}
				assert_rendering_thread();
				dv->advance_and_render_if_on_render_thread();
			}

			get_job_system()->execute_main_executor(1); // execute one job to update screen
			::System::Core::sleep(0.001f);
		}
		
		Framework::Library::get_current()->get_meshes().do_for_every(
			[](LibraryStored* _libraryStored)
			{
				if (Mesh* mesh = fast_cast<Mesh>(_libraryStored))
				{
					if (mesh->get_mesh_generator() &&
						! mesh->is_prepared_for_game())
					{
						Framework::Library::get_current()->prepare_for_game(mesh); // prepare mesh again, but only those that use mesh generation
					}
				}
			});
		quickReloading = false;

		result = true;

		// rebuild dir entries
		update_interesting_dirs();
#endif
	}

	debug_clear();
	debug_renderer_next_frame_clear();
	debug_clear();

	// clear to log new info to display (and to output/log file)
	prevShowObject = nullptr;

	reloadInProgress = false;

	reloadOnlyEnvironments = false;

	post_reload();

	return result;
}

void PreviewGame::post_reload()
{
	if (auto* m = fast_cast<Framework::Mesh>(showObject.object))
	{
		m->generated_on_demand_if_required();
	}
}

void PreviewGame::initialise_system()
{
	DebugConfig::previewMode = true;

	MainConfig::access_global().access_video().resolution.x = 0;
	MainConfig::access_global().access_video().resolution.y = 0;
	MainConfig::access_global().access_video().vsync = true;

	TestConfig::access_params().clear(); // clear all settings that could be loaded from other config files

	::Library::load_global_config_from_file(startedUsingPreviewConfigFile, LibrarySource::Assets);

	if (MainConfig::access_global().access_video().resolution.is_zero())
	{
		int res = min(::System::Video3D::get()->get_desktop_size().x * 2 / 3,
					  ::System::Video3D::get()->get_desktop_size().y * 2 / 3);
		MainConfig::access_global().access_video().resolution.x = res;
		MainConfig::access_global().access_video().resolution.y = res;
	}

	MainConfig::access_global().access_video().vsync = true;

	base::initialise_system();

	if (::System::Video3D::get()->get_desktop_size().y > 1200)
	{
		textScale = Vector2::one * 2.0f;
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
	generateDevMeshes = 4; // override testConfig's
#endif
	generateAtLOD = 0; // override testConfig's
	generateMoreDetails.clear(); // override testConfig's
}

void PreviewGame::close_system()
{
	base::close_system();

	DebugConfig::previewMode = false;
}

void PreviewGame::sound()
{
	// mark sound unblocked/allowed and wake up waiting for pre render sync
	soundBlockGate.allow_one();

	// update sound system through base class
	base::sound();
}

void PreviewGame::render()
{
	if (!get_main_render_buffer())
	{
		return;
	}

	MEASURE_PERFORMANCE(renderThread);

	::System::Video3D * v3d = ::System::Video3D::get();

	// we haven't rendered yet
	debug_renderer_not_yet_rendered();

	usedLODInfo.clear();

	camera.set_placement(showActualObject.is_set()? testRoomForActualObjects : (showTemporaryObject? testRoomForTemporaryObjects : (testRoomForCustomPreview? testRoomForCustomPreview : testRoom)), cameraPlacement);
	camera.set_fov(fov);
	camera.set_near_far_plane(zRange.min, zRange.max);
	camera.set_background_near_far_plane();
	camera.set_view_origin(VectorInt2::zero);
	camera.set_view_size(get_main_render_buffer()->get_size());
	camera.set_view_aspect_ratio(aspect_ratio(get_main_render_buffer()->get_full_size_for_aspect_ratio_coef()));
	debug_camera(cameraPlacement, fov, NP);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (testRoomForActualObjects)
	{
		testRoomForActualObjects->mark_recently_seen_by_player(cameraPlacement);
	}
	if (testRoomForTemporaryObjects)
	{
		testRoomForTemporaryObjects->mark_recently_seen_by_player(cameraPlacement);
	}
	if (testRoom)
	{
		testRoom->mark_recently_seen_by_player(cameraPlacement);
	}
	if (testRoomForCustomPreview)
	{
		testRoomForCustomPreview->mark_recently_seen_by_player(cameraPlacement);
	}
#endif

	Framework::Render::Context rc(v3d);
	camera.setup(rc);

	{
		Framework::Render::SceneBuildRequest sceneBuildRequest;
		if (noRoomMeshes >= 2)
		{
			sceneBuildRequest.no_environment_meshes();
		}
		if (noRoomMeshes >= 1)
		{
			sceneBuildRequest.no_room_meshes();
		}
		renderScene = Framework::Render::Scene::build(REF_ camera, sceneBuildRequest);
	}

	Framework::Sound::Camera soundCamera;
	soundCamera.set_placement(showActualObject.is_set() ? testRoomForActualObjects : (showTemporaryObject? testRoomForTemporaryObjects : (testRoomForCustomPreview? testRoomForCustomPreview : testRoom)), cameraPlacement);
	soundScene->update(::System::Core::get_delta_time(), soundCamera, nullptr);

	// mark pre rendering done and wake up waiting for pre render sync
	renderBlockGate.allow_one();

	// update (debug keys?)
	v3d->update();

	// display buffer now, this way we gain on time when we ready scene to be rendered
	v3d->display_buffer_if_ready();

	if (postProcessChain)
	{
		postProcessChain->setup_render_target_output_usage(get_main_render_buffer());
	}

	// make sure nothing is bound
	::System::RenderTarget::unbind_to_none();

	// render to main buffer because "none" render buffer doesn't have depth stencil and is not made to support rendering
	get_main_render_buffer()->bind();

	v3d->set_viewport(VectorInt2::zero, v3d->get_screen_size());

	v3d->clear_colour_depth(Colour::lerp(0.6f, Colour::c64LightBlue, Colour::whiteWarm));

#ifdef AN_DEBUG_KEYS
#ifdef AN_STANDARD_INPUT
#ifdef AN_GL
	{
		static bool wireFrame = false;
		if (::System::Input::get()->has_key_been_pressed(::System::Key::F8))
		{
			wireFrame = !wireFrame;
		}
		DIRECT_GL_CALL_ glPolygonMode(GL_FRONT_AND_BACK, wireFrame ? GL_LINE : GL_FILL); AN_GL_CHECK_FOR_ERROR
	}
#endif
#endif
#endif

	v3d->set_near_far_plane(0.02f, 30000.0f);
	
	if (environments.is_index_valid(environmentIndex) &&
		environments[environmentIndex].environmentType &&
		debug_renderer_shader_program_binding_context())
	{
		// we have to use camera placement directly and ready it to rendering as it this point we don't have matrix loaded into v3d
		Matrix44 viewMatrix = cameraPlacement.to_matrix().inverted();
		::System::Video3D::ready_matrix_for_video_x_right_y_forward_z_up(viewMatrix);
		environments[environmentIndex].environmentType->get_info().get_params().setup_shader_binding_context(debug_renderer_shader_program_binding_context(), viewMatrix);
		// and overlays
		for_every_ptr(ao, environments[environmentIndex].environmentOverlays)
		{
			ao->get_params().setup_shader_binding_context(debug_renderer_shader_program_binding_context(), viewMatrix);
		}
	}

	// make sure nothing is bound
	::System::RenderTarget::unbind_to_none();

	renderScene.get()->render(rc, get_main_render_buffer());

	// make sure nothing is bound
	::System::RenderTarget::unbind_to_none();
	
	get_main_render_buffer()->bind();

	if (!showActualObject.is_set() || (previewWidgets.is_empty() && showInfo >= 0) || showInfo >= 1)
	{
		debug_renderer_render(v3d);
	}

	debug_renderer_undefine_contexts();
	debug_renderer_already_rendered();

#ifdef AN_DEBUG_KEYS
#ifdef AN_GL
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
#endif
#endif

	get_main_render_buffer()->unbind();
	get_main_render_buffer()->resolve();

	if (postProcessChain)
	{
		postProcessChain->set_camera_that_was_used_to_render(camera);

		{
			postProcessChain->for_every_shader_program_instance([](Framework::PostProcessNode const* _node, ::System::ShaderProgramInstance& _shaderProgramInstance)
				{
					_shaderProgramInstance.set_uniforms_from(_shaderProgramInstance.get_shader_program()->get_default_values());
					return true;
				});
		}

		postProcessChain->do_post_process(get_main_render_buffer(), v3d);

		setup_to_render_to_main_render_target(v3d);

		postProcessChain->render_output(v3d, Vector2::zero, MainConfig::global().get_video().resolution.to_vector2());

		postProcessChain->release_render_targets();
	}
	else
	{
		setup_to_render_to_main_render_target(v3d);
		get_main_render_buffer()->render(0, v3d, Vector2::zero, MainConfig::global().get_video().resolution.to_vector2());
	}

	::System::RenderTarget::bind_none();

	if (showObject.object)
	{
		if (showObject.object->preview_render())
		{
			return;
		}
	}

	if (showInfo < 0)
	{
		return;
	}

	v3d->set_default_viewport();
	v3d->setup_for_2d_display();

	sort(usedLODInfo);

	if (Framework::Font* font = get_default_font())
	{
#ifdef USE_RENDER_DOC
		if (true)
		{
			Vector2 at = v3d->get_viewport_size().to_vector2() * 0.5f;
			at.y = at.y * 0.4f;
			Vector2 scale = v3d->get_viewport_size().to_vector2() * 0.01f * 0.35f;
			scale.y = scale.x * 0.8f;
			scale *= 0.5f;
			font->draw_text_at_with_border(v3d, String(TXT("no debug renderer while using render doc")), Colour::red, Colour::blackWarm, 1.0f, at, scale, Vector2(0.5f, 1.0f));
		}
		else
#endif

		if ((previewWidgets.is_empty() && showInfo >= 1) || showInfo >= 2)
		{
			float x = 0.0f;
			float y = 0.0f;
			String modeString(TXT("??"));
			if (mode == PreviewGameMode::Quick)
			{
				modeString = TXT("quick");
			}
			else if (mode == PreviewGameMode::Normal)
			{
				modeString = TXT("normal");
			}

			Vector2 scale = textScale;

			Colour outline = Colour::c64Grey1;
			ARRAY_STACK(Colour, modeColours, PreviewGameMode::MAX);
			modeColours.set_size(PreviewGameMode::MAX);
			for_every(modeColour, modeColours)
			{
				*modeColour = Colour::c64Cyan;
			}
			modeColours[PreviewGameMode::Quick] = Colour::c64LightRed;

			String autoReloadsString(autoReloads ? TXT("auto reloads") : TXT("no auto reloads"));
			Colour autoReloadsColour = autoReloads ? Colour::c64Red : Colour::greyLight;

			Colour shortCutColour = Colour::orange;

			// start
			x = 0.0f;
			y = (float)v3d->get_viewport_size().y - font->get_height() * textScale.y;

#ifdef AN_STANDARD_INPUT
			if (::System::Input::get()->is_key_pressed(::System::Key::RightAlt) &&
				!::System::Input::get()->is_key_pressed(::System::Key::RightCtrl))
			{
				font->draw_text_at_with_border(v3d, String(TXT("[a]     all objects")), debugDraw.allObjects ? Colour::green : Colour::blue, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("[p]     presence")), debugDraw.presence? Colour::green : Colour::blue, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("[o]     POIs")), debugDraw.POIs? Colour::green : Colour::blue, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("[w]     walker")), debugDraw.walker? Colour::green : Colour::blue, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("[s]     skeleton")), debugDraw.skeleton? Colour::green : Colour::blue, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("[r]     reset preview controllers")), Colour::greyLight, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("[t]     absolute placement for preview controllers")), previewVRControllers.absolutePlacement ? Colour::green : Colour::blue, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String::printf(TXT("[l]     %S"), 
					showControlPanel == ShowControlPanel::Locomotion? TXT("locomotion") :
					(showControlPanel == ShowControlPanel::VRControllers? TXT("vr controls") : TXT("??"))),
					usePreviewAILogic ? Colour::green : Colour::blue, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
			}
			else
#endif
			{
				font->draw_text_at_with_border(v3d, String(TXT("[c]     ")) + modeString, modeColours[mode], outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[c]     ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("[c+ctl] ")) + autoReloadsString, autoReloadsColour, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[c+ctl] ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("[z/x]   ")) + showObject.name.to_string(), (showObject.object ? Colour::whiteWarm : Colour::red), outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[z/x]   ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("        ")) + showObject.type.to_string(), Colour::whiteWarm, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("[r/f]   select log line")), Colour::green, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[r/f]   ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("[+shft] faster")), Colour::green, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[+shft] ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("[+ctrl] jump to next :section")), Colour::green, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[+ctrl] ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				{
					void* to = fast_cast<TemporaryObjectType>(showObject.object);
					if (!to) { to = fast_cast<ObjectType>(showObject.object); }
					font->draw_text_at_with_border(v3d, String::printf(TXT("[v]     %S %S"), to ? TXT("restart object/particle") : TXT("new random seed"), mainRandomGenerator.get_seed_string_as_numbers().to_char()), Colour::purple, outline, 1.0f, Vector2(x, y), scale);
					font->draw_text_at_with_border(v3d, String(TXT("[v]     ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
					y -= font->get_height() * scale.y;
					font->draw_text_at_with_border(v3d, String::printf(TXT("[v+ctl]     %S"), TXT("stop particle")), Colour::purple, outline, 1.0f, Vector2(x, y), scale);
					font->draw_text_at_with_border(v3d, String(TXT("[v+ctl] ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
					y -= font->get_height() * scale.y;
#ifdef AN_DEVELOPMENT
					{
						font->draw_text_at_with_border(v3d, String::printf(TXT("[b]     ai is %S"), usePreviewAILogic ? TXT("using preview logic") : TXT("normal")),
							to ? (ModuleAI::is_ai_advanceable() ? Colour::purple : (Colour::purple * 0.75f)) : Colour::grey, outline, 1.0f, Vector2(x, y), scale);
						font->draw_text_at_with_border(v3d, String(TXT("[b]     ")), to ? shortCutColour : Colour::grey, outline, 1.0f, Vector2(x, y), scale);
						y -= font->get_height() * scale.y;
					}
#endif
					font->draw_text_at_with_border(v3d, String(TXT("[l+...] move object")), Colour::whiteWarm, outline, 1.0f, Vector2(x, y), scale);
					font->draw_text_at_with_border(v3d, String(TXT("[l    ] ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
					y -= font->get_height() * scale.y;
					font->draw_text_at_with_border(v3d, String(TXT("[h+1-0] predefined scenarios")), Colour::whiteWarm, outline, 1.0f, Vector2(x, y), scale);
					font->draw_text_at_with_border(v3d, String(TXT("[h    ] ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
					y -= font->get_height() * scale.y;
					font->draw_text_at_with_border(v3d, String(TXT("[n]     reload this")), Colour::yellow, outline, 1.0f, Vector2(x, y), scale);
					font->draw_text_at_with_border(v3d, String(TXT("[n]     ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
					y -= font->get_height() * scale.y;
					font->draw_text_at_with_border(v3d, String(TXT("[m]     reload environments")), Colour::yellow, outline, 1.0f, Vector2(x, y), scale);
					font->draw_text_at_with_border(v3d, String(TXT("[m]     ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
					y -= font->get_height() * scale.y;
					font->draw_text_at_with_border(v3d, String(TXT("[ralt+] more options")), Colour::greyLight, outline, 1.0f, Vector2(x, y), scale);
					font->draw_text_at_with_border(v3d, String(TXT("[ralt+] ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
					y -= font->get_height() * scale.y;
				}

				if (showInfo >= 3 && ! usedLODInfo.is_empty())
				{
					int triCount = 0;
					for_every(ui, usedLODInfo)
					{
						triCount += ui->triCount;
					}
					linesSeparation = font->get_height() * scale.y;
					y -= linesSeparation * 8.0f;
					{
						font->draw_text_at_with_border(v3d, String::printf(TXT("LODs [tri:%6i]"), triCount), Colour::zxWhite, Colour::black, 1.0f, Vector2(x, y), scale);
						y -= linesSeparation;
						for_every(ui, usedLODInfo)
						{
							font->draw_text_at_with_border(v3d, String::printf(TXT("[ %i : %6i ] [s:%6.2f] [d:%6.2f] [agLOD:%5.2f off:%5.2f] %S"), ui->lod, ui->triCount, ui->size, ui->dist, ui->aggressiveLOD, ui->aggressiveLODStartOffset, ui->aiName.to_char()), Colour::zxWhite, Colour::black, 1.0f, Vector2(x, y), scale);
							y -= linesSeparation;
						}
					}
				}

				if (showInfo >= 3)
				{
					float bottomY = showInfo >= 4 ? 0.0f : y - linesSeparation * 19.0f;
					linesSeparation = font->get_height() * scale.y;
					y -= linesSeparation;
					linesStartAtY = y;
					linesCharacterWidth = font->calculate_char_size().x * scale.x;
					visibleLogInfoLines = (int)(y / (font->get_height() * scale.y));
					{
						Vector2 mouseLoc = ::System::Input::get()->get_mouse_window_location();
						Concurrency::ScopedSpinLock lock(logInfoContextLock);
						int lineIdx = 0;
						for_every(line, logInfoContext.get_lines())
						{
							if (lineIdx >= logInfoLineStartIdx && y > bottomY)
							{
								Colour colour = logInfoLineIdx == lineIdx ? Colour::green : (line->text.has_prefix(TXT("::")) ? Colour::lerp(0.2f, Colour::c64Cyan, Colour::whiteWarm) : (line->text.has_prefix(TXT(":")) ? Colour::lerp(0.5f, Colour::c64Cyan, Colour::whiteWarm) : Colour::whiteWarm));
								Colour currentOutline = outline;
								if (mouseLoc.y > y&& mouseLoc.y <= y + linesSeparation &&
									mouseLoc.x >= 0 && mouseLoc.x <= linesCharacterWidth * line->text.get_length())
								{
									currentOutline = Colour::blue;
								}
								font->draw_text_at_with_border(v3d, logInfoLineIdx == lineIdx && line->text.is_empty() ? String(TXT(">")) : line->text, colour, currentOutline, 1.0f, Vector2(x, y), scale);
								y -= linesSeparation;
							}
							++lineIdx;
						}
					}
				}

				// start 2nd column
				x = 300.0f * textScale.x;
				y = (float)v3d->get_viewport_size().y - font->get_height() * textScale.y;

				y -= (font->get_height() * scale.y) * 3.0f;

#ifdef AN_DEVELOPMENT
				{
					static float timeToNextCheck = 0.0f;
					static size_t minMemoryOfLastFewSeconds = 0;
					static Optional<size_t> minMemory;
					timeToNextCheck -= ::System::Core::get_raw_delta_time();
					if (timeToNextCheck <= 0.0f)
					{
						timeToNextCheck = 1.0f;
						minMemoryOfLastFewSeconds = minMemory.get(0);
						minMemory.clear();
					}
#ifdef AN_CHECK_MEMORY_LEAKS
					if (minMemory.is_set())
					{
						minMemory = min(minMemory.get(), get_memory_allocated_checked_in());
					}
					else
					{
						minMemory = get_memory_allocated_checked_in();
					}
					font->draw_text_at_with_border(v3d, String::printf(TXT("mem (kB) %i %i min:%i"), get_memory_allocated_checked_in() / 1024, get_memory_allocated_checked_in(), minMemoryOfLastFewSeconds), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
#endif
				}
				y -= font->get_height() * scale.y;
#endif

				font->draw_text_at_with_border(v3d, String::printf(TXT("[tilde]  render level [%i]"), showInfo), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[tilde]    ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
#ifdef AN_DEVELOPMENT
				{
					tchar const* info = TXT("");
					if (MeshGenerator::does_show_generation_info())
					{
						if (MeshGenerator::does_show_generation_info_only())
						{
							info = TXT("render mesh geenration info only");
						}
						else
						{
							info = TXT("render mesh geenration info");
						}
					}
					else
					{
						info = TXT("no mesh geenration info");
					}
					font->draw_text_at_with_border(v3d, String::printf(TXT("[~+ctrl] %S"), info), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
					font->draw_text_at_with_border(v3d, String(TXT("[~+ctrl]   ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
					y -= font->get_height() * scale.y;
				}
#endif
				font->draw_text_at_with_border(v3d, String::printf(TXT("[tab]    camera mode %S"), cameraMode == 0 ? TXT("pivot") : (cameraMode == 1 ? TXT("free") : TXT("follow"))), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[tab]    ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String::printf(TXT("[num+lc] camera view front [1], side [2], top [3], distance 100/1m [4/5]")), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[num+lc] ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String::printf(TXT("[g]      show grid [%S]"), showGrid == 0? TXT("--") : (showGrid == 2? TXT("xz") : (showGrid == 3? TXT("yz") : TXT("xy")))), showGrid? Colour::c64Green : Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[g]      ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("[space]  to reload")), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[space]  ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("[sp+ct]  to reload this only")), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[sp+ct]  ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String::printf(TXT("[j+shft] environment %S (+%ieo)"),
					environments.is_index_valid(environmentIndex) ? environments[environmentIndex].useEnvironmentType.to_string().to_char() : TXT("none"),
					environments.is_index_valid(environmentIndex) ? environments[environmentIndex].environmentOverlays.get_size() : 0),
					Colour::whiteWarm, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[j+shft] ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String::printf(TXT("[k]      %S"), noRoomMeshes == 1? TXT("no room meshes") : (noRoomMeshes > 1? TXT("no room+sky meshes") : TXT("with room+sky meshes"))), Colour::whiteWarm, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[k]      ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String::printf(TXT("[k+lc]   %S"), testEnvironmentMesh == 0? TXT("plain plane") : (testEnvironmentMesh == 1? TXT("hills") : TXT("?wonderland?"))), Colour::whiteWarm, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[k+lc]   ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String(TXT("[kp-]    text scale")), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[kp-]    ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String::printf(TXT("[lc+kp+-]speed/size %.2f%%"), previewGrandScale * 100.0f), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[lc+kp+-]    ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
				font->draw_text_at_with_border(v3d, String::printf(TXT("[p]      optimise vertices %S"), optimiseVerticesAlways ? TXT("always") : TXT("not during quick reload")), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[p]      ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
#ifdef AN_DEVELOPMENT_OR_PROFILER
				font->draw_text_at_with_border(v3d, String::printf(TXT("[o]      %S"), generateDevMeshes ? String::printf(TXT("generate dev meshes level %i"), generateDevMeshes).to_char() : TXT("do not gen dev meshes")), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[o]      ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
#endif
				font->draw_text_at_with_border(v3d, String::printf(TXT("[i]      generate at LOD %i"), generateAtLOD), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[i]      ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
#ifdef AN_DEVELOPMENT_OR_PROFILER
				font->draw_text_at_with_border(v3d, String::printf(TXT("[u]      preview LOD %i"), previewLOD), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[u]      ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
#endif
				font->draw_text_at_with_border(v3d, String::printf(TXT("[y]      force system \"%S\""), MainConfig::global().get_forced_system().to_char()), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[y]      ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
#ifdef AN_DEVELOPMENT_OR_PROFILER
				font->draw_text_at_with_border(v3d, String::printf(TXT("[f10]    sound level %.0f"), MainConfig::access_global().access_sound().developmentVolume * 100.0f), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
				font->draw_text_at_with_border(v3d, String(TXT("[f10]    ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
				y -= font->get_height() * scale.y;
#endif
				if (auto* ao = showActualObject.get())
				{
					if (auto* gp = ao->get_gameplay_as<ModuleGameplayPreview>())
					{
						if (gp->get_controller_transform_num() > 0)
						{
							Name currentControllerTransform = gp->get_controller_transform_id(controllerTransformIdx);
							Transform cct = gp->get_controller_transform(controllerTransformIdx);
							font->draw_text_at_with_border(v3d, String::printf(TXT("[kp/*]   controller transform \"%S\""), currentControllerTransform.to_char()), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
							font->draw_text_at_with_border(v3d, String(TXT("[kp/*]      ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
							y -= font->get_height() * scale.y;
							font->draw_text_at_with_border(v3d, String::printf(TXT("     t:%S"), cct.get_translation().to_string().to_char()), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
							y -= font->get_height() * scale.y;
							font->draw_text_at_with_border(v3d, String::printf(TXT("     r:%S"), cct.get_orientation().to_rotator().to_string().to_char()), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
							y -= font->get_height() * scale.y;
							font->draw_text_at_with_border(v3d, String(TXT("[kpn]    to move use numpad")), Colour::c64Grey3, outline, 1.0f, Vector2(x, y), scale);
							font->draw_text_at_with_border(v3d, String(TXT("[kpn]      ")), shortCutColour, outline, 1.0f, Vector2(x, y), scale);
							y -= font->get_height() * scale.y;
						}
					}
				}
			}

			if (reloadInProgress || reloadNeeded)
			{
				font->draw_text_at_with_border(v3d, String(TXT("reloading")), Colour::c64Orange, Colour::blackWarm, 1.0f, v3d->get_viewport_size().to_vector2() * 0.5f, Vector2(5.0f, 5.0f), Vector2(0.5f, 0.5));
			}

			if (showActualObjectBeingGenerated)
			{
				font->draw_text_at_with_border(v3d, String(TXT("generating")), Colour::c64Orange, Colour::blackWarm, 1.0f, v3d->get_viewport_size().to_vector2() * 0.5f, Vector2(5.0f, 5.0f), Vector2(0.5f, 0.5));
			}
		}

		if (showControlPanel != visibleControlPanel)
		{
			previewWidgets.clear();
		}
		if (showControlPanel == ShowControlPanel::Locomotion)
		{
			if (previewWidgets.is_empty())
			{
				Vector2 at = get_main_render_buffer()->get_size().to_vector2();
				Vector2 buttonSize = Vector2(300.0f, 60.0f);
				at.y = buttonSize.y;
				at.x = at.x - buttonSize.x * 0.6f;
				{
					PreviewWidget* pw = new PreviewWidget();
					pw->at = at;
					pw->size = buttonSize;
					pw->text = TXT("random dir");
					pw->onClick = [this](Vector2 const & _relClick)
					{
						if (auto* c = this->get_show_actual_objects_controller())
						{
							Vector3 dir = Vector3::yAxis.rotated_by_yaw(Random::get_float(0.0f, 360.0f));
							c->set_requested_movement_direction(dir);
							c->set_requested_orientation(look_matrix33(dir, c->get_owner()->get_presence()->get_placement().get_axis(Axis::Up)).to_quat());
							c->set_requested_velocity_orientation(Rotator3::zero);
							MovementParameters mp = c->get_requested_movement_parameters();
							if (!mp.relativeSpeed.is_set() &&
								!mp.absoluteSpeed.is_set())
							{
								mp.full_speed();
								c->set_requested_movement_parameters(mp);
							}
						}
					};
					previewWidgets.push_back(RefCountObjectPtr<PreviewWidget>(pw));
					at.y += buttonSize.y * 1.2f;
				}
				{
					PreviewWidget* pw = new PreviewWidget();
					pw->at = at;
					pw->size = buttonSize;
					pw->text = TXT("forward");
					pw->onClick = [this](Vector2 const & _relClick)
					{
						if (auto* c = this->get_show_actual_objects_controller())
						{
							c->set_relative_requested_movement_direction(Vector3::yAxis);
							c->clear_requested_orientation();
							c->set_requested_velocity_orientation(Rotator3::zero);
							MovementParameters mp = c->get_requested_movement_parameters();
							mp.full_speed();
							c->set_requested_movement_parameters(mp);
						}
					};
					previewWidgets.push_back(RefCountObjectPtr<PreviewWidget>(pw));
					at.y += buttonSize.y * 1.2f;
				}
				{
					PreviewWidget* pw = new PreviewWidget();
					pw->at = at;
					pw->size = buttonSize;
					pw->text = TXT("speed");
					pw->onClick = [this](Vector2 const & _relClick)
					{
						if (auto* c = this->get_show_actual_objects_controller())
						{
							if (!c->get_requested_movement_direction().is_set())
							{
								c->set_relative_requested_movement_direction(Vector3::yAxis);
								c->clear_requested_orientation();
							}
							MovementParameters mp = c->get_requested_movement_parameters();
							mp.relative_speed(_relClick.x);
							c->set_requested_movement_parameters(mp);
						}
					};
					previewWidgets.push_back(RefCountObjectPtr<PreviewWidget>(pw));
					at.y += buttonSize.y * 1.2f;
				}
				{
					PreviewWidget* pw = new PreviewWidget();
					pw->at = at;
					pw->size = buttonSize;
					pw->text = TXT("turn");
					pw->onClick = [this](Vector2 const & _relClick)
					{
						if (auto* c = this->get_show_actual_objects_controller())
						{
							if (c->get_requested_movement_direction().is_set())
							{
								// switch to forward movement
								c->set_relative_requested_movement_direction(Vector3::yAxis);
								c->clear_requested_orientation();
							}
							float relativeYaw = (2.0f * (_relClick.x - 0.5f));
							if (abs(relativeYaw) < 0.1f)
							{
								relativeYaw = 0.0f;
								c->clear_requested_velocity_orientation();
							}
							else
							{
								c->set_requested_velocity_orientation(Rotator3(0.0f, relativeYaw * 180.0f, 0.0f));
							}
						}
					};
					previewWidgets.push_back(RefCountObjectPtr<PreviewWidget>(pw));
					at.y += buttonSize.y * 1.2f;
				}
				{
					PreviewWidget* pw = new PreviewWidget();
					pw->at = at;
					pw->size = buttonSize;
					pw->text = TXT("stop");
					pw->onClick = [this](Vector2 const & _relClick)
					{
						if (auto* c = this->get_show_actual_objects_controller())
						{
							c->clear_requested_velocity_linear();
							c->clear_requested_movement_direction();
							c->clear_distance_to_stop();
							c->clear_requested_orientation();
							c->clear_requested_look_orientation();
							c->clear_requested_relative_look();
							c->clear_requested_velocity_orientation();
							c->clear_snap_yaw_to_look_orientation();
							c->clear_follow_yaw_to_look_orientation();
						}
					};
					previewWidgets.push_back(RefCountObjectPtr<PreviewWidget>(pw));
					at.y += buttonSize.y * 1.2f;
				}

				// gaits
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (auto* o = showActualObject.get())
				{
					if (auto* m = o->get_movement())
					{
						Array<Name> gaits = m->list_gaits();
						if (!gaits.is_empty())
						{
							at.y += buttonSize.y * 0.5f;
							for_every(gait, gaits)
							{
								{
									PreviewWidget* pw = new PreviewWidget();
									pw->at = at;
									pw->size = buttonSize;
									pw->text = gait->to_string();
									Name gaitName = *gait;
									pw->onClick = [this, gaitName](Vector2 const & _relClick)
									{
										if (auto* c = this->get_show_actual_objects_controller())
										{
											if (!c->get_requested_movement_direction().is_set())
											{
												c->set_relative_requested_movement_direction(Vector3::yAxis);
												c->clear_requested_orientation();
											}
											MovementParameters mp = c->get_requested_movement_parameters();
											mp.gait(gaitName);
											c->set_requested_movement_parameters(mp);
										}
									};
									previewWidgets.push_back(RefCountObjectPtr<PreviewWidget>(pw));
									at.y += buttonSize.y * 1.2f;
								}
							}
						}
					}
				}
#endif
			}
		}
		else if (showControlPanel == ShowControlPanel::VRControllers)
		{
			if (previewWidgets.is_empty())
			{
				Vector2 at = get_main_render_buffer()->get_size().to_vector2();
				Vector2 buttonSize = Vector2(300.0f, 60.0f);
				at.y = buttonSize.y;
				at.x = at.x - buttonSize.x * 0.6f;
				{
					PreviewWidget* pw = new PreviewWidget();
					pw->at = at;
					pw->size = buttonSize;
					pw->text = TXT("right hand");
					pw->onClick = [this](Vector2 const& _relClick)
					{
						previewVRBone = 2;
					};
					previewWidgets.push_back(RefCountObjectPtr<PreviewWidget>(pw));
					at.y += buttonSize.y * 1.2f;
				}
				{
					PreviewWidget* pw = new PreviewWidget();
					pw->at = at;
					pw->size = buttonSize;
					pw->text = TXT("left hand");
					pw->onClick = [this](Vector2 const& _relClick)
					{
						previewVRBone = 1;
					};
					previewWidgets.push_back(RefCountObjectPtr<PreviewWidget>(pw));
					at.y += buttonSize.y * 1.2f;
				}
				{
					PreviewWidget* pw = new PreviewWidget();
					pw->at = at;
					pw->size = buttonSize;
					pw->text = TXT("head");
					pw->onClick = [this](Vector2 const& _relClick)
					{
						previewVRBone = 0;
					};
					previewWidgets.push_back(RefCountObjectPtr<PreviewWidget>(pw));
					at.y += buttonSize.y * 1.2f;
				}
				{
					PreviewWidget* pw = new PreviewWidget();
					pw->at = at;
					pw->size = buttonSize;
					pw->text = TXT("hold rctrl");
					pw->onClick = [](Vector2 const& _relClick)
					{
					};
					previewWidgets.push_back(RefCountObjectPtr<PreviewWidget>(pw));
					at.y += buttonSize.y * 1.2f;
				}
			}
		}
		else
		{
			previewWidgets.clear();
		}
		visibleControlPanel = showControlPanel;

		if (showInfo > 0)
		{
			for_every_ref(pw, previewWidgets)
			{
				::System::Video3DPrimitives::fill_rect_2d(pw->colour.with_alpha(0.3f), pw->at - pw->size * Vector2::half, pw->at + pw->size * Vector2::half);
				font->draw_text_at_with_border(v3d, pw->text, Colour::c64White, Colour::blackWarm, 1.0f, pw->at, Vector2(3.0f, 3.0f), Vector2::half);
			}
		}
	}
}

ModuleController* PreviewGame::get_show_actual_objects_controller() const
{
	if (auto* o = showActualObject.get())
	{
		return o->get_controller();
	}
	return nullptr;
}

void PreviewGame::pre_advance()
{
	if (!droppedTools)
	{
		String dirname = String::printf(TXT("%S%Sdocs"), IO::get_auto_directory_name().to_char(), IO::get_directory_separator());
		IO::Dir::create(dirname);
		String filename = String::printf(TXT("%S%SwmpTools"), dirname.to_char(), IO::get_directory_separator());
		IO::File::delete_file(filename);
		LogInfoContext log;
		log.set_output_to_file(filename);
		WheresMyPoint::RegisteredTools::development_output_all(log);

		droppedTools = true;
	}

	if (mode >= PreviewGameMode::MAX)
	{
		mode = PreviewGameMode::First;
	}
	if (!checkReload.should_wait() && checkReloadTS.get_time_since() > 0.5f)
	{
		checkReload.stop();
		get_job_system()->do_asynchronous_job(background_jobs(), check_library_change_worker_static, this);
	}
#ifdef AN_STANDARD_INPUT
	if (::System::Input::get()->has_key_been_pressed(::System::Key::Space))
	{
		if (mode == PreviewGameMode::Quick ||
			::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl) ||
			::System::Input::get()->is_key_pressed(::System::Key::RightCtrl))
		{
			quickReloadNeeded = true;
			reloadNeeded = true;
		}
		else
		{
			reload_preview_library();
		}
	}
	else
#endif
	if (reloadNeeded)
	{
		if (reloadStarting < 2) // to allow displaying frames
		{
			++ reloadStarting;
		}
		else
		{
			reload_as_needed(quickReloadNeeded);
		}
	}

	blockAutoReloadsFor = max(0, blockAutoReloadsFor - 1);

	if (!testRoom && !testRoomForActualObjects && !testRoomForTemporaryObjects)
	{
		perform_sync_world_job(TXT("create test stuff"), [this]()
		{
			testWorld = new World();
			testSubWorld = new SubWorld(testWorld);
			testSubWorld->init();
			testSubWorld->access_collision_info_provider().set_gravity_dir_based(-Vector3::zAxis, 9.81f);

			push_new_activation_group();

			testRoom = new Room(testSubWorld, nullptr, nullptr, Random::Generator().be_zero_seed());

			testRoomForActualObjects = new Room(testSubWorld, nullptr, nullptr, Random::Generator().be_zero_seed());
			// hardcoded
			testRoomForActualObjects->generate_plane(
				Library::get_current()->get_materials().find(LibraryName(String(TXT("materials:main")))),
				Library::get_current()->get_physical_materials().find(LibraryName(String(TXT("physical materials:generic")))),
				4.5f / 256.0f, false, 0.0f);
			if (testEnvironmentMesh == 1)
			{
				testRoomForActualObjects->generate_hills(
					Library::get_current()->get_materials().find(LibraryName(String(TXT("materials:main")))),
					Library::get_current()->get_physical_materials().find(LibraryName(String(TXT("physical materials:generic")))),
					4.5f / 256.0f, 2.0f, 30.0f, 1.0f, Vector3(0.0f, 20.0f, 0.0f), 150);
			}
			testRoomForActualObjects->update_bounding_box();
			testRoomForActualObjects->mark_nav_mesh_creation_required();
			
			testRoomForTemporaryObjects = new Room(testSubWorld, nullptr, nullptr, Random::Generator().be_zero_seed());
			// hardcoded
			testRoomForTemporaryObjects->generate_plane(
				Library::get_current()->get_materials().find(LibraryName(String(TXT("materials:main")))),
				Library::get_current()->get_physical_materials().find(LibraryName(String(TXT("physical materials:generic")))),
				4.5f / 256.0f, false, -0.3f);
			testRoomForTemporaryObjects->update_bounding_box();
			testRoomForTemporaryObjects->mark_nav_mesh_creation_required();
			
			while (!testWorld->ready_to_activate_all_inactive(get_current_activation_group())) {}
			testWorld->activate_all_inactive(get_current_activation_group());
			
			pop_activation_group();
		});
	}

	an_assert(testRoom);
	an_assert(testRoomForActualObjects);
	an_assert(testRoomForTemporaryObjects);

	{
		if (environments.is_index_valid(environmentIndex) &&
			environments[environmentIndex].environmentType)
		{
			if (!testEnvironment ||
				testEnvironment->get_environment_type() != environments[environmentIndex].environmentType)
			{
				perform_sync_world_job(TXT("enviro"), [this]()
				{
					testRoom->set_environment(nullptr);
					delete_and_clear(testEnvironment);

					push_new_activation_group();

#ifdef AN_DEBUG_RENDERER
					if (debug_renderer_shader_program_binding_context())
					{
						debug_renderer_shader_program_binding_context()->reset();
					}
#endif

					testEnvironment = new Environment(Name::invalid(), Name::invalid(), testSubWorld, nullptr, environments[environmentIndex].environmentType, Random::Generator().be_zero_seed());
					testEnvironment->generate();
					for_every_ptr(eo, environments[environmentIndex].environmentOverlays)
					{
						UsedLibraryStored<EnvironmentOverlay> ulseo;
						ulseo.set(eo);
						testEnvironment->access_environment_overlays().push_back(ulseo);
						testRoom->access_environment_overlays().push_back(ulseo);
						testRoomForActualObjects->access_environment_overlays().push_back(ulseo);
						testRoomForTemporaryObjects->access_environment_overlays().push_back(ulseo);
					}
					testRoom->set_environment(testEnvironment);
					testRoomForActualObjects->set_environment(testEnvironment);
					testRoomForTemporaryObjects->set_environment(testEnvironment);
					while (!testWorld->ready_to_activate_all_inactive(get_current_activation_group())) {}
					testWorld->activate_all_inactive(get_current_activation_group());
					
					pop_activation_group();
				});
			}
		}
	}

#ifdef AN_STANDARD_INPUT
	if (::System::Input::get()->has_key_been_pressed(::System::Key::P))
	{
		optimiseVerticesAlways = !optimiseVerticesAlways;
	}

	if (::System::Input::get()->has_key_been_pressed(::System::Key::N))
	{
		reloadNeeded = true;
		quickReloadNeeded = true;
	}

	if (::System::Input::get()->has_key_been_pressed(::System::Key::M))
	{
		reloadNeeded = true;
		quickReloadNeeded = true;
		reloadOnlyEnvironments = true;
	}

	if (::System::Input::get()->has_key_been_pressed(::System::Key::B))
	{
#ifdef AN_DEVELOPMENT
		usePreviewAILogic = !usePreviewAILogic;
		ModuleAI::set_ai_advanceable(!usePreviewAILogic);
#endif
	}
#endif

	bool deactivateTemporaryObject = true;
	bool deactivateActualObject = true;
	if (!testRoomForCustomPreview)
	{
		testRoomForCustomPreviewForObject = nullptr;
	}
	if (showObject.object && testRoomForCustomPreview && testRoomForCustomPreviewForObject == showObject.object)
	{
		// just keep it
	}
	else if (showObject.object &&
		showObject.object->setup_custom_preview_room(testSubWorld, REF_ testRoomForCustomPreview, mainRandomGenerator))
	{
		testRoomForCustomPreviewForObject = showObject.object;
		// do all world manager jobs, first let any currently running finish
		while (worldManager.is_busy() || worldManager.has_something_to_do())
		{
			while (worldManager.is_performing_async_job() ||
				   worldManager.is_performing_sync_job())
			{
				System::Core::sleep(0.001f);
			}
			while (worldManager.has_something_to_do())
			{
				force_advance_world_manager(true);
			}
			System::Core::sleep(0.001f);
		}
		// use testRoomForCustomPreview
	}
	else if (auto * to = fast_cast<TemporaryObjectType>(showObject.object))
	{
		deactivateTemporaryObject = false;
		if (showTemporaryObject)
		{
			if (! showTemporaryObject->is_active())
			{
				showTemporaryObjectInactive += ::System::Core::get_raw_delta_time();
			}
#ifdef AN_STANDARD_INPUT
			if (showTemporaryObject->get_object_type() != to ||
				::System::Input::get()->has_key_been_pressed(::System::Key::V) ||
				::System::Input::get()->has_key_been_pressed(::System::Key::B) ||
				showTemporaryObjectInactive > 0.5f)
			{
				if (!::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl) &&
					!::System::Input::get()->is_key_pressed(::System::Key::RightCtrl))
				{
					if (showTemporaryObject->is_active())
					{
						showTemporaryObject->mark_to_deactivate();
					}
					showTemporaryObject = nullptr;
					showTemporaryObjectSpawnCompanions = false;
				}
				else
				{
					Framework::ParticlesUtils::desire_to_deactivate(showTemporaryObject);
				}
			}
#endif
		}

		if (showTemporaryObject && showTemporaryObject->is_active() &&
			showTemporaryObjectSpawnCompanions)
		{
			ModuleTemporaryObjects::spawn_companions_of(showTemporaryObject, showTemporaryObject);
			showTemporaryObjectSpawnCompanions = false;
		}

		if (!showTemporaryObject && testRoomForTemporaryObjects)
		{
			if ((showTemporaryObject = testSubWorld->get_one_for(to, testRoomForTemporaryObjects)))
			{
				Transform at = showObjectPlacement;
				Vector3 atLoc = at.get_translation();
				atLoc.z = max(1.0f, atLoc.z);
				at.set_translation(atLoc);
				showTemporaryObject->on_activate_place_in(testRoomForTemporaryObjects, at);
				showTemporaryObject->mark_to_activate();
				showTemporaryObjectSpawnCompanions = true;

				showTemporaryObjectInactive = 0.0f;
			}
		}
	}
	else if (auto * to = fast_cast<ObjectType>(showObject.object))
	{
		deactivateActualObject = false;
		if (!showActualObjectBeingGenerated)
		{
			if (showActualObject.is_set())
			{
#ifdef AN_STANDARD_INPUT
				if (fast_cast<Object>(showActualObject.get())->get_object_type() != to ||
					::System::Input::get()->has_key_been_pressed(::System::Key::V) ||
					::System::Input::get()->has_key_been_pressed(::System::Key::B))
				{
					if (!::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl) &&
						!::System::Input::get()->is_key_pressed(::System::Key::RightCtrl))
					{
						fast_cast<Object>(showActualObject.get())->destroy();
						showActualObject = nullptr;
						// and everything else in the test room
						if (testRoomForActualObjects)
						{
							//FOR_EVERY_OBJECT_IN_ROOM(object, testRoomForActualObjects)
							for_every_ptr(object, testRoomForActualObjects->get_objects())
							{
								object->destroy();
							}
						}
					}
				}
#endif
			}

			if (!showActualObject.is_set() && testRoomForActualObjects)
			{
				showControlPanel = ShowControlPanel::None;

				ActivationGroupPtr activationGroup = push_new_activation_group();

				perform_sync_world_job(TXT("create object"), [this]()
				{
					if (auto * actorType = fast_cast<ActorType>(showObject.object))
					{
						showActualObject = new Actor(actorType, String::printf(TXT("preview")));
						showActualObject->init(testRoomForActualObjects->get_in_sub_world());
					}
					else if (auto * sceneryType = fast_cast<SceneryType>(showObject.object))
					{
						showActualObject = new Scenery(sceneryType, String::printf(TXT("preview")));
						showActualObject->init(testRoomForActualObjects->get_in_sub_world());
					}
					else if (auto * itemType = fast_cast<ItemType>(showObject.object))
					{
						showActualObject = new Item(itemType, String::printf(TXT("preview")));
						showActualObject->init(testRoomForActualObjects->get_in_sub_world());
					}
					else
					{
						an_assert(false);
					}
				});

				pop_activation_group();

				add_async_world_job(TXT("init object"), [this, activationGroup]()
				{
					push_activation_group(activationGroup.get());
					showActualObject->access_individual_random_generator() = mainRandomGenerator;// .spawn();
					showActualObject->initialise_modules();
					if (usePreviewAILogic)
					{
						if (auto* ai = showActualObject->get_ai())
						{
							if (auto* mind = ai->get_mind())
							{
								mind->use_logic(new PreviewAILogic(mind));
								mind->access_locomotion().dont_control();
							}
						}
					}
					Game::get()->perform_sync_world_job(TXT("place object"), [this]()
					{
						showActualObject->get_presence()->place_in_room(testRoomForActualObjects, showObjectPlacement);
					});
					perform_sync_world_job(TXT("activate all inactive"), [this, activationGroup]()
					{
						// ready all, if new are created, ready them too
						while (testWorld && !testWorld->ready_to_activate_all_inactive(activationGroup.get())) {}
						testWorld->activate_all_inactive(activationGroup.get());
					});
					pop_activation_group();
					showActualObjectBeingGenerated = false;
					if (usePreviewAILogic)
					{
						showControlPanel = ShowControlPanel::Locomotion;
						if (auto* o = showActualObject->get_as_object())
						{
							DEFINE_STATIC_NAME(previewVRController);
							if (o->get_tags().get_tag(NAME(previewVRController)))
							{
								showControlPanel = ShowControlPanel::VRControllers;
							}
						}
					}
				});
			}
		}
	}
#ifdef AN_STANDARD_INPUT
	if (::System::Input::get()->has_key_been_pressed(::System::Key::V))
	{
		if (!::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl) &&
			!::System::Input::get()->is_key_pressed(::System::Key::RightCtrl))
		{
			mainRandomGenerator = mainRandomGenerator.spawn();
			output(TXT("using mesh generator base=\"%i\" offset=\"%i\""), mainRandomGenerator.get_seed_hi_number(), mainRandomGenerator.get_seed_lo_number());
#ifdef AN_DEVELOPMENT
			MeshGeneratorRequest::commonRandomGenerator = mainRandomGenerator;
#endif
			quickReloadNeeded = true;
			reloadNeeded = true;
		}
	}
#endif
	if (deactivateTemporaryObject && showTemporaryObject)
	{
		if (showTemporaryObject->is_active())
		{
			showTemporaryObject->mark_to_deactivate();
		}
		showTemporaryObject = nullptr;
		showTemporaryObjectSpawnCompanions = false;
	}
	if (deactivateActualObject && showActualObject.is_set() && !showActualObjectBeingGenerated)
	{
		fast_cast<Object>(showActualObject.get())->destroy();
		showActualObject = nullptr;
		// and everything else in the test room
		if (testRoomForActualObjects)
		{
			//FOR_EVERY_OBJECT_IN_ROOM(object, testRoomForActualObjects)
			for_every_ptr(object, testRoomForActualObjects->get_objects())
			{
				object->destroy();
			}
		}
	}
	if (! showActualObject.is_set())
	{
		showControlPanel = ShowControlPanel::None;
	}

	if (DebugRenderer::get())
	{
		bool shouldBlockFilters = showInfo <= 2;
		bool shouldBlockFilterNoise = showInfo <= 3;
		if (blockFilters != shouldBlockFilters ||
			blockFilterNoise != shouldBlockFilterNoise ||
			forceUpdateBlockFilters)
		{
			forceUpdateBlockFilters = false;
			int blocked = 0;
			int all = 0;
			++all; blocked += ! debugDraw.presence
						   && ! debugDraw.POIs
						   && ! debugDraw.walker
						   && ! debugDraw.skeleton
						   ;
			++all; blocked += shouldBlockFilters ? 1 : 0;
			++all; blocked += shouldBlockFilterNoise ? 1 : 0;
			++all; blocked += showActualObject.get() != nullptr;
			debug_renderer_mode(blocked == all? DebugRendererMode::Off :
							   (blocked > 0? DebugRendererMode::OnlyActiveFilters : DebugRendererMode::All));
			blockFilters = shouldBlockFilters;
			blockFilterNoise = shouldBlockFilterNoise;

			DEFINE_STATIC_NAME(locomotion);
			DEFINE_STATIC_NAME(locomotionPath);
			DEFINE_STATIC_NAME(navMesh);
			DEFINE_STATIC_NAME(moduleMovementInspect);
			DEFINE_STATIC_NAME(moduleMovementDebug);
			DEFINE_STATIC_NAME(moduleMovementPath);
			DEFINE_STATIC_NAME(moduleMovementSwitch);
			DEFINE_STATIC_NAME(moduleCollisionUpdateGradient);
			DEFINE_STATIC_NAME(moduleCollisionQuerries);
			DEFINE_STATIC_NAME(moduleCollisionCheckIfColliding);
			DEFINE_STATIC_NAME(ai_draw);
			DEFINE_STATIC_NAME(ai_enemyPerception);
			DEFINE_STATIC_NAME(ac_walkers);
			DEFINE_STATIC_NAME(ac_walkers_hitLocations);
			DEFINE_STATIC_NAME(ac_footAdjuster);
			DEFINE_STATIC_NAME(ac_ikSolver2);
			DEFINE_STATIC_NAME(ac_ikSolver3);
			DEFINE_STATIC_NAME(ac_tentacle);
			DEFINE_STATIC_NAME(ac_readyHumanoidArm);
			DEFINE_STATIC_NAME(ac_readyHumanoidSpine);
			DEFINE_STATIC_NAME(ac_rotateTowards);
			DEFINE_STATIC_NAME(ac_alignAxes);
			DEFINE_STATIC_NAME(ac_alignmentRoll);
			DEFINE_STATIC_NAME(ac_dragBone);
			DEFINE_STATIC_NAME(ac_elevatorCartRotatingCore);
			DEFINE_STATIC_NAME(ac_elevatorCartAttacherArm);
			DEFINE_STATIC_NAME(ac_gyro);
			DEFINE_STATIC_NAME(pilgrim_physicalViolence);
			DEFINE_STATIC_NAME(soundSceneFindSounds);
			DEFINE_STATIC_NAME(soundSceneShowSounds);
			DEFINE_STATIC_NAME(MissingSounds);
			DEFINE_STATIC_NAME(SoundScene);
			DEFINE_STATIC_NAME(SoundSceneLogEveryFrame);
			DEFINE_STATIC_NAME(MissingTemporaryObjects);
			DEFINE_STATIC_NAME(explosions);
			DEFINE_STATIC_NAME(grabable);
			DEFINE_STATIC_NAME(pilgrimGrab);
			DEFINE_STATIC_NAME(ai_energyKiosk);
			DEFINE_STATIC_NAME(ai_elevatorCart);

			DebugRenderer::get()->clear_filters();
			DebugRenderer::get()->block_filter(NAME(moduleMovementDebug), blockFilters);
			DebugRenderer::get()->block_filter(NAME(moduleMovementPath), blockFilters);
			DebugRenderer::get()->block_filter(NAME(moduleMovementSwitch), blockFilters);
			DebugRenderer::get()->block_filter(NAME(moduleCollisionUpdateGradient), blockFilters);
			DebugRenderer::get()->block_filter(NAME(moduleCollisionQuerries), blockFilters);
			DebugRenderer::get()->block_filter(NAME(moduleCollisionCheckIfColliding), blockFilters);
			DebugRenderer::get()->block_filter(NAME(locomotion), blockFilters);
			DebugRenderer::get()->block_filter(NAME(locomotionPath), blockFilters);
			DebugRenderer::get()->block_filter(NAME(navMesh), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ai_draw), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ai_enemyPerception), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ac_walkers), blockFilters && ! debugDraw.walker);
			DebugRenderer::get()->block_filter(NAME(ac_walkers_hitLocations), blockFilters && ! debugDraw.walker);
			DebugRenderer::get()->block_filter(NAME(ac_footAdjuster), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ac_ikSolver2), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ac_ikSolver3), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ac_tentacle), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ac_readyHumanoidArm), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ac_readyHumanoidSpine), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ac_rotateTowards), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ac_alignAxes), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ac_elevatorCartRotatingCore), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ac_elevatorCartAttacherArm), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ac_alignmentRoll), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ac_dragBone), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ac_gyro), blockFilters);
			DebugRenderer::get()->block_filter(NAME(pilgrim_physicalViolence), blockFilters);
			DebugRenderer::get()->block_filter(NAME(soundSceneShowSounds), blockFilterNoise);
			DebugRenderer::get()->block_filter(NAME(soundSceneFindSounds), blockFilterNoise);
			DebugRenderer::get()->block_filter(NAME(explosions), blockFilters);
			DebugRenderer::get()->block_filter(NAME(grabable), blockFilters);
			DebugRenderer::get()->block_filter(NAME(pilgrimGrab), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ai_energyKiosk), blockFilters);
			DebugRenderer::get()->block_filter(NAME(ai_elevatorCart), blockFilters);
			DebugRenderer::get()->mark_filters_ready();
		}
	}

	if (showActualObject.get())
	{
		process_delayed_object_creation();
	}
	else
	{
		drop_all_delayed_object_creations();
	}
}

void PreviewGame::update_interesting_dirs()
{
	quickInterestingDirEntries.clear();
	environmentInterestingDirEntries.clear();
	
	{
#ifdef LIBRARY_STORED_WITH_LOADED_FROM_FILE
		if (showObject.object)
		{
			IO::DirEntry dirEntry;
			dirEntry.setup_for_file(showObject.object->get_loaded_from_file());
			quickInterestingDirEntries.push_back_unique(dirEntry);
		}
		for_every(meshGen, ::Framework::Library::get_current()->get_mesh_generators().get_all_stored())
		{
			bool reload = showObject.object == meshGen->get();
			if (auto* mesh = fast_cast<Framework::Mesh>(showObject.object))
			{
				if (mesh->get_mesh_generator() == meshGen->get())
				{
					reload = true;
				}
			}
			if (reload)
			{
				IO::DirEntry dirEntry;
				dirEntry.setup_for_file(meshGen->get()->get_loaded_from_file());
				quickInterestingDirEntries.push_back_unique(dirEntry);
#ifdef AN_DEVELOPMENT
				meshGen->get()->for_included_mesh_generators([this](::Framework::MeshGenerator * meshGen)
				{
					IO::DirEntry dirEntry;
					dirEntry.setup_for_file(meshGen->get_loaded_from_file());
					quickInterestingDirEntries.push_back_unique(dirEntry);
				});
#endif
			}
		}
		for_every(mesh, ::Framework::Library::get_current()->get_meshes_static().get_all_stored())
		{
			bool reload = showObject.object == mesh->get();
			if (reload)
			{
				if (mesh->get()->get_mesh_generator())
				{
					IO::DirEntry dirEntry;
					dirEntry.setup_for_file(mesh->get()->get_loaded_from_file());
					quickInterestingDirEntries.push_back_unique(dirEntry);
#ifdef AN_DEVELOPMENT
					mesh->get()->get_mesh_generator()->for_included_mesh_generators([this](::Framework::MeshGenerator * meshGen)
					{
						IO::DirEntry dirEntry;
						dirEntry.setup_for_file(meshGen->get_loaded_from_file());
						quickInterestingDirEntries.push_back_unique(dirEntry);
					});
#endif
				}
			}
		}
		for_every(skel, ::Framework::Library::get_current()->get_skeletons().get_all_stored())
		{
			bool reload = showObject.object == skel->get();
			if (auto* mesh = fast_cast<Framework::Mesh>(showObject.object))
			{
				if (mesh->get_skeleton() == skel->get())
				{
					reload = true;
				}
			}
			if (reload)
			{
				if (skel->get()->get_mesh_generator())
				{
					IO::DirEntry dirEntry;
					dirEntry.setup_for_file(skel->get()->get_loaded_from_file());
					quickInterestingDirEntries.push_back_unique(dirEntry);
#ifdef AN_DEVELOPMENT
					skel->get()->get_mesh_generator()->for_included_mesh_generators([this](::Framework::MeshGenerator * meshGen)
					{
						IO::DirEntry dirEntry;
						dirEntry.setup_for_file(meshGen->get_loaded_from_file());
						quickInterestingDirEntries.push_back_unique(dirEntry);
					});
#endif
				}
			}
		}
		for_every(customData, ::Framework::Library::get_current()->get_custom_datas().get_all_stored())
		{
			IO::DirEntry dirEntry;
			dirEntry.setup_for_file(customData->get()->get_loaded_from_file());
			quickInterestingDirEntries.push_back_unique(dirEntry);
		}
		for_every(eo, ::Framework::Library::get_current()->get_environment_types().get_all_stored())
		{
			IO::DirEntry dirEntry;
			dirEntry.setup_for_file(eo->get()->get_loaded_from_file());
			environmentInterestingDirEntries.push_back_unique(dirEntry);
		}
		for_every(eo, ::Framework::Library::get_current()->get_environment_overlays().get_all_stored())
		{
			IO::DirEntry dirEntry;
			dirEntry.setup_for_file(eo->get()->get_loaded_from_file());
			environmentInterestingDirEntries.push_back_unique(dirEntry);
		}
#endif
	}
}

void PreviewGame::DebugDraw::draw(Framework::IModulesOwner* imo)
{
	if (presence)
	{
		if (auto* p = imo->get_presence())
		{
			p->debug_draw();
		}
	}
	if (POIs)
	{
		if (auto* a = imo->get_appearance())
		{
			a->debug_draw_pois(false);
		}
	}
	if (skeleton)
	{
		if (auto* a = imo->get_appearance())
		{
			a->debug_draw_skeleton();
		}
	}
}

void PreviewGame::game_loop()
{
	MEASURE_PERFORMANCE(gameThread);

	if (testWorld && testSubWorld)
	{
		WorldManagerImmediateSyncJobsScopedLock lockImmediateSyncJobs(this);
		// begin advancing (first ready to advance to advance "advance once" objects
		testWorld->ready_to_advance(testSubWorld);
		testWorld->advance_logic(get_job_system(), worker_jobs(), testSubWorld);
	}

	// wait for render & sound stuff to be done
	renderBlockGate.wait();
	soundBlockGate.wait();

	if (testWorld && testSubWorld)
	{
		WorldManagerImmediateSyncJobsScopedLock lockImmediateSyncJobs(this);
		testWorld->advance_physically(get_job_system(), worker_jobs(), testSubWorld);
		testWorld->advance_temporary_objects(get_job_system(), worker_jobs(), testSubWorld);
		testWorld->advance_build_presence_links(get_job_system(), worker_jobs(), testSubWorld);
		SoftPoolMT<PresenceLink>::redistribute();
		testWorld->advance_finalise_frame(get_job_system(), worker_jobs(), testSubWorld);
		testWorld->finalise_advance(testSubWorld);
	}

	if (!showObject.object && showObject.name.is_valid() && showObject.type.is_valid())
	{
		showObject.object = Library::get_current()->find_stored(showObject.name, showObject.type);
		selectLogInfoLine = true;
	}

	// update log here, so when we enter process_controls with selectLogInfoLine set to true, we will have most recent log
	if (showObject.object)
	{
		if (prevShowObject != showObject.object
#ifdef AN_DEVELOPMENT_OR_PROFILER
			|| prevPreviewLOD != previewLOD
#endif
			)
		{
			Concurrency::ScopedSpinLock lock(logInfoContextLock);
			logInfoContext.clear();
			showObject.object->log(logInfoContext);
			// do not logInfoContext.output_to_output(); as we have log on screen
			prevShowObject = showObject.object;
		}
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
	prevPreviewLOD = previewLOD;
#endif
	
	process_controls();

	if (auto* sbo = fast_cast<SpaceBlockingObject>(showActualObject.get()))
	{
		if (sbo->get_presence())
		{
			debug_push_transform(sbo->get_presence()->get_placement());
			sbo->get_space_blocker_local().debug_draw();
			debug_pop_transform();
		}
	}

	if (testRoomForCustomPreview)
	{
		// no debug draw yet?
	}
	else if (fast_cast<TemporaryObjectType>(showObject.object) ||
		fast_cast<ObjectType>(showObject.object))
	{
		// handled through world / test room
		Transform at = Transform::identity;
		IModulesOwner* ao = nullptr;
		if (auto* sao = showActualObject.get())
		{
			ao = sao;
		}
		else if (showTemporaryObject)
		{
			ao = showTemporaryObject;
		}
		if (ao)
		{
			if (auto* p = ao->get_presence())
			{
				at = p->get_placement();
			}

			if (debugDraw.allObjects && testRoomForActualObjects && !showTemporaryObject)
			{
				FOR_EVERY_OBJECT_IN_ROOM(obj, testRoomForActualObjects)
				{
					debugDraw.draw(obj);
				}
			}
			else
			{
				debugDraw.draw(ao);
			}
		}
		debug_push_transform(at);
		if (showInfo >= 2)
		{
			debug_draw_line(true, Colour::red, Vector3::zero, Vector3::xAxis);
			debug_draw_line(true, Colour::green, Vector3::zero, Vector3::yAxis);
			debug_draw_line(true, Colour::blue, Vector3::zero, Vector3::zAxis);
		}
		debug_pop_transform();
	}
	else if (showObject.object)
	{
		showObject.object->debug_draw();

#ifdef AN_DEVELOPMENT
		if (!MeshGenerator::does_show_generation_info() || !MeshGenerator::does_show_generation_info_only())
#endif
		{
			if (showInfo >= 2)
			{
				debug_draw_line(true, Colour::red, Vector3::zero, Vector3::xAxis);
				debug_draw_line(true, Colour::green, Vector3::zero, Vector3::yAxis);
				debug_draw_line(true, Colour::blue, Vector3::zero, Vector3::zAxis);
			}
			else if (showInfo < 0)
			{
				DebugRenderer::get()->get_gather_frame()->clear_non_mesh();
			}
		}
	}
	else
	{
		if (prevShowObject != showObject.object)
		{
			Concurrency::ScopedSpinLock lock(logInfoContextLock);
			logInfoContext.clear();
			logInfoContext.log(TXT(""));
			logInfoContext.log(TXT("NOT FOUND!"));
			prevShowObject = showObject.object;
		}
		debug_draw_line(true, Colour::red, Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 0.0f, 1.0f));
		debug_draw_triangle(true, Colour::red * Colour::alpha(0.4f), Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 0.0f, 0.8f), Vector3(0.4f, 0.0f, 0.9f));
	}

	if (showInfo >= 0 &&
		showGrid != 0)
	{
		float const maxDist = 5.0f;
		Vector3 axis0 = Vector3::xAxis;
		Vector3 axis1 = Vector3::yAxis;
		if (showGrid == 2)
		{
			axis0 = Vector3::xAxis;
			axis1 = Vector3::zAxis;
		}
		if (showGrid == 3)
		{
			axis0 = Vector3::yAxis;
			axis1 = Vector3::zAxis;
		}
		for_count(int, axis, 2)
		{
			for (float x = -maxDist; x <= maxDist; x += 0.1f)
			{
				float a = 0.2f;
				float ax = x;
				if (abs(ax - round(ax)) < 0.001f)
				{
					a = max(a, 1.0f);
				}
				ax *= 2.0f;
				if (abs(ax - round(ax)) < 0.001f)
				{
					a = max(a, 0.5f);
				}
				{
					Vector3 vAxis = axis == 0 ? axis0 : axis1;
					Vector3 oAxis = axis == 0 ? axis1 : axis0;
					debug_draw_line(false, Colour::greyLight.with_set_alpha(a), vAxis * x + oAxis * maxDist, vAxis * x - oAxis * maxDist);
				}
			}
		}
	}
}

void PreviewGame::process_controls()
{
#ifdef AN_STANDARD_INPUT
	if (::System::Input::get()->is_key_pressed(::System::Key::Esc))
	{
		wantsToQuit = true;
	}

	bool shiftHold = ::System::Input::get()->is_key_pressed(::System::Key::LeftShift) ||
					 ::System::Input::get()->is_key_pressed(::System::Key::RightShift);
	bool ctrlHold = ::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl) ||
					::System::Input::get()->is_key_pressed(::System::Key::RightCtrl);
	bool leftAltHold = ::System::Input::get()->is_key_pressed(::System::Key::LeftAlt);
	bool rightAltHold = ::System::Input::get()->is_key_pressed(::System::Key::RightAlt);
	bool rightCtrlHold = ::System::Input::get()->is_key_pressed(::System::Key::RightCtrl);

	bool grabMouse = false;

	if (rightAltHold && !rightCtrlHold)
	{
		if (::System::Input::get()->has_key_been_pressed(::System::Key::A))
		{
			debugDraw.allObjects = !debugDraw.allObjects;
			forceUpdateBlockFilters = true;
		}
		if (::System::Input::get()->has_key_been_pressed(::System::Key::P))
		{
			debugDraw.presence = !debugDraw.presence;
			forceUpdateBlockFilters = true;
		}
		if (::System::Input::get()->has_key_been_pressed(::System::Key::O))
		{
			debugDraw.POIs = !debugDraw.POIs;
			forceUpdateBlockFilters = true;
		}
		if (::System::Input::get()->has_key_been_pressed(::System::Key::W))
		{
			debugDraw.walker = !debugDraw.walker;
			forceUpdateBlockFilters = true;
		}
		if (::System::Input::get()->has_key_been_pressed(::System::Key::S))
		{
			debugDraw.skeleton = !debugDraw.skeleton;
			forceUpdateBlockFilters = true;
		}
		if (::System::Input::get()->has_key_been_pressed(::System::Key::R))
		{
			previewVRControllers = PreviewVRControllers();
		}
		if (::System::Input::get()->has_key_been_pressed(::System::Key::T))
		{
			previewVRControllers.absolutePlacement = !previewVRControllers.absolutePlacement;
		}
		if (::System::Input::get()->has_key_been_pressed(::System::Key::L))
		{
			if (usePreviewAILogic)
			{
				if (showControlPanel == ShowControlPanel::Locomotion)
				{
					showControlPanel = ShowControlPanel::VRControllers;
				}
				else
				{
					showControlPanel = ShowControlPanel::Locomotion;
				}
			}
		}
	}
	if (rightCtrlHold && showControlPanel == ShowControlPanel::VRControllers)
	{
		grabMouse = true;

		Transform& modify = previewVRBone == 0 ? previewVRControllers.head :
			(previewVRBone == 1 ? previewVRControllers.hands[Hand::Left] :
				previewVRControllers.hands[Hand::Right]);

		{
			Vector3 velocityLocal = Vector3::zero;
			if (::System::Input::get()->is_key_pressed(::System::Key::W))
			{
				velocityLocal.y += 1.0f;
			}
			if (::System::Input::get()->is_key_pressed(::System::Key::S))
			{
				velocityLocal.y -= 1.0f;
			}
			if (::System::Input::get()->is_key_pressed(::System::Key::A))
			{
				velocityLocal.x -= 1.0f;
			}
			if (::System::Input::get()->is_key_pressed(::System::Key::D))
			{
				velocityLocal.x += 1.0f;
			}
			if (::System::Input::get()->is_key_pressed(::System::Key::Q))
			{
				velocityLocal.z -= 1.0f;
			}
			if (::System::Input::get()->is_key_pressed(::System::Key::E))
			{
				velocityLocal.z += 1.0f;
			}

			float speed = 0.15f;
			if (::System::Input::get()->is_key_pressed(::System::Key::LeftShift))
			{
				speed = 0.4f;
			}
			if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl))
			{
				speed = 0.02f;
			}

			Vector3 offset = velocityLocal * speed * deltaTime;
			if (rightAltHold)
			{
				offset = modify.vector_to_world(offset);
			}

			modify.set_translation(modify.get_translation() + offset);
		}
		{
			Rotator3 rotateBy = Rotator3::zero;
			if (::System::Input::get()->is_mouse_button_pressed(2))
			{
				rotateBy.roll += ::System::Input::get()->get_mouse_relative_location().x * 0.1f;
			}
			else
			{
				if (previewVRBone == 0 || rightAltHold)
				{
					rotateBy.yaw += ::System::Input::get()->get_mouse_relative_location().x * 0.1f;
					rotateBy.pitch -= ::System::Input::get()->get_mouse_relative_location().y * 0.1f;
				}
				else
				{
					float dir = previewVRBone == 1? -1.0f : 1.0f;
					rotateBy.pitch += ::System::Input::get()->get_mouse_relative_location().x * 0.1f * dir;
					rotateBy.yaw += ::System::Input::get()->get_mouse_relative_location().y * 0.1f * dir;
				}
			}

			float speed = 45.0f;
			if (::System::Input::get()->is_key_pressed(::System::Key::LeftShift))
			{
				speed = 90.0f;
			}
			if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl))
			{
				speed = 11.25f;
			}

			speed *= 3.0f;
			rotateBy *= speed * deltaTime;

			if (rightAltHold)
			{
				modify.set_orientation(rotateBy.to_quat().to_world(modify.get_orientation()));
			}
			else
			{
				modify.set_orientation(modify.get_orientation().to_world(rotateBy.to_quat()));
			}
		}
	}
	if (!rightAltHold)
	{
		{
	#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (::System::Input::get()->has_key_been_pressed(::System::Key::O))
			{
				generateDevMeshes = mod(generateDevMeshes + (shiftHold ? -1 : 1), 5);
			}
	#endif

			if (::System::Input::get()->has_key_been_pressed(::System::Key::I))
			{
				generateAtLOD = mod(generateAtLOD + (shiftHold ? -1 : 1), 5);
			}

	#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (::System::Input::get()->has_key_been_pressed(::System::Key::U))
			{
				previewLOD = clamp(previewLOD + (shiftHold ? -1 : 1), 0, 5);
			}
	#endif

			if (::System::Input::get()->has_key_been_pressed(::System::Key::Y))
			{
				if (MainConfig::global().get_forced_system() == TXT("quest"))
				{
					MainConfig::access_global().force_system(String::empty());
				}
				else
				{
					MainConfig::access_global().force_system(String(TXT("quest")));
				}

				// required to get proper modifiers
				::System::Core::update_on_system_change();
				update_on_system_change();
				// keep the main config intact
				MainConfig mcStored = MainConfig::global();
				load_config_files();
				MainConfig::access_global() = mcStored;
			}

			if (::System::Input::get()->has_key_been_pressed(::System::Key::J))
			{
				environmentIndex = mod((environmentIndex + (shiftHold ? -1 : 1)), environments.get_size());
			}

			if (::System::Input::get()->has_key_been_pressed(::System::Key::K))
			{
				if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl))
				{
					testEnvironmentMesh = mod(testEnvironmentMesh + 1, 2);
				}
				else
				{
					noRoomMeshes = mod(noRoomMeshes + 1, 3);
				}
			}

			bool doDeselect = selectLogInfoLine;

			{
				Concurrency::ScopedSpinLock lock(logInfoContextLock);

				int prevLogInfoLineIdx = logInfoLineIdx;
				bool allowScroll = false;
				if (::System::Input::get()->has_key_been_pressed(::System::Key::R))
				{
					if (ctrlHold)
					{
						logInfoLineIdx--;
						while (logInfoLineIdx >= 0)
						{
							if (logInfoContext.get_lines()[logInfoLineIdx].indentDepth == 0 &&
								logInfoContext.get_lines()[logInfoLineIdx].text.has_prefix(TXT(":")))
							{
								break;
							}
							logInfoLineIdx--;
						}
					}
					else
					{
						logInfoLineIdx -= shiftHold ? 10 : 1;
					}
					selectLogInfoLine = true;
					doDeselect = true;
					allowScroll = true;
				}
				if (::System::Input::get()->has_key_been_pressed(::System::Key::F))
				{
					if (ctrlHold)
					{
						logInfoLineIdx++;
						while (logInfoLineIdx < logInfoContext.get_lines().get_size())
						{
							if (logInfoContext.get_lines()[logInfoLineIdx].indentDepth == 0 &&
								logInfoContext.get_lines()[logInfoLineIdx].text.has_prefix(TXT(":")))
							{
								break;
							}
							logInfoLineIdx++;
						}
					}
					else
					{
						logInfoLineIdx += shiftHold ? 10 : 1;
					}
					selectLogInfoLine = true;
					doDeselect = true;
					allowScroll = true;
				}
				bool mousePressHandled = false;
				if (!mousePressHandled && ::System::Input::get()->is_mouse_button_pressed(0))
				{
					Vector2 mouseLoc = ::System::Input::get()->get_mouse_window_location();
					if (clickedPreviewWidget)
					{
						auto* pw = clickedPreviewWidget;
						Vector2 br = pw->at - pw->size * Vector2::half;
						Vector2 tl = pw->at + pw->size * Vector2::half;
						if (mouseLoc.x >= br.x && mouseLoc.x <= tl.x &&
							mouseLoc.y >= br.y && mouseLoc.y <= tl.y)
						{
							clickedPreviewWidget = pw;
							Vector2 rel = mouseLoc - br;
							rel /= pw->size;
							rel.x = clamp(rel.x, 0.0f, 1.0f);
							rel.y = clamp(rel.y, 0.0f, 1.0f);
							pw->onClick(rel);
						}
					}
					else
					{
						for_every_ref(pw, previewWidgets)
						{
							if (pw == clickedPreviewWidget || ::System::Input::get()->has_mouse_button_been_pressed(0))
							{
								if (pw->onClick)
								{
									Vector2 br = pw->at - pw->size * Vector2::half;
									Vector2 tl = pw->at + pw->size * Vector2::half;
									if (mouseLoc.x >= br.x && mouseLoc.x <= tl.x &&
										mouseLoc.y >= br.y && mouseLoc.y <= tl.y)
									{
										clickedPreviewWidget = pw;
										Vector2 rel = mouseLoc - br;
										rel /= pw->size;
										pw->onClick(rel);
									}
								}
							}
						}
					}
				}
				else
				{
					clickedPreviewWidget = nullptr;
				}

				if (! mousePressHandled && ::System::Input::get()->has_mouse_button_been_pressed(0))
				{
					Vector2 mouseLoc = ::System::Input::get()->get_mouse_window_location();
					int lineIdx = 0;
					float y = linesStartAtY;
					for_every(line, logInfoContext.get_lines())
					{
						if (lineIdx >= logInfoLineStartIdx && y > 0.0f)
						{
							if (mouseLoc.y > y && mouseLoc.y <= y + linesSeparation &&
								mouseLoc.x >= 0 && mouseLoc.x <= linesCharacterWidth * line->text.get_length())
							{
								logInfoLineIdx = lineIdx;
								selectLogInfoLine = true;
								doDeselect = true;
							}
							y -= linesSeparation;
						}
						++lineIdx;
					}
				}
				if (::System::Input::get()->has_key_been_pressed(::System::Key::MouseWheelDown))
				{
					logInfoLineStartIdx = min(logInfoLineStartIdx + 5, logInfoContext.get_lines().get_size() - visibleLogInfoLines);
				}
				if (::System::Input::get()->has_key_been_pressed(::System::Key::MouseWheelUp))
				{
					logInfoLineStartIdx = max(logInfoLineStartIdx - 5, 0);
				}

				int logInfoLineCount = logInfoContext.get_lines().get_size();
				logInfoLineIdx = clamp(logInfoLineIdx, 0, logInfoLineCount - 1);
				if (allowScroll)
				{	// scroll
					int border = 10;
					if (logInfoLineIdx < logInfoLineStartIdx + border)
					{
						logInfoLineStartIdx = max(0, logInfoLineIdx - border);
					}
					if (logInfoLineIdx > logInfoLineStartIdx + visibleLogInfoLines - border)
					{
						logInfoLineStartIdx = logInfoLineIdx - visibleLogInfoLines + border;
					}
				}

				if ((::System::Input::get()->is_key_pressed(::System::Key::R) && ::System::Input::get()->has_key_been_pressed(::System::Key::F)) ||
					(::System::Input::get()->is_key_pressed(::System::Key::F) && ::System::Input::get()->has_key_been_pressed(::System::Key::R)) ||
					selectLogInfoLine)
				{
					if (logInfoLineIdx >= 0 && logInfoLineIdx < logInfoLineCount)
					{
						int callDeselectOnLine = NONE;
						if (doDeselect &&
							prevLogInfoLineIdx >= 0 && prevLogInfoLineIdx < logInfoLineCount)
						{
							auto const & line = logInfoContext.get_lines()[prevLogInfoLineIdx];
							int depth = line.indentDepth;
							int idx = prevLogInfoLineIdx;
							while (idx > 0)
							{
								auto const & line = logInfoContext.get_lines()[idx];
								if (line.on_deselect && (depth > line.indentDepth || idx == prevLogInfoLineIdx))
								{
									callDeselectOnLine = idx;
									break;
								}
								--idx;
								depth = min(depth, line.indentDepth);
							}
						}
						auto const & line = logInfoContext.get_lines()[logInfoLineIdx];
						int depth = line.indentDepth;
						int idx = logInfoLineIdx;
						while (idx > 0)
						{
							auto const & line = logInfoContext.get_lines()[idx];
							if (line.on_select && (depth > line.indentDepth || idx == logInfoLineIdx))
							{
								if (callDeselectOnLine != NONE)
								{
									if (callDeselectOnLine == idx)
									{
										// do not call deselect and select
										callDeselectOnLine = NONE;
										break;
									}
									logInfoContext.get_lines()[callDeselectOnLine].on_deselect();
									callDeselectOnLine = NONE;
								}
								line.on_select();
								break;
							}
							--idx;
							depth = min(depth, line.indentDepth);
						}
						if (callDeselectOnLine != NONE)
						{
							logInfoContext.get_lines()[callDeselectOnLine].on_deselect();
						}
					}
				}
				selectLogInfoLine = false;
			}

			// do it here, as we will fill log info later and we want to keep selectLogInfoLine to next frame
			if (::System::Input::get()->has_key_been_pressed(::System::Key::Z))
			{
				debug_clear();
				debug_renderer_next_frame_clear();
				debug_clear();
				show_previous_object_from_list();
				selectLogInfoLine = true;
				update_interesting_dirs();
				forceUpdateBlockFilters = true;
			}
			if (::System::Input::get()->has_key_been_pressed(::System::Key::X))
			{
				debug_clear();
				debug_renderer_next_frame_clear();
				debug_clear();
				show_next_object_from_list();
				selectLogInfoLine = true;
				update_interesting_dirs();
				forceUpdateBlockFilters = true;
			}

			if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl))
			{
				if (::System::Input::get()->has_key_been_pressed(::System::Key::KP_Minus))
				{
					previewGrandScale *= 0.8f;
				}
				if (::System::Input::get()->has_key_been_pressed(::System::Key::KP_Plus))
				{
					previewGrandScale *= 1.25f;
				}
			}
			else
			{
				if (::System::Input::get()->has_key_been_pressed(::System::Key::KP_Minus))
				{
					if (textScale.x == 1.0f)
					{
						textScale = Vector2::one * 2.0f;
					}
					else
					{
						textScale = Vector2::one;
					}
				}
			}

			// preview control transforms
			{
				if (auto* ao = showActualObject.get())
				{
					if (auto* gp = ao->get_gameplay_as<ModuleGameplayPreview>())
					{
						if (gp->get_controller_transform_num() > 0)
						{
							if (::System::Input::get()->has_key_been_pressed(::System::Key::KP_Divide))
							{
								--controllerTransformIdx;
							}
							if (::System::Input::get()->has_key_been_pressed(::System::Key::KP_Multiply))
							{
								++controllerTransformIdx;
							}
					
							controllerTransformIdx = mod(controllerTransformIdx, gp->get_controller_transform_num());

							Vector3 moveBy = Vector3::zero;
							Rotator3 rotateBy = Rotator3::zero;

							Vector3 controlBy = Vector3::zero;

							if (::System::Input::get()->is_key_pressed(::System::Key::KP_N4)) { controlBy.x += -1.0f; }
							if (::System::Input::get()->is_key_pressed(::System::Key::KP_N6)) { controlBy.x +=  1.0f; }
							if (::System::Input::get()->is_key_pressed(::System::Key::KP_N2)) { controlBy.y += -1.0f; }
							if (::System::Input::get()->is_key_pressed(::System::Key::KP_N8)) { controlBy.y +=  1.0f; }
							if (leftAltHold)
							{
								if (::System::Input::get()->is_key_pressed(::System::Key::KP_N7)) { controlBy.z += -1.0f; }
								if (::System::Input::get()->is_key_pressed(::System::Key::KP_N9)) { controlBy.z += 1.0f; }
							}
							else
							{
								if (::System::Input::get()->is_key_pressed(::System::Key::KP_N1)) { controlBy.z += -1.0f; }
								if (::System::Input::get()->is_key_pressed(::System::Key::KP_N7)) { controlBy.z += 1.0f; }
							}

							if (leftAltHold)
							{
								float speed = 45.0f;
								if (::System::Input::get()->is_key_pressed(::System::Key::LeftShift))
								{
									speed = 90.0f;
								}
								if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl))
								{
									speed = 11.25f;
								}
								speed *= previewGrandScale;
								rotateBy.yaw += ::System::Core::get_raw_delta_time() * controlBy.x * speed;
								rotateBy.pitch += ::System::Core::get_raw_delta_time() * controlBy.y * speed;
								rotateBy.roll += ::System::Core::get_raw_delta_time() * controlBy.z * speed;
							}
							else
							{
								float speed = 0.2f;
								if (::System::Input::get()->is_key_pressed(::System::Key::LeftShift))
								{
									speed = 0.5f;
								}
								if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl))
								{
									speed = 0.05f;
								}
								speed *= previewGrandScale;
								moveBy += ::System::Core::get_raw_delta_time() * controlBy * speed;
							}

							if (!moveBy.is_zero() ||
								!rotateBy.is_zero())
							{
								Transform placement = gp->get_controller_transform(controllerTransformIdx);
								placement.set_translation(placement.get_translation() + moveBy);
								placement.set_orientation(placement.get_orientation().to_world(rotateBy.to_quat()));
								gp->set_controller_transform(controllerTransformIdx, placement);
							}
						}
					}
				}
			}

			if (::System::Input::get()->has_key_been_pressed(::System::Key::C))
			{
				if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl) ||
					::System::Input::get()->is_key_pressed(::System::Key::RightCtrl))
				{
					autoReloads = !autoReloads;
					blockAutoReloadsFor = 5;
					if (autoReloads)
					{
						store_dir_entries();
					}
				}
				else
				{
					mode = (PreviewGameMode::Type)((mode + 1) % PreviewGameMode::MAX);
				}
			}

			//
			{
				float speed = 1.0f;
				if (::System::Input::get()->is_key_pressed(::System::Key::LeftShift))
				{
					speed = 5.0f;
					if (::System::Input::get()->is_key_pressed(::System::Key::LeftAlt))
					{
						speed = 30.0f;
					}
					if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl))
					{
						speed = 0.02f;
					}
				}
				else if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl))
				{
					speed = 0.2f;
					if (::System::Input::get()->is_key_pressed(::System::Key::LeftAlt))
					{
						speed = 0.02f;
					}
				}
				else
				{
					speed = 1.0f;
				}
				speed *= previewGrandScale;

				Vector3& pivotToUse = cameraMode == 2 && showActualObject.is_set() ? pivotFollow : pivot;

				if (::System::Input::get()->is_key_pressed(::System::Key::L))
				{
					Vector3 velocityLocal = Vector3::zero;
					if (!rightCtrlHold)
					{
						if (::System::Input::get()->is_key_pressed(::System::Key::W))
						{
							velocityLocal.y += 1.0f;
						}
						if (::System::Input::get()->is_key_pressed(::System::Key::S))
						{
							velocityLocal.y -= 1.0f;
						}
						if (::System::Input::get()->is_key_pressed(::System::Key::A))
						{
							velocityLocal.x -= 1.0f;
						}
						if (::System::Input::get()->is_key_pressed(::System::Key::D))
						{
							velocityLocal.x += 1.0f;
						}
						if (::System::Input::get()->is_key_pressed(::System::Key::Q))
						{
							velocityLocal.z -= 1.0f;
						}
						if (::System::Input::get()->is_key_pressed(::System::Key::E))
						{
							velocityLocal.z += 1.0f;
						}
					}

					Rotator3 rotateLocal = Rotator3::zero;
					if (!clickedPreviewWidget && !rightCtrlHold)
					{
						if (::System::Input::get()->is_mouse_button_pressed(0))
						{
							grabMouse = true;
							rotateLocal.yaw += ::System::Input::get()->get_mouse_relative_location().x * 0.3f;
							rotateLocal.pitch += ::System::Input::get()->get_mouse_relative_location().y * 0.3f;
						}
						if (::System::Input::get()->is_mouse_button_pressed(2))
						{
							grabMouse = true;
							rotateLocal.roll += ::System::Input::get()->get_mouse_relative_location().x * 0.3f;
						}
					}

					showObjectPlacement = showObjectPlacement.to_world(Transform(velocityLocal, rotateLocal.to_quat()));
					if (!Transform::same_with_rotation(showObjectPlacement, Transform::identity))
					{
						if (showTemporaryObject)
						{
							showTemporaryObject->get_presence()->request_teleport_within_room(showObjectPlacement);
						}
						if (showActualObject.get())
						{
							showActualObject->get_presence()->request_teleport_within_room(showObjectPlacement);
						}
					}
				}
				else
				{
					if (cameraMode == 0 || cameraMode == 2)
					{
						Vector3 velocityLocal = Vector3::zero;
						if (!rightCtrlHold)
						{
							if (::System::Input::get()->is_key_pressed(::System::Key::W))
							{
								velocityLocal.z += 1.0f;
							}
							if (::System::Input::get()->is_key_pressed(::System::Key::S))
							{
								velocityLocal.z -= 1.0f;
							}
							if (::System::Input::get()->is_key_pressed(::System::Key::A))
							{
								velocityLocal.x -= 1.0f;
							}
							if (::System::Input::get()->is_key_pressed(::System::Key::D))
							{
								velocityLocal.x += 1.0f;
							}
						}

						velocityLocal *= speed;

						pivotToUse += rotation.to_quat().rotate(velocityLocal) * ::System::Core::get_raw_delta_time();

						if (::System::Input::get()->has_key_been_pressed(::System::Key::Q))
						{
							pivotToUse = Vector3::zero;
						}
					
						if (::System::Input::get()->is_key_pressed(::System::Key::H))
						{
							if (::System::Input::get()->has_key_been_pressed(::System::Key::N1))
							{
								pivotToUse = Vector3(0.0f, 0.0f, -0.265f);
								rotation = Rotator3(-40.0f, -45.0f, 0.0f);
								distance = 0.4f;
								noRoomMeshes = 2;
								showInfo = 0;
								find_environment_index(Framework::LibraryName(String(TXT("sky box:interior light lit from below"))));
							}
							if (::System::Input::get()->has_key_been_pressed(::System::Key::N2))
							{
								pivotToUse = Vector3(0.0f, 0.0f, -0.5f);
								rotation = Rotator3(10.0f, -45.0f, 0.0f);
								distance = 0.4f;
								noRoomMeshes = 2;
								showInfo = 0;
								find_environment_index(Framework::LibraryName(String(TXT("sky box:interior light lit from below"))));
							}
							if (::System::Input::get()->has_key_been_pressed(::System::Key::N3))
							{
								pivotToUse = Vector3(0.0f, 0.0f, -0.5f);
								rotation = Rotator3(-10.0f, -45.0f, 0.0f);
								distance = 40.0f;
								noRoomMeshes = 1;
								showInfo = 0;
								find_environment_index(Framework::LibraryName(String(TXT("sky box:06 dusty noon"))));
							}
							if (::System::Input::get()->has_key_been_pressed(::System::Key::N8))
							{
								Transform altPlacement = Transform(Vector3::zero, Rotator3(-90.0f, 0.0f, 0.0f).to_quat());
								if (Transform::same_with_orientation(showObjectPlacement, Transform::identity))
								{
									showObjectPlacement = altPlacement;
								}
								else
								{
									showObjectPlacement = Transform::identity;
								}
								if (showTemporaryObject)
								{
									showTemporaryObject->get_presence()->request_teleport_within_room(showObjectPlacement);
								}
								if (showActualObject.get())
								{
									showActualObject->get_presence()->request_teleport_within_room(showObjectPlacement);
								}
							}
							if (::System::Input::get()->has_key_been_pressed(::System::Key::N9))
							{
	#ifdef TIME_MANIPULATIONS_DEV
								if (System::Core::get_time_pace() == 1.0f)
								{
									System::Core::normal_time_pace();
									System::Core::slow_down_time_pace();
								}
								else
								{
									System::Core::normal_time_pace();
								}
	#endif
							}
							if (::System::Input::get()->has_key_been_pressed(::System::Key::N0))
							{
	#ifdef TIME_MANIPULATIONS_DEV
								if (System::Core::get_time_pace() == 1.0f)
								{
									System::Core::normal_time_pace();
									System::Core::speed_up_time_pace();
									System::Core::speed_up_time_pace();
								}
								else
								{
									System::Core::normal_time_pace();
								}
	#endif
							}
						}


						if (cameraMode == 2 && showActualObject.is_set() && showActualObject->get_presence() && showActualObject->get_presence()->get_in_room())
						{
							pivotToUse = showActualObject->get_presence()->get_centre_of_presence_WS();
						}

						if (!clickedPreviewWidget && !rightCtrlHold)
						{
							if (::System::Input::get()->is_mouse_button_pressed(0))
							{
								grabMouse = true;
								rotation.yaw += ::System::Input::get()->get_mouse_relative_location().x * 0.3f;
								rotation.pitch += ::System::Input::get()->get_mouse_relative_location().y * 0.3f;
							}
							if (::System::Input::get()->is_mouse_button_pressed(1))
							{
								grabMouse = true;
								if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl) ||
									::System::Input::get()->is_key_pressed(::System::Key::RightCtrl))
								{
									fov += ::System::Input::get()->get_mouse_relative_location().y * 0.3f;
									fov = clamp(fov, 1.0f, 178.0f);
								}
								else
								{
									rotation.roll += ::System::Input::get()->get_mouse_relative_location().x * 0.3f;
								}
							}
							if (::System::Input::get()->is_mouse_button_pressed(2))
							{
								grabMouse = true;
								distance -= ::System::Input::get()->get_mouse_relative_location().y * 0.01f * distance;
								distance = clamp(distance, 0.1f, 1000.0f);
							}
						}
					}
					else if (cameraMode == 1)
					{
						Vector3 velocityLocal = Vector3::zero;
						if (!rightCtrlHold)
						{
							if (::System::Input::get()->is_key_pressed(::System::Key::W))
							{
								velocityLocal.y += 1.0f;
							}
							if (::System::Input::get()->is_key_pressed(::System::Key::S))
							{
								velocityLocal.y -= 1.0f;
							}
							if (::System::Input::get()->is_key_pressed(::System::Key::A))
							{
								velocityLocal.x -= 1.0f;
							}
							if (::System::Input::get()->is_key_pressed(::System::Key::D))
							{
								velocityLocal.x += 1.0f;
							}
							if (::System::Input::get()->is_key_pressed(::System::Key::Q))
							{
								velocityLocal.z -= 1.0f;
							}
							if (::System::Input::get()->is_key_pressed(::System::Key::E))
							{
								velocityLocal.z += 1.0f;
							}
						}

						velocityLocal *= speed;

						Rotator3 prevRotation = rotation;
						if (!clickedPreviewWidget && !rightCtrlHold)
						{
							if (::System::Input::get()->is_mouse_button_pressed(0) ||
								::System::Input::get()->is_mouse_button_pressed(2))
							{
								grabMouse = true;
								rotation.yaw += ::System::Input::get()->get_mouse_relative_location().x * 0.3f;
								rotation.pitch -= ::System::Input::get()->get_mouse_relative_location().y * 0.3f;
							}
							if (::System::Input::get()->is_mouse_button_pressed(1))
							{
								grabMouse = true;
								fov += ::System::Input::get()->get_mouse_relative_location().y * 0.3f;
								fov = clamp(fov, 1.0f, 178.0f);
							}
						}

						pivotToUse = pivotToUse - distance * prevRotation.to_quat().to_matrix().get_y_axis();
						auto const rotMat = rotation.to_quat().to_matrix();
						pivotToUse += rotMat.vector_to_world(velocityLocal * ::System::Core::get_raw_delta_time());
						pivotToUse += distance * rotMat.get_y_axis();
					}
				}

				if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl))
				{
					if (::System::Input::get()->is_key_pressed(::System::Key::N1))
					{
						rotation = Rotator3(0.0f, 0.0f, 0.0f);
						if (showGrid) showGrid = 2;
					}
					if (::System::Input::get()->is_key_pressed(::System::Key::N2))
					{
						rotation = Rotator3(0.0f, -90.0f, 0.0f);
						if (showGrid) showGrid = 3;
					}
					if (::System::Input::get()->is_key_pressed(::System::Key::N3))
					{
						rotation = Rotator3(-90.0f, 0.0f, 0.0f);
						if (showGrid) showGrid = 1;
					}
					if (::System::Input::get()->is_key_pressed(::System::Key::N4))
					{
						zRange = z_range_ortho();
						distance = 100.0f;
						fov = 2.0f;
					}
					if (::System::Input::get()->is_key_pressed(::System::Key::N5))
					{
						zRange = z_range_persp();
						distance = 1.0f;
						fov = 90.0f;
					}
				}

				cameraPlacement = Transform(pivotToUse - distance * rotation.to_quat().to_matrix().get_y_axis(), rotation.to_quat());

				if (::System::Input::get()->has_key_been_pressed(::System::Key::G))
				{
					showGrid = mod(showGrid + 1, 4);
				}

				if (::System::Input::get()->has_key_been_pressed(::System::Key::Tilde))
				{
					if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl) ||
						::System::Input::get()->is_key_pressed(::System::Key::RightCtrl))
					{
						// -show, -only
						// +show, -only
						// +show, +only

	#ifdef AN_DEVELOPMENT
						if (MeshGenerator::does_show_generation_info())
						{
							if (MeshGenerator::does_show_generation_info_only())
							{
								MeshGenerator::show_generation_info(false);
							}
							else
							{
								MeshGenerator::show_generation_info(true, true);
							}
						}
						else
						{
							MeshGenerator::show_generation_info(true, false);
						}
	#endif
					}
					else
					{
						if (::System::Input::get()->is_key_pressed(::System::Key::LeftShift) ||
							::System::Input::get()->is_key_pressed(::System::Key::RightShift))
						{
							--showInfo;
						}
						else
						{
							++showInfo;
						}
						int const minShow = -1;
						int const maxShow = 4;
						if (showInfo < minShow)
						{
							showInfo = maxShow;
						}
						if (showInfo > maxShow)
						{
							showInfo = minShow;
						}
					}
				}
				if (::System::Input::get()->has_key_been_pressed(::System::Key::Tab))
				{
					if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl) ||
						::System::Input::get()->is_key_pressed(::System::Key::RightCtrl))
					{
						fov = 90.0f;
						rotation.roll = 0.0f;
					}
					cameraMode = (cameraMode + 1) % 3;
				}
			}
		}
	}
	//
	::System::Input::get()->grab(grabMouse);
#endif
}

void PreviewGame::load_library_and_prepare_worker(Concurrency::JobExecutionContext const * _executionContext, Framework::GameLoadingLibraryContext* gameLoadingLibraryContext)
{
	PreviewGame* previewGame = (PreviewGame*)gameLoadingLibraryContext->game;

	output_colour(0, 1, 1, 0);
	output(TXT("  load..."));
	output_colour();
	previewGame->loadLibraryResult = true;
	Framework::Library::get_current()->before_reload(); // in case we're reloading the library, we may need to reinitialise some stuff
	previewGame->loadLibraryResult &= Framework::Library::get_current()->load_from_directories(gameLoadingLibraryContext->fromDirectories, LibrarySource::Assets, gameLoadingLibraryContext->libraryLoadLevel);
	for_every(file, gameLoadingLibraryContext->loadFiles)
	{
		previewGame->loadLibraryResult &= Framework::Library::get_current()->load_from_file(*file, LibrarySource::Assets, gameLoadingLibraryContext->libraryLoadLevel);
	}

	Framework::Library::get_current()->wait_till_loaded();

	// add defaults after everything is loaded - because maybe we already loaded defaults from file!
	output_colour(0, 1, 1, 0);
	output(TXT("  add defaults..."));
	output_colour();
	Library::get_current()->add_defaults();

	output_colour(0, 1, 1, 0);
	output(TXT("  prepare..."));
	output_colour();
	previewGame->prepareLibraryResult = Library::get_current()->prepare_for_game();

	for_every(environment, environments)
	{
		environment->environmentType = nullptr;
		if (environment->useEnvironmentType.is_valid())
		{
			environment->environmentType = Library::get_current()->get_environment_types().find(environment->useEnvironmentType);
		}
		environment->environmentOverlays.clear();
		for_every(aeo, environment->addEnvironmentOverlays)
		{
			if (auto* eo = Library::get_current()->get_environment_overlays().find(*aeo))
			{
				environment->environmentOverlays.push_back(eo);
			}
		}
	}

	// not in debug
#ifdef AN_ALLOW_QUICK
	store_dir_entries();
#endif
	output_colour(0, 1, 1, 0);
	output(TXT("  done..."));
	output_colour();
}

void PreviewGame::check_library_change_worker_static(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	PreviewGame* previewGame = (PreviewGame*)_data;
	if (previewGame->reloadNeeded || previewGame->reloadInProgress)
	{
		previewGame->checkReload.allow();
		return;
	}

	// not in debug
#ifdef AN_ALLOW_QUICK
	if (previewGame->autoReloads)
	{
		Array<IO::DirEntry> dirEntries;
		store_dir_entries(REF_ dirEntries, previewGame->get_system_library_directory());
		if (!previewGame->noLibrary)
		{
			if (previewGame->useLibraryDirectories.is_empty())
			{
				store_dir_entries(REF_ dirEntries, previewGame->get_library_directory());
			}
			else
			{
				for_every(libraryDirectory, previewGame->useLibraryDirectories)
				{
					store_dir_entries(REF_ dirEntries, *libraryDirectory);
				}
			}
		}
		if (previewGame->mode == PreviewGameMode::Normal)
		{
			// compare
			previewGame->dirEntriesLock.acquire();
			if (dirEntries.get_size() != previewGame->dirEntries.get_size())
			{
				previewGame->reloadNeeded = true;
			}
			else
			{
				for (auto iDE = dirEntries.begin(), iDEEnd = dirEntries.end(), ipgDE = previewGame->dirEntries.begin(); iDE != iDEEnd; ++iDE, ++ipgDE)
				{
					if (*iDE != *ipgDE)
					{
						previewGame->reloadNeeded = true;
						break;
					}
				}
			}
			previewGame->dirEntriesLock.release();
		}
		if (previewGame->mode == PreviewGameMode::Quick)
		{
#ifdef AN_DEVELOPMENT
			for_every(dirEntry, previewGame->quickInterestingDirEntries)
			{
				IO::DirEntry currentDirEntry;
				currentDirEntry.setup_for_file(dirEntry->path);
				if (currentDirEntry != *dirEntry)
				{
					previewGame->reloadNeeded = true;
					break;
				}
			}
#endif
		}
		if (previewGame->reloadNeeded && previewGame->blockAutoReloadsFor > 0)
		{
			previewGame->reloadNeeded = false;
		}
	}
#endif

	previewGame->checkReloadTS.reset();
	previewGame->checkReload.allow();
}

void PreviewGame::store_dir_entries()
{
	// store all dir entries
	dirEntriesLock.acquire();
	dirEntries.clear();
	store_dir_entries(REF_ dirEntries, get_system_library_directory());
	if (!noLibrary)
	{
		if (useLibraryDirectories.is_empty())
		{
			store_dir_entries(REF_ dirEntries, get_library_directory());
		}
		else
		{
			for_every(libraryDirectory, useLibraryDirectories)
			{
				store_dir_entries(REF_ dirEntries, *libraryDirectory);
			}
		}
	}
	dirEntriesLock.release();
}

void PreviewGame::store_dir_entries(REF_ Array<IO::DirEntry> & _dirEntries, String const & _loadFromDir)
{
	IO::Dir dir;
	dir.list(_loadFromDir);
	_dirEntries.add_from(dir.get_infos());
	for_every(directory, dir.get_directories())
	{
		store_dir_entries(REF_ _dirEntries, _loadFromDir + TXT("/") + *directory);
	}
}

void PreviewGame::add_to_show_object_list(LibraryName const & _name, Name const & _type)
{
	if (_name.is_valid())
	{
		ShowObject object;
		object.object = nullptr;
		object.name = _name;
		object.type = _type;
		showObjectList.push_back(object);
	}
}

void PreviewGame::show_next_object_from_list()
{
	if (showObjectList.is_empty())
	{
		return;
	}
	auto iObject = showObjectList.begin();
	while (iObject != showObjectList.end())
	{
		if (iObject->name == showObject.name &&
			iObject->type == showObject.type)
		{
			break;
		}
		++iObject;
	}
	if (iObject != showObjectList.end())
	{
		++iObject;
		if (iObject == showObjectList.end())
		{
			iObject = showObjectList.begin();
		}
	}
	else
	{
		iObject = showObjectList.begin();
	}
	if (iObject)
	{
		show_object(iObject->name, iObject->type);
	}
}

void PreviewGame::show_previous_object_from_list()
{
	if (showObjectList.is_empty())
	{
		return;
	}
	auto iObject = showObjectList.begin();
	while (iObject != showObjectList.end())
	{
		if (iObject->name == showObject.name &&
			iObject->type == showObject.type)
		{
			break;
		}
		++iObject;
	}
	if (iObject != showObjectList.end() && iObject != showObjectList.begin())
	{
		--iObject;
	}
	else
	{
		iObject = showObjectList.end();
		--iObject;
	}
	if (iObject)
	{
		show_object(iObject->name, iObject->type);
	}
}

void PreviewGame::show_object(LibraryName const & _name, Name const & _type)
{
	showObject.name = _name;
	showObject.type = _type;
	showObject.object = Library::get_current()->find_stored(_name, _type);
	if (auto* m = fast_cast<Framework::Mesh>(showObject.object))
	{
		m->generated_on_demand_if_required();
	}
}

void PreviewGame::close_post_process_chain()
{
	if (!postProcessChain)
	{
		return;
	}
	postProcessChain->clear();
}

void PreviewGame::before_game_starts()
{
	base::before_game_starts();

	create_post_process_chain();

	an_assert(soundScene == nullptr);
	soundScene = new ::Framework::Sound::Scene();
}

void PreviewGame::create_post_process_chain()
{
	if (!postProcessChain)
	{
		return;
	}
	postProcessChain->clear();

	if (Framework::PostProcessChainDefinition* chainDef = Framework::Library::get_current()->get_global_references().get<Framework::PostProcessChainDefinition>(
		Name(String(TXT("post process chain preview")))))
	{
		chainDef->setup(postProcessChain);
	}
}

void PreviewGame::destroy_test()
{
	cancel_world_managers_jobs(true);
	perform_sync_world_job(TXT("destroy"), [this]()
	{
		showTemporaryObject = nullptr;
		showTemporaryObjectSpawnCompanions = false;
		showActualObject = nullptr;
		renderScene = nullptr;
		delete_and_clear(testEnvironment);
		delete_and_clear(testRoomForCustomPreview);
		delete_and_clear(testRoom);
		delete_and_clear(testRoomForActualObjects);
		delete_and_clear(testRoomForTemporaryObjects);
		delete_and_clear(testSubWorld);
		delete_and_clear(testWorld);
	});
}

void PreviewGame::store_lod_info(String const& _aiName, int _lodLevel, float _size, float _dist, float _aggressiveLOD, float _aggressiveLODStartOffset, int _triCount)
{
	Concurrency::ScopedSpinLock lock(usedLODInfoLock);

	UsedLODInfo ulodi;
	ulodi.aiName = _aiName;
	ulodi.lod = _lodLevel;
	ulodi.size = _size;
	ulodi.dist = _dist;
	ulodi.aggressiveLOD = _aggressiveLOD;
	ulodi.aggressiveLODStartOffset = _aggressiveLODStartOffset;
	ulodi.triCount = _triCount;
	usedLODInfo.push_back(ulodi);
}

void PreviewGame::find_environment_index(Framework::LibraryName const& _ln)
{
	for_every(env, environments)
	{
		if (env->environmentType &&
			env->environmentType->get_name() == _ln)
		{
			environmentIndex = for_everys_index(env);
			break;
		}
	}
}
