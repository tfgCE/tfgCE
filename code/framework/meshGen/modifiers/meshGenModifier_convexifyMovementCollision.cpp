#include "meshGenModifiers.h"

#include "meshGenModifierUtils.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenValueDef.h"

#include "..\..\..\core\collision\model.h"

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
 *		break collision mesh into meshes that are convex (not convex hulls!)
 *	This is recommended for meshes that may leak in front of a door.
 * 
 */
class ConvexifyMovementCollisionData
: public ElementModifierData
{
	FAST_CAST_DECLARE(ConvexifyMovementCollisionData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
};

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_convexify_movement_collision_data()
{
	return new ConvexifyMovementCollisionData();
}

//

bool Modifiers::convexify_movement_collision(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
		if (checkpoint.movementCollisionPartsSoFarCount != now.movementCollisionPartsSoFarCount)
		{
			for (int i = checkpoint.movementCollisionPartsSoFarCount; i < now.movementCollisionPartsSoFarCount; ++i)
			{
				::Collision::Model* colModel = _context.get_movement_collision_parts()[i];
				colModel->convexify_static_meshes();
			}
		}
	}

	return result;
}

//

REGISTER_FOR_FAST_CAST(ConvexifyMovementCollisionData);

bool ConvexifyMovementCollisionData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;


	return result;
}
