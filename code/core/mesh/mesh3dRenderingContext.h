#pragma once

#include "..\globalDefinitions.h"
#include "..\system\video\videoFunctions.h"
#include "..\types\optional.h"

namespace System
{
	class ShaderProgramBindingContext;
	class Video3D;
};

namespace Meshes
{
	struct Mesh3DRenderingContext;
	interface_class IMesh3DRenderBonesProvider;

	/**
	 *	This should be always just simple context and should contain only pointers to classes that do something.
	 */
	struct Mesh3DRenderingContext
	{
	public:
		IMesh3DRenderBonesProvider const * meshBonesProvider;
		::System::ShaderProgramBindingContext const * shaderProgramBindingContext;
		::System::SetupShaderProgram setup_shader_program_on_bound = nullptr;
		bool useExistingFaceDisplay = false;
		bool useExistingBlend = false;
		bool useExistingDepthMask = false;
		bool useExistingFallbackColour = false;
		Optional<bool> renderBlending; // if not set, renders all, as they are

		static Mesh3DRenderingContext const & none() { return s_none; }

		Mesh3DRenderingContext();

		Mesh3DRenderingContext& with_existing_face_display() { useExistingFaceDisplay = true; return *this; }
		Mesh3DRenderingContext& with_existing_blend() { useExistingBlend = true; return *this; }
		Mesh3DRenderingContext& with_existing_depth_mask() { useExistingDepthMask = true; return *this; }
		Mesh3DRenderingContext& with_existing_fallback_colour() { useExistingFallbackColour = true; return *this; }

	private:
		static Mesh3DRenderingContext s_none;
	};

};
