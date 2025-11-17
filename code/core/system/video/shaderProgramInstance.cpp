#include "shaderProgramInstance.h"
#include "shaderProgram.h"
#include "..\..\io\xml.h"

#include "..\..\other\simpleVariableStorage.h"

#include "..\..\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace System;

//

String ShaderParam::value_to_string() const
{
	if (type == ShaderParamType::texture) return String::printf(TXT("texture %i"), textureID);
	if (type == ShaderParamType::uniform1i) return String::printf(TXT("%i"), valueI[0]);
	if (type == ShaderParamType::uniform1f) return String::printf(TXT("%.3f"), valueF[0]);
	if (type == ShaderParamType::uniform2f) return String::printf(TXT("%.3f %.3f"), valueF[0], valueF[1]);
	if (type == ShaderParamType::uniform3f) return String::printf(TXT("%.3f %.3f %.3f"), valueF[0], valueF[1], valueF[2]);
	if (type == ShaderParamType::uniform4i) return String::printf(TXT("%i %i %i %i"), valueI[0], valueI[1], valueI[2], valueI[3]);
	if (type == ShaderParamType::uniform4f) return String::printf(TXT("%.3f %.3f %.3f %.3f"), valueF[0], valueF[1], valueF[2], valueF[3]);
	return String::printf(TXT("not implemented"));
}

//

ShaderProgramInstance::ShaderProgramInstance()
: shaderProgram(nullptr)
{
	SET_EXTRA_DEBUG_INFO(uniforms, TXT("ShaderProgramInstance.uniforms"));
}

ShaderProgramInstance::~ShaderProgramInstance()
{
#ifdef AN_DEVELOPMENT
	for_every(u, uniforms)
	{
		u->type = ShaderParamType::notSet;
	}
	memory_zero(uniforms.get_data(), uniforms.get_data_size());
#endif
}

void ShaderProgramInstance::bind(ShaderProgramBindingContext const * _bindingContext, SetupShaderProgram _setup_shader_program) const
{
	if (shaderProgram)
	{
		shaderProgram->bind(false); // set build in uniforms, but not custom ones
		{
			auto* v3d = Video3D::get();
			Vector2 outputSize = v3d->get_viewport_size().to_vector2();
			shaderProgram->set_build_in_uniform_in_texture_size_related_uniforms(outputSize, outputSize, v3d->get_fov());
		}
		if (_bindingContext)
		{
			shaderProgram->apply(*this, *_bindingContext);
			shaderProgram->use_default_textures_for_missing(*_bindingContext); // set missing textures to what is provided by binding context
		}
		else
		{
			shaderProgram->apply(*this); // set uniforms provided by this shader program instance
		}
		shaderProgram->set_custom_build_in_uniforms(); // post other parameters as some custom build in parameters may rely on actual values here
		if (_setup_shader_program)
		{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
			MEASURE_PERFORMANCE(_setup_shader_program);
#endif
			_setup_shader_program(shaderProgram);
		}
	}
}

void ShaderProgramInstance::unbind() const
{
	if (shaderProgram)
	{
		shaderProgram->unbind();
	}
}

void ShaderProgramInstance::set_shader_program(ShaderProgram * _shaderProgram)
{
	if (shaderProgram == _shaderProgram)
	{
		return; // no need to change
	}
	shaderProgram = _shaderProgram;
	uniforms.set_size(shaderProgram ? shaderProgram->uniforms.get_size() : 0);
	clear();
	if (shaderProgram)
	{
		an_assert(this != &_shaderProgram->defaultValues);
		if (this != &_shaderProgram->currentValues) // this does not make sense
		{
			// copy default values from what is set in shader program
			// ... not all? just textures? why?
			ShaderParam const * sourceUniform = _shaderProgram->currentValues.uniforms.get_data();
			for_every(uniform, uniforms)
			{
				if (!uniform->is_set() && sourceUniform->type == ShaderParamType::texture && sourceUniform->is_set())
				{
					uniform->store(*sourceUniform);
				}
				++sourceUniform;
			}
		}
	}
}

void ShaderProgramInstance::set_shader_program_being_part_of_shader_program(ShaderProgram * _shaderProgram)
{
	if (this == &_shaderProgram->defaultValues ||
		this == &_shaderProgram->currentValues)
	{
		shaderProgram = _shaderProgram;
		if (this == &_shaderProgram->currentValues)
		{
			uniforms.set_size(shaderProgram ? shaderProgram->uniforms.get_size() : 0);
			clear();
		}
		// do not copy current values from default values as we may mess up with textures
		/*
		uniforms = _shaderProgram->defaultValues.uniforms;
		// copy except for textures
		ShaderParam const * sourceUniform = _shaderProgram->defaultValues.uniforms.get_data();
		for_every(uniform, uniforms)
		{
			if (sourceUniform->type != ShaderParamType::texture)
			{
				*uniform = *sourceUniform;
			}
			++sourceUniform;
		}
		*/
	}
	else
	{
		an_assert(TXT("this should be called only for defaultValues and currentValues"));
	}
}

void ShaderProgramInstance::set_uniforms_from(ShaderProgramInstance const & _source)
{
	int uniformIndex = 0;
	for_every(sourceUniformValue, _source.uniforms)
	{
		set_uniform(_source.shaderProgram->get_uniform_name(uniformIndex), *sourceUniformValue);
		++uniformIndex;
	}
}

void ShaderProgramInstance::set_uniforms_from(SimpleVariableStorage const & _source)
{
	for_every(simpleVariableInfo, _source.get_all())
	{
		int uniformIndex = find_uniform_index(simpleVariableInfo->get_name());
		if (uniformIndex != NONE)
		{
			if (simpleVariableInfo->type_id() == type_id<float>())
			{
				set_uniform(uniformIndex, simpleVariableInfo->access<float>());
			}
			else if (simpleVariableInfo->type_id() == type_id<int>())
			{
				set_uniform(uniformIndex, simpleVariableInfo->access<int>());
			}
			else if (simpleVariableInfo->type_id() == type_id<Vector2>())
			{
				set_uniform(uniformIndex, simpleVariableInfo->access<Vector2>());
			}
			else if (simpleVariableInfo->type_id() == type_id<Vector3>())
			{
				set_uniform(uniformIndex, simpleVariableInfo->access<Vector3>());
			}
			else if (simpleVariableInfo->type_id() == type_id<Vector4>())
			{
				set_uniform(uniformIndex, simpleVariableInfo->access<Vector4>());
			}
			else if (simpleVariableInfo->type_id() == type_id<Colour>())
			{
				set_uniform(uniformIndex, simpleVariableInfo->access<Colour>().to_vector4());
			}
			else if (simpleVariableInfo->type_id() == type_id<Matrix33>())
			{
				set_uniform(uniformIndex, simpleVariableInfo->access<Matrix33>());
			}
			else if (simpleVariableInfo->type_id() == type_id<Matrix44>())
			{
				set_uniform(uniformIndex, simpleVariableInfo->access<Matrix44>());
			}
			else if (simpleVariableInfo->type_id() == type_id<Transform>())
			{
				set_uniform(uniformIndex, simpleVariableInfo->access<Transform>().to_matrix());
			}
			else
			{
				todo_important(TXT("implement case for ShaderProgramInstance::set_uniforms_from"));
			}
		}
	}
}

void ShaderProgramInstance::hard_copy_from(ShaderProgramInstance const & _source)
{
	shaderProgram = _source.shaderProgram;
	uniforms.set_size(_source.uniforms.get_size());
	ShaderParam const * sourceUniform = _source.uniforms.get_data();
	for_every(uniform, uniforms)
	{
		uniform->store(*sourceUniform);
		++sourceUniform;
	}
}

bool ShaderProgramInstance::hard_copy_fill_missing_with(ShaderProgramInstance const & _source)
{
	bool someMissing = false;
	an_assert(shaderProgram == _source.shaderProgram);
	ShaderParam const * sourceUniform = _source.uniforms.get_data();
	for_every(uniform, uniforms)
	{
		if (!uniform->is_set())
		{
			if (sourceUniform->is_set())
			{
				uniform->store(*sourceUniform);
			}
			else
			{
				someMissing = true;
			}
		}
		++sourceUniform;
	}
	return someMissing;
}

void ShaderProgramInstance::clear()
{
	for_every(uniform, uniforms)
	{
		uniform->clear();
	}
}

void ShaderProgramInstance::clear_uniform(int _idx) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].clear();
	}
}

void ShaderProgramInstance::set_uniform(int _idx, ShaderParam const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].store(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, TextureID _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, int32 _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, float _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, Array<float> const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, Array<Vector3> const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, Array<Vector4> const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, Array<Colour> const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, ArrayStack<float> const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, ArrayStack<Vector3> const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, ArrayStack<Vector4> const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, ArrayStack<Colour> const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, Vector2 const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, Vector3 const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, Vector4 const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform_ptr(int _idx, Vector4 const* _v, int32 _count, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform_ptr(_v, _count, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, Vector4 const* _v, int32 _count, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _count, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, VectorInt4 const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, Matrix33 const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, Matrix44 const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, Array<Matrix44> const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, ArrayStack<Matrix44> const & _v, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _forced);
	}
}

void ShaderProgramInstance::set_uniform_ptr(int _idx, Matrix44 const * _v, int32 _count, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform_ptr(_v, _count, _forced);
	}
}

void ShaderProgramInstance::set_uniform(int _idx, Matrix44 const * _v, int32 _count, bool _forced) const
{
	if (_idx >= 0 && _idx < uniforms.get_size())
	{
		uniforms[_idx].set_uniform(_v, _count, _forced);
	}
}

int ShaderProgramInstance::find_uniform_index(Name const & _name) const
{
	return shaderProgram? shaderProgram->find_uniform_index(_name) : NONE;
}

bool ShaderProgramInstance::do_match(ShaderProgramInstance const & _a, ShaderProgramInstance const & _b)
{
	if (_a.shaderProgram != _b.shaderProgram)
	{
		return false;
	}
	auto bUniformValue = _b.uniforms.begin_const();
	for_every_const(aUniformValue, _a.uniforms)
	{
		if (*aUniformValue != *bUniformValue)
		{
			return false;
		}
		++bUniformValue;
	}
	return true;
}

void ShaderProgramInstance::log(LogInfoContext & _log) const
{
	for_every(uniform, uniforms)
	{
		if (uniform->is_set())
		{
			_log.log(TXT("%S : %S"), shaderProgram->get_uniform_name(for_everys_index(uniform)).to_char(), uniform->value_to_string().to_char());
		}
	}
}
