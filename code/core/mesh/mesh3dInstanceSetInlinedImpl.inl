#include "mesh3dInstanceSetInlined.h"

template<typename Class>
void Meshes::Mesh3DInstanceSetInlined::set_shader_param(Name const& _paramName, Class const& _value, int _materialIdx)
{
	if (_materialIdx != NONE)
	{
		for_every(meshInstance, instances)
		{
			if (auto* mat = meshInstance->get_material_instance(_materialIdx))
			{
				mat->set_uniform(_paramName, _value);
			}
		}
	}
	else
	{
		for_every(meshInstance, instances)
		{
			for_count(int, i, meshInstance->get_material_instance_count())
			{
				if (auto* mat = meshInstance->get_material_instance(i))
				{
					mat->set_uniform(_paramName, _value);
				}
			}
		}
	}
}
