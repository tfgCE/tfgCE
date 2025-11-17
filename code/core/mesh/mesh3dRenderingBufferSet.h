#pragma once

#include "..\containers\arrayStatic.h"

#include "mesh3dRenderingBuffer.h"

namespace Meshes
{
	/**
	 *	Couple of rendering buffers, there is always default one, unnamed
	 */
	struct Mesh3DRenderingBufferSet
	{
	public:
		Mesh3DRenderingBufferSet();

		Mesh3DRenderingBuffer & add(Name const & _name);
		Mesh3DRenderingBuffer & get(Name const & _name);

		void clear_each_buffer();
		void sort_each_buffer(Matrix44 const & _currentViewMatrix);

		void add(Mesh3DInstance const * _instance, Matrix44 const & _placement = Matrix44::identity, ModifyRenderingContextOnRender _modify_rendering_context_on_render = nullptr, Optional<PlaneSet> const & _clipPlanes = NP);
		void add_placement_override(Mesh3DInstance const * _instance, Matrix44 const & _placement = Matrix44::identity, ModifyRenderingContextOnRender _modify_rendering_context_on_render = nullptr, Optional<PlaneSet> const & _clipPlanes = NP);

		void add(Mesh3DInstance const * _instance, bool _placementOverride = false, Matrix44 const & _placement = Matrix44::identity, ModifyRenderingContextOnRender _modify_rendering_context_on_render = nullptr, Optional<PlaneSet> const & _clipPlanes = NP);

	private:
		struct NamedRenderingBuffer
		{
			Name name;
			Mesh3DRenderingBuffer renderingBuffer;
		};
		ArrayStatic<NamedRenderingBuffer, 4> renderingBuffers;
	};

};
