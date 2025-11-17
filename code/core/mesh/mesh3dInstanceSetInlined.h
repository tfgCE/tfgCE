#pragma once

#include "mesh3dInstance.h"
#include "mesh3dRenderingBuffer.h"
#include "mesh3dTypes.h"

// it's hard copy, inlined set

namespace Meshes
{
	class Mesh3DInstance;
	class Mesh3DInstanceSet;
	struct Mesh3DRenderingBuffer;
	struct Mesh3DRenderingBufferSet;
	struct Mesh3DRenderingContext;

	// TODO replace with Appearance?
	class Mesh3DInstanceSetInlined
	{
	public:
		void clear();

		void hard_copy_add(Mesh3DInstance const & _instance, Transform const & _placement = Transform::identity, ::System::MaterialInstance const* _materialOverride = nullptr);
		void hard_copy_from(Mesh3DInstanceSet const & _instanceSet, ::System::MaterialInstance const* _materialOverride = nullptr);
		void hard_copy_from_add(Mesh3DInstanceSet const & _instanceSet, ::System::MaterialInstance const* _materialOverride = nullptr); // add to already existing

		void prepare_to_render();

		void render(::System::Video3D* _v3d, Mesh3DRenderingContext const & _renderingContext) const;

		void add_to(REF_ Mesh3DRenderingBuffer & _renderingBuffer, Matrix44 const & _placement = Matrix44::identity, Meshes::ModifyRenderingContextOnRender _modify_rendering_context_on_render = nullptr, Optional<PlaneSet> const & _clipPlanes = NP) const;
		void add_to(REF_ Mesh3DRenderingBufferSet & _renderingBufferSet, Matrix44 const & _placement = Matrix44::identity, Meshes::ModifyRenderingContextOnRender _modify_rendering_context_on_render = nullptr, Optional<PlaneSet> const & _clipPlanes = NP) const;

		// #include "mesh3dInstanceSetInlinedImpl.inl"
		template<typename Class>
		void set_shader_param(Name const& _paramName, Class const& _value, int _materialIdx = NONE); // sets params in all materials or just in one

	public:
		Array<Mesh3DInstance> & access_instances() { return instances; }

	private:
		Array<Mesh3DInstance> instances;

		Mesh3DInstanceSetInlined const & operator = (Mesh3DInstanceSetInlined const & _other); // do not implement_

		void hard_copy_from_fill(Mesh3DInstanceSet const & _instanceSet, Mesh3DInstance* _destInstance, ::System::MaterialInstance const* _materialOverride); // fill starting at
	};

};
