#include "renderScene.h"

#include "environmentProxy.h"
#include "presenceLinkProxy.h"
#include "renderCamera.h"
#include "renderContext.h"
#include "renderSystem.h"
#include "sceneBuildContext.h"
#include "sceneRenderContext.h"

#include "..\display\display.h"
#include "..\modulesOwner\modulesOwner.h"

#include "..\world\room.h"

#include "..\..\core\globalSettings.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\mesh\mesh3d.h"
#include "..\..\core\vr\iVR.h"
#include "..\..\core\system\input.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\renderTargetSave.h"
#include "..\..\core\system\video\videoClipPlaneStackImpl.inl"

//

#ifdef USE_LESS_PIXEL_FOR_FOVEATED_RENDERING
#define USE_LESS_PIXEL
#endif

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT
//#define AN_TEST_PRE_RENDER_OFFSET
#ifdef AN_TEST_PRE_RENDER_OFFSET
#include "..\..\core\system\core.h"
#endif
#endif

//

using namespace Framework;
using namespace Framework::Render;

//

DEFINE_STATIC_NAME(fpp);
DEFINE_STATIC_NAME(tpp);
DEFINE_STATIC_NAME(foreground);
DEFINE_STATIC_NAME(foregroundOwnerLocal);
DEFINE_STATIC_NAME(foregroundCameraLocal);
DEFINE_STATIC_NAME(foregroundCameraLocalOwnerOrientation);
DEFINE_STATIC_NAME(scaleOutPosition);

DEFINE_STATIC_NAME(inColour);
DEFINE_STATIC_NAME(inRadius2x);
DEFINE_STATIC_NAME(inRadius4x);
DEFINE_STATIC_NAME(inRadius8x);
DEFINE_STATIC_NAME(inResolveHelper);

//

Scene* Scene::build(Render::Camera const & _camera, Optional<SceneBuildRequest> const& _sceneBuildRequest, std::function<void(SceneBuildContext& _context)> _alterContext, Optional<SceneView::Type> _sceneView, Optional<SceneMode::Type> _sceneMode, Optional<float> _vrScale, Optional<AllowVRInputOnRender> _allowVRInputOnRender)
{
	MEASURE_PERFORMANCE_COLOURED(buildRenderScene, Colour::zxYellowBright);

	// we've already switched frames to render, but we still want go gather data for this frame
	debug_gather_in_renderer();

	_sceneMode.set_if_not_set(SceneMode::Normal);
	_vrScale.set_if_not_set(1.0f);
	_allowVRInputOnRender.set_if_not_set(AllowVRInputOnRender(true));

	Scene* scene = get_one();
	scene->view = _sceneView.get(SceneView::FromCamera);

#ifdef RENDER_SCENE_WITH_ON_SHOULD_ADD_TO_RENDER_SCENE
	if (_sceneBuildRequest.is_set() &&
		_sceneBuildRequest.get().on_should_add_to_render_scene)
	{
		scene->set_on_should_add_to_render_scene(_sceneBuildRequest.get().on_should_add_to_render_scene);
	}
#endif

#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
	if (_sceneBuildRequest.is_set() &&
		_sceneBuildRequest.get().on_prepare_to_render)
	{
		scene->set_on_prepare_to_render(_sceneBuildRequest.get().on_prepare_to_render);
	}
#endif

	if (VR::IVR::get() && VR::IVR::get()->is_render_view_valid())
	{
		scene->preparedForVRView = VR::IVR::get()->get_render_view();
	}
	else
	{
		scene->preparedForVRView.clear();
	}
	scene->allowVRInputOnRender = _allowVRInputOnRender.get();

	{	// build room proxy
		// store camera
		scene->vr = _sceneMode.get() == SceneMode::VR_Left ||
					_sceneMode.get() == SceneMode::VR_Right;
		scene->vrEyeIdx = _sceneMode.get() == SceneMode::VR_Right ? 1 : 0;
		scene->camera = _camera;
		scene->viewOffset = Transform::identity;

		if (scene->vr)
		{
			auto const& ri = VR::IVR::get()->get_render_info();
			scene->viewOffset = ri.get_eye_offset_to_use(scene->vrEyeIdx).to_world(Transform(Vector3::zero, Quat::identity, _vrScale.get()));
			scene->camera.set_eye_idx(scene->vrEyeIdx);
			scene->camera.adjust_for_vr((VR::Eye::Type)scene->vrEyeIdx, scene->viewOffset); // keep within same room if not using separate scenes!
			VectorInt2 viewOrigin = VectorInt2::zero;
			VectorInt2 viewSize = VR::IVR::get()->get_render_size((VR::Eye::Type)scene->vrEyeIdx);
			float aspectRatioCoef = VR::IVR::get()->get_render_aspect_ratio_coef((VR::Eye::Type)scene->vrEyeIdx);
			VectorInt2 viewSizeWithAspectRatioCoef = get_full_for_aspect_ratio_coef(viewSize, aspectRatioCoef);
			scene->camera.set_view_origin(viewOrigin);
			scene->camera.set_view_size(viewSize);
			scene->camera.set_view_aspect_ratio(aspect_ratio(viewSizeWithAspectRatioCoef));
		}
#ifdef AN_TEST_PRE_RENDER_OFFSET
		else
		{
			// pretend we're left eye
			scene->viewOffset = Transform(Vector3(-0.03f, 0.0f, 0.0f), Quat::identity);
			scene->camera.offset_location(scene->viewOffset.get_translation());
		}
#endif

		scene->renderCamera = scene->camera; // store as we may change it later
		scene->sceneBuildContext.access_build_request() = _sceneBuildRequest.get(SceneBuildRequest());
		scene->sceneBuildContext.setup(*scene, scene->camera);

#ifdef AN_RENDERER_STATS
		Stats::get().renderedScenes.increase();
		scene->sceneBuildContext.access_stats().renderedScenes.increase();
#endif

		todo_note(TXT("TN#3 get this name from some config? settings?"));
		static bool fpp = true;
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
		if (::System::Input::get()->has_key_been_pressed(::System::Key::N0))
		{
			fpp = !fpp;
		}
#endif
#endif
		scene->sceneBuildContext.set_appearance_name_for_owner_to_build(fpp ? NAME(fpp) : NAME(tpp));
		scene->sceneBuildContext.set_appearance_name_for_non_owner_to_build(NAME(tpp));

#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
		if (::System::Input::get()->has_key_been_pressed(::System::Key::I))
		{
			output_colour(1, 1, 0, 1);
			output(TXT(" scene"));
			output_colour();
		}
#endif
#endif

		scene->sceneBuildContext.reset_custom();
		if (_alterContext)
		{
			_alterContext(scene->sceneBuildContext);
		}

		// build rooms
		scene->room = RoomProxy::build(scene->sceneBuildContext, scene->camera.get_in_room());

		// store anchor to vr space
		scene->vrAnchor = scene->camera.get_vr_anchor();

		// build front plane
		scene->foregroundCameraOffset = Transform::identity;
		scene->foregroundPresenceLinksRenderingBuffer.clear();
		if (scene->camera.get_owner() && fpp)
		{
#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
			if (::System::Input::get()->has_key_been_pressed(::System::Key::I))
			{
				output_colour(1, 1, 0, 0);
				output(TXT(" fpp"));
				output_colour();
			}
#endif
#endif

			todo_note(TXT("TN#3 get this name from some config? settings?"));
			if (!scene->sceneBuildContext.get_build_request().noForeground)
			{
				PresenceLinkProxy::build_owner_for_foreground(scene->sceneBuildContext, NAME(foreground),
					[scene](PresenceLinkProxy* _link) { PresenceLinkProxy::add_to_list(REF_ scene->foregroundPresenceLinks, _link); }
				);
				if (!scene->sceneBuildContext.get_build_request().noForegroundOwnerLocal)
				{
					PresenceLinkProxy::build_owner_for_foreground_owner_local(scene->sceneBuildContext, NAME(foregroundOwnerLocal),
						[scene](PresenceLinkProxy* _link) { PresenceLinkProxy::add_to_list(REF_ scene->foregroundOwnerLocalPresenceLinks, _link); }
					);
				}
				if (!scene->sceneBuildContext.get_build_request().noForegroundCameraLocal)
				{
					PresenceLinkProxy::build_owner_for_foreground_camera_local(scene->sceneBuildContext, NAME(foregroundCameraLocal),
						[scene](PresenceLinkProxy* _link) { PresenceLinkProxy::add_to_list(REF_ scene->foregroundCameraLocalPresenceLinks, _link); }
					);
				}
				if (!scene->sceneBuildContext.get_build_request().noForegroundCameraLocalOwnerOrientation)
				{
					PresenceLinkProxy::build_owner_for_foreground_camera_local_owner_orientation(scene->sceneBuildContext, NAME(foregroundCameraLocalOwnerOrientation),
						[scene](PresenceLinkProxy* _link) { PresenceLinkProxy::add_to_list(REF_ scene->foregroundCameraLocalOwnerOrientationPresenceLinks, _link); }
					);
				}
			}

			if (scene->foregroundPresenceLinks)
			{
				scene->foregroundPresenceLinks->add_all_to(scene->sceneBuildContext, scene->foregroundPresenceLinksRenderingBuffer);
			}
			
			if (scene->foregroundOwnerLocalPresenceLinks)
			{
				scene->foregroundOwnerLocalPresenceLinks->add_all_to(scene->sceneBuildContext, scene->foregroundPresenceLinksRenderingBuffer);
			}

			if (scene->foregroundCameraLocalPresenceLinks)
			{
				scene->foregroundCameraLocalPresenceLinks->add_all_to(scene->sceneBuildContext, scene->foregroundPresenceLinksRenderingBuffer, Matrix44::identity, true); // relative to camera
			}

			if (scene->foregroundCameraLocalOwnerOrientationPresenceLinks)
			{
				scene->foregroundCameraLocalOwnerOrientationPresenceLinks->add_all_to(scene->sceneBuildContext, scene->foregroundPresenceLinksRenderingBuffer);// , Matrix44::identity, true); // relative to camera... partially
			}

			scene->foregroundPresenceLinksRenderingBuffer.sort(scene->sceneBuildContext.get_view_matrix());
		}

		scene->additionalMeshes.clear();
		scene->additionalBackgroundMeshes.clear();
		scene->additionalMeshesRenderingBuffer.clear();
		scene->additionalBackgroundMeshesRenderingBuffer.clear();
#ifdef AN_DEVELOPMENT
		scene->additionalMeshesPrepared = true;
#endif

#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
		if (::System::Input::get()->has_key_been_pressed(::System::Key::I))
		{
			output_colour(1, 1, 0, 0);
			output(TXT(" scene done"));
			output_colour();
			output(TXT("   room proxies          : %5i"), scene->sceneBuildContext.roomProxyCount);
			output(TXT("   presence link proxies : %5i"), scene->sceneBuildContext.presenceLinkProxyCount);
			output(TXT("   door in room proxies  : %5i"), scene->sceneBuildContext.doorInRoomProxyCount);
		}
#endif
#endif
	}

	debug_gather_restore();

	return scene;
}

void Scene::add_extra(Meshes::Mesh3DInstance const * _mesh, Transform const & _placement, bool _background)
{
	if (!_mesh)
	{
		return;
	}
	if (_background)
	{
		additionalBackgroundMeshes.hard_copy_add(*_mesh, _placement);
	}
	else
	{
		additionalMeshes.hard_copy_add(*_mesh, _placement);
	}
#ifdef AN_DEVELOPMENT
	additionalMeshesPrepared = false;
#endif
}

void Scene::prepare_extras()
{
	additionalMeshesRenderingBuffer.clear();
	additionalMeshes.add_to(additionalMeshesRenderingBuffer);
	additionalMeshesRenderingBuffer.sort(sceneBuildContext.get_view_matrix());
	additionalBackgroundMeshesRenderingBuffer.clear();
	additionalBackgroundMeshes.add_to(additionalBackgroundMeshesRenderingBuffer);
	additionalBackgroundMeshesRenderingBuffer.sort(sceneBuildContext.get_view_matrix());
#ifdef AN_DEVELOPMENT
	additionalMeshesPrepared = true;
#endif
}

void Scene::on_get()
{
	SoftPooledObject<Scene>::on_get();
	backgroundColour = Colour::blackWarm;
	backgroundMaterialInstance = nullptr;
}

void Scene::on_release()
{
	preparedForVRView.clear();
	on_pre_render = nullptr;
	on_post_render = nullptr;
	if (foregroundPresenceLinks)
	{
		foregroundPresenceLinks->release();
		foregroundPresenceLinks = nullptr;
	}	
	if (foregroundOwnerLocalPresenceLinks)
	{
		foregroundOwnerLocalPresenceLinks->release();
		foregroundOwnerLocalPresenceLinks = nullptr;
	}
	if (foregroundCameraLocalPresenceLinks)
	{
		foregroundCameraLocalPresenceLinks->release();
		foregroundCameraLocalPresenceLinks = nullptr;
	}
	if (foregroundCameraLocalOwnerOrientationPresenceLinks)
	{
		foregroundCameraLocalOwnerOrientationPresenceLinks->release();
		foregroundCameraLocalOwnerOrientationPresenceLinks = nullptr;
	}
	if (room)
	{
		room->release();
		room = nullptr;
	}
	displays.clear();
	SoftPooledObject<Scene>::on_release();
}

void Scene::render(REF_ Render::Context & _context, ::System::RenderTarget* _nonVRRenderTarget)
{
	MEASURE_PERFORMANCE(renderSceneRender);

#ifdef AN_USE_RENDER_CONTEXT_DEBUG
	_context.access_debug().start_rendering();
#endif

	DRAW_INFO(TXT("render scene%S"), vr ? (vrEyeIdx == VR::Eye::Left ? TXT(" (left)") : TXT(" (right)")) : TXT(""));

	DRAW_INFO(TXT("update displays for scene"))
	// update displays first
	{
		MEASURE_PERFORMANCE_COLOURED(renderScene_updateDisplays, Colour::zxGreen);
		for_every_ref(display, displays)
		{
			DRAW_INFO(TXT("display %i"), for_everys_index(display));
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
			MEASURE_PERFORMANCE_COLOURED(renderScene_updateDisplay, Colour::zxGreenBright);
#endif
			display->update_display();
		}
	}

	DRAW_INFO(TXT("render scene"))

	::System::Video3D * v3d = ::System::Video3D::get();

	_context.set_vr(vr);

	_context.set_eye_idx(vrEyeIdx);

	v3d->set_eye_offset(viewOffset.get_translation());

	::System::RenderTarget* renderToRT = _nonVRRenderTarget;

	if (vr)
	{
		::System::RenderTarget* rt = nullptr;
		rt = VR::IVR::get()->get_render_render_target((VR::Eye::Type)vrEyeIdx);
		an_assert(rt);
		renderToRT = rt;
	}
	else
	{
		an_assert(_nonVRRenderTarget);
	}

	if (!renderToRT)
	{
		warn(TXT("no valid render target, skip render for one frame"));
		return;
	}

	::System::RenderTarget* actuallyRenderToRT = renderToRT;
#ifdef USE_LESS_PIXEL
	bool setupRenderToRT = false;
	bool useLessPixel = true;
	float useLessPixelLevel = 0.0f;
	int useLessPixelMaxDepth = 1000;
	bool testUseLessPixel = false;
	if (::System::FoveatedRenderingSetup::should_test_use_less_pixel_for_foveated_rendering())
	{
		an_assert(::System::FoveatedRenderingSetup::get_test_use_less_pixel_for_foveated_rendering().is_set());
		useLessPixelLevel = ::System::FoveatedRenderingSetup::get_test_use_less_pixel_for_foveated_rendering().get();
		useLessPixelMaxDepth = ::System::FoveatedRenderingSetup::get_test_use_less_pixel_for_foveated_rendering_maxDepth().get();
		useLessPixel = true;
		testUseLessPixel = true;
	}
	else
	{
		if (useLessPixel)
		{
			useLessPixelLevel = MainConfig::global().get_video().vrFoveatedRenderingLevel;
			useLessPixelMaxDepth = max(1, MainConfig::global().get_video().vrFoveatedRenderingMaxDepth);
		}
		if (useLessPixel &&
			::System::FoveatedRenderingSetup::is_temporary_no_foveation())
		{
			useLessPixel = false;
		}
		if (useLessPixel)
		{
			if (::System::FoveatedRenderingSetup::is_hi_res_scene())
			{
				useLessPixel = false;
			}
		}
	}

	if (useLessPixelLevel <= 0.0f)
	{
		useLessPixel = false;
	}

	::System::ShaderProgram* lessPixelPrepareStencilShaderProgram = nullptr;
	::System::ShaderProgram* lessPixelPrepareResolveShaderProgram = nullptr;
	::System::ShaderProgram* lessPixelResolveShaderProgram = nullptr;
	if (useLessPixel)
	{
		useLessPixel = false;
		lessPixelPrepareStencilShaderProgram = System::get_less_pixel_prepare_stencil_shader_program();
		if (lessPixelPrepareStencilShaderProgram)
		{
			lessPixelPrepareResolveShaderProgram = System::get_less_pixel_prepare_resolve_shader_program();
			if (testUseLessPixel)
			{
				lessPixelResolveShaderProgram = System::get_less_pixel_test_resolve_shader_program();
			}
			else
			{
				lessPixelResolveShaderProgram = System::get_less_pixel_resolve_shader_program();
			}
		}
		if (lessPixelPrepareStencilShaderProgram &&
			lessPixelPrepareResolveShaderProgram &&
			lessPixelResolveShaderProgram)
		{
			useLessPixel = true;
		}
	}

	float useLessPixelRadius2x = 0.25f;
	float useLessPixelRadius4x = 0.45f;
	float useLessPixelRadius8x = 0.55f;
	if (useLessPixelLevel <= 1.0f)
	{
		useLessPixelRadius2x = lerp(useLessPixelLevel, 0.5f, 0.25f);
		useLessPixelRadius4x = lerp(useLessPixelLevel, 0.7f, 0.45f);
		useLessPixelRadius8x = lerp(useLessPixelLevel, 0.8f, 0.55f);
	}
	else 
	{
		useLessPixelRadius2x /= useLessPixelLevel;
		useLessPixelRadius4x /= lerp(0.25f, useLessPixelLevel, 1.0f);
		useLessPixelRadius8x /= lerp(0.5f, useLessPixelLevel, 1.0f);
	}
	if (useLessPixelMaxDepth < 2)
	{
		useLessPixelRadius4x = 100.0f;
	}
	if (useLessPixelMaxDepth < 3)
	{
		useLessPixelRadius8x = 100.0f;
	}
	::System::RenderTarget* useLessPixelBackground = nullptr;
	::System::RenderTarget* useLessPixelResolveHelper = nullptr;
#endif

#ifdef USE_LESS_PIXEL
	if (useLessPixel)
	{
		actuallyRenderToRT = Render::System::get_render_target_for(actuallyRenderToRT, &setupRenderToRT);
		useLessPixelBackground = Render::System::get_render_target_for(actuallyRenderToRT, nullptr, [](::System::RenderTargetSetup & _rtSetup) { _rtSetup.set_msaa_samples(0); });
		useLessPixelResolveHelper = Render::System::get_render_target_for(useLessPixelBackground, nullptr, [](::System::RenderTargetSetup & _rtSetup) { _rtSetup.set_msaa_samples(0); });

		setupRenderToRT = true; // for time being do it always
	}
#endif

#ifdef USE_LESS_PIXEL
	if (useLessPixel)
	{
		if (setupRenderToRT)
		{
			if (auto* shaderProgram = lessPixelPrepareStencilShaderProgram)
			{
				useLessPixelBackground->bind();
				v3d->clear_colour_depth_stencil(backgroundColour);
				v3d->setup_for_2d_display();
				v3d->set_viewport(VectorInt2::zero, useLessPixelBackground->get_size());
				v3d->set_2d_projection_matrix_ortho(useLessPixelBackground->get_size().to_vector2());
				
				v3d->push_state();
				v3d->colour_mask(true);
				v3d->depth_mask(false);
				v3d->stencil_test(::System::Video3DCompareFunc::Always, 1);
				v3d->stencil_op(::System::Video3DOp::Replace, ::System::Video3DOp::Keep, ::System::Video3DOp::Keep);
				v3d->depth_test(::System::Video3DCompareFunc::None);

				shaderProgram->bind();
				shaderProgram->set_build_in_uniform_in_texture_size_related_uniforms(useLessPixelBackground->get_size(true).to_vector2(), useLessPixelBackground->get_size().to_vector2());
				shaderProgram->set_uniform(shaderProgram->find_uniform_index(NAME(inColour)), backgroundColour.to_vector4());
				shaderProgram->set_uniform(shaderProgram->find_uniform_index(NAME(inRadius2x)), useLessPixelRadius2x);
				shaderProgram->set_uniform(shaderProgram->find_uniform_index(NAME(inRadius4x)), useLessPixelRadius4x);
				shaderProgram->set_uniform(shaderProgram->find_uniform_index(NAME(inRadius8x)), useLessPixelRadius8x);
				renderToRT->render_for_shader_program(v3d, Vector2::zero, useLessPixelBackground->get_size().to_vector2(), NP, shaderProgram);
				shaderProgram->unbind();

				v3d->pop_state();

				::System::RenderTarget::bind_none();
				useLessPixelBackground->resolve();
			}
			if (auto* shaderProgram = lessPixelPrepareResolveShaderProgram)
			{
				useLessPixelResolveHelper->bind();
				v3d->clear_colour_depth_stencil(Colour::black);
				v3d->setup_for_2d_display();
				v3d->set_viewport(VectorInt2::zero, useLessPixelResolveHelper->get_size());
				v3d->set_2d_projection_matrix_ortho(useLessPixelResolveHelper->get_size().to_vector2());
				
				v3d->push_state();
				//v3d->colour_mask(false);
				v3d->depth_mask(false);
				v3d->stencil_test(::System::Video3DCompareFunc::None, 1);
				v3d->stencil_op(::System::Video3DOp::Keep);
				v3d->depth_test(::System::Video3DCompareFunc::None);

				shaderProgram->bind();
				shaderProgram->set_build_in_uniform_in_texture_size_related_uniforms(useLessPixelResolveHelper->get_size(true).to_vector2(), useLessPixelResolveHelper->get_size().to_vector2());
				shaderProgram->set_uniform(shaderProgram->find_uniform_index(NAME(inRadius2x)), useLessPixelRadius2x);
				shaderProgram->set_uniform(shaderProgram->find_uniform_index(NAME(inRadius4x)), useLessPixelRadius4x);
				shaderProgram->set_uniform(shaderProgram->find_uniform_index(NAME(inRadius8x)), useLessPixelRadius8x);
				renderToRT->render_for_shader_program(v3d, Vector2::zero, useLessPixelResolveHelper->get_size().to_vector2(), NP, shaderProgram);
				shaderProgram->unbind();
				
				v3d->pop_state();

				::System::RenderTarget::bind_none();
				useLessPixelResolveHelper->resolve();
			}
		}
		::System::RenderTarget::bind_none();
		::System::RenderTarget::copy(useLessPixelBackground, VectorInt2::zero, useLessPixelBackground->get_size(), actuallyRenderToRT, ::System::RenderTarget::CopyAll);
		actuallyRenderToRT->bind();

		_context.increase_stencil_depth(); REMOVE_OR_CHANGE_BEFORE_COMMIT_
	}
	else
#endif
	{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE(readyToRender);
#endif

		actuallyRenderToRT->bind();
		// avoid RAM->tile memory, check: https://developer.oculus.com/blog/loads-stores-passes-and-advanced-gpu-pipelines
		v3d->forget_all_buffer_data(); // although clearing should do the same?
	}

	// flush here messes up foveated rendering

	v3d->setup_for_3d_display();

	// viewports are set through camera

#ifdef AN_TEST_PRE_RENDER_OFFSET
	{
		static Vector3 differenceLoc = Vector3::zero;
#ifdef AN_STANDARD_INPUT
		if (auto* i = ::System::Input::get())
		{
			float speed = 0.1f;
			if (i->is_key_pressed(::System::Key::LeftCtrl))
			{
				speed *= 0.1f;
			}
			if (i->is_key_pressed(::System::Key::PageUp))
			{
				differenceLoc.y += ::System::Core::get_raw_delta_time() * speed;
			}
			if (i->is_key_pressed(::System::Key::PageDown))
			{
				differenceLoc.y -= ::System::Core::get_raw_delta_time() * speed;
			}
			if (i->is_key_pressed(::System::Key::End))
			{
				differenceLoc = Vector3::zero;
			}
		}
#endif
		// apply difference we had when we were building scene and is now
		Transform difference = Transform(differenceLoc, Quat::identity);
		Transform moveToCentre = viewOffset.inverted();
		Transform moveToEye = viewOffset;

		Transform apply = moveToEye.to_world(difference.to_world(moveToCentre));

		Transform intoRoomTransform;
		RoomProxy::adjust_for_camera_offset(REF_ room, renderCamera, apply, intoRoomTransform);
		foregroundCameraOffset = intoRoomTransform.to_local(foregroundCameraOffset);
	}
#endif
	// modify renderCamera if required
	if (vr && VR::IVR::get() && preparedForVRView.is_set() && VR::IVR::get()->is_render_view_valid() && allowVRInputOnRender.is_any_allowed() && view == SceneView::FromCamera)
	{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE(moveRenderCameraIfRequired);
#endif

		// apply difference we had when we were building scene and is now
		Transform currentVRView = VR::IVR::get()->get_render_view();
		Transform difference = preparedForVRView.get().to_local(currentVRView);
		if (!allowVRInputOnRender.allowRotation)
		{
			difference.set_orientation(Quat::identity);
		}
		if (!allowVRInputOnRender.allowTranslation)
		{
			difference.set_translation(Vector3::zero);
		}
		Transform moveToCentre = viewOffset.inverted();
		Transform moveToEye = viewOffset;

		Transform apply = moveToEye.to_world(difference.to_world(moveToCentre));

		Transform intoRoomTransform;
		RoomProxy::adjust_for_camera_offset(REF_ room, REF_ renderCamera, apply, intoRoomTransform);
		foregroundCameraOffset = intoRoomTransform.to_local(foregroundCameraOffset);
		vrAnchor = intoRoomTransform.to_local(vrAnchor);
	}

	// render only if less - just so we don't hit door
	v3d->depth_test(::System::Video3DCompareFunc::Less);

	{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE(readyRenderCamera);
#endif
		renderCamera.update_for(actuallyRenderToRT); // update view size as it could change (I am looking at you VR that may apply scaling!)
		renderCamera.setup(_context);
		renderCamera.ready_to_render(v3d);
	}

	if (on_pre_render)
	{
		MEASURE_PERFORMANCE(onPreRender);
		on_pre_render(this, _context);
	}

	SceneRenderContext sceneRenderContext(sceneBuildContext, renderCamera);

	{
#ifdef BUILD_NON_RELEASE
		if (::System::RenderTargetSaveSpecialOptions::get().transparent)
		{
			v3d->clear_colour_depth_stencil(backgroundColour.with_set_alpha(0.0f));
		}
		else
#endif
		{
			if (backgroundMaterialInstance)
			{
				::System::VideoMatrixStack& modelViewStack = v3d->access_model_view_matrix_stack();
				::System::VideoClipPlaneStack& clipPlaneStack = v3d->access_clip_plane_stack();
				modelViewStack.push_set(Matrix44::identity);
				clipPlaneStack.push(); // we want to render whole background and existing clipplanes may cause problems with that, we will be using
				v3d->colour_mask(true);
				v3d->depth_mask(true);
				v3d->stencil_test(::System::Video3DCompareFunc::Equal, _context.get_current_stencil_depth());
				v3d->stencil_op(::System::Video3DOp::Keep);
				v3d->depth_test(::System::Video3DCompareFunc::Always);
#ifdef USE_DEPTH_CLAMP
				_v3d->depth_clamp(true);
#endif
				v3d->set_fallback_colour(backgroundColour);
				_context.render_background_far_plane(NP, backgroundMaterialInstance);
				v3d->set_fallback_colour();
				// restore some that are not obvious
#ifdef USE_DEPTH_CLAMP
				_v3d->depth_clamp(false);
#endif
				// no sending, we will update that anyway
				modelViewStack.pop();
				clipPlaneStack.pop();
			}
			else
			{
				v3d->clear_colour_depth_stencil(backgroundColour);
			}
		}
	}

#ifdef AN_DEVELOPMENT
	an_assert(additionalMeshesPrepared);
#endif
	if (!additionalBackgroundMeshesRenderingBuffer.is_empty())
	{
		DRAW_INFO(TXT("render additional meshes"));

		::System::VideoMatrixStack& modelViewStack = v3d->access_model_view_matrix_stack();
		modelViewStack.push_to_world(vrAnchor.to_matrix());

		{
			// setup masks
			v3d->colour_mask(true);
			v3d->depth_mask(true);

			// setup tests for rendering
#ifdef USE_LESS_PIXEL
			if (useLessPixel)
			{
				v3d->stencil_test(::System::Video3DCompareFunc::Equal, _context.get_current_stencil_depth());
				v3d->stencil_op(::System::Video3DOp::Keep);
			}
			else
#endif
			{
				v3d->stencil_test(::System::Video3DCompareFunc::None);
				v3d->stencil_op(::System::Video3DOp::Keep);
			}
			v3d->depth_test(::System::Video3DCompareFunc::LessEqual);
		}

		additionalBackgroundMeshesRenderingBuffer.render(v3d, _context.get_mesh_rendering_context());

		v3d->clear_depth(); // background meshes, do not mess with foreground

		modelViewStack.pop();
	}

	if (room)
	{
		MEASURE_PERFORMANCE(renderRoomProxies);

		_context.access_shader_param(NAME(scaleOutPosition)).param.set_uniform(1.0f);

		DRAW_INFO(TXT("render rooms"));
		RoomProxy::render(room, _context, sceneRenderContext);
	}

#ifdef AN_DEVELOPMENT
	an_assert(additionalMeshesPrepared);
#endif
	if (!additionalMeshesRenderingBuffer.is_empty())
	{
		DRAW_INFO(TXT("render additional meshes"));

		::System::VideoMatrixStack& modelViewStack = v3d->access_model_view_matrix_stack();
		modelViewStack.push_to_world(vrAnchor.to_matrix());

		todo_note(TXT("this will not render properly against doors that rendered into depth buffer"));
		{
			// setup masks
			v3d->colour_mask(true);
			v3d->depth_mask(true);

			// setup tests for rendering
#ifdef USE_LESS_PIXEL
			if (useLessPixel)
			{
				v3d->stencil_test(::System::Video3DCompareFunc::Equal, _context.get_current_stencil_depth());
				v3d->stencil_op(::System::Video3DOp::Keep);
			}
			else
#endif
			{
				v3d->stencil_test(::System::Video3DCompareFunc::None);
				v3d->stencil_op(::System::Video3DOp::Keep);
			}
			v3d->depth_test(::System::Video3DCompareFunc::LessEqual);
		}

		additionalMeshesRenderingBuffer.render(v3d, _context.get_mesh_rendering_context());

		modelViewStack.pop();
	}

	if (!foregroundPresenceLinksRenderingBuffer.is_empty())
	{
		DRAW_INFO(TXT("render foreground presence links"));

		::System::VideoMatrixStack& modelViewStack = v3d->access_model_view_matrix_stack();
		modelViewStack.push_to_world(foregroundCameraOffset.to_matrix());

		{
			// setup masks
			v3d->colour_mask(true);
			v3d->depth_mask(true);

			// setup tests for rendering
#ifdef USE_LESS_PIXEL
			if (useLessPixel)
			{
				v3d->stencil_test(::System::Video3DCompareFunc::Equal, _context.get_current_stencil_depth());
				v3d->stencil_op(::System::Video3DOp::Keep);
			}
			else
#endif
			{
				v3d->stencil_test(::System::Video3DCompareFunc::None);
				v3d->stencil_op(::System::Video3DOp::Keep);
			}
			v3d->depth_test(::System::Video3DCompareFunc::LessEqual);
		}

		todo_hack(TXT("for now we do not require to clear depth as we use hit indicator that ignores depth anyway"));
		if (0)
		{
			// clear depth & stencil
			todo_note(TXT("maybe only if required?"));
			//v3d->clear_depth_stencil();
			v3d->clear_depth();
		}

		_context.access_shader_param(NAME(scaleOutPosition)).param.set_uniform(_context.get_foreground_scale_out_position());

		// setup shader program binding context
		::System::ShaderProgramBindingContext prevShaderProgramBindingContext = _context.get_shader_program_binding_context();
		::System::ShaderProgramBindingContext currentShaderProgramBindingContext = prevShaderProgramBindingContext;
		// clear environment for foreground!
		EnvironmentProxy::setup_shader_binding_context_no_proxy(&currentShaderProgramBindingContext, modelViewStack);
		_context.set_shader_program_binding_context(currentShaderProgramBindingContext);

		foregroundPresenceLinksRenderingBuffer.render(v3d, _context.get_mesh_rendering_context());

		// restore shader program binding context
		_context.set_shader_program_binding_context(prevShaderProgramBindingContext);

		modelViewStack.pop();
	}

	if (on_post_render)
	{
		MEASURE_PERFORMANCE(onPostRender);
		DRAW_INFO(TXT("post render"));
		on_post_render(this, _context);
	}

#ifdef USE_LESS_PIXEL
	if (useLessPixel)
	{
		::System::RenderTarget::bind_none();
		actuallyRenderToRT->resolve();
		renderToRT->bind();
		if (auto* shaderProgram = lessPixelResolveShaderProgram)
		{
			v3d->setup_for_2d_display();
			v3d->set_viewport(VectorInt2::zero, renderToRT->get_size());
			v3d->set_2d_projection_matrix_ortho(renderToRT->get_size().to_vector2());
			
			v3d->push_state();
			v3d->stencil_test(::System::Video3DCompareFunc::None);
			v3d->stencil_op(::System::Video3DOp::Keep, ::System::Video3DOp::Keep, ::System::Video3DOp::Keep);
			v3d->depth_test(::System::Video3DCompareFunc::None);

			shaderProgram->bind();
			shaderProgram->set_build_in_uniform_in_texture(actuallyRenderToRT->get_data_texture_id(0));
			shaderProgram->set_build_in_uniform_in_texture_size_related_uniforms(actuallyRenderToRT->get_size(true).to_vector2(), renderToRT->get_size().to_vector2());
			shaderProgram->set_uniform(shaderProgram->find_uniform_index(NAME(inResolveHelper)), useLessPixelResolveHelper->get_data_texture_id(0));
			actuallyRenderToRT->render_for_shader_program(v3d, Vector2::zero, renderToRT->get_size().to_vector2(), NP, shaderProgram);
			shaderProgram->unbind();

			v3d->pop_state();
		}
		_context.decrease_stencil_depth();
	}
#endif

	{
		// avoid memory->RAM Store, check: https://developer.oculus.com/blog/loads-stores-passes-and-advanced-gpu-pipelines
		v3d->forget_depth_stencil_buffer_data();
		// we can't invalidate render buffer as we won't show anything
	}
	::System::RenderTarget::unbind_to_none();

	v3d->set_eye_offset(); // clear

	// off we go to the driver!
	//DIRECT_GL_CALL_ glFlush(); AN_GL_CHECK_FOR_ERROR
}

void Scene::add_display(Display* _display)
{
	for_every_ref(display, displays)
	{
		if (display == _display)
		{
			return;
		}
	}
	displays.push_back(DisplayPtr(_display));
}

void Scene::combine_displays_from(Scene* _scene)
{
	if (_scene == this)
	{
		return;
	}
	for_every(displayPtr, _scene->displays)
	{
		bool add = true;
		for_every_ref(display, displays)
		{
			if (display == displayPtr->get())
			{
				add = false;
				break;
			}
		}
		if (add)
		{
			displays.push_back(*displayPtr);
		}
	}

	_scene->displays.clear();
}

void Scene::combine_displays(Array<RefCountObjectPtr<Framework::Render::Scene> > & _scenes)
{
	if (_scenes.is_empty())
	{
		return;
	}
	Scene* first = _scenes.get_first().get();
	for_every_ref(scene, _scenes)
	{
		first->combine_displays_from(scene);
	}
}

void Scene::log(LogInfoContext& _log) const
{
	if (room)
	{
		room->log(_log);
	}
}
