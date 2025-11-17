#include "mesh3dInstanceSetInlined.h"

template<typename Class>
void Meshes::Mesh3DInstance::set_shader_param(Name const& _paramName, Class const& _value, int _materialIdx)
{
	if (_materialIdx != NONE)
	{
		if (auto* mat = get_material_instance(_materialIdx))
		{
			mat->set_uniform(_paramName, _value);
		}
	}
	else
	{
		for_count(int, i, get_material_instance_count())
		{
			if (auto* mat = get_material_instance(i))
			{
				mat->set_uniform(_paramName, _value);
			}
		}
	}
}
