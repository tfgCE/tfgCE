#include "loaderText.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\render\renderCamera.h"
#include "..\render\renderContext.h"
#include "..\render\renderScene.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\mainConfig.h"
#include "..\..\core\math\math.h"
#include "..\..\core\splash\splashLogos.h"
#include "..\..\core\system\video\fragmentShaderOutputSetup.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\vertexFormatUtils.h"
#include "..\..\core\system\video\video3d.h"
#include "..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Loader;

//

// global references
DEFINE_STATIC_NAME_STR(grLoaderTextFont, TXT("loader text font"));

//

Text::Text(String const& _text, Framework::Font* _font, Optional<float> const& _scale, Optional<Colour> const & _backgroundColour, Optional<Colour> const & _textColour, Optional<Colour> const & _fadeFrom, Optional<Colour> const& _fadeTo)
: text(_text)
, backgroundColour(_backgroundColour.get(Colour::black))
, textColour(_textColour.get(Colour::white))
, fadeFromColour(_fadeFrom.get(Colour::black))
, fadeToColour(_fadeTo.get(Colour::black))
{
	font = _font;

	if (!font)
	{
		font = Framework::Library::get_current()->get_global_references().get<Framework::Font>(NAME(grLoaderTextFont));
	}

	construct_text_mesh();

	if (auto * vr = VR::IVR::get())
	{
		if (vr->is_render_view_valid())
		{
			auto viewTransform = vr->get_render_view();
			startingYaw = Rotator3::get_yaw(viewTransform.get_axis(Axis::Forward));
		}
	}
}

Text::~Text()
{
}

void Text::setup_static_display()
{
	activation = 1.0f;
	targetActivation = 1.0f;
}

void Text::construct_text_mesh()
{
	text2DMesh = Framework::Library::get_current()->get_meshes_static().create_temporary();
	text3DMesh = Framework::Library::get_current()->get_meshes_static().create_temporary();
	float height = 1.0f;
	font->construct_mesh(text2DMesh.get(), text, false, height);
	font->construct_mesh(text3DMesh.get(), text, true, height);

	text2DMeshInstance.set_placement(Transform::identity);
	text2DMeshInstance.set_mesh(text2DMesh->get_mesh());
	Framework::MaterialSetup::apply_material_setups(text2DMesh->get_material_setups(), text2DMeshInstance, System::MaterialSetting::NotIndividualInstance);

	text3DMeshInstance.set_placement(Transform::identity);
	text3DMeshInstance.set_mesh(text3DMesh->get_mesh());
	Framework::MaterialSetup::apply_material_setups(text3DMesh->get_material_setups(), text3DMeshInstance, System::MaterialSetting::NotIndividualInstance);
}

void Text::update(float _deltaTime)
{
	blockDeactivationFor = max(0.0f, blockDeactivationFor - _deltaTime);
	float currentTargetActivation = blockDeactivationFor > 0.0f ? 1.0f : targetActivation;
	activation = blend_to_using_speed_based_on_time(activation, currentTargetActivation, currentTargetActivation > activation ? 0.6f : 0.45f, min(_deltaTime, 0.03f));
	if (activation <= 0.0f && targetActivation <= 0.0f)
	{
		base::deactivate();
	}
}

void Text::display(System::Video3D * _v3d, bool _vr)
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

void Text::display_at(::System::Video3D * _v3d, bool _vr, Vector2 const & _at, float _scale)
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
	Colour mainColour = Colour::lerp(BlendCurve::cubic(activation), currentBackgroundColour, textColour);

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

		_v3d->set_fallback_colour(mainColour);
	}
	else
	{
		_v3d->setup_for_2d_display();
		_v3d->set_2d_projection_matrix_ortho(size);
		matrixStack.clear();
		matrixStack.push_to_world(translation_scale_xy_matrix(-_at * _scale, Vector2(_scale, _scale)));

		_v3d->clear_colour_depth_stencil(currentBackgroundColour);

		_v3d->set_fallback_colour(mainColour);
	}

	// display text

	Colour fallbackColour = Colour::lerp(BlendCurve::cubic(activation), currentBackgroundColour, textColour);
	{
		Matrix44 textPlacement = translation_matrix(Vector3(centre.x, centre.y, 0.0f));
		if (_vr)
		{
			Transform textTransform = vrPlacement.to_world(textPlacement.to_transform());
			text3DMeshInstance.set_fallback_colour(fallbackColour);
			Transform rotate(Vector3::zero, Rotator3(0.0f, startingYaw, 0.0f).to_quat());
			scenes[0]->add_extra(&text3DMeshInstance, rotate.to_world(textTransform));
			scenes[1]->add_extra(&text3DMeshInstance, rotate.to_world(textTransform));
		}
		else
		{
			matrixStack.push_to_world(textPlacement);
			Meshes::Mesh3DRenderingContext mrc;
			text2DMeshInstance.set_fallback_colour(fallbackColour);
			text2DMeshInstance.render(_v3d, mrc);
			matrixStack.pop();
		}
	}

	if (_vr)
	{
		scenes[0]->prepare_extras();
		scenes[1]->prepare_extras();

		Framework::Render::Context rc(_v3d);

		todo_note(TXT("store default texture somewhere"));
		//rc.set_default_texture_id(Framework::Library::get_current()->get_default_texture()->get_texture()->get_texture_id());

		_v3d->set_fallback_colour(fallbackColour);

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

		_v3d->set_fallback_colour(fallbackColour);

		System::RenderTarget::bind_none();

		rt->resolve();

		matrixStack.clear();
		matrixStack.force_requires_send_all();

		render_rt(_v3d);
	}
}

