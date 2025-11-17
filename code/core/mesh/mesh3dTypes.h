#pragma once

#include "..\globalDefinitions.h"

namespace Meshes
{
	class Mesh3DInstance;
	struct Mesh3DRenderingContext;

	typedef std::function<void(Matrix44 const& _renderingBufferModelViewMatrixForRendering, Mesh3DRenderingContext& _renderingContext)> ModifyRenderingContextOnRender;
};
