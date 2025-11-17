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
#include "..\..\..\core\system\video\vertexFormatUtils.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

/**
 *	Sets V component of UV along segment
 */
class SetVAlongData
: public ElementModifierData
{
	FAST_CAST_DECLARE(SetVAlongData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	ValueDef<Vector3> aPoint;
	ValueDef<Vector3> bPoint;
	ValueDef<float> aV = 0.0f;
	ValueDef<float> bV = 1.0f;
};

//

bool Modifiers::set_v_along(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
		if (SetVAlongData const * data = fast_cast<SetVAlongData>(_data))
		{
			if (data->aPoint.is_set() &&
				data->bPoint.is_set())
			{
				float aV = data->aV.get_with_default(_context, 0.0f);
				float bV = data->bV.get_with_default(_context, 1.0f);

				Vector3 aPoint = data->aPoint.get(_context);
				Vector3 bPoint = data->bPoint.get(_context);

				Segment a2b = Segment(aPoint, bPoint);
				if (checkpoint.partsSoFarCount != now.partsSoFarCount)
				{
					RefCountObjectPtr<::Meshes::Mesh3DPart> const* pPart = &_context.get_parts()[checkpoint.partsSoFarCount];
					for (int i = checkpoint.partsSoFarCount; i < now.partsSoFarCount; ++i, ++pPart)
					{
						::Meshes::Mesh3DPart* part = pPart->get();
						auto & vertexData = part->access_vertex_data();
						auto & vertexFormat = part->get_vertex_format();
						if (vertexFormat.get_texture_uv() >= System::VertexFormatTextureUV::UV)
						{
							auto numberOfVertices = part->get_number_of_vertices();
							int stride = vertexFormat.get_stride();
							int locationOffset = vertexFormat.get_location_offset();
							int uvOffset = vertexFormat.get_texture_uv_offset();
							int8* currentVertexData = vertexData.get_data();
							for (int vrtIdx = 0; vrtIdx < numberOfVertices; ++vrtIdx, currentVertexData += stride)
							{
								if (vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
								{
									Vector3* location = (Vector3*)(currentVertexData + locationOffset);
									float t = a2b.get_t(*location);

									float v = aV * (1.0f - t) + bV * t;
									v = clamp(v, min(aV, bV), max(aV, bV));

									::System::VertexFormatUtils::store(currentVertexData + uvOffset, vertexFormat.get_texture_uv_type(), v, 1);
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

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_set_v_along_data()
{
	return new SetVAlongData();
}

//

REGISTER_FOR_FAST_CAST(SetVAlongData);

bool SetVAlongData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	aPoint.load_from_xml_child_node(_node, TXT("aPoint"));
	bPoint.load_from_xml_child_node(_node, TXT("bPoint"));
	aV.load_from_xml_child_node(_node, TXT("aV"));
	bV.load_from_xml_child_node(_node, TXT("bV"));

	if (!aPoint.is_set() ||
		!bPoint.is_set())
	{
		error_loading_xml(_node, TXT("not all parameters for set v along were defined"));
		result = false;
	}

	return result;
}
