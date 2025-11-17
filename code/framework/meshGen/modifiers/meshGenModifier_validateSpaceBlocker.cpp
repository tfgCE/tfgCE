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

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

//

/**
 *	Allows:
 *		weld points that are close
 */
class ValidateSpaceBlockerData
: public ElementModifierData
{
	FAST_CAST_DECLARE(ValidateSpaceBlockerData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	Name outParam;
};

//

bool Modifiers::validate_space_blocker(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
	
	bool valid = false;

	if (checkpoint != now)
	{
		if (checkpoint.spaceBlockersSoFarCount != now.spaceBlockersSoFarCount)
		{
			valid = true;

			SpaceBlocker const * sb = &_context.get_space_blockers().blockers[checkpoint.spaceBlockersSoFarCount];
			for (int i = checkpoint.spaceBlockersSoFarCount; i < now.spaceBlockersSoFarCount && valid; ++i, ++sb)
			{
				SpaceBlocker const* psb = &_context.get_space_blockers().blockers[0]; // always all
				for (int j = 0; j < checkpoint.spaceBlockersSoFarCount && valid; ++j, ++psb)
				{
					if (!sb->check(*psb))
					{
						valid = false;
					}
				}
			}
			if (!valid)
			{
				// remove stuff
				for (int i = checkpoint.partsSoFarCount; i < _context.get_parts().get_size(); ++i)
				{
					_context.get_parts()[i]->clear();
				}
				for (int i = checkpoint.movementCollisionPartsSoFarCount; i < _context.get_movement_collision_parts().get_size(); ++i)
				{
					_context.get_movement_collision_parts()[i]->clear();
				}
				for (int i = checkpoint.preciseCollisionPartsSoFarCount; i < _context.get_precise_collision_parts().get_size(); ++i)
				{
					_context.get_precise_collision_parts()[i]->clear();
				}
				_context.access_sockets().access_sockets().set_size(checkpoint.socketsGenerationIdSoFar);
				_context.access_mesh_nodes().set_size(checkpoint.meshNodesSoFarCount);
				_context.access_pois().set_size(checkpoint.poisSoFarCount);
				_context.access_space_blockers().blockers.set_size(checkpoint.spaceBlockersSoFarCount);
				_context.access_generated_bones().set_size(checkpoint.bonesSoFarCount);
				_context.access_appearance_controllers().set_size(checkpoint.appearanceControllersSoFarCount);
			}
		}
	}

	if (auto* d = fast_cast<ValidateSpaceBlockerData>(_data))
	{
		if (d->outParam.is_valid())
		{
			_context.set_parameter(d->outParam, type_id<bool>(), &valid, 0); // in parent
		}
	}

	return result;
}

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_validate_space_blocker_data()
{
	return new ValidateSpaceBlockerData();
}

//

REGISTER_FOR_FAST_CAST(ValidateSpaceBlockerData);

bool ValidateSpaceBlockerData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	outParam = _node->get_name_attribute_or_from_child(TXT("outParam"), outParam);

	return result;
}
