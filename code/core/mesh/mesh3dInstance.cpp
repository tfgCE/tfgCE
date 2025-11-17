#include "mesh3dInstance.h"
#include "iMesh3d.h"
#include "mesh3d.h"

#include "skeleton.h"
#include "pose.h"

#include "..\random\random.h"

#include "..\system\video\viewFrustum.h"
#include "..\system\video\videoClipPlaneStackImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define AN_OUTPUT_SHADER_PARAMS

//

using namespace ::System;
using namespace Meshes;

//

Mesh3DInstance::Mesh3DInstance()
: mesh(nullptr)
, skeleton(nullptr)
, poseMS(nullptr)
{
	set_placement(Transform::identity);

	individualOffset = Random::get_float(-1000.0f, 1000.0f);
}

Mesh3DInstance::Mesh3DInstance(IMesh3D const * _mesh, Skeleton const * _skeleton, Transform const & _placement)
: mesh(nullptr)
, skeleton(nullptr)
, poseMS(nullptr)
{
	set_mesh(_mesh, _skeleton);
	set_placement(_placement);

	individualOffset = Random::get_float(-1000.0f, 1000.0f);
}

Mesh3DInstance::~Mesh3DInstance()
{
	if (poseMS)
	{
		poseMS->release();
		poseMS = nullptr;
	}
}

void Mesh3DInstance::set_material(int _idx, Material* _material)
{
	materials.set_size(max(materials.get_size(), _idx + 1));
	materials[_idx].set_material(_material, mesh->get_material_usage());
	useMaterialCount = max(useMaterialCount, _idx + 1);
	materialChainsSet = false;
}

void Mesh3DInstance::fill_materials_with_missing()
{
	for_every(material, materials)
	{
		if (for_everys_index(material) >= useMaterialCount)
		{
			break;
		}
		material->fill_missing();
	}
	materialChainsSet = true;
}

void Mesh3DInstance::hard_copy_materials_at(int _partsAsBits, ::System::MaterialInstance const* _materialOverride)
{
	int partIdx = 0;
	for_every(destMat, materials)
	{
		if (is_flag_set(_partsAsBits, bit(partIdx)))
		{
			destMat->hard_copy_from_with_filled_missing(*_materialOverride);
		}
		++partIdx;
	}
}

void Mesh3DInstance::hard_copy_from(Mesh3DInstance const & _instance, ::System::MaterialInstance const * _materialOverride)
{
	individualOffset = _instance.individualOffset;
	mesh = _instance.mesh;
	placement = _instance.placement;
	placementMat = _instance.placementMat;
	dontProvideMaterials = _instance.dontProvideMaterials;
	materials.set_size(max(materials.get_size(), _instance.materials.get_size())); // keep whatever there is (to avoid skeletons with great numbers of bones resetting their bones).
	useMaterialCount = _instance.materials.get_size();
	MaterialInstance * destMat = materials.get_data();
	for_every(srcMat, _instance.materials)
	{
		auto destUsage = destMat->get_material_usage();
		destMat->hard_copy_from_with_filled_missing(_materialOverride ? *_materialOverride : *srcMat, destUsage);
		++destMat;
	}
	materialChainsSet = true;
	renderBoneMatrices.set_size(_instance.renderBoneMatrices.get_size());
	if (renderBoneMatrices.get_size())
	{
		memory_copy(renderBoneMatrices.get_data(), _instance.renderBoneMatrices.get_data(), _instance.renderBoneMatrices.get_data_size());
	}
	canBeCombined = _instance.canBeCombined;
	canBeCombinedAsImmovable = _instance.canBeCombinedAsImmovable;
	onRender = _instance.onRender;
	prepareToRender = _instance.prepareToRender;

	overrideRenderingOrderPriority = _instance.overrideRenderingOrderPriority;
	preferredRenderingBuffer = _instance.preferredRenderingBuffer;
	clipPlanes = _instance.clipPlanes;

	srcBlend = _instance.srcBlend;
	destBlend = _instance.destBlend;
	depthMask = _instance.depthMask;
	fallbackColour = _instance.fallbackColour;
}

void Mesh3DInstance::render(Video3D* _v3d, Mesh3DRenderingContext const & _renderingContext) const
{
	if (onRender)
	{
		onRender(*this);
	}

	if (!mesh)
	{
		return;
	}

	if (srcBlend.is_set() && !_renderingContext.useExistingBlend && _renderingContext.renderBlending.is_set())
	{
		// check if this is blending pass
		an_assert(destBlend.is_set());
		if (srcBlend.get() == ::System::BlendOp::One && destBlend.get() == ::System::BlendOp::None)
		{
			if (_renderingContext.renderBlending.get())
			{
				return;
			}
		}
		else
		{
			if (! _renderingContext.renderBlending.get())
			{
				return;
			}
		}
	}

	_v3d->set_current_individual_offset(individualOffset);

	// create copy and setup mesh bones provider
	Mesh3DRenderingContext renderingContext = _renderingContext;
	renderingContext.meshBonesProvider = this;

	if (!materialChainsSet)
	{
		renderingMaterialsLock.acquire();
		todo_future(TXT("add some stats here, some info that this should be avoided"));
		// fill rendering materials to have materials have all parameters set right to the top most parent
		renderingMaterials.set_size(materials.get_size());
		MaterialInstance const * srcMat = materials.get_data();
		auto* mesh3d = fast_cast<Mesh3D>(mesh);
		for_every(renderingMaterial, renderingMaterials)
		{
			int idx = for_everys_index(renderingMaterial);
			if (idx >= useMaterialCount)
			{
				break;
			}
			if (!mesh3d || mesh3d->is_part_valid(idx))
			{
				renderingMaterial->hard_copy_from_with_filled_missing(*srcMat);
			}
			++srcMat;
		}
	}

	bool restoreBlend = false;
	if (srcBlend.is_set() && !renderingContext.useExistingBlend)
	{
		an_assert(destBlend.is_set());
		if (srcBlend.get() == ::System::BlendOp::None || destBlend.get() == ::System::BlendOp::None)
		{
			_v3d->mark_disable_blend();
		}
		else
		{
			_v3d->mark_enable_blend(srcBlend.get(), destBlend.get());
			_v3d->mark_enable_polygon_offset(-0.1f, -0.1f);
		}
		renderingContext.with_existing_blend();
		restoreBlend = true;
	}

	bool restoreDepthMask = false;
	if (depthMask.is_set() && ! renderingContext.useExistingDepthMask)
	{
		_v3d->depth_mask(depthMask.get());
		renderingContext.with_existing_depth_mask();
		restoreDepthMask = true;
	}

	bool restoreFallbackColour = false;
	if (fallbackColour.is_set() && !renderingContext.useExistingFallbackColour)
	{
		_v3d->set_fallback_colour(fallbackColour.get());
		renderingContext.with_existing_fallback_colour();
		restoreFallbackColour = true;
	}

	VideoMatrixStack& modelViewStack = _v3d->access_model_view_matrix_stack();
	VideoClipPlaneStack& clipPlaneStack = _v3d->access_clip_plane_stack();

	modelViewStack.push_to_world(placementMat);
	if (! clipPlanes.is_empty())
	{
		clipPlaneStack.push_current_as_is();
		clipPlaneStack.add_to_current(clipPlanes);
	}
#ifdef AN_OUTPUT_SHADER_PARAMS
	{
		if (!this->materials.is_empty())
		{
			if (auto* spi = materials[0].get_shader_program_instance())
			{
				DEFINE_STATIC_NAME(particleInfos);
				if (auto* sp = spi->get_shader_param(NAME(particleInfos)))
				{
					output(TXT("render mshinst%p"), this);
					DEFINE_STATIC_NAME(inSkinningBones);
					if (auto* spb = spi->get_shader_param(NAME(inSkinningBones)))
					{
						LogInfoContext l;
						l.log(TXT("MESH INSTANCE RENDERING"));
						{
							LOG_INDENT(l);
							l.log(TXT("particleInfos(data size : %i, count : %i)"), sp->valueDsize, sp->valueDsize / sizeof(Vector4));
							l.log(TXT("inSkinningBones(data size : %i, count : %i)"), spb->valueDsize, spb->valueDsize / sizeof(Matrix44));
							Array<Vector4> a;
							sp->get_as_array_vector4(a);
							Vector4 const* ptr = (Vector4 const*)sp->get_uniform_data();
							Matrix44 const* ptrB = (Matrix44 const*)spb->get_uniform_data();
							int i = 0;
							for_every(pd, a)
							{
								l.log(TXT("dissolve %2i : %.6f [%.6f]"), for_everys_index(pd), pd->y, *ptr);
								if (i < (int)(spb->valueDsize / sizeof(Matrix44)))
								{
									l.log(TXT("scale %.6f"), ptrB->extract_scale());
									//ptrB->log(l);
								}
								++i;
								++ptr;
								++ptrB;
							}
						}
						l.output_to_output();
					}
				}
			}
		}
	}
#endif
#ifdef ALLOW_SELECTIVE_RENDERING
	SelectiveRendering::renderNow = shouldRender;
#endif
	mesh->render(_v3d, dontProvideMaterials? nullptr : this, renderingContext);
#ifdef ALLOW_SELECTIVE_RENDERING
	SelectiveRendering::renderNow = true;
#endif
	if (!clipPlanes.is_empty())
	{
		clipPlaneStack.pop();
	}
	modelViewStack.pop();

	if (restoreBlend)
	{
		_v3d->mark_disable_blend(); // it's a lazy op, so we should be fine
		_v3d->mark_disable_polygon_offset();
	}

	if (restoreDepthMask)
	{
		_v3d->depth_mask(true);
	}

	if (restoreFallbackColour)
	{
		_v3d->set_fallback_colour();
	}

	if (!materialChainsSet)
	{
		renderingMaterialsLock.release();
	}
}

void Mesh3DInstance::set_render_bone_matrices(Array<Matrix44> const & _bonesMS, OPTIONAL_ Array<int> const* _remapBones)
{
	an_assert(skeleton);
	Array<Matrix44> const& bonesDefaultInvertedMatrixMS = skeleton->get_bones_default_inverted_matrix_MS();
	renderBoneMatrices.set_size(bonesDefaultInvertedMatrixMS.get_size());

	auto iDefInvMatrixMS = bonesDefaultInvertedMatrixMS.begin_const();

	if (!_remapBones || _remapBones->is_empty())
	{
		auto iMatrixMS = _bonesMS.begin_const();
		for_every(renderBoneMatrix, renderBoneMatrices)
		{
			*renderBoneMatrix = iMatrixMS->to_world(*iDefInvMatrixMS);
			++iMatrixMS;
			++iDefInvMatrixMS;
		}
	}
	else
	{
		an_assert(_remapBones->get_size() == renderBoneMatrices.get_size());
		auto iRemapBone = _remapBones->begin_const();
		for_every(renderBoneMatrix, renderBoneMatrices)
		{
			int from = *iRemapBone;
			if (from >= 0)
			{
				*renderBoneMatrix = _bonesMS[from].to_world(*iDefInvMatrixMS);
			}
			else
			{
				*renderBoneMatrix = Matrix44::identity;
			}
			++iDefInvMatrixMS;
			++iRemapBone;
		}
	}
}

void Mesh3DInstance::set_render_bone_matrix(int _idx, Matrix44 const & _boneMS)
{
	an_assert(skeleton);
	Array<Matrix44> const & bonesDefaultInvertedMatrixMS = skeleton->get_bones_default_inverted_matrix_MS();
	renderBoneMatrices.set_size(bonesDefaultInvertedMatrixMS.get_size());
	if (renderBoneMatrices.is_index_valid(_idx))
	{
		renderBoneMatrices[_idx] = _boneMS.to_world(bonesDefaultInvertedMatrixMS[_idx]);
	}
}

void Mesh3DInstance::set_mesh(IMesh3D const * _mesh, Skeleton const * _skeleton)
{
	mesh = _mesh;
	skeleton = _skeleton;
	materials.clear();
	materialChainsSet = false;
	if (poseMS)
	{
		poseMS->release();
		poseMS = nullptr;
	}
	if (skeleton)
	{
		poseMS = Pose::get_one(skeleton);
		poseMS->fill_with(skeleton->get_bones_default_placement_MS());
	}
}

bool Mesh3DInstance::can_be_combined_as_immovable() const
{
	if (!can_be_combined())
	{
		return false;
	}
	if (canBeCombinedAsImmovable.is_set())
	{
		return canBeCombinedAsImmovable.get();
	}
	else if (Mesh3D const * mesh3d = fast_cast<Mesh3D const>(mesh))
	{
		return mesh3d->can_be_combined_as_immovable();
	}
	else
	{
		// just in any case, do not combine as immovable
		return false;
	}
}

void Mesh3DInstance::override_rendering_order(REF_ Mesh3DRenderingOrder & _renderingOrder) const
{
	if (overrideRenderingOrderPriority.is_set())
	{
		_renderingOrder.priority = overrideRenderingOrderPriority.get();
	}
}

void Mesh3DInstance::log(LogInfoContext & _log) const
{
	if (auto * m3d = fast_cast<Meshes::Mesh3D>(get_mesh()))
	{
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
		_log.log(m3d->get_debug_mesh_name());
#endif
	}
	LOG_INDENT(_log);
	for_every(material, materials)
	{
		if (for_everys_index(material) >= useMaterialCount)
		{
			_log.log(TXT("material %i there, but unused"), for_everys_index(material));
			break;
		}
		_log.log(TXT("material %i"), for_everys_index(material));
		LOG_INDENT(_log);
		material->log(_log);
	}
}

int Mesh3DInstance::calculate_triangles() const
{
	return mesh ? mesh->calculate_triangles() : 0;
}
