#include "schematicElement.h"

#include "..\..\core\io\xml.h"

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(SchematicElement);

SchematicElement::SchematicElement()
{
}

SchematicElement::~SchematicElement()
{
}

bool SchematicElement::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	sortPriority = _node->get_int_attribute_or_from_child(TXT("sortPriority"), sortPriority);
	fullSchematicOnly = _node->get_bool_attribute_or_from_child_presence(TXT("fullSchematicOnly"), fullSchematicOnly);

	return result;
}
