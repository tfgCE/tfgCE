#include "material.h"

#include "..\..\mainSettings.h"
#include "..\..\other\parserUtils.h"
#include "..\..\tags\tagCondition.h"
#include "..\..\system\core.h"
#include "..\..\utils\utils_loadingForSystemShader.h"

//

using namespace ::System;

//

Material::Material()
{
}

Material::~Material()
{
}

void Material::set_shader(MaterialShaderUsage::Type _usage, ShaderProgram* _shaderProgram)
{
	an_assert(parent == nullptr);
	if (parent)
	{
		return;
	}
	an_assert(_shaderProgram);
	set_shader_internal(_usage, _shaderProgram);
}

ShaderProgram * Material::get_shader_program(MaterialShaderUsage::Type _usage)
{
	return shaderPrograms[_usage].get();
}

ShaderProgramInstance & Material::access_shader_program_instance(MaterialShaderUsage::Type _usage)
{
	return shaderProgramInstances[_usage];
}

ShaderProgramInstance const & Material::get_shader_program_instance(MaterialShaderUsage::Type _usage) const
{
	return shaderProgramInstances[_usage];
}

void Material::set_shader_internal(MaterialShaderUsage::Type _usage, ShaderProgram* _shaderProgram)
{
	shaderPrograms[_usage] = _shaderProgram;
	shaderProgramInstances[_usage].set_shader_program(_shaderProgram);
}

void Material::set_parent(Material* _material)
{
	// clear all shader programs
	RefCountObjectPtr<ShaderProgram>* pShaderProgram = shaderPrograms;
	ShaderProgramInstance* shaderProgramInstance = shaderProgramInstances;
	for (int i = 0; i < MaterialShaderUsage::Num; ++i, ++pShaderProgram, ++shaderProgramInstance)
	{
		*pShaderProgram = nullptr;
		shaderProgramInstance->set_shader_program(nullptr);
	}

	parent = _material;

	// copy from parent
	if (parent)
	{
		RefCountObjectPtr<ShaderProgram>* pSourceShaderProgram = parent->shaderPrograms;
		for (int i = 0; i < MaterialShaderUsage::Num; ++i, ++pSourceShaderProgram)
		{
			set_shader_internal((MaterialShaderUsage::Type)i, pSourceShaderProgram->get());
		}
	}
}

void Material::clear()
{
	RefCountObjectPtr<ShaderProgram>* pShaderProgram = shaderPrograms;
	ShaderProgramInstance* shaderProgramInstance = shaderProgramInstances;
	for (int i = 0; i < MaterialShaderUsage::Num; ++i, ++pShaderProgram, ++shaderProgramInstance)
	{
		if (pShaderProgram->get())
		{
			shaderProgramInstance->clear();
		}
	}
}

void Material::clear_uniform(Name const & _name)
{
	RefCountObjectPtr<ShaderProgram>* pShaderProgram = shaderPrograms;
	ShaderProgramInstance* shaderProgramInstance = shaderProgramInstances;
	for (int i = 0; i < MaterialShaderUsage::Num; ++i, ++pShaderProgram, ++shaderProgramInstance)
	{
		if (pShaderProgram->get())
		{
			shaderProgramInstance->clear_uniform(_name);
		}
	}
}

void Material::set_uniform(Name const & _name, ShaderParam const & _v, bool _force)
{
	RefCountObjectPtr<ShaderProgram>* pShaderProgram = shaderPrograms;
	ShaderProgramInstance* shaderProgramInstance = shaderProgramInstances;
	for (int i = 0; i < MaterialShaderUsage::Num; ++i, ++pShaderProgram, ++shaderProgramInstance)
	{
		if (pShaderProgram->get())
		{
			shaderProgramInstance->set_uniform(_name, _v, _force);
		}
	}
}

void Material::set_uniform(Name const & _name, TextureID _v, bool _force)
{
	RefCountObjectPtr<ShaderProgram>* pShaderProgram = shaderPrograms;
	ShaderProgramInstance* shaderProgramInstance = shaderProgramInstances;
	for (int i = 0; i < MaterialShaderUsage::Num; ++i, ++pShaderProgram, ++shaderProgramInstance)
	{
		if (pShaderProgram->get())
		{
			shaderProgramInstance->set_uniform(_name, _v, _force);
		}
	}
}

void Material::set_uniform(Name const & _name, int32 _v, bool _force)
{
	RefCountObjectPtr<ShaderProgram>* pShaderProgram = shaderPrograms;
	ShaderProgramInstance* shaderProgramInstance = shaderProgramInstances;
	for (int i = 0; i < MaterialShaderUsage::Num; ++i, ++pShaderProgram, ++shaderProgramInstance)
	{
		if (pShaderProgram->get())
		{
			shaderProgramInstance->set_uniform(_name, _v, _force);
		}
	}
}

void Material::set_uniform(Name const & _name, float _v, bool _force)
{
	RefCountObjectPtr<ShaderProgram>* pShaderProgram = shaderPrograms;
	ShaderProgramInstance* shaderProgramInstance = shaderProgramInstances;
	for (int i = 0; i < MaterialShaderUsage::Num; ++i, ++pShaderProgram, ++shaderProgramInstance)
	{
		if (pShaderProgram->get())
		{
			shaderProgramInstance->set_uniform(_name, _v, _force);
		}
	}
}

void Material::set_uniform(Name const & _name, Vector2 const & _v, bool _force)
{
	RefCountObjectPtr<ShaderProgram>* pShaderProgram = shaderPrograms;
	ShaderProgramInstance* shaderProgramInstance = shaderProgramInstances;
	for (int i = 0; i < MaterialShaderUsage::Num; ++i, ++pShaderProgram, ++shaderProgramInstance)
	{
		if (pShaderProgram->get())
		{
			shaderProgramInstance->set_uniform(_name, _v, _force);
		}
	}
}

void Material::set_uniform(Name const & _name, Vector4 const & _v, bool _force)
{
	RefCountObjectPtr<ShaderProgram>* pShaderProgram = shaderPrograms;
	ShaderProgramInstance* shaderProgramInstance = shaderProgramInstances;
	for (int i = 0; i < MaterialShaderUsage::Num; ++i, ++pShaderProgram, ++shaderProgramInstance)
	{
		if (pShaderProgram->get())
		{
			shaderProgramInstance->set_uniform(_name, _v, _force);
		}
	}
}

void Material::set_uniform(Name const & _name, Matrix44 const & _v, bool _force)
{
	RefCountObjectPtr<ShaderProgram>* pShaderProgram = shaderPrograms;
	ShaderProgramInstance* shaderProgramInstance = shaderProgramInstances;
	for (int i = 0; i < MaterialShaderUsage::Num; ++i, ++pShaderProgram, ++shaderProgramInstance)
	{
		if (pShaderProgram->get())
		{
			shaderProgramInstance->set_uniform(_name, _v, _force);
		}
	}
}

void Material::set_uniform(Name const & _name, Array<Matrix44> const & _v, bool _force)
{
	RefCountObjectPtr<ShaderProgram>* pShaderProgram = shaderPrograms;
	ShaderProgramInstance* shaderProgramInstance = shaderProgramInstances;
	for (int i = 0; i < MaterialShaderUsage::Num; ++i, ++pShaderProgram, ++shaderProgramInstance)
	{
		if (pShaderProgram->get())
		{
			shaderProgramInstance->set_uniform(_name, _v, _force);
		}
	}
}

void Material::set_uniform(Name const & _name, Matrix44 const * _v, int32 _count, bool _force)
{
	RefCountObjectPtr<ShaderProgram>* pShaderProgram = shaderPrograms;
	ShaderProgramInstance* shaderProgramInstance = shaderProgramInstances;
	for (int i = 0; i < MaterialShaderUsage::Num; ++i, ++pShaderProgram, ++shaderProgramInstance)
	{
		if (pShaderProgram->get())
		{
			shaderProgramInstance->set_uniform(_name, _v, _count, _force);
		}
	}
}

bool Material::load_from_xml(IO::XML::Node const* _node)
{
	return internal_load_from_xml(_node, false);
}

bool Material::internal_load_from_xml(IO::XML::Node const * _node, bool _nested)
{
	bool result = true;

	allow_individual_instances(_node->get_bool_attribute_or_from_child_presence(TXT("allowIndividualInstances"), are_individual_instances_allowed()));
	allow_individual_instances(! _node->get_bool_attribute_or_from_child_presence(TXT("noIndividualInstances"), ! are_individual_instances_allowed()));

	if (! _nested || _node->has_attribute_or_child(TXT("faceDisplay")))
	{
		System::FaceDisplay::Type fd = _nested? faceDisplay.get(System::FaceDisplay::Front) : System::FaceDisplay::Front;
		if (System::FaceDisplay::parse(_node->get_string_attribute_or_from_child(TXT("faceDisplay")), fd))
		{
			faceDisplay = fd;
		}
		else
		{
			error_loading_xml(_node, TXT("problem loading face display"));
			result = false;
		}
	}

	if (!_nested || _node->has_attribute_or_child(TXT("srcBlendOp")))
	{
		System::BlendOp::Type blendOp = _nested? srcBlendOp.get(System::BlendOp::None) : System::BlendOp::None;
		if (System::BlendOp::parse(_node->get_string_attribute_or_from_child(TXT("srcBlendOp")), blendOp))
		{
			srcBlendOp = blendOp;
		}
		else
		{
			error_loading_xml(_node, TXT("problem loading src blend op"));
			result = false;
		}
	}

	if (!_nested || _node->has_attribute_or_child(TXT("destBlendOp")))
	{
		System::BlendOp::Type blendOp = _nested ? destBlendOp.get(System::BlendOp::None) : System::BlendOp::None;
		if (System::BlendOp::parse(_node->get_string_attribute_or_from_child(TXT("destBlendOp")), blendOp))
		{
			destBlendOp = blendOp;
		}
		else
		{
			error_loading_xml(_node, TXT("problem loading dest blend op"));
			result = false;
		}
	}

	if (!_nested || _node->has_attribute_or_child(TXT("depthMask")))
	{
		String string = _node->get_string_attribute_or_from_child(TXT("depthMask"));
		bool value;
		if (ParserUtils::parse_bool_ref(string.to_char(), value))
		{
			depthMask = value;
		}
	}

	if (!_nested || _node->has_attribute_or_child(TXT("depthClamp")))
	{
		String string = _node->get_string_attribute_or_from_child(TXT("depthClamp"));
		bool value;
		if (ParserUtils::parse_bool_ref(string.to_char(), value))
		{
			depthClamp = value;
		}
	}

	if (!_nested || _node->has_attribute_or_child(TXT("depthFunc")))
	{
		String string = _node->get_string_attribute_or_from_child(TXT("depthFunc"));
		if (!string.is_empty())
		{
			Video3DCompareFunc::Type value;
			if (Video3DCompareFunc::parse(string, REF_ value))
			{
				depthFunc = value;
			}
			else
			{
				error_loading_xml(_node, TXT("invalid depth func"));
				result = false;
			}
		}
	}

	if (!_nested || _node->has_attribute_or_child(TXT("polygonOffset")))
	{
		String string = _node->get_string_attribute_or_from_child(TXT("polygonOffset"));
		float value;
		if (ParserUtils::parse_float_ref(string.to_char(), value))
		{
			polygonOffset = value;
		}
	}

	renderingOrderPriority.load_from_xml(_node, TXT("renderingPriority"));
	renderingOrderPriority.load_from_xml(_node, TXT("renderingOrderPriority"));

	if (!_nested)
	{
		for_every(forNode, _node->children_named(TXT("for")))
		{
			if (CoreUtils::Loading::should_load_for_system_or_shader_option(forNode))
			{
				// nested load, load anything!
				result &= internal_load_from_xml(forNode, true);
			}
		}
	}

	update_any_blending();

	return result;
}

void Material::update_any_blending()
{
	anyBlending = false;
	if (srcBlendOp.is_set() &&
		srcBlendOp.get() != System::BlendOp::None &&
		srcBlendOp.get() != System::BlendOp::One)
	{
		anyBlending = true;
	}
	else if (destBlendOp.is_set() &&
		destBlendOp.get() != System::BlendOp::None &&
		destBlendOp.get() != System::BlendOp::Zero)
	{
		anyBlending = true;
	}
}