#include "meshGenMeshProcOp_delete.h"

#include "..\meshGenGenerationContext.h"

#include "..\modifiers\meshGenModifierUtils.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace MeshProcessorOperators;

//

REGISTER_FOR_FAST_CAST(Delete);

bool Delete::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool Delete::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool Delete::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
		part->clear();
		part->be_empty_part(true);
	}

	return result;
}
