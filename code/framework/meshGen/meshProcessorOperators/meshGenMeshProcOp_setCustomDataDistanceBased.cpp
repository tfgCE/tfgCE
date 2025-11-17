#include "meshGenMeshProcOp_setCustomDataDistanceBased.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenValueDefImpl.inl"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\system\video\vertexFormatUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace MeshProcessorOperators;

//

REGISTER_FOR_FAST_CAST(SetCustomDataDistanceBased);

bool SetCustomDataDistanceBased::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= name.load_from_xml(_node, TXT("name"));
	result &= distanceMul.load_from_xml(_node, TXT("distanceMul"));
	result &= normaliseDistanceTo.load_from_xml(_node, TXT("normaliseDistanceTo"));

	result &= useCrossSectionVertexCount.load_from_xml(_node, TXT("useCrossSectionVertexCount"));
	result &= fromPoint.load_from_xml(_node, TXT("fromPoint"));

	return result;
}

bool SetCustomDataDistanceBased::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	Name cdName = this->name.get_with_default(_context, Name::invalid());
	float distanceMul = this->distanceMul.get_with_default(_context, 1.0f);
	float normaliseDistanceTo = this->normaliseDistanceTo.get_with_default(_context, 0.0f);

	System::VertexFormatUtils::FillCustomDataParams params;
	params.add_to_existing();

	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
		auto& vertexData = part->access_vertex_data();
		auto& vertexFormat = part->get_vertex_format();
		if (auto* cd = vertexFormat.get_custom_data(cdName))
		{
			int cdOffset = vertexFormat.get_custom_data_offset(cdName);
			if (cdOffset != NONE)
			{
				auto numberOfVertices = part->get_number_of_vertices();
				int stride = vertexFormat.get_stride();
				int locOffset = vertexFormat.get_location_offset();
				if (this->useCrossSectionVertexCount.is_set())
				{
					int crossSectionVertexCount = this->useCrossSectionVertexCount.get(_context);
					if (crossSectionVertexCount > 0)
					{
						float totalDistance = 0.0f;
						for (int passIdx = normaliseDistanceTo != 0.0f ? 0 : 1; passIdx < 2; ++passIdx)
						{
							Optional<Vector3> prevCentre;
							float distance = 0.0f;
							int8* currentVertexData = vertexData.get_data();
							for (int vrtIdx = 0; vrtIdx < numberOfVertices;)
							{
								int8* cscvData = currentVertexData;
								Vector3 centre = Vector3::zero;
								int vertexCountNow = 0;
								for (int csIdx = 0; csIdx < crossSectionVertexCount && vrtIdx + csIdx < numberOfVertices; ++csIdx, cscvData += stride, ++vertexCountNow)
								{
									Vector3 vLoc = *(Vector3*)(currentVertexData + locOffset);
									centre += vLoc;
								}

								if (vertexCountNow > 0)
								{
									centre /= (float)(vertexCountNow);

									if (prevCentre.is_set())
									{
										distance += (centre - prevCentre.get()).length();
									}

									prevCentre = centre;

									if (passIdx == 1)
									{
										params.with_float(distance * distanceMul);
										if (normaliseDistanceTo != 0.0f)
										{
											params.with_float(totalDistance != 0.0f ? normaliseDistanceTo * distance / totalDistance : 0.0f);
										}
										System::VertexFormatUtils::fill_custom_data(vertexFormat, currentVertexData, stride * vertexCountNow, *cd, params);
									}

									vrtIdx += vertexCountNow;
									currentVertexData += stride * vertexCountNow;
								}
								else
								{
									break;
								}
							}

							totalDistance = distance;
						}
					}
				}
				if (this->fromPoint.is_set())
				{
					Vector3 fromPoint = this->fromPoint.get(_context);
					auto numberOfVertices = part->get_number_of_vertices();
					int stride = vertexFormat.get_stride();
					int8* currentVertexData = vertexData.get_data();
					int locOffset = vertexFormat.get_location_offset();
					for (int vrtIdx = 0; vrtIdx < numberOfVertices; ++vrtIdx, currentVertexData += stride)
					{
						Vector3 vLoc = *(Vector3*)(currentVertexData + locOffset);
					
						float distance = (vLoc - fromPoint).length();

						params.with_float(distance * distanceMul);
						System::VertexFormatUtils::fill_custom_data(vertexFormat, currentVertexData, stride, *cd, params);
					}
				}
			}
		}
	}

	return result;
}
