#pragma once

#include "video.h"

namespace System
{
	struct MaterialInstance;

	interface_class IMaterialInstanceProvider
	{
	public:
		// index is part index (in all cases, at least at the moment of writing)
		virtual MaterialInstance * get_material_instance_for_rendering(int _idx) = 0;
		virtual MaterialInstance const * get_material_instance_for_rendering(int _idx) const = 0;
		virtual ShaderProgramInstance const * get_fallback_shader_program_instance() const { return nullptr; }
	};

	class FallbackShaderProgramInstanceForMaterialInstanceProvider
	: public IMaterialInstanceProvider
	{
	public:
		FallbackShaderProgramInstanceForMaterialInstanceProvider(ShaderProgramInstance const * _spi) : shaderProgramInstance(_spi) {}

	public: // IMaterialInstanceProvider
		implement_ MaterialInstance * get_material_instance_for_rendering(int _idx) { return nullptr; }
		implement_ MaterialInstance const * get_material_instance_for_rendering(int _idx) const { return nullptr; }
		implement_ ShaderProgramInstance const * get_fallback_shader_program_instance() const { return shaderProgramInstance; }

	private:
		ShaderProgramInstance const * shaderProgramInstance;
	};

};
