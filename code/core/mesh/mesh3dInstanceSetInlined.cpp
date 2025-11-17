#include "mesh3dInstanceSetInlined.h"

#include "mesh3dInstanceSet.h"
#include "mesh3dRenderingBufferSet.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace ::System;
using namespace Meshes;

//

void Mesh3DInstanceSetInlined::clear()
{
	instances.clear();
}

void Mesh3DInstanceSetInlined::hard_copy_add(Mesh3DInstance const & _instance, Transform const & _placement, ::System::MaterialInstance const* _materialOverride)
{
	int prevInstancesCount = instances.get_size();
	instances.set_size(prevInstancesCount + 1);

	instances[prevInstancesCount].hard_copy_from(_instance, _materialOverride);
	instances[prevInstancesCount].set_placement(_placement.to_world(instances[prevInstancesCount].get_placement()));
}

void Mesh3DInstanceSetInlined::hard_copy_from(Mesh3DInstanceSet const & _instanceSet, ::System::MaterialInstance const* _materialOverride)
{
	instances.set_size(_instanceSet.instances.get_size());

	hard_copy_from_fill(_instanceSet, instances.get_data(), _materialOverride);
}

void Mesh3DInstanceSetInlined::hard_copy_from_add(Mesh3DInstanceSet const & _instanceSet, ::System::MaterialInstance const* _materialOverride)
{
	int prevInstancesCount = instances.get_size();
	instances.set_size(prevInstancesCount + _instanceSet.instances.get_size());

	hard_copy_from_fill(_instanceSet, &instances[prevInstancesCount], _materialOverride);
}

void Mesh3DInstanceSetInlined::hard_copy_from_fill(Mesh3DInstanceSet const & _instanceSet, Mesh3DInstance* _destInstance, ::System::MaterialInstance const* _materialOverride)
{
	for_every_const_ptr(instance, _instanceSet.instances)
	{
		_destInstance->hard_copy_from(*instance, _materialOverride);
		++_destInstance;
	}
}

void Mesh3DInstanceSetInlined::prepare_to_render()
{
	for_every(instance, instances)
	{
		instance->prepare_to_render();
	}
}

void Mesh3DInstanceSetInlined::render(Video3D* _v3d, Mesh3DRenderingContext const & _renderingContext) const
{
	for_every_const(instance, instances)
	{
		instance->render(_v3d, _renderingContext);
	}
}

void Mesh3DInstanceSetInlined::add_to(REF_ Mesh3DRenderingBuffer & _renderingBuffer, Matrix44 const & _placement, ModifyRenderingContextOnRender _modify_rendering_context_on_render, Optional<PlaneSet> const & _clipPlanes) const
{
	for_every_const(instance, instances)
	{
		_renderingBuffer.add(instance, _placement, _modify_rendering_context_on_render, _clipPlanes);
	}
}

void Mesh3DInstanceSetInlined::add_to(REF_ Mesh3DRenderingBufferSet & _renderingBufferSet, Matrix44 const & _placement, ModifyRenderingContextOnRender _modify_rendering_context_on_render, Optional<PlaneSet> const & _clipPlanes) const
{
	for_every_const(instance, instances)
	{
		_renderingBufferSet.get(instance->get_preferred_rendering_buffer()).add(instance, _placement, _modify_rendering_context_on_render, _clipPlanes);
	}
}
