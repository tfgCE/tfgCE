#include "meshGenMeshProcOp_cut.h"

#include "..\meshGenGenerationContext.h"

#include "..\modifiers\meshGenModifierUtils.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace MeshProcessorOperators;

//

REGISTER_FOR_FAST_CAST(Cut);

bool Cut::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	cutDir.load_from_xml_child_node(_node, TXT("dir"));
	cutPoint.load_from_xml_child_node(_node, TXT("point"));

	if (cutDir.is_value_set())
	{
		if (cutDir.get_value().is_zero())
		{
			cutDir.set(Vector3::xAxis);
		}
	}

	return result;
}

bool Cut::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	Plane cutPlane = Plane(cutDir.get(_context).normal(), cutPoint.get(_context));

	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
		Modifiers::Utils::cut_using_plane(_context, part, cutPlane);
	}

	return result;
}
