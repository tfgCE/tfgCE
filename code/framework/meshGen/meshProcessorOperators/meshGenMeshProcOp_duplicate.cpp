#include "meshGenMeshProcOp_duplicate.h"

#include "..\meshGenGenerationContext.h"

#include "..\modifiers\meshGenModifierUtils.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace MeshProcessorOperators;

//

REGISTER_FOR_FAST_CAST(Duplicate);

bool Duplicate::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= onDuplicated.load_from_xml_child_node(_node, _lc, TXT("onDuplicate"));
	result &= onDuplicated.load_from_xml_child_node(_node, _lc, TXT("onDuplicated"));
	result &= onOriginal.load_from_xml_child_node(_node, _lc, TXT("onOriginal"));

	return result;
}

bool Duplicate::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= onDuplicated.prepare_for_game(_library, _pfgContext);
	result &= onOriginal.prepare_for_game(_library, _pfgContext);

	return result;
}

bool Duplicate::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	Array<int> newlyCreatedMPIndices;

	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
		RefCountObjectPtr<::Meshes::Mesh3DPart> copy(part->create_copy());
		an_assert(copy->get_vertex_format().is_ok_to_be_used());
		int newIdx = _context.store_part_just_as(copy.get(), part);
		newlyCreatedMPIndices.push_back(newIdx);
	}

	_newMeshPartIndices.add_from(newlyCreatedMPIndices);

	result &= onOriginal.process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);
	result &= onDuplicated.process(_context, _instance, newlyCreatedMPIndices, _newMeshPartIndices);

	return result;
}
