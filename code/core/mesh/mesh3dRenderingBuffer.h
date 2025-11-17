#pragma once

#include "..\globalDefinitions.h"
#include "..\math\math.h"
#include "..\system\video\shaderProgramBindingContext.h"

#include "iMesh3d.h"
#include "mesh3dTypes.h"

namespace System
{
	class Video3D;
	class VideoMatrixStack;
};

namespace Meshes
{
	class Mesh3DInstance;
	struct Mesh3DRenderingContext;

	typedef std::function<void(Matrix44 const & _renderingBufferModelViewMatrixForRendering, Mesh3DRenderingContext & _renderingContext)> ModifyRenderingContextOnRender;

	/**
	 *	This is used to group meshes together and order their rendering by priority
	 */
	struct Mesh3DRenderingBuffer
	{
	public:
		Mesh3DRenderingBuffer();

		bool is_empty() const { return elements.is_empty(); }

		void clear();
		void add(Mesh3DInstance const * _instance, Matrix44 const & _placement = Matrix44::identity, ModifyRenderingContextOnRender _modify_rendering_context_on_render = nullptr, Optional<PlaneSet> const & _clipPlanes = NP);
		void add_placement_override(Mesh3DInstance const * _instance, Matrix44 const & _placement = Matrix44::identity, ModifyRenderingContextOnRender _modify_rendering_context_on_render = nullptr, Optional<PlaneSet> const & _clipPlanes = NP);

		void add(Mesh3DInstance const * _instance, bool _placementOverride = false, Matrix44 const & _placement = Matrix44::identity, ModifyRenderingContextOnRender _modify_rendering_context_on_render = nullptr, Optional<PlaneSet> const & _clipPlanes = NP);

		void sort(Matrix44 const & _currentViewMatrix);

		void render(::System::Video3D * _v3d, Mesh3DRenderingContext const & _renderingContext) const;

	private:
		struct Element
		{
			Mesh3DInstance const * instance;
			Mesh3DRenderingOrder renderingOrder;
			bool blending;
			float distance; // is set during sort
			float distanceY; // just along Y axis
			int distanceGroup; // to group objects that should be rendered together
			Matrix44 placement;
			bool placementOverride;
			Optional<PlaneSet> clipPlanes; // per eye
			ModifyRenderingContextOnRender modify_rendering_context_on_render = nullptr;

			static int compare(void const * _a, void const * _b);
		};
		Array<Element> elements;
		Array<Element*> elementsOrdered;
	};

};
