#include "meshGenMeshProcOp_changeMaterial.h"

#include "..\meshGenGenerationContext.h"

#include "..\modifiers\meshGenModifierUtils.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\system\video\indexFormatUtils.h"
#include "..\..\..\core\system\video\vertexFormatUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace MeshProcessorOperators;

//

REGISTER_FOR_FAST_CAST(ChangeMaterial);

bool ChangeMaterial::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	toMaterialIdx.load_from_xml(_node, TXT("materialIdx"));
	toMaterialIdx.load_from_xml(_node, TXT("toMaterialIdx"));
	fromMaterialIdx.load_from_xml(_node, TXT("fromMaterialIdx"));

	result &= onKept.load_from_xml_child_node(_node, _lc, TXT("onKept"));
	result &= onChanged.load_from_xml_child_node(_node, _lc, TXT("onChanged"));

	return result;
}

bool ChangeMaterial::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= onKept.prepare_for_game(_library, _pfgContext);
	result &= onChanged.prepare_for_game(_library, _pfgContext);

	return result;
}

bool ChangeMaterial::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	Array<int> keptMPIndices;
	Array<int> changedMPIndices;

	int toMaterialIdx = 0;
	Optional<int> fromMaterialIdx;
	if (this->toMaterialIdx.is_set())
	{
		toMaterialIdx = this->toMaterialIdx.get(_context);
	}
	if (this->fromMaterialIdx.is_set())
	{
		fromMaterialIdx = this->fromMaterialIdx.get(_context);
	}

	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* processPart = _context.get_parts()[*pIdx].get();

		if (fromMaterialIdx.is_set() &&
			_context.get_material_index_for_part(processPart) != fromMaterialIdx.get())
		{
			// skip changing
			keptMPIndices.push_back(*pIdx);
		}
		else
		{
			// just quickly change material index
			_context.change_material_index_for_part(processPart, toMaterialIdx);
			changedMPIndices.push_back(*pIdx);
		}
	}

	result &= onKept.process(_context, _instance, keptMPIndices, _newMeshPartIndices);
	result &= onChanged.process(_context, _instance, changedMPIndices, _newMeshPartIndices);

	return result;
}
