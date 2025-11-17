#include "meshGenModifiers.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"
#include "..\meshGenElementModifier.h"
#include "..\meshGenValueDef.h"

#include "..\..\appearance\appearanceControllerData.h"

#include "..\..\world\pointOfInterest.h"

#include "..\..\..\core\collision\model.h"
#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\mesh\socketDefinitionSet.h"
#include "..\..\..\core\types\vectorPacked.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

/**
 *	Allows:
 *		drop meshes (parts) alone
 */
class DropMeshData
: public ElementModifierData
{
	FAST_CAST_DECLARE(DropMeshData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
};

//

bool apply_drop(GenerationContext & _context, Checkpoint const & checkpoint, Checkpoint const & now, DropMeshData const * _data)
{
	bool result = true;

	if (DropMeshData const * data = fast_cast<DropMeshData>(_data))
	{
		if (checkpoint.partsSoFarCount != now.partsSoFarCount)
		{
			// for meshes apply transform and reverse order of elements (triangles, quads)
			RefCountObjectPtr<::Meshes::Mesh3DPart> const * pPart = &_context.get_parts()[checkpoint.partsSoFarCount];
			for (int i = checkpoint.partsSoFarCount; i < now.partsSoFarCount; ++i, ++pPart)
			{
				::Meshes::Mesh3DPart* part = pPart->get();
				part->clear();
			}
		}
	}

	return result;
}

//

bool Modifiers::drop_mesh(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
{
	bool result = true;

	Checkpoint checkpoint(_context);

	if (_subject)
	{
		result &= _context.process(_subject, &_instigatorInstance);
	}
	else
	{
		checkpoint = _context.get_checkpoint(1); // of parent
	}

	Checkpoint now(_context);

	if (checkpoint != now)
	{
		if (DropMeshData const * data = fast_cast<DropMeshData>(_data))
		{
			apply_drop(_context, checkpoint, now, data);
		}
	}

	return result;
}

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_drop_mesh_data()
{
	return new DropMeshData();
}

//

REGISTER_FOR_FAST_CAST(DropMeshData);

bool DropMeshData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	return result;
}
