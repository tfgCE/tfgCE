#include "mesh3dRenderingBufferSet.h"

#include "mesh3dInstance.h"

using namespace Meshes;

Mesh3DRenderingBufferSet::Mesh3DRenderingBufferSet()
{
	SET_EXTRA_DEBUG_INFO(renderingBuffers, TXT("Mesh3DRenderingBufferSet.renderingBuffers"));
	add(Name::invalid());
}

Mesh3DRenderingBuffer & Mesh3DRenderingBufferSet::add(Name const & _name)
{
	for_every(rb, renderingBuffers)
	{
		if (rb->name == _name)
		{
			return rb->renderingBuffer;
		}
	}
	an_assert(renderingBuffers.get_size() < renderingBuffers.get_max_size(), TXT("just make static array bigger"));
	renderingBuffers.grow_size(1);
	renderingBuffers.get_last().name = _name;
	return renderingBuffers.get_last().renderingBuffer;
}

Mesh3DRenderingBuffer & Mesh3DRenderingBufferSet::get(Name const & _name)
{
	for_every(rb, renderingBuffers)
	{
		if (rb->name == _name)
		{
			return rb->renderingBuffer;
		}
	}
	warn(TXT("rendering buffer \"%S\" not available"), _name.to_char());
	an_assert(!renderingBuffers.is_empty());
	an_assert(! renderingBuffers.get_first().name.is_valid());
	return renderingBuffers.get_first().renderingBuffer;
}

void Mesh3DRenderingBufferSet::clear_each_buffer()
{
	for_every(rb, renderingBuffers)
	{
		rb->renderingBuffer.clear();
	}
}

void Mesh3DRenderingBufferSet::sort_each_buffer(Matrix44 const & _currentViewMatrix)
{
	for_every(rb, renderingBuffers)
	{
		rb->renderingBuffer.sort(_currentViewMatrix);
	}
}

void Mesh3DRenderingBufferSet::add(Mesh3DInstance const * _instance, Matrix44 const & _placement, ModifyRenderingContextOnRender _modify_rendering_context_on_render, Optional<PlaneSet> const & _clipPlanes)
{
	if (_instance)
	{
		get(_instance->get_preferred_rendering_buffer()).add(_instance, _placement, _modify_rendering_context_on_render, _clipPlanes);
	}
}

void Mesh3DRenderingBufferSet::add_placement_override(Mesh3DInstance const * _instance, Matrix44 const & _placement, ModifyRenderingContextOnRender _modify_rendering_context_on_render, Optional<PlaneSet> const & _clipPlanes)
{
	if (_instance)
	{
		get(_instance->get_preferred_rendering_buffer()).add_placement_override(_instance, _placement, _modify_rendering_context_on_render, _clipPlanes);
	}
}

void Mesh3DRenderingBufferSet::add(Mesh3DInstance const * _instance, bool _placementOverride, Matrix44 const & _placement, ModifyRenderingContextOnRender _modify_rendering_context_on_render, Optional<PlaneSet> const & _clipPlanes)
{
	if (_instance)
	{
		get(_instance->get_preferred_rendering_buffer()).add(_instance, _placementOverride, _placement, _modify_rendering_context_on_render, _clipPlanes);
	}
}


