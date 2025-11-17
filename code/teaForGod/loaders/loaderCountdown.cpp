#include "loaderCountdown.h"

#include "..\game\game.h"

#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\renderTargetUtils.h"
#include "..\..\core\vr\iVR.h"
#include "..\..\core\system\video\viewFrustum.h"
#include "..\..\core\system\video\videoClipPlaneStackImpl.inl"

#include "..\..\framework\library\library.h"
#include "..\..\framework\render\renderCamera.h"
#include "..\..\framework\render\renderScene.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

DEFINE_STATIC_NAME_STR(grFont, TXT("loader text font"));

//

using namespace Loader;

//

Countdown::Countdown(int _preCountdown, int _countdown)
{
	timeToCountdown = (float)_preCountdown;
	timeToNextCountdown = 1.0f;
	countdown = _countdown;

	font = Framework::Library::get_current()->get_global_references().get<Framework::Font>(NAME(grFont));
}

Countdown::~Countdown()
{
}

bool Countdown::activate()
{
	::System::FoveatedRenderingSetup::set_temporary_no_foveation(true);
	::System::FoveatedRenderingSetup::force_set_foveation(); // enforce changes

	return base::activate();
}

void Countdown::deactivate()
{
	// ignore, we are countdown
}

void Countdown::update(float _deltaTime)
{
	if (timeToCountdown > 0.0f)
	{
		timeToCountdown -= _deltaTime;
	}
	else
	{
		timeToNextCountdown -= _deltaTime;
		if (timeToNextCountdown <= 0.0f)
		{
			timeToNextCountdown += 1.0f;
			--countdown;
			if (countdown <= 0)
			{
				::System::FoveatedRenderingSetup::set_temporary_no_foveation(false);
				::System::FoveatedRenderingSetup::force_set_foveation(); // enforce changes

				base::deactivate();
			}
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
}

void Countdown::display(System::Video3D * _v3d, bool _vr)
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

	if (viewPlacement.is_set())
	{
		Vector3 loc = viewPlacement.get().get_translation();
		Vector3 up = viewPlacement.get().get_axis(Axis::Up);//Vector3::zAxis;
		Vector3 fwd = viewPlacement.get().get_axis(Axis::Forward);//calculate_flat_forward_from(viewPlacement.get());

		Vector3 curLoc = refViewPlacement.is_set() ? refViewPlacement.get().get_translation() : loc;
		Vector3 curUp = refViewPlacement.is_set() ? refViewPlacement.get().get_axis(Axis::Up) : up;
		Vector3 curFwd = refViewPlacement.is_set() ? refViewPlacement.get().get_axis(Axis::Forward) : fwd;
		
		float deltaTime = ::System::Core::get_raw_delta_time();
		float useNew = clamp(deltaTime / 0.3f, 0.0f, 0.5f);
		loc = lerp(useNew, curLoc, loc);
		up = lerp(useNew, curUp, up);
		fwd = lerp(useNew, curFwd, fwd);
		refViewPlacement = look_matrix(loc, fwd.normal(), up.normal()).to_transform();
	}

	Framework::Render::Camera camera;
	camera.set_placement(nullptr, viewPlacement.get(Transform::identity));
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
		scenes[i]->set_background_colour(MainConfig::global().get_video().forcedBlackColour.get(backgroundColour));
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

					render_countdown(rt, scenes[i]->get_render_camera(), true);
				}
			}
		}
		else
		{
			scenes[i]->render(rc, renderTarget);
			render_countdown(renderTarget, scenes[i]->get_render_camera(), false);
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

void Countdown::render_countdown(::System::RenderTarget* _rt, ::Framework::Render::Camera const& _camera, bool _vr)
{
	_rt->bind();

	auto* v3d = ::System::Video3D::get();

	v3d->setup_for_2d_display();

	auto& clipPlaneStack = v3d->access_clip_plane_stack();
	clipPlaneStack.clear();

	v3d->ready_for_rendering();
	v3d->set_fallback_colour(Colour::white);

	if (refViewPlacement.is_set() && font && countdown > 0 && timeToCountdown <= 0.0f)
	{
		float distance = 2.0f;

		auto& ms = v3d->access_model_view_matrix_stack();
		ms.push_to_world(refViewPlacement.get().to_matrix());
		ms.push_to_world(matrix_from_up_forward(Vector3::yAxis * distance, Vector3::zAxis, Vector3::yAxis));

		String countdownAsText = String::printf(TXT("%i"), countdown);

		font->draw_text_at(v3d, countdownAsText, Colour::white, Vector2::zero, Vector2::one * (0.1f / font->get_height()), Vector2::half);

		ms.pop();
		ms.pop();
	}

	v3d->set_fallback_colour();

	_rt->unbind();
}