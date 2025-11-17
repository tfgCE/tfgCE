#include "meshGenCreateCollisionInfo.h"

#include "meshGenUtils.h"

#include "meshGenGenerationContext.h"

#include "meshGenValueDefImpl.inl"

#include "..\library\library.h"

//

using namespace Framework;
using namespace MeshGeneration;

//

DEFINE_STATIC_NAME(autoU);
DEFINE_STATIC_NAME(materialIndex);

//

bool CreateCollisionInfo::load_from_xml(IO::XML::Node const * _node, tchar const * _childAttrName, LibraryLoadingContext & _lc)
{
	bool result = true;

	create = _node->get_bool_attribute(_childAttrName);

	for_every(node, _node->children_named(_childAttrName))
	{
		create = true;
		makeSizeValid = node->get_bool_attribute_or_from_child_presence(TXT("makeSizeValid"), makeSizeValid);
		result &= createCondition.load_from_xml(node, TXT("createCondition"));
		result &= physicalMaterial.load_from_xml(node, TXT("physicalMaterial"));
		result &= physicalMaterialMap.load_from_xml(node, TXT("physicalMaterialMap"));
		result &= _lc.load_group_into(physicalMaterial);
		result &= subStep.load_from_xml(node, TXT("subStep"));
		result &= sizeCoef.load_from_xml(node, TXT("sizeCoef"));
	}
	
	return result;
}

PhysicalMaterial * CreateCollisionInfo::get_provided_physical_material(GenerationContext const & _context) const
{
	if (physicalMaterial.is_set())
	{
		return Library::get_current()->get_physical_materials().find(physicalMaterial.get(_context));
	}
	else
	{
		return nullptr;
	}
}

PhysicalMaterial* CreateCollisionInfo::get_physical_material(GenerationContext const & _context, ElementInstance const & _instance, Optional<float> const & _uOverride) const
{
	return get_physical_material(_context, _instance.get_material_index_from_params(), _uOverride);
}

PhysicalMaterial* CreateCollisionInfo::get_physical_material(GenerationContext const & _context, int _materialIndex, Optional<float> const & _uOverride) const
{
	if (physicalMaterial.is_set())
	{
		LibraryName pmName = physicalMaterial.get(_context);
		if (pmName.is_valid())
		{
			if (auto* pm = Library::get_current()->get_physical_materials().find(pmName))
			{
				return pm;
			}
		}
	}

	// get from map if available, if not, from general setup
	{
		Optional<float> u = _uOverride;
		if (! u.is_set())
		{
			FOR_PARAM(_context, float, autoU)
			{
				u = *autoU;
			}
		}

		if (u.is_set())
		{
			if (physicalMaterialMap.is_set())
			{
				if (auto* pmm = Library::get_current()->get_physical_material_maps().find(physicalMaterialMap.get(_context)))
				{
					return pmm->get_for(u.get());
				}
			}
			else
			{
				if (_context.get_material_setups().is_index_valid(_materialIndex))
				{
					auto const & materialSetup = _context.get_material_setups()[_materialIndex];
					if (auto* material = materialSetup.get_material())
					{
						if (auto *pmm = material->get_physical_material_map())
						{
							return pmm->get_for(u.get());
						}
					}
				}
			}
		}
	}

	return nullptr;
}

PhysicalMaterialMap* CreateCollisionInfo::get_physical_material_map(GenerationContext const & _context, ElementInstance const & _instance) const
{
	return get_physical_material_map(_context, _instance.get_material_index_from_params());
}

PhysicalMaterialMap* CreateCollisionInfo::get_physical_material_map(GenerationContext const & _context, int _materialIndex) const
{
	if (physicalMaterialMap.is_set())
	{
		return Library::get_current()->get_physical_material_maps().find(physicalMaterialMap.get(_context));
	}
	else
	{
		if (_context.get_material_setups().is_index_valid(_materialIndex))
		{
			auto const & materialSetup = _context.get_material_setups()[_materialIndex];
			if (auto* material = materialSetup.get_material())
			{
				if (auto *pmm = material->get_physical_material_map())
				{
					return pmm;
				}
			}
		}
	}

	return nullptr;
}

Vector3 CreateCollisionInfo::get_size_coef(GenerationContext const& _context) const
{
	if (sizeCoef.is_set())
	{
		Vector3 actualSizeCoef = sizeCoef.get(_context);
		if (actualSizeCoef.x == 0.0f) actualSizeCoef.x = 1.0f;
		if (actualSizeCoef.y == 0.0f) actualSizeCoef.y = 1.0f;
		if (actualSizeCoef.z == 0.0f) actualSizeCoef.z = 1.0f;
		return actualSizeCoef;
	}
	else
	{
		return Vector3::one;
	}
}

