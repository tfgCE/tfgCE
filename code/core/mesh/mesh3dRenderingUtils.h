#pragma once

#include "..\profilePerformanceSettings.h"

#include "..\system\video\video.h"
#include "..\system\video\video3d.h"
#include "..\system\video\vertexFormat.h"
#include "..\system\video\indexFormat.h"

#include "iMesh3d.h"
#include "primitiveType.h"
#include "usage.h"

#include "mesh3dPart.h"

// .inl
#include "mesh3dRenderingUtils.h"
#include "mesh3dRenderingContext.h"

namespace System
{
	struct MaterialInstance;
	struct ShaderProgramInstance;
};

namespace Meshes
{
	/**
	 *	render function
	 */
	class Mesh3DRenderingUtils
	{
	public:
		inline static void render_worker_begin(::System::Video3D * _v3d, ::System::Video3DStateStore & _stateStore, ::System::ShaderProgramInstance const * _shaderProgramInstance, System::Material const * _usingMaterial, Mesh3DRenderingContext const & _renderingContext, void const * _dataPointer, Primitive::Type _primitiveType, uint32 _numberOfVertices, ::System::VertexFormat const & _vertexFormat, ::System::BufferObjectID* _vbo = nullptr, ::System::BufferObjectID* _ebo = nullptr);
		inline static void render_worker_render_non_indexed(::System::Video3D * _v3d, Primitive::Type _primitiveType, uint32 _numberOfVertices);
		inline static void render_worker_render_indexed(::System::Video3D * _v3d, Primitive::Type _primitiveType, uint32 _numberOfElements, ::System::IndexFormat const & _indexFormat);
		inline static void render_worker_end(::System::Video3D * _v3d, ::System::Video3DStateStore const & _stateStore, ::System::ShaderProgramInstance const * _shaderProgramInstance, System::Material const * _usingMaterial, Mesh3DRenderingContext const & _renderingContext, void const * _dataPointer, Primitive::Type _primitiveType, uint32 _numberOfVertices, ::System::VertexFormat const & _vertexFormat, ::System::BufferObjectID* _vbo = nullptr, ::System::BufferObjectID* _ebo = nullptr);
	};

	#include "mesh3dRenderingUtils.inl"
};
