#include "meshGenCreateSpaceBlockerInfo.h"

#include "meshGenUtils.h"

#include "..\library\library.h"

using namespace Framework;
using namespace MeshGeneration;

//

bool CreateSpaceBlockerInfo::load_from_xml(IO::XML::Node const * _node, tchar const * _childAttrName)
{
	bool result = true;

	create = _node->get_bool_attribute(_childAttrName, create);
	create = _node->get_bool_attribute_or_from_child_presence(_childAttrName, create);

	for_every(node, _node->children_named(_childAttrName))
	{
		create = true;
		result &= createCondition.load_from_xml(node, TXT("createCondition"));
		result &= blocks.load_from_xml(node, TXT("blocks"));
		result &= requiresToBeEmpty.load_from_xml(node, TXT("requiresToBeEmpty"));
	}
	
	return result;
}

bool CreateSpaceBlockerInfo::get_blocks(GenerationContext const & _context) const
{
	if (blocks.is_set())
	{
		return blocks.get(_context);
	}
	else
	{
		return true;
	}
}

bool CreateSpaceBlockerInfo::get_requires_to_be_empty(GenerationContext const & _context) const
{
	if (requiresToBeEmpty.is_set())
	{
		return requiresToBeEmpty.get(_context);
	}
	else
	{
		return true;
	}
}
