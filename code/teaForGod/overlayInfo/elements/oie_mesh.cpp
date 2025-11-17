#include "oie_mesh.h"

#include "..\overlayInfoSystem.h"

#include "..\..\..\core\containers\arrayStack.h"
#include "..\..\..\core\system\core.h"
#include "..\..\..\core\system\video\videoEnums.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\game\gameInputDefinition.h"
#include "..\..\..\framework\video\font.h"

#include "..\..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace OverlayInfo;
using namespace Elements;

//

DEFINE_STATIC_NAME(inputPromptPt);

// shader program params
DEFINE_STATIC_NAME(highlightColour);
DEFINE_STATIC_NAME(totalColour);
DEFINE_STATIC_NAME(useAlpha);

//

REGISTER_FOR_FAST_CAST(Mesh);

void Mesh::request_render_layers(OUT_ ArrayStatic<int, 8>& _renderLayers) const
{
	_renderLayers.push_back_unique(RenderLayer::Meshes);
}

void Mesh::render(OverlayInfo::System const* _system, int _renderLayer) const
{
	if (_renderLayer != RenderLayer::Meshes || !meshInstance)
	{
		return;
	}
	auto* v3d = ::System::Video3D::get();
	auto& mvs = v3d->access_model_view_matrix_stack();
	mvs.push_to_world(get_placement().to_matrix());
	
	float active = get_faded_active();
	//
	::Meshes::Mesh3DRenderingContext mrc;
	//mrc.with_existing_blend().with_existing_depth_mask().with_existing_face_display();
	Vector3 rAt = Vector3::zero;
	_system->clear_depth_buffer_once_per_render();
	mvs.push_to_world(translation_matrix(rAt));

	float useScaleBase = scale;
	float useScaleForMesh = useScaleBase;
	if (placementType == FaceCamera)
	{
		Vector3 relAt = mvs.get_current().get_translation();
		float additionalScale = calculate_additional_scale(relAt.length(), useAdditionalScale);
		useScaleBase *= additionalScale;
		useScaleForMesh *= additionalScale;
		{
			Vector3 rel = mvs.get_current().get_translation();
			mvs.push_set(look_matrix(rel, rel.normal(), mvs.get_current().vector_to_world(Vector3::zAxis).normal())); // orientate to be always facing same dir - kind of a billboard
		}
		mvs.push_to_world(look_matrix(Vector3::zero, Vector3::zAxis, -Vector3::yAxis)); // to face camera
	}
	else if (placementType == InWorld)
	{
		mvs.push_to_world(look_matrix(Vector3::zero, Vector3::zAxis, -Vector3::yAxis)); // to face camera
	}
	else
	{
		todo_implement;
	}
	float activeScale = 1.0f;
	float activeDueToCamera = 1.0f;
	if (activeShaderParamName.is_valid())
	{
		activeDueToCamera *= active;
	}
	else
	{
		activeScale *= active;
	}
	mvs.push_to_world(scale_matrix(Vector3::one * (useScaleForMesh * (BlendCurve::cubic(clamp(activeScale * 1.5f, 0.0f, 1.0f))))));
	if (placementType == InWorld && 
		frontAxis.is_set())
	{
		Transform cameraPlacement = _system->get_camera_placement();
		Transform currPlacement = get_placement();
		Vector3 dir = currPlacement.vector_to_world(frontAxis.get());
		//debug_draw_arrow(true, Colour::red, currPlacement.get_translation(), currPlacement.get_translation() + dir * 0.1f);
		//debug_draw_arrow(true, Colour::green, currPlacement.get_translation(), cameraPlacement.get_translation());
		Vector3 curr2camera = cameraPlacement.get_translation() - currPlacement.get_translation();
		float alongDir = Vector3::dot(dir, curr2camera);
		float maxDist = maxDistBlend.get(0.3f);
		activeDueToCamera *= clamp(alongDir / maxDist, 0.0f, 1.0f);
	}
	v3d->push_state();
	v3d->depth_test(::System::Video3DCompareFunc::Less);
	v3d->stencil_test();
	v3d->depth_mask();
	Colour totalColour = Colour::white;
	{	// set shader params
		if (activeShaderParamName.is_valid())
		{
			for (int i = 0; i < meshInstance->get_material_instance_count(); ++i)
			{
				if (auto* mi = meshInstance->get_material_instance(i))
				{
					if (auto* spi = mi->get_shader_program_instance())
					{
						spi->set_uniform(activeShaderParamName, active * activeDueToCamera);
					}
				}
			}
		}
		{
			Colour highlightColour = Colour::none;// HubColours::highlight().with_set_alpha(0.5f);
			_system->apply_fade_out_due_to_tips(this, REF_ totalColour);
			if (totalColour != Colour::white)
			{
				float useAlpha = totalColour.a;
				for (int i = 0; i < meshInstance->get_material_instance_count(); ++i)
				{
					if (auto* mi = meshInstance->get_material_instance(i))
					{
						if (auto* spi = mi->get_shader_program_instance())
						{
							spi->set_uniform(NAME(highlightColour), highlightColour.to_vector4());
							spi->set_uniform(NAME(totalColour), totalColour.to_vector4());
							spi->set_uniform(NAME(useAlpha), useAlpha);
						}
					}
				}
			}
		}
	}
	meshInstance->render(v3d, mrc);
	{ // revert / reset
		if (activeShaderParamName.is_valid())
		{
			for (int i = 0; i < meshInstance->get_material_instance_count(); ++i)
			{
				if (auto* mi = meshInstance->get_material_instance(i))
				{
					if (auto* spi = mi->get_shader_program_instance())
					{
						spi->set_uniform(activeShaderParamName, 1.0f); // reset
					}
				}
			}
		}
		if (totalColour != Colour::white)
		{
			for (int i = 0; i < meshInstance->get_material_instance_count(); ++i)
			{
				if (auto* mi = meshInstance->get_material_instance(i))
				{
					if (auto* spi = mi->get_shader_program_instance())
					{
						spi->set_uniform(NAME(highlightColour), Colour::none.to_vector4());
						spi->set_uniform(NAME(totalColour), Colour::white.to_vector4());
						spi->set_uniform(NAME(useAlpha), 1.0f);
					}
				}
			}
		}
	}
	v3d->pop_state();
	mvs.pop();
	mvs.pop();
	mvs.pop();
	mvs.pop();
	if (placementType == FaceCamera)
	{
		mvs.pop();
	}
}
