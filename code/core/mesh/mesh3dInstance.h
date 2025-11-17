#pragma once

#include "..\profilePerformanceSettings.h"
#include "..\system\video\video3d.h"
#include "..\system\video\materialInstance.h"
#include "..\system\video\iMaterialInstanceProvider.h"
#include "iMesh3dRenderBonesProvider.h"
#include "..\types\optional.h"

#include <functional>

namespace Meshes
{
	interface_class IMesh3D;
	class Mesh3DInstance;
	class Skeleton;
	struct Pose;
	struct Mesh3DRenderingContext;
	struct Mesh3DRenderingOrder;

	typedef std::function<void(Mesh3DInstance &)> OnMesh3DInstance;
	typedef std::function<void(Mesh3DInstance const &)> OnMesh3DInstanceConst;

	/**
	 *	Proper setup requires:
	 *		setting mesh
	 *		setting materials (this can be done by hand or using framework's material setup)
	 */
	class Mesh3DInstance
	: public ::System::IMaterialInstanceProvider
	, public IMesh3DRenderBonesProvider
	{
#ifdef ALLOW_SELECTIVE_RENDERING
	public:
		bool shouldRender = true;
#endif
	public:
		Mesh3DInstance();
		Mesh3DInstance(IMesh3D const * _mesh, Skeleton const * _skeleton, Transform const & _placement);
		virtual ~Mesh3DInstance();

		void set_individual_offset(float _individualOffset) { individualOffset = _individualOffset; }
		void use_individual_offset_of(Mesh3DInstance const& _of) { individualOffset = _of.individualOffset; }

		void set_preferred_rendering_buffer(Name const & _preferredRenderingBuffer) { preferredRenderingBuffer = _preferredRenderingBuffer; }
		Name const & get_preferred_rendering_buffer() const { return preferredRenderingBuffer; }

		void set_rendering_order_priority(int _priority) { overrideRenderingOrderPriority = _priority; }
		void override_rendering_order(REF_ Mesh3DRenderingOrder & _renderingOrder) const;
		
		void set_on_render(OnMesh3DInstanceConst _onRender) { onRender = _onRender; }
		void set_prepare_to_render(OnMesh3DInstance _prepareToRender) { prepareToRender = _prepareToRender; }

		inline void prepare_to_render() { if (prepareToRender) prepareToRender(*this); }

		Optional<bool> & access_can_be_combined() { return canBeCombined; }
		bool can_be_combined() const { return canBeCombined.get(true); } // explicitly
		Optional<bool> & access_can_be_combined_as_immovable() { return canBeCombinedAsImmovable; }
		bool can_be_combined_as_immovable() const;

		void hard_copy_from(Mesh3DInstance const & _instance, ::System::MaterialInstance const * _materialOverride = nullptr); // creates copy of instance parameters
		void hard_copy_materials_at(int _partsAsBits, ::System::MaterialInstance const* _materialOverride);

		bool does_any_blending() const { return srcBlend.is_set() || destBlend.is_set(); }
		void set_blend(::System::BlendOp::Type _srcBlend, ::System::BlendOp::Type _destBlend) { srcBlend = _srcBlend; destBlend = _destBlend; }
		void clear_blend() { srcBlend.clear(); destBlend.clear(); }
		
		void set_depth_mask(bool _depthMask) { depthMask = _depthMask; }
		void clear_depth_mask() { depthMask.clear(); }

		void set_fallback_colour(Colour const & _fallbackColour) { fallbackColour = _fallbackColour; }
		void clear_fallback_colour() { fallbackColour.clear(); }

		void set_mesh(IMesh3D const * _mesh, Skeleton const * _skeleton = nullptr);
		IMesh3D const * get_mesh() const { return mesh; }
		Skeleton const * get_skeleton() const { return skeleton; }

		void set_placement(Transform const & _placement) { placement = _placement; placementMat = _placement.to_matrix(); }
		Transform const & get_placement() const { return placement; }
		Matrix44 const & get_placement_matrix() const { return placementMat; }

		Pose * get_pose_MS() { return poseMS; }
		Pose const * get_pose_MS() const { return poseMS; }

		Array<Matrix44> & access_render_bone_matrices() { return renderBoneMatrices; }
		void set_render_bone_matrices(Array<Matrix44> const & _bonesMS, OPTIONAL_ Array<int> const * _remapBones = nullptr); // _remapBones[target] = from
		void set_render_bone_matrix(int _idx, Matrix44 const& _boneMS);

		PlaneSet & access_clip_planes() { return clipPlanes; }
		PlaneSet const & get_clip_planes() const { return clipPlanes; }

		int calculate_triangles() const;

		void render(::System::Video3D* _v3d, Mesh3DRenderingContext const & _renderingContext) const;

		void dont_provide_materials(bool _dontProvideMaterials = true) { dontProvideMaterials = _dontProvideMaterials; }
		void requires_at_least_materials(int _count) { materials.set_size(max(materials.get_size(), _count)); }
		void set_material(int _idx, ::System::Material* _material);
		void fill_materials_with_missing();
		int get_material_instance_count() const { return useMaterialCount; }
		::System::MaterialInstance * get_material_instance(int _idx) { return materials.is_index_valid(_idx) && _idx < useMaterialCount ? &materials[_idx] : nullptr; }
		::System::MaterialInstance const * get_material_instance(int _idx) const { return materials.is_index_valid(_idx) && _idx < useMaterialCount? &materials[_idx] : nullptr; }
		::System::MaterialInstance * access_material_instance(int _idx) { useMaterialCount = max(useMaterialCount, _idx + 1); materials.set_size(max(materials.get_size(), _idx + 1)); return &materials[_idx]; }

		void log(LogInfoContext & _log) const;

		// #include "mesh3dInstanceImpl.inl"
		template<typename Class>
		void set_shader_param(Name const& _paramName, Class const& _value, int _materialIdx = NONE); // sets params in all materials or just in one

	public: // IMaterialInstanceProvider
		implement_::System::MaterialInstance * get_material_instance_for_rendering(int _idx) { an_assert(materialChainsSet || !renderingMaterials.is_empty(), TXT("you did not setup materials for instance, did you call apply_material_setups?")); return materialChainsSet ? (materials.is_index_valid(_idx) ? &materials[_idx] : nullptr) : (renderingMaterials.is_index_valid(_idx) ? &renderingMaterials[_idx] : nullptr); }
		implement_::System::MaterialInstance const * get_material_instance_for_rendering(int _idx) const { an_assert(materialChainsSet || !renderingMaterials.is_empty(), TXT("you did not setup materials for instance, did you call apply_material_setups?")); return materialChainsSet ? (materials.is_index_valid(_idx) ? &materials[_idx] : nullptr) : (renderingMaterials.is_index_valid(_idx) ? &renderingMaterials[_idx] : nullptr); }

	public:
		// this is for rendering buffer setup - it doesn't require all missing to be setup, just material info
		::System::MaterialInstance const * get_material_instance_for_rendering_buffer_setup(int _idx) const { an_assert(materialChainsSet, TXT("you did not setup materials for instance, did you call apply_material_setups?")); return materialChainsSet ? (materials.is_index_valid(_idx) ? &materials[_idx] : nullptr) : nullptr; }

	public: // IMesh3DRenderBonesProvider
		implement_ Matrix44 const * get_render_bone_matrices(OUT_ int & _matricesCount) const { _matricesCount = renderBoneMatrices.get_size(); return renderBoneMatrices.get_data(); }

	private:
		float individualOffset;
		IMesh3D const * mesh;
		Skeleton const * skeleton;
		Pose * poseMS;
		Transform placement;
		CACHED_ Matrix44 placementMat;
		bool dontProvideMaterials = false;
		Array<::System::MaterialInstance> materials; // stored state of materials
		int useMaterialCount = 0; // this is hack to avoid setting lots of stuff in shared materials, so we keep instances with multiple materials but indicate we have less and we use less
		bool materialChainsSet; // if material chains are set (all missing are filled) it is set to true, otherwise it requires going through parents of materials
		Array<Matrix44> renderBoneMatrices; // stored state of render bone matrices
		Optional<bool> canBeCombined; 
		Optional<bool> canBeCombinedAsImmovable;
		mutable Array<::System::MaterialInstance> renderingMaterials; // materials for rendering, filled if chains are not set
		mutable Concurrency::SpinLock renderingMaterialsLock = Concurrency::SpinLock(TXT("Meshes.Mesh3DInstance.renderingMaterialsLock"));
		
		// other params
		OnMesh3DInstanceConst onRender = nullptr;
		OnMesh3DInstance prepareToRender = nullptr;
		Optional<int> overrideRenderingOrderPriority; // high number means to render last
		Name preferredRenderingBuffer; // to choose one rendering buffer
		PlaneSet clipPlanes; // to allow individual clip planes

		Optional<::System::BlendOp::Type> srcBlend;
		Optional<::System::BlendOp::Type> destBlend;
		Optional<bool> depthMask;
		Optional<Colour> fallbackColour;

	public: // do not use/implement
		Mesh3DInstance & operator = (Mesh3DInstance const& _other)
#ifndef AN_CLANG
			; // do not implement
#else
		{
			// implemented for clang as it compiles everything 
			an_assert(false, TXT("do not use!"));
			return *this;
		}
#endif

	};

};
