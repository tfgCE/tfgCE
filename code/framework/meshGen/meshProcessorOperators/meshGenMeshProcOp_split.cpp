#include "meshGenMeshProcOp_split.h"

#include "..\meshGenGenerationContext.h"

#include "..\modifiers\meshGenModifierUtils.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace MeshProcessorOperators;

//

REGISTER_FOR_FAST_CAST(Split);

bool Split::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	splitDir.load_from_xml_child_node(_node, TXT("dir"));
	splitPoint.load_from_xml_child_node(_node, TXT("point"));

	if (splitDir.is_value_set())
	{
		if (splitDir.get_value().is_zero())
		{
			splitDir.set(Vector3::xAxis);
		}
	}

	result &= onFront.load_from_xml_child_node(_node, _lc, TXT("onFront"));
	result &= onBack.load_from_xml_child_node(_node, _lc, TXT("onBack"));

	return result;
}

bool Split::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= onFront.prepare_for_game(_library, _pfgContext);
	result &= onBack.prepare_for_game(_library, _pfgContext);

	return result;
}

bool Split::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	Vector3 normal = splitDir.get(_context).normal();
	Vector3 loc = splitPoint.get(_context);
	Plane splitPlaneBack = Plane(-normal, loc);
	Plane splitPlaneFront = Plane(normal, loc);

	Array<int> newlyCreatedMPIndices;

	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
		RefCountObjectPtr<::Meshes::Mesh3DPart> copy(part->create_copy());
		an_assert(copy->get_vertex_format().is_ok_to_be_used());
		int newIdx = _context.store_part_just_as(copy.get(), part);
		newlyCreatedMPIndices.push_back(newIdx);

		{
			::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
			Modifiers::Utils::cut_using_plane(_context, part, splitPlaneBack);
		}
		{
			::Meshes::Mesh3DPart* part = _context.get_parts()[newIdx].get();
			Modifiers::Utils::cut_using_plane(_context, part, splitPlaneFront);
		}
	}

	_newMeshPartIndices.add_from(newlyCreatedMPIndices);

	result &= onBack.process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);
	result &= onFront.process(_context, _instance, newlyCreatedMPIndices, _newMeshPartIndices);

	return result;
}
