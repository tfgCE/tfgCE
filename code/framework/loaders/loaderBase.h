#pragma once

#include "iLoader.h"

#include "..\..\core\system\video\shader.h"
#include "..\..\core\system\video\shaderProgram.h"
#include "..\..\core\system\video\renderTargetPtr.h"

namespace Loader
{
	class Base
	: public ILoader
	{
		typedef ILoader base;
	public:
		Base();
		~Base();

	protected:
		::System::RenderTargetPtr rt;

		RefCountObjectPtr<::System::Shader> fxaaVertexShader;
		RefCountObjectPtr<::System::Shader> fxaaFragmentShader;
		RefCountObjectPtr<::System::ShaderProgram> fxaaShaderProgram;

		Optional<Vector3> vrCentre;

		void construct_rt();
		void construct_fxaa_shaders();

		void render_rt(::System::Video3D* _v3d);

		void update_vr_centre(Vector3 const & _eyeLoc);
	};
};
