#include "meshGenModifiers.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"
#include "..\meshGenElementModifier.h"
#include "..\meshGenValueDef.h"

#include "..\..\world\pointOfInterest.h"

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

/**
 *	Allows:
 */
class InvertNormalsData
: public ElementModifierData
{
	FAST_CAST_DECLARE(InvertNormalsData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
};

//

bool Modifiers::invert_normals(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
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
		if (InvertNormalsData const * data = fast_cast<InvertNormalsData>(_data))
		{
			if (checkpoint.partsSoFarCount != now.partsSoFarCount)
			{
				RefCountObjectPtr<::Meshes::Mesh3DPart> const * pPart = &_context.get_parts()[checkpoint.partsSoFarCount];
				for (int i = checkpoint.partsSoFarCount; i < now.partsSoFarCount; ++i, ++pPart)
				{
					::Meshes::Mesh3DPart* part = pPart->get();
					auto& _vertexFormat = part->get_vertex_format();
					if (_vertexFormat.get_normal() == ::System::VertexFormatNormal::Yes)
					{
						int _numberOfVertices = part->get_number_of_vertices();
						auto& _vertexData = part->access_vertex_data();
						auto& _vertexFormat = part->get_vertex_format();
						int stride = _vertexFormat.get_stride();
						int8* normalPtr = _vertexData.get_data() + _vertexFormat.get_normal_offset();
						for (int mainVrtIdx = 0; mainVrtIdx < _numberOfVertices; ++mainVrtIdx, normalPtr += stride)
						{
							if (_vertexFormat.is_normal_packed())
							{
								VectorPacked* outNormal = (VectorPacked*)(normalPtr);
								outNormal->set_as_vertex_normal(-outNormal->get_as_vertex_normal());
							}
							else
							{
								Vector3& normal = *(Vector3*)(normalPtr);
								normal = -(normal);
							}
						}
					}
				}
			}
		}
	}
	
	return result;
}

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_invert_normals_data()
{
	return new InvertNormalsData();
}

//

REGISTER_FOR_FAST_CAST(InvertNormalsData);

bool InvertNormalsData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	return result;
}
