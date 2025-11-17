#include "meshGenModifiers.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"
#include "..\meshGenElementModifier.h"
#include "..\meshGenValueDef.h"
#include "..\meshGenValueDefImpl.inl"

#include "..\..\world\pointOfInterest.h"

#include "..\..\..\core\collision\model.h"
#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\mesh\socketDefinitionSet.h"

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
 *	Sets V component of UV along segment
 */
class SetCustomDataData
: public ElementModifierData
{
	FAST_CAST_DECLARE(SetCustomDataData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	ValueDef<Name> name;
	ValueDef<float> asFloat1;
	ValueDef<int> asInt;
	ValueDef<Vector3> asVector31;
	ValueDef<float> asFloat0;
	ValueDef<int> asInt0;
	ValueDef<Vector3> asVector30;

	bool add = false;
	
	ValueDef<Vector3> interpolate0;
	ValueDef<Vector3> interpolate1;
	ValueDef<bool> setYaw;
	ValueDef<bool> normaliseYaw;
	ValueDef<float> baseYaw;
};

//

bool Modifiers::set_custom_data(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
		if (SetCustomDataData const * data = fast_cast<SetCustomDataData>(_data))
		{
			Name cdName = data->name.get_with_default(_context, Name::invalid());
			Optional<float> asFloat0;
			Optional<float> asFloat1;
			if (data->asFloat0.is_set()) { asFloat0 = data->asFloat0.get_with_default(_context, 0.0f); }
			if (data->asFloat1.is_set()) { asFloat1 = data->asFloat1.get_with_default(_context, 0.0f); }
			Optional<Vector3> asVector30;
			Optional<Vector3> asVector31;
			if (data->asVector30.is_set()) { asVector30 = data->asVector30.get_with_default(_context, Vector3::zero); }
			if (data->asVector31.is_set()) { asVector31 = data->asVector31.get_with_default(_context, Vector3::zero); }
			Optional<int> asInt;
			if (data->asInt.is_set())
			{
				asInt = data->asInt.get_with_default(_context, 0);
			}

			Vector3 i0 = Vector3::zero;
			Vector3 i1 = Vector3::zero;
			Vector3 i0i1Dir = Vector3::zero;
			float i0i1LenInv = 1.0f;
			bool interpolate = false;
			bool setYaw = data->setYaw.get_with_default(_context, false);
			bool normaliseYaw = data->normaliseYaw.get_with_default(_context, false);
			float baseYaw = data->baseYaw.get_with_default(_context, 0.0f);

			if (data->interpolate0.is_set() &&
				data->interpolate1.is_set())
			{
				i0 = data->interpolate0.get(_context);
				i1 = data->interpolate1.get(_context);
				i0i1Dir = (i1 - i0).normal();
				float i0i1Len = (i1 - i0).length();
				i0i1LenInv = i0i1Len != 0.0f ? 1.0f / i0i1Len : 1.0f;
				interpolate = true;
			}

			if (checkpoint.partsSoFarCount != now.partsSoFarCount)
			{
				RefCountObjectPtr<::Meshes::Mesh3DPart> const* pPart = &_context.get_parts()[checkpoint.partsSoFarCount];
				for (int i = checkpoint.partsSoFarCount; i < now.partsSoFarCount; ++i, ++pPart)
				{
					::Meshes::Mesh3DPart* part = pPart->get();
					auto & vertexData = part->access_vertex_data();
					auto & vertexFormat = part->get_vertex_format();
					if (auto* cd = vertexFormat.get_custom_data(cdName))
					{
						int cdOffset = vertexFormat.get_custom_data_offset(cdName);
						if (cdOffset != NONE)
						{
							auto numberOfVertices = part->get_number_of_vertices();
							int stride = vertexFormat.get_stride();
							int8* currentVertexData = vertexData.get_data();
							int locationOffset = vertexFormat.get_location_offset();
							for (int vrtIdx = 0; vrtIdx < numberOfVertices; ++vrtIdx, currentVertexData += stride)
							{
								if (setYaw)
								{
									Vector3 location = Vector3::zero;
									if (vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
									{
										location = *(Vector3*)(currentVertexData + locationOffset);
									}
									float yaw = Rotator3::normalise_axis(Rotator3::get_yaw(location) + baseYaw);
									if (normaliseYaw)
									{
										yaw /= 180.0f;
									}
									if (cd->dataType == ::System::DataFormatValueType::Float)
									{
										float* dest = (float*)(currentVertexData + cdOffset);
										for_count(int, i, cd->count)
										{
											*dest = yaw;
											++dest;
										}
									}
									else
									{
										todo_important(TXT("implement_ another type"));
									}
								}
								else
								{
									float amount = 1.0f;
									if (interpolate)
									{
										Vector3 i0 = data->interpolate0.get(_context);
										Vector3 i1 = data->interpolate1.get(_context);
										Vector3 location = i1;
										if (vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
										{
											location = *(Vector3*)(currentVertexData + locationOffset);
										}
										amount = clamp(Vector3::dot(location - i0, i0i1Dir) * i0i1LenInv, 0.0f, 1.0f);
									}

									if (cd->dataType == ::System::DataFormatValueType::Float)
									{
										if (asVector31.is_set())
										{
											if (cd->count == 3)
											{
												Vector3* dest = (Vector3*)(currentVertexData + cdOffset);
												*dest = asVector31.get(Vector3::zero) * amount + asVector30.get(Vector3::zero) * (1.0f - amount);
												++dest;
											}
											else
											{
												error_generating_mesh(_instigatorInstance, TXT("vector3 expected to be in 3 floats)"));
											}
										}
										else
										{
											float* dest = (float*)(currentVertexData + cdOffset);
											for_count(int, i, cd->count)
											{
												if (!asFloat1.is_set())
												{
													error_generating_mesh(_instigatorInstance, TXT("expected float value"));
												}
												*dest = asFloat1.get(0.0f) * amount + asFloat0.get(0.0f) * (1.0f - amount);
												++dest;
											}
										}
									}
									else if (cd->dataType == ::System::DataFormatValueType::Uint8)
									{
										uint8* dest = (uint8*)(currentVertexData + cdOffset);
										for_count(int, i, cd->count)
										{
											if (!asInt.is_set())
											{
												error_generating_mesh(_instigatorInstance, TXT("expected int value"));
											}
											*dest = (uint8)asInt.get(0);
											++dest;
										}
									}
									else if (cd->dataType == ::System::DataFormatValueType::Uint16)
									{
										uint16* dest = (uint16*)(currentVertexData + cdOffset);
										for_count(int, i, cd->count)
										{
											if (!asInt.is_set())
											{
												error_generating_mesh(_instigatorInstance, TXT("expected int value"));
											}
											*dest = (uint16)asInt.get(0);
											++dest;
										}
									}
									else if (cd->dataType == ::System::DataFormatValueType::Uint32)
									{
										uint32* dest = (uint32*)(currentVertexData + cdOffset);
										for_count(int, i, cd->count)
										{
											if (!asInt.is_set())
											{
												error_generating_mesh(_instigatorInstance, TXT("expected int value"));
											}
											*dest = (uint32)asInt.get(0);
											++dest;
										}
									}
									else
									{
										todo_important(TXT("implement_ another type"));
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	return result;
}

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_set_custom_data_data()
{
	return new SetCustomDataData();
}

//

REGISTER_FOR_FAST_CAST(SetCustomDataData);

bool SetCustomDataData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	name.load_from_xml(_node, TXT("name"));
	todo_note(TXT("now only supports single value"));
	asFloat1.load_from_xml(_node, TXT("float"));
	asInt.load_from_xml(_node, TXT("int"));
	asVector31.load_from_xml_child_node(_node, TXT("vector3"));
	asFloat1.load_from_xml(_node, TXT("float1"));
	asVector31.load_from_xml_child_node(_node, TXT("vector31"));
	asFloat0.load_from_xml(_node, TXT("float0"));
	asVector30.load_from_xml_child_node(_node, TXT("vector30"));
	interpolate0.load_from_xml_child_node(_node, TXT("interpolate0"));
	interpolate1.load_from_xml_child_node(_node, TXT("interpolate1"));
	setYaw.load_from_xml_child_node(_node, TXT("setYaw"));
	normaliseYaw.load_from_xml_child_node(_node, TXT("normaliseYaw"));
	baseYaw.load_from_xml_child_node(_node, TXT("baseYaw"));

	if (! name.is_set())
	{
		error_loading_xml(_node, TXT("requires name"));
		result = false;
	}
	int noSet = (asFloat1.is_set()? 1 : 0)
			  + (asInt.is_set() ? 1 : 0)
			  + (asVector31.is_set() ? 1 : 0)
			  ;
	if (noSet != 1 && ! setYaw.is_set())
	{
		error_loading_xml(_node, TXT("exactly one value has to be provided"));
		result = false;
	}

	add = _node->get_bool_attribute_or_from_child_presence(TXT("add"), add);
	an_assert(!add, TXT("implement"));

	return result;
}
