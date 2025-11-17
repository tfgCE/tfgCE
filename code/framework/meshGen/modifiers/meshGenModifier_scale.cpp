#include "meshGenModifiers.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"
#include "..\meshGenElementModifier.h"
#include "..\meshGenValueDef.h"

#include "..\..\world\pointOfInterest.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\mesh\socketDefinitionSet.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

/**
 *	Allows:
 *		skew using anchor point and source point and destination point (all can be params)
 */
class ScaleData
: public ElementModifierData
{
	FAST_CAST_DECLARE(ScaleData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	ValueDef<Transform> anchor;
	ValueDef<Vector3> scale;
};

//

bool Modifiers::scale(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
		if (ScaleData const * data = fast_cast<ScaleData>(_data))
		{
			if (data->scale.is_set())
			{
				Transform anchor = data->anchor.is_set()? data->anchor.get(_context) : Transform::identity;
				Vector3 scale = data->scale.get(_context);
				{
					Matrix44 sourceMat = anchor.to_matrix();
					Matrix44 destMat = sourceMat;
					destMat.set_x_axis(destMat.get_x_axis() * scale.x);
					destMat.set_y_axis(destMat.get_y_axis() * scale.y);
					destMat.set_z_axis(destMat.get_z_axis() * scale.z);
					Matrix44 transformMat = destMat.to_world(sourceMat.inverted());
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
						::Collision::Model* const * pPart = &_context.get_movement_collision_parts()[checkpoint.movementCollisionPartsSoFarCount];
						for (int i = checkpoint.movementCollisionPartsSoFarCount; i < now.movementCollisionPartsSoFarCount; ++i, ++pPart)
						{
							::Collision::Model* part = *pPart;
							apply_transform_to_collision_model(part, transformMat);
						}
					}
					if (checkpoint.preciseCollisionPartsSoFarCount != now.preciseCollisionPartsSoFarCount)
					{
						// for meshes apply transform and reverse order of elements (triangles, quads)
						::Collision::Model* const * pPart = &_context.get_precise_collision_parts()[checkpoint.preciseCollisionPartsSoFarCount];
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
						MeshNodePtr * meshNode = &_context.access_mesh_nodes()[checkpoint.meshNodesSoFarCount];
						for (int i = checkpoint.meshNodesSoFarCount; i < now.meshNodesSoFarCount; ++i, ++meshNode)
						{
							meshNode->get()->apply(transformMat);
						}
					}
					if (checkpoint.poisSoFarCount != now.poisSoFarCount)
					{
						PointOfInterestPtr * poi = &_context.access_pois()[checkpoint.poisSoFarCount];
						for (int i = checkpoint.poisSoFarCount; i < now.poisSoFarCount; ++i, ++poi)
						{
							poi->get()->apply(transformMat);
						}
					}
					if (checkpoint.bonesSoFarCount != now.bonesSoFarCount)
					{
						todo_important(TXT("implement_"));
					}
					if (checkpoint.appearanceControllersSoFarCount != now.appearanceControllersSoFarCount)
					{
						todo_important(TXT("implement_"));
					}
				}
			}
		}
	}
	
	return result;
}

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_scale_data()
{
	return new ScaleData();
}

//

REGISTER_FOR_FAST_CAST(ScaleData);

bool ScaleData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	anchor.load_from_xml_child_node(_node, TXT("anchor"));
	scale.load_from_xml_child_node(_node, TXT("scale"));

	if (!scale.is_set())
	{
		error_loading_xml(_node, TXT("scale not set for scale"));
		result = false;
	}

	return result;
}
