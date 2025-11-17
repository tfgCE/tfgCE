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

float split_threshold() { return 0.001f; }

/**
 *	Allows:
 *		split using split axis and point
 *	This is recommended for meshes that may leak in front of a door. Split it behind the door.
 * 
 */
class SplitMovementCollisionData
: public ElementModifierData
{
	FAST_CAST_DECLARE(SplitMovementCollisionData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	ValueDef<Vector3> splitDir = ValueDef<Vector3>(Vector3::zero);
	ValueDef<Vector3> splitPoint = ValueDef<Vector3>(Vector3::zero);
	ValueDef<Range3> inRange = ValueDef<Range3>(Range3::empty);
};

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_split_movement_collision_data()
{
	return new SplitMovementCollisionData();
}

//

bool Modifiers::split_movement_collision(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
		if (SplitMovementCollisionData const * data = fast_cast<SplitMovementCollisionData>(_data))
		{
			// cut all behind - keep those at the plane, when making a copy, we will not copy those at the plane
			float threshold = split_threshold();

			Vector3 normal = data->splitDir.get(_context).normal();
			Vector3 loc = data->splitPoint.get(_context) - threshold * normal;
			Optional<Range3> inRange;
			{
				Range3 range = data->inRange.get(_context);
				if (!range.is_empty())
				{
					inRange = range;
				}
			}
			Plane splitPlane = Plane(normal, loc);

			if (checkpoint.movementCollisionPartsSoFarCount != now.movementCollisionPartsSoFarCount)
			{
				for (int i = checkpoint.movementCollisionPartsSoFarCount; i < now.movementCollisionPartsSoFarCount; ++i)
				{
					::Collision::Model* colModel = _context.get_movement_collision_parts()[i];
					colModel->split_meshes_at(splitPlane, inRange, threshold);
				}
			}

			return result;
		}
	}

	return result;
}

//

REGISTER_FOR_FAST_CAST(SplitMovementCollisionData);

bool SplitMovementCollisionData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	splitDir.load_from_xml_child_node(_node, TXT("dir"));
	splitPoint.load_from_xml_child_node(_node, TXT("point"));
	inRange.load_from_xml_child_node(_node, TXT("inRange"));

	if (splitDir.is_value_set())
	{
		if (splitDir.get_value().is_zero())
		{
			splitDir.set(Vector3::xAxis);
		}
	}

	return result;
}
