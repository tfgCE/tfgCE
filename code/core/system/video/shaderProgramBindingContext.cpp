#include "shaderProgramBindingContext.h"

using namespace System;

ShaderProgramBindingContext::ShaderProgramBindingContext()
: defaultTexture(::System::texture_id_none())
{
	SET_EXTRA_DEBUG_INFO(shaderParams, TXT("ShaderProgramBindingContext.shaderParams"));
}

void ShaderProgramBindingContext::reset()
{
	defaultTexture = ::System::texture_id_none();
	shaderParams.clear();
}

NamedShaderParam* ShaderProgramBindingContext::access_existing_shader_param(Name const & _name)
{
	for_every(shaderParam, shaderParams)
	{
		if (shaderParam->name == _name)
		{
			return shaderParam;
		}
	}
	return nullptr;
}

NamedShaderParam& ShaderProgramBindingContext::access_shader_param(Name const & _name)
{
	NamedShaderParam* shaderParam = access_existing_shader_param(_name);
	if (!shaderParam)
	{
		NamedShaderParam newShaderParam;
		newShaderParam.name = _name;
		shaderParams.push_back(newShaderParam);
		shaderParam = &shaderParams.get_last();
	}
	return *shaderParam;
}

void ShaderProgramBindingContext::remove_shader_param(Name const & _name)
{
	int idx = 0;
	for_every(shaderParam, shaderParams)
	{
		if (shaderParam->name == _name)
		{
			shaderParams.remove_at(idx);
			break;
		}
		++idx;
	}
}

void ShaderProgramBindingContext::fill_with(ShaderProgramBindingContext const & _parent)
{
	if (defaultTexture == ::System::texture_id_none())
	{
		defaultTexture = _parent.defaultTexture;
	}
	for_every(parentParam, _parent.shaderParams)
	{
		if (!access_existing_shader_param(parentParam->name))
		{
			shaderParams.push_back(*parentParam);
		}
	}
}
