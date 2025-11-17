#include "meshGenModifiers.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"
#include "..\meshGenElementModifier.h"
#include "..\meshGenValueDef.h"
#include "..\meshGenValueDefImpl.inl"

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
 *		matches placement, if something not provided, transform:identity assumed
 *		if only fromPOI provided, will read POI position and move everything (including POI) to be in "to" transform
 */
class MatchPlacementData
: public ElementModifierData
{
	FAST_CAST_DECLARE(MatchPlacementData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	Name fromPOI;
	Name fromMeshNode;
	ValueDef<Transform> to;
};

//

bool apply_match_placement(GenerationContext & _context, Checkpoint const & checkpoint, Checkpoint const & now, MatchPlacementData const * _data)
{
	bool result = true;

	if (MatchPlacementData const * data = fast_cast<MatchPlacementData>(_data))
	{
		Transform transformFrom = Transform::identity;
		Transform transformTo = _data->to.get_with_default(_context, Transform::identity);
		if (_data->fromPOI.is_valid())
		{
			if (checkpoint.poisSoFarCount != now.poisSoFarCount)
			{
				for_range(int, i, checkpoint.poisSoFarCount, now.poisSoFarCount - 1)
				{
					auto& poi = _context.get_pois()[i];
					if (poi->name == _data->fromPOI)
					{
						transformFrom = poi->offset;
					}
				}
			}
		}
		if (_data->fromMeshNode.is_valid())
		{
			if (checkpoint.meshNodesSoFarCount != now.meshNodesSoFarCount)
			{
				for_range(int, i, checkpoint.meshNodesSoFarCount, now.meshNodesSoFarCount - 1)
				{
					auto& meshNode = _context.get_mesh_nodes()[i];
					if (meshNode->name == _data->fromMeshNode)
					{
						transformFrom = meshNode->placement;
					}
				}
			}
		}

		Transform transform = transformTo.to_world(transformFrom.inverted());
		Matrix44 transformMat = transform.to_matrix();

		{
			if (checkpoint.partsSoFarCount != now.partsSoFarCount)
			{
				RefCountObjectPtr<::Meshes::Mesh3DPart> const* pPart = &_context.get_parts()[checkpoint.partsSoFarCount];
				for (int i = checkpoint.partsSoFarCount; i < now.partsSoFarCount; ++i, ++pPart)
				{
					::Meshes::Mesh3DPart* part = pPart->get();
					apply_transform_to_vertex_data(part->access_vertex_data(), part->get_number_of_vertices(), part->get_vertex_format(), transformMat);
				}
			}
			if (checkpoint.movementCollisionPartsSoFarCount != now.movementCollisionPartsSoFarCount)
			{
				// for meshes apply transform and reverse order of elements (triangles, quads)
				::Collision::Model* const* pPart = &_context.get_movement_collision_parts()[checkpoint.movementCollisionPartsSoFarCount];
				for (int i = checkpoint.movementCollisionPartsSoFarCount; i < now.movementCollisionPartsSoFarCount; ++i, ++pPart)
				{
					::Collision::Model* part = *pPart;
					apply_transform_to_collision_model(part, transformMat);
				}
			}
			if (checkpoint.preciseCollisionPartsSoFarCount != now.preciseCollisionPartsSoFarCount)
			{
				// for meshes apply transform and reverse order of elements (triangles, quads)
				::Collision::Model* const* pPart = &_context.get_precise_collision_parts()[checkpoint.preciseCollisionPartsSoFarCount];
				for (int i = checkpoint.preciseCollisionPartsSoFarCount; i < now.preciseCollisionPartsSoFarCount; ++i, ++pPart)
				{
					::Collision::Model* part = *pPart;
					apply_transform_to_collision_model(part, transformMat);
				}
			}
			if (checkpoint.socketsGenerationIdSoFar != now.socketsGenerationIdSoFar)
			{
				for_every(socket, _context.access_sockets().access_sockets())
				{
					if (socket->get_generation_id() > checkpoint.socketsGenerationIdSoFar)
					{
						socket->apply_to_placement_MS(transformMat);
					}
				}
			}
			if (checkpoint.meshNodesSoFarCount != now.meshNodesSoFarCount)
			{
				MeshNodePtr* meshNode = &_context.access_mesh_nodes()[checkpoint.meshNodesSoFarCount];
				for (int i = checkpoint.meshNodesSoFarCount; i < now.meshNodesSoFarCount; ++i, ++meshNode)
				{
					meshNode->get()->apply(transformMat);
				}
			}
			if (checkpoint.poisSoFarCount != now.poisSoFarCount)
			{
				PointOfInterestPtr* poi = &_context.access_pois()[checkpoint.poisSoFarCount];
				for (int i = checkpoint.poisSoFarCount; i < now.poisSoFarCount; ++i, ++poi)
				{
					poi->get()->apply(transformMat);
				}
			}
			if (checkpoint.bonesSoFarCount != now.bonesSoFarCount)
			{
				todo_important(TXT("implement_"));
			}
			if (checkpoint.spaceBlockersSoFarCount != now.spaceBlockersSoFarCount)
			{
				SpaceBlocker* sb = &_context.access_space_blockers().blockers[checkpoint.spaceBlockersSoFarCount];
				for (int i = checkpoint.spaceBlockersSoFarCount; i < now.spaceBlockersSoFarCount; ++i, ++sb)
				{
					sb->box.apply_transform(transform);
				}
			}
			if (checkpoint.appearanceControllersSoFarCount != now.appearanceControllersSoFarCount)
			{
				todo_important(TXT("implement_"));
			}
		}
	}

	return result;
}

//

bool Modifiers::match_placement(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
		if (MatchPlacementData const * data = fast_cast<MatchPlacementData>(_data))
		{
			apply_match_placement(_context, checkpoint, now, data);
		}
	}

	return result;
}

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_match_placement_data()
{
	return new MatchPlacementData();
}

//

REGISTER_FOR_FAST_CAST(MatchPlacementData);

bool MatchPlacementData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	fromPOI = _node->get_name_attribute_or_from_child(TXT("fromPOI"), fromPOI);
	fromMeshNode = _node->get_name_attribute_or_from_child(TXT("fromMeshNode"), fromMeshNode);
	to.load_from_xml_child_node(_node, TXT("to"));

	return result;
}
