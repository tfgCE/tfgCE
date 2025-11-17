#include "meshGenModifiers.h"

#include "meshGenModifierSelection.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"
#include "..\meshGenElementModifier.h"
#include "..\meshGenValueDef.h"

#include "..\..\world\pointOfInterest.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\mesh\socketDefinitionSet.h"
#include "..\..\..\core\types\vectorPacked.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

/**
 *	Allows:
 *		skew using anchor point and source point and destination point (all can be params)
 */
class SkewData
: public ElementModifierData
{
	FAST_CAST_DECLARE(SkewData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	Selection selection;
	ValueDef<bool> onlyBetweenAnchorAndSource;
	ValueDef<Vector3> up;
	ValueDef<Vector3> anchor;
	ValueDef<Vector3> source;
	ValueDef<Vector3> destination;
	ValueDef<Vector2> scale; // scales at destination, anchor remains
	ValueDef<float> twist; // angle of twist at destination, anchor remains
	bool keepNormals = false;
};

//

bool Modifiers::skew(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
		if (SkewData const * data = fast_cast<SkewData>(_data))
		{
			if (data->anchor.is_set() &&
				data->source.is_set() &&
				data->destination.is_set())
			{
				bool onlyBetweenAnchorAndSource = data->onlyBetweenAnchorAndSource.is_set() ? data->onlyBetweenAnchorAndSource.get(_context) : false;
				Vector3 anchor = data->anchor.get(_context);
				Vector3 source = data->source.get(_context);
				Vector3 fwdSource = source - anchor;
				Vector3 fwdDest = data->destination.get(_context) - anchor;
				Vector2 scale = data->scale.is_set()? data->scale.get(_context) : Vector2::one;
				float twist = data->twist.is_set()? data->twist.get(_context) : 0.0f;
				if ((fwdSource - fwdDest).length() > MARGIN || ! scale.is_one() || twist != 0.0f)
				{
					// make source vector sane and scale destination by it
					// this will make it possible to transform vectors properly
					float fwdSourceLength = fwdSource.length();
					fwdSource /= fwdSourceLength;
					fwdDest /= fwdSourceLength;
					Vector3 upDir;
					if (data->up.is_set())
					{
						upDir = data->up.get(_context);
					}
					else if ((fwdSource - fwdDest).length() > MARGIN)
					{
						upDir = Vector3::cross(fwdSource, fwdDest).normal();
					}
					else
					{
						upDir = Vector3::zAxis;
						if (Vector3::dot(upDir, fwdDest) > 0.7f)
						{
							upDir = Vector3::xAxis;
							if (Vector3::dot(upDir, fwdDest) > 0.7f)
							{
								upDir = Vector3::yAxis;
							}
						}
					}
					Vector3 rightDir = Vector3::cross(upDir, fwdSource).normal();
					Matrix44 sourceMat = matrix_from_axes_orthonormal_check(rightDir, fwdSource, upDir, anchor);
					Matrix44 destMat = matrix_from_axes(rightDir, fwdDest, upDir, anchor);
					Matrix44 destScaledTwistedMat = matrix_from_axes(rightDir * scale.x, fwdDest, upDir * scale.y, anchor).to_world(Rotator3(0.0f, 0.0f, twist).to_quat().to_matrix());
					Matrix44 transformMat = destMat.to_world(sourceMat.inverted());
					Matrix44 transformScaledTwistedMat = destScaledTwistedMat.to_world(sourceMat.inverted());
					Segment anchorToSource(anchor, source);
					if (checkpoint.partsSoFarCount != now.partsSoFarCount)
					{
						RefCountObjectPtr<::Meshes::Mesh3DPart> const* pPart = &_context.get_parts()[checkpoint.partsSoFarCount];
						for (int i = checkpoint.partsSoFarCount; i < now.partsSoFarCount; ++i, ++pPart)
						{
							::Meshes::Mesh3DPart* part = pPart->get();
							//apply_transform_to_vertex_data(part->access_vertex_data(), part->get_number_of_vertices(), part->get_vertex_format(), transformMat);
							int _numberOfVertices = part->get_number_of_vertices();
							auto& _vertexData = part->access_vertex_data();
							auto& _vertexFormat = part->get_vertex_format();
							int stride = _vertexFormat.get_stride();
							{
								int8* currentVertexData = _vertexData.get_data() + _vertexFormat.get_location_offset();
								for (int vrtIdx = 0; vrtIdx < _numberOfVertices; ++vrtIdx, currentVertexData += stride)
								{
									if (_vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
									{
										Vector3* location = (Vector3*)(currentVertexData);
										if (data->selection.is_selected(*location))
										{
											float t = onlyBetweenAnchorAndSource ? anchorToSource.get_t_not_clamped(*location) : anchorToSource.get_t(*location);
											if (onlyBetweenAnchorAndSource &&
												(t < 0.0f || t > 1.0f))
											{
												// leave it as it is
												continue;
											}
											*location = transformMat.location_to_world(*location) * (1.0f - t)
												+ transformScaledTwistedMat.location_to_world(*location) * t;
										}
									}
								}
							}
							if (_vertexFormat.get_normal() == ::System::VertexFormatNormal::Yes && ! data->keepNormals)
							{
								int8* currentVertexDataLoc = _vertexData.get_data() + _vertexFormat.get_location_offset();
								int8* currentVertexData = _vertexData.get_data() + _vertexFormat.get_normal_offset();
								if (_vertexFormat.is_normal_packed())
								{
									for (int vrtIdx = 0; vrtIdx < _numberOfVertices; ++vrtIdx, currentVertexDataLoc += stride, currentVertexData += stride)
									{
										Vector3* location = (Vector3*)(currentVertexDataLoc);
										if (data->selection.is_selected(*location))
										{
											float t = onlyBetweenAnchorAndSource ? anchorToSource.get_t_not_clamped(*location) : anchorToSource.get_t(*location);
											if (onlyBetweenAnchorAndSource &&
												(t < 0.0f || t > 1.0f))
											{
												// leave it as it is
												continue;
											}
											Vector3 normal = ((VectorPacked*)(currentVertexData))->get_as_vertex_normal();
											normal = (transformMat.vector_to_world(normal) * (1.0f - t)
													+ transformScaledTwistedMat.vector_to_world(normal) * t).normal();
											((VectorPacked*)(currentVertexData))->set_as_vertex_normal(normal);
										}
									}
								}
								else
								{
									for (int vrtIdx = 0; vrtIdx < _numberOfVertices; ++vrtIdx, currentVertexDataLoc += stride, currentVertexData += stride)
									{
										Vector3* location = (Vector3*)(currentVertexDataLoc);
										if (data->selection.is_selected(*location))
										{
											float t = onlyBetweenAnchorAndSource ? anchorToSource.get_t_not_clamped(*location) : anchorToSource.get_t(*location);
											if (onlyBetweenAnchorAndSource &&
												(t < 0.0f || t > 1.0f))
											{
												// leave it as it is
												continue;
											}
											Vector3* normal = (Vector3*)(currentVertexData);
											*normal = (transformMat.vector_to_world(*normal) * (1.0f - t)
													 + transformScaledTwistedMat.vector_to_world(*normal) * t).normal();
											normal->normalise();
										}
									}
								}
							}
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

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_skew_data()
{
	return new SkewData();
}

//

REGISTER_FOR_FAST_CAST(SkewData);

bool SkewData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= selection.load_from_child_node_xml(_node);

	onlyBetweenAnchorAndSource.load_from_xml(_node, TXT("onlyBetweenAnchorAndSource"));
	up.load_from_xml_child_node(_node, TXT("up"));
	anchor.load_from_xml_child_node(_node, TXT("anchor"));
	source.load_from_xml_child_node(_node, TXT("from"));
	source.load_from_xml_child_node(_node, TXT("source"));
	destination.load_from_xml_child_node(_node, TXT("to"));
	destination.load_from_xml_child_node(_node, TXT("destination"));
	destination.load_from_xml_child_node(_node, TXT("dest"));

	scale.load_from_xml_child_node(_node, TXT("scale"));
	twist.load_from_xml_child_node(_node, TXT("twist"));

	keepNormals = _node->get_bool_attribute_or_from_child_presence(TXT("keepNormals"), keepNormals);

	if (!anchor.is_set() ||
		!source.is_set() ||
		!destination.is_set())
	{
		error_loading_xml(_node, TXT("not all parameters for skew were defined"));
		result = false;
	}

	return result;
}
