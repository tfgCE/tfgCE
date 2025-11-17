#include "moduleAppearance.h"

template<typename Class>
void Framework::ModuleAppearance::set_shader_param(Name const & _paramName, Class const & _value, int _materialIdx)
{
	if (_materialIdx != NONE)
	{
		if (auto * mat = meshInstance.get_material_instance(_materialIdx))
		{
			mat->set_uniform(_paramName, _value);
		}
		for_count(int, lod, lodMeshInstances.get_size())
		{
			auto& mi = lodMeshInstances[lod].meshInstance;
			if (auto* mat = mi.get_material_instance(_materialIdx))
			{
				mat->set_uniform(_paramName, _value);
			}
		}
	}
	else
	{
		for_count(int, i, meshInstance.get_material_instance_count())
		{
			if (auto * mat = meshInstance.get_material_instance(i))
			{
				mat->set_uniform(_paramName, _value);
			}
		}
		for_count(int, lod, lodMeshInstances.get_size())
		{
			auto& mi = lodMeshInstances[lod].meshInstance;
			for_count(int, i, mi.get_material_instance_count())
			{
				if (auto* mat = mi.get_material_instance(i))
				{
					mat->set_uniform(_paramName, _value);
				}
			}
		}
	}
}

template<typename Class>
void Framework::ModuleAppearance::set_shader_param(Name const& _paramName, Class const& _value, std::function<bool(::System::MaterialInstance const*)> _validateMaterialInstance)
{
	for_count(int, i, meshInstance.get_material_instance_count())
	{
		if (auto* mat = meshInstance.get_material_instance(i))
		{
			if (_validateMaterialInstance(mat))
			{
				mat->set_uniform(_paramName, _value);
			}
		}
	}
	for_count(int, lod, lodMeshInstances.get_size())
	{
		auto& mi = lodMeshInstances[lod].meshInstance;
		for_count(int, i, mi.get_material_instance_count())
		{
			if (auto* mat = mi.get_material_instance(i))
			{
				if (_validateMaterialInstance(mat))
				{
					mat->set_uniform(_paramName, _value);
				}
			}
		}
	}
}
