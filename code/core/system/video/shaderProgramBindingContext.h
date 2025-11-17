#pragma once

#include "video.h"
#include "shaderProgramInstance.h"

namespace System
{
	class ShaderProgram;

	/**
	 *	Contains defaults to be set when something is missing in shader program (eg. texture)
	 *	and parameters that should be always set (and should override_ anything else)
	 */
	class ShaderProgramBindingContext
	{
	public:
		static const int MAX_SHADER_PARAMS = 80;
	public:
		ShaderProgramBindingContext();

		void reset();

		TextureID const & get_default_texture_id() const { return defaultTexture; }
		void set_default_texture_id(TextureID const & _textureID) { defaultTexture = _textureID; }

		NamedShaderParam* access_existing_shader_param(Name const & _name);
		NamedShaderParam& access_shader_param(Name const & _name);
		void remove_shader_param(Name const & _name);

		void fill_with(ShaderProgramBindingContext const & _parent);

	private:
		TextureID defaultTexture;

		ArrayStatic<NamedShaderParam, MAX_SHADER_PARAMS> shaderParams;

		friend class ShaderProgram;
	};

};

 