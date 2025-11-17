#include "meshGenBasicShapeData.h"

#include "..\meshGenBasicShapes.h"

#include "..\..\..\..\core\io\xml.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace BasicShapes;

//

ElementShapeData* BasicShapes::create_data()
{
	return new Data();
}

//

REGISTER_FOR_FAST_CAST(Data);

bool Data::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	createMovementCollision.load_from_xml(_node, TXT("createMovementCollision"), _lc);
	createMovementCollisionBox.load_from_xml(_node, TXT("createMovementCollisionBox"), _lc);
	createMovementCollisionMesh.load_from_xml(_node, TXT("createMovementCollisionMesh"), _lc);
	createPreciseCollision.load_from_xml(_node, TXT("createPreciseCollision"), _lc);
	createPreciseCollisionBox.load_from_xml(_node, TXT("createPreciseCollisionBox"), _lc);
	createPreciseCollisionMesh.load_from_xml(_node, TXT("createPreciseCollisionMesh"), _lc);
	error_loading_xml_on_assert(!_node->first_child_named(TXT("createPreciseCollisionMeshSkinned")), result, _node, TXT("createPreciseCollisionMeshSkinned not handled"));
	createSpaceBlocker.load_from_xml(_node, TXT("createSpaceBlocker"));
	noMesh = _node->get_bool_attribute_or_from_child_presence(TXT("noMesh"), noMesh);
	ignoreForCollision = _node->get_bool_attribute_or_from_child_presence(TXT("ignoreForCollision"), ignoreForCollision);

	return result;
}
