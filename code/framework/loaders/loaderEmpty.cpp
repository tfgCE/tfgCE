#include "loaderEmpty.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\render\renderCamera.h"
#include "..\render\renderContext.h"
#include "..\render\renderScene.h"

#include "..\..\core\mainConfig.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\vertexFormatUtils.h"
#include "..\..\core\system\video\video3d.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Loader;

//

Empty::Empty(Optional<Colour> const & _backgroundColour, Optional<Colour> const & _fadeFrom, Optional<Colour> const& _fadeTo)
: backgroundColour(_backgroundColour.get(Colour::black))
, fadeFromColour(_fadeFrom.get(Colour::black))
, fadeToColour(_fadeTo.get(Colour::black))
{
}

Empty::~Empty()
{
}

void Empty::setup_static_display()
{
	activation = 1.0f;
	targetActivation = 1.0f;
}

void Empty::update(float _deltaTime)
{
	blockDeactivationFor = max(0.0f, blockDeactivationFor - _deltaTime);
	float currentTargetActivation = blockDeactivationFor > 0.0f ? 1.0f : targetActivation;
	activation = blend_to_using_speed_based_on_time(activation, currentTargetActivation, currentTargetActivation > activation ? 0.6f : 0.45f, min(_deltaTime, 0.03f));
	if (activation <= 0.0f && targetActivation <= 0.0f)
	{
		base::deactivate();
	}
}

void Empty::display(System::Video3D * _v3d, bool _vr)
{
	if (!VR::IVR::get() ||
		VR::IVR::get()->get_available_functionality().renderToScreen)
	{
		display_at(_v3d, false);
	}
	if (_vr)
	{
		display_at(_v3d, _vr);
	}
}

void Empty::display_at(::System::Video3D * _v3d, bool _vr, Vector2 const & _at, float _scale)
{
	rt->bind();
	_v3d->set_default_viewport();
	_v3d->set_near_far_plane(0.02f, 100.0f);

	// ready sizes
	Vector2 size(12.0f, 12.0f);
	size.x = size.y * _v3d->get_aspect_ratio();
	Vector2 centre = size * 0.5f / _scale;

	if (_vr)
	{
		centre = Vector2::zero;
	}

	::System::VideoMatrixStack& matrixStack = _v3d->access_model_view_matrix_stack();

	::Framework::Render::Scene* scenes[2];

	Transform vrPlacement;

	Colour currentBackgroundColour = Colour::lerp(BlendCurve::cubic(activation), (blockDeactivationFor > 0.0f ? 1.0f : targetActivation) > 0.0f? fadeFromColour : fadeToColour, backgroundColour);

	if (_vr)
	{
		auto* vr = VR::IVR::get();
		Vector3 eyeLoc = Vector3(0.0f, 0.0f, 1.65f);
		Transform eyePlacement = look_at_matrix(eyeLoc, eyeLoc + Vector3(0.0f, 2.0f, 0.0f), Vector3::zAxis).to_transform();
		if (vr->is_render_view_valid())
		{
			eyePlacement = vr->get_render_view();
		}
		eyeLoc = eyePlacement.get_translation();

		update_vr_centre(eyeLoc);
		Vector3 vrLoc = vrCentre.get() + 5.0f * Vector3::yAxis + Vector3(0.0f, 0.0f, 1.3f);
		Vector3 vrAtLoc = vrLoc + Vector3(-0.0f, 1.0f, 0.0f);
		vrPlacement = look_at_matrix(vrLoc, vrAtLoc, Vector3::zAxis).to_transform();
		vrPlacement.set_scale(0.2f);

		::Framework::Render::Camera camera;
		camera.set_placement(nullptr, eyePlacement);
		camera.set_near_far_plane(0.02f, 30000.0f);
		camera.set_background_near_far_plane();

		scenes[0] = Framework::Render::Scene::build(camera, NP, nullptr, NP, Framework::Render::SceneMode::VR_Left);
		scenes[1] = Framework::Render::Scene::build(camera, NP, nullptr, NP, Framework::Render::SceneMode::VR_Right);

		scenes[0]->set_background_colour(currentBackgroundColour);
		scenes[1]->set_background_colour(currentBackgroundColour);

		_v3d->set_fallback_colour(currentBackgroundColour);
	}
	else
	{
		_v3d->setup_for_2d_display();
		_v3d->set_2d_projection_matrix_ortho(size);
		matrixStack.clear();
		matrixStack.push_to_world(translation_scale_xy_matrix(-_at * _scale, Vector2(_scale, _scale)));

		_v3d->clear_colour_depth_stencil(currentBackgroundColour);

		_v3d->set_fallback_colour(currentBackgroundColour);
	}

	if (_vr)
	{
		scenes[0]->prepare_extras();
		scenes[1]->prepare_extras();

		Framework::Render::Context rc(_v3d);

		todo_note(TXT("store default texture somewhere"));
		//rc.set_default_texture_id(Framework::Library::get_current()->get_default_texture()->get_texture()->get_texture_id());

		_v3d->set_fallback_colour(currentBackgroundColour);

		for_count(int, i, 2)
		{
			scenes[i]->render(rc, rt.get());
			scenes[i]->release();
		}

		VR::IVR::get()->copy_render_to_output(_v3d);
	}
	else
	{
		matrixStack.pop();

		_v3d->set_fallback_colour(currentBackgroundColour);

		System::RenderTarget::bind_none();

		rt->resolve();

		matrixStack.clear();
		matrixStack.force_requires_send_all();

		render_rt(_v3d);
	}
}

