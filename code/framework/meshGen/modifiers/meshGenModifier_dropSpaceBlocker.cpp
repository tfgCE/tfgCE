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
 *		drop space blockers alone
 */
class DropSpaceBlockerData
: public ElementModifierData
{
	FAST_CAST_DECLARE(DropSpaceBlockerData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
};

//

bool apply_drop(GenerationContext & _context, Checkpoint const & checkpoint, Checkpoint const & now, DropSpaceBlockerData const * _data)
{
	bool result = true;

	if (DropSpaceBlockerData const * data = fast_cast<DropSpaceBlockerData>(_data))
	{
		if (checkpoint.spaceBlockersSoFarCount != now.spaceBlockersSoFarCount)
		{
			// for meshes apply transform and reverse order of elements (triangles, quads)
			_context.access_space_blockers().blockers.set_size(checkpoint.spaceBlockersSoFarCount);
		}
	}

	return result;
}

//

bool Modifiers::drop_space_blocker(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
		if (DropSpaceBlockerData const * data = fast_cast<DropSpaceBlockerData>(_data))
		{
			apply_drop(_context, checkpoint, now, data);
		}
	}

	return result;
}

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_drop_space_blocker_data()
{
	return new DropSpaceBlockerData();
}

//

REGISTER_FOR_FAST_CAST(DropSpaceBlockerData);

bool DropSpaceBlockerData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	return result;
}
