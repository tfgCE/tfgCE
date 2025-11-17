#include "loaderStarField.h"

#include "..\game\game.h"
#include "..\overlayInfo\overlayInfoElement.h"
#include "..\overlayInfo\overlayInfoSystem.h"
#include "..\pilgrimage\pilgrimage.h"
#include "..\pilgrimage\pilgrimageInstance.h"
#include "..\sound\voiceoverSystem.h"

#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\renderTargetUtils.h"
#include "..\..\core\system\video\video3dPrimitives.h"
#include "..\..\core\system\video\viewFrustum.h"
#include "..\..\core\system\video\videoClipPlaneStackImpl.inl"
#include "..\..\core\vr\iVR.h"

#include "..\..\framework\appearance\meshStatic.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\render\renderCamera.h"
#include "..\..\framework\render\renderScene.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// loader params (learn from)
DEFINE_STATIC_NAME(fadeInDelay);
DEFINE_STATIC_NAME(fadeInTime);
DEFINE_STATIC_NAME(fadeOutTime);
DEFINE_STATIC_NAME(postFadeOutWait);
DEFINE_STATIC_NAME(minActiveTime);
DEFINE_STATIC_NAME(blockPromptSkipSceneFor);
DEFINE_STATIC_NAME(voiceoverDelay);
DEFINE_STATIC_NAME(voiceoverPostDelay);
DEFINE_STATIC_NAME(voiceoverLine);
DEFINE_STATIC_NAME(voiceoverActor);
DEFINE_STATIC_NAME(playSample);
DEFINE_STATIC_NAME(playSampleDelay);
DEFINE_STATIC_NAME(appearFadedOutOnNoSample);
DEFINE_STATIC_NAME(planetMesh);
DEFINE_STATIC_NAME(planetMeshAt);
DEFINE_STATIC_NAME(planetRefAt);
DEFINE_STATIC_NAME(planetScalePerSecond);
DEFINE_STATIC_NAME(moonMesh);
DEFINE_STATIC_NAME(moonMeshAt);
DEFINE_STATIC_NAME(moonRefAt);
DEFINE_STATIC_NAME(moonScalePerSecond);
DEFINE_STATIC_NAME(sunMesh);
DEFINE_STATIC_NAME(sunMeshAt);
DEFINE_STATIC_NAME(shipMesh);
DEFINE_STATIC_NAME(ship);
DEFINE_STATIC_NAME(shipAt);
DEFINE_STATIC_NAME(shipSpeed);
DEFINE_STATIC_NAME(shipColour);
DEFINE_STATIC_NAME(shipScalePerSecond);
DEFINE_STATIC_NAME(starsDimmer);

DEFINE_STATIC_NAME(forcePostFadeInImmediately);
DEFINE_STATIC_NAME(speed);

// game input definitions
DEFINE_STATIC_NAME_STR(gidAllTimeControls, TXT("allTimeControls"));

// shader/material param
DEFINE_STATIC_NAME(apparentActivation);

#define OVERLAY_DISTANCE 10.0f

//

using namespace Loader;

//

StarField::StarField(Optional<Transform> const& _forward)
{
	forward = _forward.get(Transform::identity);

	starsVertexFormat = new ::System::VertexFormat();
	starsVertexFormat->with_location_3d().with_colour_rgba().no_padding().calculate_stride_and_offsets();

	movingStars.make_space_for(movingStarCount);
	staticStars.make_space_for(staticStarCount);
}

StarField::~StarField()
{
	delete_and_clear(starsVertexFormat);
}

void StarField::setup_overlay(bool _active)
{
	if (_active)
	{
		TeaForGodEmperor::SubtitleSystem::set_distance_for_subtitles(OVERLAY_DISTANCE);
		TeaForGodEmperor::SubtitleSystem::set_background_for_subtitles(Colour::none);
		TeaForGodEmperor::SubtitleSystem::set_offset_for_subtitiles(Rotator3::zero); // right in front
		Optional<Transform> shouldBeAt = forward;
		if (shouldBeAt.is_set())
		{
			if (auto* ois = TeaForGodEmperor::OverlayInfo::System::get())
			{
				Rotator3 anchor = shouldBeAt.get().get_orientation().to_rotator();
				ois->force_anchor_rotation(anchor);
			}
		}
	}
	else
	{
		TeaForGodEmperor::SubtitleSystem::set_distance_for_subtitles();
		TeaForGodEmperor::SubtitleSystem::set_background_for_subtitles();
		TeaForGodEmperor::SubtitleSystem::set_offset_for_subtitiles();
		if (auto* ois = TeaForGodEmperor::OverlayInfo::System::get())
		{
			ois->force_anchor_rotation();
		}
	}

	{
		skipScene.set_overlay_distance(OVERLAY_DISTANCE);
		float pitch = -25.0f; // for non vr (to appear on screen, at the bottom)
		if (VR::IVR::get())
		{
			pitch = -40.0f; // we will notice it but it should not be all the time in the view
		}
		skipScene.set_overlay_pitch(pitch);
	}
}

bool StarField::activate()
{
	timeActive = 0.0f;

	// update to be in front
	if (MainConfig::global().should_be_immobile_vr())
	{
		if (auto* vr = VR::IVR::get())
		{
			if (vr->is_render_view_valid())
			{
				Transform view = vr->get_render_view();
				Vector3 fwdDir = calculate_flat_forward_from(view);
				forward = look_matrix(view.get_translation() * Vector3::xy, fwdDir, Vector3::zAxis).to_transform();
			}
		}
	}

	setup_overlay(true);

	shouldDeactivate = false;
	
	::System::FoveatedRenderingSetup::set_temporary_foveation_level_offset(-2.0f);
	::System::FoveatedRenderingSetup::set_temporary_no_foveation(true);
	::System::FoveatedRenderingSetup::force_set_foveation(); // enforce changes

	return base::activate();
}

void StarField::deactivate()
{
	if (!shouldDeactivate)
	{
		shouldDeactivate = true;
		if (voiceover.get())
		{
			skipScene.allow_skip_scene();
			promptSkipScene = true;
		}
	}
}

void StarField::update(float _deltaTime)
{
	timeActive += _deltaTime;

	{
		bool wasEmpty = movingStars.is_empty();
		while (movingStars.get_size() < movingStarCount)
		{
			Star st;
			float xzPt = 0.6f;
			st.loc = Vector3(xzPt * rg.get_float(-radius, radius), wasEmpty? rg.get_float(-radius, radius) : radius, xzPt * rg.get_float(-radius, radius));
			st.colour = Colour::white;
			st.colour.r = rg.get_float(0.6f, 1.0f);
			st.colour.g = rg.get_float(0.6f, 1.0f);
			st.colour.b = rg.get_float(0.6f, 1.0f);
			st.colour = st.colour.mul_rgb(rg.get_float(0.6f, 1.5f));
			movingStars.push_back(st);
		}
	}

	{
		bool wasEmpty = staticStars.is_empty();
		while (staticStars.get_size() < staticStarCount)
		{
			Star st;
			float xzPt = 0.6f;
			st.loc = Vector3(xzPt * rg.get_float(-radius, radius), wasEmpty? rg.get_float(-radius, radius) : radius, xzPt * rg.get_float(-radius, radius));
			st.colour = Colour::white;
			st.colour.r = rg.get_float(0.6f, 1.0f);
			st.colour.g = rg.get_float(0.6f, 1.0f);
			st.colour.b = rg.get_float(0.6f, 1.0f);
			st.colour = st.colour.mul_rgb(rg.get_float(0.2f, 1.2f));
			staticStars.push_back(st);
		}
	}

	{
		Star* s = movingStars.begin();
		for (int i = 0; i < movingStars.get_size(); ++i, ++s)
		{
			s->loc.y -= speed * _deltaTime;

			if (s->loc.y <= -radius * 1.1f)
			{
				movingStars.remove_fast_at(i);
				--i;
				--s;
			}
		}
	}

	shipAt.set_translation(shipAt.get_translation() + shipSpeed * _deltaTime);

	blockPromptSkipSceneForLeft = max(0.0f, blockPromptSkipSceneForLeft - _deltaTime);
	if (skipScene.is_prompting_skip_scene())
	{
		promptSkipScene = false;
	}
	if (promptSkipScene && blockPromptSkipSceneForLeft <= 0.0f)
	{
		promptSkipScene = false;
		skipScene.prompt_skip_scene();
	}

	if (!voiceoverSpoke && voiceoverDelayLeft >= 0.0f && voiceover.get())
	{
		voiceoverDelayLeft -= _deltaTime;
		if (voiceoverDelayLeft < 0.0f)
		{
			if (auto* vos = TeaForGodEmperor::VoiceoverSystem::get())
			{
				vos->speak(voiceoverActor, false, voiceover.get());
				voiceoverSpoke = true;
				silenceVoiceoverAtEnd = true;
			}
		}
	}

	if (!playedSample && playSample.get())
	{
		playSampleDelayLeft -= _deltaTime;
		if (playSampleDelayLeft < 0.0f)
		{
			playedSample = true;
			playedSamplePlayback = playSample->play();
		}
	}

	if ((!shouldDeactivate || targetActivation > 0.0f) && TeaForGodEmperor::Game::get()->does_want_to_cancel_creation())
	{
		// force to end if wants to cancel creation
		remainActiveTime = 0.0f;
		postFadeOutWait = 0.0f;
		deactivate();
	}

	if (shouldDeactivate && targetActivation > 0.0f)
	{
		bool shouldFadeOut = false;
		if (voiceover.get())
		{
			if (voiceoverSpoke)
			{
				if (auto* vos = TeaForGodEmperor::VoiceoverSystem::get())
				{
					if (voiceoverPostDelayLeft < 0.0f)
					{
						if (vos->get_time_to_end_speaking(voiceoverActor, voiceover.get()) < abs(voiceoverPostDelayLeft))
						{
							silenceVoiceoverAtEnd = false;
							targetActivation = 0.0f;
						}
					}
					else
					{
						if (!vos->is_speaking(voiceoverActor, voiceover.get()))
						{
							shouldFadeOut = true;
						}
					}
				}
			}
		}
		else
		{
			shouldFadeOut = true;
		}
		if (shouldFadeOut)
		{
			voiceoverPostDelayLeft = max(0.0f, voiceoverPostDelayLeft - _deltaTime);
			if (voiceoverPostDelayLeft <= 0.0f)
			{
				targetActivation = 0.0f;
			}
		}
	}

	if (remainActiveTime > 0.0f && targetActivation == 0.0f)
	{
		remainActiveTime -= _deltaTime;
		targetActivation = 1.0f;
	}

	float prevActivation = activation;
	if (fadeInDelayLeft > 0.0f && targetActivation > activation)
	{
		fadeInDelayLeft -= _deltaTime;
	}
	else
	{
		activation = blend_to_using_speed_based_on_time(activation, targetActivation, targetActivation > activation ? fadeInTime : fadeOutTime, min(_deltaTime, 0.03f));
		if (targetActivation > 0.0f || fadeOutTime > 0.0f)
		{
			apparentActivation = activation;
		}
		if (appearFadedOutOnNoSample &&
			playedSample && !playedSamplePlayback.is_playing())
		{
			apparentActivation = 0.0f;
		}
	}

	// this mechanism requires full fade in before fading out
	if (activation > prevActivation && prevActivation == 0.0f)
	{
		TeaForGodEmperor::Game::get()->fade_in(fadeInTime * 0.3f, NP, NP, true); // fade in quicker, we want stars to fade in slower
	}
	else if (activation < prevActivation && prevActivation == 1.0f)
	{
		TeaForGodEmperor::Game::get()->fade_out(fadeOutTime, NP, NP, true, NP, silenceVoiceoverAtEnd);
		if (auto* vos = TeaForGodEmperor::VoiceoverSystem::get())
		{
			// if we bail out earlier, as requested (post delay is negative), don't silence
			if (silenceVoiceoverAtEnd)
			{
				vos->silence(fadeOutTime);
			}
		}
		if (playedSamplePlayback.is_playing())
		{
			playedSamplePlayback.fade_out(fadeOutTime);
		}
	}

	if (activation <= 0.0f && targetActivation <= 0.0f)
	{
		if (postFadeOutWait > 0.0f)
		{
			postFadeOutWait -= _deltaTime;
		}
		else
		{
			if (forcePostFadeInImmediately)
			{
				TeaForGodEmperor::Game::get()->fade_in_immediately();
			}
			if (auto* ss = TeaForGodEmperor::SubtitleSystem::get())
			{
				ss->remove_all();
			}
			if (auto* ois = TeaForGodEmperor::OverlayInfo::System::get())
			{
				ois->force_clear();
			}
			setup_overlay(false);

			::System::FoveatedRenderingSetup::set_temporary_foveation_level_offset(NP);
			::System::FoveatedRenderingSetup::set_temporary_no_foveation(false);
			::System::FoveatedRenderingSetup::force_set_foveation(); // enforce changes

			// stop/ remove
			{
				if (playedSamplePlayback.is_playing())
				{
					playedSamplePlayback.fade_out(0.1f);
				}
				if (auto* vos = TeaForGodEmperor::VoiceoverSystem::get())
				{
					vos->silence(0.1f);
				}
			}

			base::deactivate();
		}
	}

	if (auto* vr = VR::IVR::get())
	{
		Vector3 eyeLoc = Vector3(0.0f, 0.0f, 1.65f);
		viewPlacement = look_at_matrix(eyeLoc, eyeLoc - Vector3::xAxis, Vector3::zAxis).to_transform();
		if (vr->is_render_view_valid())
		{
			viewPlacement = vr->get_render_view();
		}
	}
	else
	{
		Vector3 eyeLoc = Vector3(0.0f, 0.0f, 1.65f);
		viewPlacement = look_at_matrix(eyeLoc, eyeLoc - Vector3::xAxis, Vector3::zAxis).to_transform();
	}

	if (auto* ois = TeaForGodEmperor::OverlayInfo::System::get())
	{
		skipScene.update(_deltaTime);
		if (skipScene.should_skip_scene())
		{
			promptSkipScene = false;
			skipScene.allow_skip_scene(false);
			// skip!
			targetActivation = 0.0f;
		}

		ois->advance(TeaForGodEmperor::OverlayInfo::Usage::CustomLoader, viewPlacement, nullptr);
	}
}

void StarField::display(System::Video3D * _v3d, bool _vr)
{
	System::RenderTarget* renderTarget = TeaForGodEmperor::Game::get()->get_main_render_buffer();

	// update view for vr, to have the most valid placement
	if (auto* vr = VR::IVR::get())
	{
		if (vr->is_render_view_valid())
		{
			viewPlacement = vr->get_render_view();
		}
	}

	Framework::Render::Camera camera;
	camera.set_placement(nullptr, viewPlacement);
	camera.set_near_far_plane(TeaForGodEmperor::Game::s_renderingNearZ, TeaForGodEmperor::Game::s_renderingFarZ);
	camera.set_background_near_far_plane();
	if (! _vr)
	{
		camera.set_fov(70.0f);
		camera.set_view_origin(VectorInt2::zero);
		camera.set_view_size(renderTarget->get_size());
		camera.set_view_aspect_ratio(aspect_ratio(renderTarget->get_full_size_for_aspect_ratio_coef()));
	}

	Framework::Render::Scene* scenes[2];
	int sceneCount = 1;
	if (auto* vr = VR::IVR::get())
	{
		sceneCount = 2;
		scenes[0] = Framework::Render::Scene::build(camera, NP, nullptr, NP, Framework::Render::SceneMode::VR_Left);
		scenes[1] = Framework::Render::Scene::build(camera, NP, nullptr, NP, Framework::Render::SceneMode::VR_Right);
	}
	else
	{
		sceneCount = 1;
		scenes[0] = Framework::Render::Scene::build(camera);
	}

	for_count(int, i, sceneCount)
	{
		scenes[i]->set_background_colour(MainConfig::global().get_video().forcedBlackColour.get(Colour::lerp(0.2f * apparentActivation, Colour::black, Colour::blackCold)));
		scenes[i]->prepare_extras();
	}

	Framework::Render::Context rc(_v3d);

	rc.set_default_texture_id(Framework::Library::get_current()->get_default_texture()->get_texture()->get_texture_id());

	for_count(int, i, sceneCount)
	{
		if (_vr)
		{
			scenes[i]->render(rc, nullptr); // it's ok, it's vr
			VR::Eye::Type eye = (VR::Eye::Type)scenes[i]->get_render_camera().get_eye_idx();
			{
				if (auto* vr = VR::IVR::get())
				{
					auto* rt = vr->get_render_render_target(eye);

					render_star_field(rt, scenes[i]->get_render_camera(), true, eye);
				}
			}
			if (auto* ois = TeaForGodEmperor::OverlayInfo::System::get())
			{
				ois->render(Transform::identity /* vr anchor at 0,0 */, TeaForGodEmperor::OverlayInfo::Usage::CustomLoader, eye);
			}
		}
		else
		{
			scenes[i]->render(rc, renderTarget);
			render_star_field(renderTarget, scenes[i]->get_render_camera(), false, VR::Eye::Right);
			if (auto* ois = TeaForGodEmperor::OverlayInfo::System::get())
			{
				ois->render(Transform::identity /* vr anchor at 0,0 */, TeaForGodEmperor::OverlayInfo::Usage::CustomLoader, NP, renderTarget);
			}
		}
		scenes[i]->release();
	}

	if (_vr)
	{
		VR::IVR::get()->copy_render_to_output(_v3d);
	}
	else
	{
		renderTarget->resolve();

		TeaForGodEmperor::Game::get()->setup_to_render_to_main_render_target(::System::Video3D::get());
		::System::Video3D::get()->clear_colour(Colour::black);
		Vector2 leftBottom;
		Vector2 size;
		VectorInt2 targetSize = MainConfig::global().get_video().resolution;
		::System::RenderTargetUtils::fit_into(renderTarget->get_full_size_for_aspect_ratio_coef(), targetSize, OUT_ leftBottom, OUT_ size);
		renderTarget->render(0, ::System::Video3D::get(), leftBottom, size);
	}
}

void StarField::learn_from(SimpleVariableStorage const& _params)
{
	fadeInTime = _params.get_value<float>(NAME(fadeInTime), fadeInTime);
	fadeOutTime = _params.get_value<float>(NAME(fadeOutTime), fadeOutTime);
	fadeInDelayLeft = _params.get_value<float>(NAME(fadeInDelay), fadeInDelayLeft);

	postFadeOutWait = _params.get_value<float>(NAME(postFadeOutWait), postFadeOutWait);
	remainActiveTime = _params.get_value<float>(NAME(minActiveTime), remainActiveTime);
	
	forcePostFadeInImmediately = _params.get_value<bool>(NAME(forcePostFadeInImmediately), forcePostFadeInImmediately);

	blockPromptSkipSceneForLeft = _params.get_value<float>(NAME(blockPromptSkipSceneFor), blockPromptSkipSceneForLeft);
	voiceoverDelayLeft = _params.get_value<float>(NAME(voiceoverDelay), voiceoverDelayLeft);
	voiceoverPostDelayLeft = _params.get_value<float>(NAME(voiceoverPostDelay), voiceoverPostDelayLeft);
	voiceover.set_name(_params.get_value<Framework::LibraryName>(NAME(voiceoverLine), voiceover.get_name()));
	voiceover.find(Framework::Library::get_current());
	voiceoverActor = _params.get_value<Name>(NAME(voiceoverActor), voiceoverActor);

	playSampleDelayLeft = _params.get_value<float>(NAME(playSampleDelay), playSampleDelayLeft);
	playSample.set_name(_params.get_value<Framework::LibraryName>(NAME(playSample), playSample.get_name()));
	playSample.find(Framework::Library::get_current());
	appearFadedOutOnNoSample = _params.get_value<bool>(NAME(appearFadedOutOnNoSample), appearFadedOutOnNoSample);
	
	speed = _params.get_value<float>(NAME(speed), speed);

	{
		planetMesh.set_name(_params.get_value<Framework::LibraryName>(NAME(planetMesh), planetMesh.get_name()));
		planetMesh.find(Framework::Library::get_current());
		planetMeshAt = _params.get_value<Transform>(NAME(planetMeshAt), planetMeshAt);
		planetRefAt = _params.get_value<Vector3>(NAME(planetRefAt), planetRefAt);
		planetScalePerSecond = _params.get_value<float>(NAME(planetScalePerSecond), planetScalePerSecond);

		if (auto* pm = planetMesh.get())
		{
			if (auto* m = pm->get_mesh())
			{
				planetMeshInstance.set_mesh(m);
				Framework::MaterialSetup::apply_material_setups(pm->get_material_setups(), planetMeshInstance, ::System::MaterialSetting::IndividualInstance);
				planetMeshInstance.fill_materials_with_missing();
			}
		}
	}

	{
		moonMesh.set_name(_params.get_value<Framework::LibraryName>(NAME(moonMesh), moonMesh.get_name()));
		moonMesh.find(Framework::Library::get_current());
		moonMeshAt = _params.get_value<Transform>(NAME(moonMeshAt), moonMeshAt);
		moonRefAt = _params.get_value<Vector3>(NAME(moonRefAt), moonRefAt);
		moonScalePerSecond = _params.get_value<float>(NAME(moonScalePerSecond), moonScalePerSecond);

		if (auto* pm = moonMesh.get())
		{
			if (auto* m = pm->get_mesh())
			{
				moonMeshInstance.set_mesh(m);
				Framework::MaterialSetup::apply_material_setups(pm->get_material_setups(), moonMeshInstance, ::System::MaterialSetting::IndividualInstance);
				moonMeshInstance.fill_materials_with_missing();
			}
		}
	}

	{
		sunMesh.set_name(_params.get_value<Framework::LibraryName>(NAME(sunMesh), sunMesh.get_name()));
		sunMesh.find(Framework::Library::get_current());
		sunMeshAt = _params.get_value<Transform>(NAME(sunMeshAt), sunMeshAt);

		if (auto* pm = sunMesh.get())
		{
			if (auto* m = pm->get_mesh())
			{
				sunMeshInstance.set_mesh(m);
				Framework::MaterialSetup::apply_material_setups(pm->get_material_setups(), sunMeshInstance, ::System::MaterialSetting::IndividualInstance);
				sunMeshInstance.fill_materials_with_missing();
			}
		}
	}

	{
		shipMesh.set_name(_params.get_value<Framework::LibraryName>(NAME(shipMesh), shipMesh.get_name()));
		shipMesh.find(Framework::Library::get_current());

		if (auto* pm = shipMesh.get())
		{
			if (auto* m = pm->get_mesh())
			{
				shipMeshInstance.set_mesh(m);
				Framework::MaterialSetup::apply_material_setups(pm->get_material_setups(), shipMeshInstance, ::System::MaterialSetting::IndividualInstance);
				shipMeshInstance.fill_materials_with_missing();
			}
		}
	}

	{
		ship = _params.get_value<bool>(NAME(ship), ship);
		shipAt = _params.get_value<Transform>(NAME(shipAt), shipAt);
		shipSpeed = _params.get_value<Vector3>(NAME(shipSpeed), shipSpeed);
		shipColour = _params.get_value<Colour>(NAME(shipColour), shipColour);
		shipScalePerSecond = _params.get_value<float>(NAME(shipScalePerSecond), shipScalePerSecond);
	}

	starsDimmer = _params.get_value<float>(NAME(starsDimmer), starsDimmer);
}

void StarField::render_star_field(::System::RenderTarget* _rt, ::Framework::Render::Camera const& _camera, bool _vr, VR::Eye::Type _eye)
{
	_rt->bind();

	auto* v3d = ::System::Video3D::get();

	v3d->setup_for_2d_display();

	auto& clipPlaneStack = v3d->access_clip_plane_stack();
	clipPlaneStack.clear();

	v3d->ready_for_rendering();
	v3d->set_fallback_colour(Colour::white);

	Quat reorient = _vr ? forward.get_orientation() : Rotator3(0.0f, -90.0f, 0.0f).to_quat();

	{
		starVertices.clear();
		starVertices.make_space_for(movingStars.get_size() * 2);
		for_every(st, movingStars)
		{
			Star s0;
			s0.loc = reorient.to_world(st->loc) + _camera.get_placement().get_translation();
			s0.colour = st->colour * clamp(sqr(1.0f - st->loc.length() / radius), 0.0f, 1.0f) * apparentActivation;
			s0.colour = s0.colour.mul_rgb(1.0f - starsDimmer);
			Star s1;
			s1 = s0;
			s1.loc += reorient.to_world(Vector3::yAxis * (speed * 0.025f));
			starVertices.push_back(s0);
			starVertices.push_back(s1);
		}
		Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
			Meshes::Mesh3DRenderingContext().with_existing_face_display(), starVertices.get_data(), ::Meshes::Primitive::Line, movingStars.get_size(), *starsVertexFormat);
	}

	{
		starVertices.clear();
		starVertices.make_space_for(staticStars.get_size());
		for_every(st, staticStars)
		{
			Star s0;
			s0.loc = reorient.to_world(st->loc) + _camera.get_placement().get_translation();
			s0.colour = st->colour * apparentActivation;
			s0.colour = s0.colour.mul_rgb(1.0f - starsDimmer);
			starVertices.push_back(s0);
		}
		Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
			Meshes::Mesh3DRenderingContext().with_existing_face_display(), starVertices.get_data(), ::Meshes::Primitive::Point, staticStars.get_size(), *starsVertexFormat);
	}

	if (sunMeshInstance.get_mesh())
	{
		auto& ms = v3d->access_model_view_matrix_stack();
		Transform t = sunMeshAt;
		t.set_orientation(reorient.to_world(t.get_orientation()));
		t.set_translation(reorient.to_world(t.get_translation()) + _camera.get_placement().get_translation());
		ms.push_to_world(t.to_matrix());
		for_count(int, i, sunMeshInstance.get_material_instance_count())
		{
			if (auto* mi = sunMeshInstance.access_material_instance(i))
			{
				mi->set_uniform(NAME(apparentActivation), apparentActivation);
			}
		}
		Meshes::Mesh3DRenderingContext rc;
		sunMeshInstance.render(v3d, rc);
		ms.pop();
	}

	if (planetMeshInstance.get_mesh())
	{
		auto& ms = v3d->access_model_view_matrix_stack();
		Transform t = planetMeshAt;
		float planetScale = pow(planetScalePerSecond, timeActive);
		t.set_translation(planetRefAt + (t.get_translation() - planetRefAt) * planetScale);
		t.set_orientation(reorient.to_world(t.get_orientation()));
		t.set_translation(reorient.to_world(t.get_translation()) + _camera.get_placement().get_translation());
		t.set_scale(planetScale);
		ms.push_to_world(t.to_matrix());
		for_count(int, i, planetMeshInstance.get_material_instance_count())
		{
			if (auto* mi = planetMeshInstance.access_material_instance(i))
			{
				mi->set_uniform(NAME(apparentActivation), apparentActivation);
			}
		}
		Meshes::Mesh3DRenderingContext rc;
		planetMeshInstance.render(v3d, rc);
		ms.pop();
	}

	if (moonMeshInstance.get_mesh())
	{
		auto& ms = v3d->access_model_view_matrix_stack();
		Transform t = moonMeshAt;
		float moonScale = pow(moonScalePerSecond, timeActive);
		t.set_translation(moonRefAt + (t.get_translation() - moonRefAt) * moonScale);
		t.set_orientation(reorient.to_world(t.get_orientation()));
		t.set_translation(reorient.to_world(t.get_translation()) + _camera.get_placement().get_translation());
		t.set_scale(moonScale);
		ms.push_to_world(t.to_matrix());
		for_count(int, i, moonMeshInstance.get_material_instance_count())
		{
			if (auto* mi = moonMeshInstance.access_material_instance(i))
			{
				mi->set_uniform(NAME(apparentActivation), apparentActivation);
			}
		}
		Meshes::Mesh3DRenderingContext rc;
		moonMeshInstance.render(v3d, rc);
		ms.pop();
	}

	if (ship)
	{
		if (shipMeshInstance.get_mesh())
		{
			v3d->depth_test(System::Video3DCompareFunc::Less);
			v3d->depth_mask(true);
			v3d->face_display(System::FaceDisplay::Front);
			v3d->requires_send_state();

			/*
			Vector3 eyeOffset = Vector3::zero;
			if (auto* vr = VR::IVR::get())
			{
				eyeOffset = vr->get_render_info().eyeOffset[_eye].get_translation();
			}
			eyeOffset *= 0.005f; // the ship will appear big
			*/

			auto& ms = v3d->access_model_view_matrix_stack();
			Transform t = shipAt;
			float shipScale = pow(shipScalePerSecond, timeActive);
			t.set_orientation(reorient.to_world(t.get_orientation()));
			t.set_translation(reorient.to_world(t.get_translation()) + _camera.get_placement().get_translation()/* - _camera.get_placement().vector_to_world(eyeOffset)*/);
			t.set_scale(shipScale);
			ms.push_to_world(t.to_matrix());
			for_count(int, i, shipMeshInstance.get_material_instance_count())
			{
				if (auto* mi = shipMeshInstance.access_material_instance(i))
				{
					mi->set_uniform(NAME(apparentActivation), apparentActivation);
				}
			}
			Meshes::Mesh3DRenderingContext rc;
			shipMeshInstance.render(v3d, rc);
			ms.pop();
		}
		else
		{
			starVertices.clear();
			starVertices.make_space_for(1);
			{
				Star s0;
				s0.loc = reorient.to_world(shipAt.get_translation()) + _camera.get_placement().get_translation();
				s0.colour = shipColour * apparentActivation;
				Star s1;
				s1 = s0;
				s1.loc -= reorient.to_world(shipSpeed * 0.025f);
				starVertices.push_back(s0);
				starVertices.push_back(s1);
			}
			Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
				Meshes::Mesh3DRenderingContext().with_existing_face_display(), starVertices.get_data(), ::Meshes::Primitive::Line, 1, *starsVertexFormat);
		}
	}

	v3d->set_fallback_colour();

	_rt->unbind();
}