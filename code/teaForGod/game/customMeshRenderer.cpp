#include "customMeshRenderer.h"

#include "..\..\core\mainConfig.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\video3d.h"

#include "..\..\framework\appearance\mesh.h"
#include "..\..\framework\video\material.h"
#include "..\..\framework\pipelines\renderingPipeline.h"

using namespace TeaForGodEmperor;

//

CustomMeshRenderer::CustomMeshRenderer()
{
}

CustomMeshRenderer::~CustomMeshRenderer()
{
}

void CustomMeshRenderer::setup_for(VectorInt2 const & _resolution)
{
	assert_rendering_thread();

	renderContext.sceneRenderTarget = nullptr;
	renderContext.finalRenderTarget = nullptr;

	resolution = _resolution;

	{
		::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(
			resolution,
			MainConfig::global().get_video().get_msaa_samples(),
			::System::DepthStencilFormat::defaultFormat);
		rtSetup.copy_output_textures_from(::Framework::RenderingPipeline::get_default_output_definition());
		RENDER_TARGET_CREATE_INFO(TXT("custom mesh renderer, scene render target"));
		renderContext.sceneRenderTarget = new ::System::RenderTarget(rtSetup);
	}

	{
		::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(
			resolution);
		DEFINE_STATIC_NAME(colour);
		// we use ordinary rgb 8bit textures
		rtSetup.add_output_texture(::System::OutputTextureDefinition(NAME(colour),
			::System::VideoFormat::rgba8,
			::System::BaseVideoFormat::rgba));
		RENDER_TARGET_CREATE_INFO(TXT("custom mesh renderer, final render target")); 
		renderContext.finalRenderTarget = new ::System::RenderTarget(rtSetup);
	}
}

void CustomMeshRenderer::update(float _deltaTime)
{
	placement.set_orientation(placement.get_orientation().to_world((rotationSpeed * _deltaTime).to_quat()));
}

void CustomMeshRenderer::render()
{
	if (Game* game = Game::get_as<Game>())
	{
		assert_rendering_thread();

		//game->render_empty_background_on_render(backgroundColour, &renderContext);

		Framework::Render::Camera camera;
		camera.set_fov(cameraFOV);
		camera.set_placement(nullptr, Transform::identity);
		camera.set_view_origin(VectorInt2::zero);
		camera.set_view_size(renderContext.sceneRenderTarget->get_size());
		camera.set_view_aspect_ratio(aspect_ratio(renderContext.sceneRenderTarget->get_full_size_for_aspect_ratio_coef()));
		camera.set_near_far_plane(Game::s_renderingNearZ, Game::s_renderingFarZ);
		camera.set_background_near_far_plane();

		renderContext.sceneRenderTarget->bind();

		::System::Video3D * v3d = ::System::Video3D::get();

		v3d->setup_for_3d_display();
		v3d->clear_colour_depth(backgroundColour);

		Framework::Render::Context rc(v3d);
		camera.setup(rc);

		renderContext.renderCamera = camera; // store camera now, as it has projection matrix set

		System::VideoMatrixStack& modelViewStack = v3d->access_model_view_matrix_stack();
		Transform relativeMeshPlacement = Transform(-meshCentreOffset, Quat::identity);
		Transform moveAwayFromCamera = Transform(Vector3::yAxis * meshDistance, Quat::identity);
		Transform actualMeshPlacement = Transform(moveAwayFromCamera).to_world(placement.to_world(relativeMeshPlacement));
		modelViewStack.push_to_world(actualMeshPlacement.to_matrix());

		if (renderMesh)
		{
			Meshes::Mesh3DRenderingContext renderingContext;
			renderingContext.meshBonesProvider = &meshInstance;
			meshInstance.render(v3d, renderingContext);
		}

		modelViewStack.pop();

		System::RenderTarget::unbind_to_none();

		game->render_post_process(&renderContext);
	}
}

void CustomMeshRenderer::set_mesh(Framework::Mesh const * _mesh)
{
	if (_mesh && _mesh->get_mesh())
	{
		renderMesh = _mesh;

		if (auto const * skeleton = _mesh->get_skeleton())
		{
			meshInstance.set_mesh(_mesh->get_mesh(), skeleton->get_skeleton());
			meshInstance.set_render_bone_matrices(skeleton->get_skeleton()->get_bones_default_matrix_MS());
		}
		else
		{
			meshInstance.set_mesh(_mesh->get_mesh(), nullptr);
		}
#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
		::System::MaterialShaderUsage::Type materialShaderUsage = _mesh->get_mesh()->get_material_usage();
#endif
#endif
		meshInstance.set_placement(Transform::identity);

		Framework::MaterialSetup::apply_material_setups(_mesh->get_material_setups(), meshInstance, System::MaterialSetting::NotIndividualInstance);
	}
	else
	{
		renderMesh = nullptr;
		meshInstance.set_mesh(nullptr, nullptr);
	}
	update_offsets_and_distances();
}

void CustomMeshRenderer::update_offsets_and_distances()
{
	float objectSize = 1.0f;
	meshCentreOffset = Vector3::zero;
	if (renderMesh)
	{
		todo_important(TXT("store it somewhere? base on vertices?"));
	}

	float verticalViewSize = takesViewSpacePt != 0.0f ? objectSize / takesViewSpacePt : objectSize;

	// fov is vertical, if vertical resolution is smaller, we're fine, if horizontal, we need to upscale verticalViewSize
	if (resolution.x < resolution.y)
	{
		verticalViewSize *= (float)resolution.y / (float)resolution.x;
	}

	// as we know our fov, we may calculate distance to get object to take this view space
	//				   ^
	//				   |
	//	Camera *-------* Object
	//				   |
	//				   v 
	// our vertical view size is whole thing, we take half of it and half of fov
	
	float const fovHalf = cameraFOV * 0.5f;
	float const sizeHalf = verticalViewSize * 0.5f;
	meshDistance = sizeHalf / tan_deg(fovHalf);
}

