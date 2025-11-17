#include "physicalMaterialMap.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

using namespace Framework;

REGISTER_FOR_FAST_CAST(PhysicalMaterialMap);
LIBRARY_STORED_DEFINE_TYPE(PhysicalMaterialMap, physicalMaterialMap);

PhysicalMaterialMap::PhysicalMaterialMap(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
}

PhysicalMaterial* PhysicalMaterialMap::get_for(float _u) const
{
	for_every(entry, map)
	{
		if (is_entry_for(entry, _u))
		{
			return entry->physicalMaterial.get();
		}
	}

	return defaultPhysicalMaterial.get();
}

bool PhysicalMaterialMap::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= defaultPhysicalMaterial.load_from_xml_child_node(_node, TXT("default"), TXT("use"), _lc);

	for_every(node, _node->children_named(TXT("for")))
	{
		float u = node->get_float_attribute(TXT("u"));
		Entry* found = nullptr;
		for_every(entry, map)
		{
			if (is_entry_for(entry, u))
			{
				found = entry;
				break;
			}
		}
		if (!found)
		{
			map.grow_size(1);
			found = &map.get_last();
			found->u = u;
		}

		result &= found->physicalMaterial.load_from_xml(node, TXT("use"), _lc);
	}

	return result;
}

bool PhysicalMaterialMap::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= defaultPhysicalMaterial.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	for_every(entry, map)
	{
		result &= entry->physicalMaterial.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	return result;
}

