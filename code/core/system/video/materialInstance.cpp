#include "materialInstance.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace System;

//

bool MaterialInstance::do_match(MaterialInstance const & _a, MaterialInstance const & _b)
{
	if (_a.material != _b.material ||
		_a.materialUsage != _b.materialUsage)
	{
		return false;
	}
	return ShaderProgramInstance::do_match(_a.shaderProgramInstance, _b.shaderProgramInstance);
}

MaterialInstance::MaterialInstance()
: material(nullptr)
{
}

void MaterialInstance::hard_copy_from_internal(MaterialInstance const & _instance, Optional<MaterialShaderUsage::Type> const& _forceMaterialUsage, bool _fillMissing)
{
	if (_forceMaterialUsage.is_set() &&
		materialUsage != _forceMaterialUsage.get())
	{
		// change material and set with uniforms from source material instance
		// we have to do it this way as uniforms may be setup differently with different material usage (they shouldn't be but who knows)
		set_material(_instance.material, _forceMaterialUsage.get());
		shaderProgramInstance.set_uniforms_from(_instance.shaderProgramInstance);
	}
	else
	{
		material = _instance.material;
		materialUsage = _instance.materialUsage;
		shaderProgramInstance.hard_copy_from(_instance.shaderProgramInstance);
	}

	renderingOrderPriorityOffset = _instance.renderingOrderPriorityOffset;

	if (_fillMissing)
	{
		fill_missing();
	}
}

void MaterialInstance::hard_copy_from(MaterialInstance const & _instance, Optional<MaterialShaderUsage::Type> const& _forceMaterialUsage)
{
	hard_copy_from_internal(_instance, _forceMaterialUsage, false);
}

void MaterialInstance::hard_copy_from_with_filled_missing(MaterialInstance const & _instance, Optional<MaterialShaderUsage::Type> const& _forceMaterialUsage)
{
	hard_copy_from_internal(_instance, _forceMaterialUsage, true);
}

void MaterialInstance::fill_missing()
{
	// fill missing params with material ladder
	Material const * mat = material;
	while (mat)
	{
		if (!shaderProgramInstance.hard_copy_fill_missing_with(mat->get_shader_program_instance(materialUsage)))
		{
			// all set
			return;
		}
		mat = mat->get_parent();
	}
	// fill with default values to not let them be modified by someone else and deal with someone else's values
	if (auto * shaderProgram = shaderProgramInstance.get_shader_program())
	{
		shaderProgramInstance.hard_copy_fill_missing_with(shaderProgram->get_default_values());
	}
}

void MaterialInstance::set_material(Material* _material, MaterialShaderUsage::Type _materialUsage)
{
	material = _material;
	materialUsage = _materialUsage;
	if (material)
	{
		ShaderProgram* shaderProgram = material->get_shader_program(_materialUsage);
		an_assert(shaderProgram, TXT("shader program is missing, maybe not defined for given usage?"));
		shaderProgramInstance.set_shader_program(shaderProgram);
	}
	else
	{
		shaderProgramInstance.set_shader_program(nullptr);
	}
}

void MaterialInstance::clear(MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.clear();
	}
}

void MaterialInstance::clear_uniform(Name const & _name, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.clear_uniform(_name);
	}
}

void MaterialInstance::set_uniform(Name const & _name, ShaderParam const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, TextureID _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, int32 _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, float _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, Array<float> const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, Array<Vector3> const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, Array<Vector4> const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, Array<Colour> const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, ArrayStack<float> const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, ArrayStack<Vector3> const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, ArrayStack<Vector4> const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, ArrayStack<Colour> const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, Vector2 const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, Vector3 const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, Vector4 const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, Matrix33 const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, Matrix44 const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, Array<Matrix44> const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, ArrayStack<Matrix44> const & _v, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v);
	}
}

void MaterialInstance::set_uniform(Name const & _name, Matrix44 const * _v, int32 _count, MaterialSetting::Type _settingMaterial)
{
	if (material && (material->are_individual_instances_allowed() || _settingMaterial == MaterialSetting::NotIndividualInstance || _settingMaterial == MaterialSetting::Forced))
	{
		shaderProgramInstance.set_uniform(_name, _v, _count);
	}
}

void MaterialInstance::log(LogInfoContext & _log) const
{
	shaderProgramInstance.log(_log);
}
