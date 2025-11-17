#include "mesh3dRenderingBuffer.h"

#include "mesh3dInstance.h"

#include "..\performance\performanceUtils.h"

#include "..\system\video\viewFrustum.h"
#include "..\system\video\videoClipPlaneStackImpl.inl"

#ifdef AN_DEVELOPMENT
#include "..\system\input.h"
#include "..\debugKeys.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define DISABLE_CLIP_PLANES

//

using namespace Meshes;

//

int Mesh3DRenderingBuffer::Element::compare(void const * _a, void const * _b)
{
	Element const * a = *(Element const * const *)(_a);
	Element const * b = *(Element const * const *)(_b);
	if (a->blending ^ b->blending)
	{
		return a->blending ? B_BEFORE_A : A_BEFORE_B; // blending should be rendered later
	}
	else if (a->blending)
	{
		int diff = a->renderingOrder.priority - b->renderingOrder.priority; // render with higher priority later (so they are visible)
		if (diff == 0)
		{
			diff = a->distance < b->distance ? B_BEFORE_A : A_BEFORE_B; // render further first
		}
		return diff;
	}
	else
	{
		pointerint diff = a->renderingOrder.priority - b->renderingOrder.priority; // render with higher priority later (background should therefore have a higher value for priority order)
		todo_note(TXT("profile this - check which one is a better approach - reducing number of calls or fragment shaders"));
		if (diff == 0)
		{
#ifdef AN_GLES
			diff = a->distanceY < b->distanceY ? A_BEFORE_B : B_BEFORE_A; // render closer first, we want to minimizee overwriting pixels
#else
			diff = a->distanceGroup - b->distanceGroup; // lower distance group, render sooner
#endif
		}
		if (diff == 0)
		{
			diff = (a->renderingOrder.meshPtr - b->renderingOrder.meshPtr); // just group by mesh to reduce number of calls
		}
		return (int)diff;
	}
}

//

Mesh3DRenderingBuffer::Mesh3DRenderingBuffer()
{
	elements.make_space_for(10);
	elementsOrdered.make_space_for(10);
}

void Mesh3DRenderingBuffer::clear()
{
	elements.clear();
	elementsOrdered.clear();
}

void Mesh3DRenderingBuffer::add(Mesh3DInstance const * _instance, Matrix44 const & _placement, ModifyRenderingContextOnRender _modify_rendering_context_on_render, Optional<PlaneSet> const & _clipPlanes)
{
	add(_instance, false, _placement, _modify_rendering_context_on_render, _clipPlanes);
}

void Mesh3DRenderingBuffer::add_placement_override(Mesh3DInstance const * _instance, Matrix44 const & _placement, ModifyRenderingContextOnRender _modify_rendering_context_on_render, Optional<PlaneSet> const & _clipPlanes)
{
	add(_instance, true, _placement, _modify_rendering_context_on_render, _clipPlanes);
}

void Mesh3DRenderingBuffer::add(Mesh3DInstance const * _instance, bool _placementOverride, Matrix44 const & _placement, ModifyRenderingContextOnRender _modify_rendering_context_on_render, Optional<PlaneSet> const & _clipPlanes)
{
	auto const * mesh = _instance->get_mesh();
	if (!mesh)
	{
		return;
	}
	an_assert(elementsOrdered.is_empty(), TXT("do not modify after sorting!"));
#ifdef AN_N_RENDERING
	if (::System::Input::get()->has_key_been_pressed(System::Key::N))
	{
		LogInfoContext log;
		log.log(TXT("add instance to rendering buffer"));
		{
			LOG_INDENT(log);
			_instance->log(log);
		}
		log.output_to_output();
	}
#endif
	bool nonBlending = false;
	bool blending = false;
	Optional<int> renderingPriority;
	if (_instance->does_any_blending())
	{
		blending = true;
	}
	else
	{
		for_count(int, materialInstanceIndex, _instance->get_material_instance_count())
		{
			if (auto const * materialInstance = _instance->get_material_instance_for_rendering_buffer_setup(materialInstanceIndex))
			{
				if (auto const * material = materialInstance->get_material())
				{
					if (material->get_rendering_order_priority().is_set())
					{
						if (renderingPriority.is_set())
						{
							// prefer lowest
							renderingPriority = min(renderingPriority.get(), material->get_rendering_order_priority().get());
						}
						else
						{
							renderingPriority = material->get_rendering_order_priority();
						}
					}
					if (materialInstance->get_rendering_order_priority_offset().is_set())
					{
						renderingPriority = renderingPriority.get(0) + materialInstance->get_rendering_order_priority_offset().get();
					}
					if (material->does_any_blending())
					{
						blending = true;
					}
					else
					{
						nonBlending = true;
					}
				}
			}
		}
	}
	int addCount = (blending && nonBlending) ? 2 : 1;
	elements.make_space_for_additional(addCount);
	for_count(int, add, addCount)
	{
		elements.set_size(elements.get_size() + 1);
		Element & newElement = elements.get_last();
		newElement.instance = _instance;
		{
			newElement.renderingOrder = mesh->get_rendering_order();
			if (renderingPriority.is_set())
			{
				newElement.renderingOrder.priority = renderingPriority.get();
			}
			_instance->override_rendering_order(REF_ newElement.renderingOrder);
		}
		newElement.placementOverride = _placementOverride;
		newElement.placement = _placement;
		newElement.clipPlanes = _clipPlanes;
		newElement.blending = blending ? true : false; // do we have blending left?
		newElement.modify_rendering_context_on_render = _modify_rendering_context_on_render;
		
		// if we added blending already, another one will be non blending
		blending = false;
	}
}

void Mesh3DRenderingBuffer::sort(Matrix44 const & _currentViewMatrix)
{
	elementsOrdered.clear();
	elementsOrdered.make_space_for(elements.get_size());
	for_every(element, elements)
	{
		// get into MVS
		Vector3 off = (element->placementOverride ? element->placement.get_translation() : _currentViewMatrix.location_to_world(element->placement.get_translation()));
		element->distanceY = off.y;
		element->distance = off.length();
		element->distanceGroup = (int)sqrt(max(0.0f, element->distanceY));
		elementsOrdered.push_back(element);
	}
	::sort(elementsOrdered);
}

void Mesh3DRenderingBuffer::render(::System::Video3D * _v3d, Mesh3DRenderingContext const & _renderingContext) const
{
	an_assert(elementsOrdered.get_size() == elements.get_size(), TXT("did you forgot to sort?"));

	System::VideoMatrixStack& modelViewStack = _v3d->access_model_view_matrix_stack();
	System::VideoClipPlaneStack& clipPlaneStack = _v3d->access_clip_plane_stack();

	// so the element will have rendering buffer context
	Matrix44 modelViewMatrixForRendering = modelViewStack.get_current_for_rendering();

	for_every_ptr(element, elementsOrdered)
	{
		// if we have precise demands regarding blending, skip not matching element
		if (_renderingContext.renderBlending.is_set() &&
			(_renderingContext.renderBlending.get() ^ element->blending))
		{
			continue;
		}

#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE_COLOURED(renderingBufferElement, Colour::zxCyan);
#endif

		// setup clip planes
#ifndef DISABLE_CLIP_PLANES
		if (element->clipPlanes.is_set())
		{
			clipPlaneStack.push();
			clipPlaneStack.set_current(element->clipPlanes.get());
		}
#endif
		if (element->placementOverride)
		{
			modelViewStack.push_set(element->placement);
		}
		else
		{
			modelViewStack.push_to_world(element->placement);
		}

		// render
		{
			Mesh3DRenderingContext renderingContext = _renderingContext;
			renderingContext.renderBlending = element->blending;
			if (element->modify_rendering_context_on_render)
			{
				element->modify_rendering_context_on_render(modelViewMatrixForRendering, renderingContext);
			}
			element->instance->render(_v3d, renderingContext);
		}

		modelViewStack.pop();

		// get clip planes back
#ifndef DISABLE_CLIP_PLANES
		if (element->clipPlanes.is_set())
		{
			clipPlaneStack.pop();
		}
#endif
	}
}
