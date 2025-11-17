#include "meshGenMeshProcOp_deform.h"

#include "..\meshGenGenerationContext.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\system\video\vertexFormatUtils.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace MeshProcessorOperators;

//

REGISTER_FOR_FAST_CAST(Deform);

bool Deform::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	voxelSize.load_from_xml(_node, TXT("voxelSize"));
	deformDist.load_from_xml(_node, TXT("deformDist"));
	deformVector.load_from_xml(_node, TXT("alongVector"));
	deformNormal.load_from_xml(_node, TXT("againstNormal"));

	if (!deformDist.is_set())
	{
		error_loading_xml(_node, TXT("not all parameters for deform were defined"));
		result = false;
	}

	deformPoint.load_from_xml(_node, TXT("point"));
	deformDir.load_from_xml(_node, TXT("dir"));
	deformDistanceFull.load_from_xml(_node, TXT("distanceFull"));
	deformDistanceFull.load_from_xml(_node, TXT("fullAtDistance"));
	deformDistanceNone.load_from_xml(_node, TXT("distanceNone"));
	deformDistanceNone.load_from_xml(_node, TXT("noneAtDistance"));

	return result;
}

bool Deform::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	if (deformDist.is_set())
	{
		float deformDist = this->deformDist.get(_context);
		float voxelSize = 0.1f;
		Optional<Vector3> deformVector;
		Optional<Vector3> deformNormal;
		Optional<Vector3> deformPoint;
		Optional<Vector3> deformDir;
		Optional<float> deformDistanceFull;
		Optional<float> deformDistanceNone;
		// all or none
		if (this->deformPoint.is_set())
		{
			deformPoint = this->deformPoint.get(_context);
			if (this->deformDir.is_set())
			{
				deformDir = this->deformDir.get(_context).normal();
			}
			if (this->deformDistanceFull.is_set())
			{
				deformDistanceFull = this->deformDistanceFull.get(_context);
			}
			else
			{
				deformDistanceFull = 0.0f;
			}
			if (this->deformDistanceNone.is_set())
			{
				deformDistanceNone = this->deformDistanceNone.get(_context);
			}
		}
		if (this->voxelSize.is_set())
		{
			voxelSize = this->voxelSize.get(_context);
		}
		if (this->deformVector.is_set())
		{
			deformVector = this->deformVector.get(_context);
		}
		if (this->deformNormal.is_set())
		{
			deformNormal = this->deformNormal.get(_context);
		}
		Random::Generator rgBase = _context.get_random_generator();
		for_every(pIdx, _actOnMeshPartIndices)
		{
			::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
			auto& vertexFormat = part->get_vertex_format();
			if (vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
			{
				int numberOfVertices = part->get_number_of_vertices();
				auto* vertexData = part->access_vertex_data().get_data();
				for (int vIdx = 0; vIdx < numberOfVertices; ++vIdx)
				{
					Vector3 loc = ::System::VertexFormatUtils::restore_location(vertexData, vIdx, vertexFormat);
					Vector3 locVox = loc / voxelSize;
					VectorInt3 loIdx = TypeConversions::Normal::f_i_cells(locVox);
					VectorInt3 hiIdx = loIdx + VectorInt3::one;

					float useDeformDist = deformDist;

					if (deformPoint.is_set())
					{
						if (deformDir.is_set())
						{
							float alongDir = Vector3::dot(loc - deformPoint.get(), deformDir.get());
							useDeformDist *= deformDistanceFull.get() > 0.0f? clamp(alongDir / deformDistanceFull.get(), 0.0f, 1.0f) : (alongDir > 0.0f? 1.0f : 0.0f);
							if (deformDistanceNone.is_set())
							{
								useDeformDist *= clamp(1.0f - alongDir / deformDistanceNone.get(), 0.0f, 1.0f);
							}
						}
						else
						{
							float dist = (loc - deformPoint.get()).length();
							if (deformDistanceFull.get() > 0.0f)
							{
								useDeformDist *= clamp(dist / deformDistanceFull.get(), 0.0f, 1.0f);
							}
							if (deformDistanceNone.is_set())
							{
								useDeformDist *= clamp(1.0f - dist / deformDistanceNone.get(), 0.0f, 1.0f);
							}
						}
					}
					if (useDeformDist > 0.0f)
					{
						struct DeformOffset
						{
							static Vector3 calculate(VectorInt3 const& _coord, float deformDist, Optional<Vector3> const & deformVector, Optional<Vector3> const & deformNormal, Random::Generator const& _rgBase)
							{
								Random::Generator rg = _rgBase;
								rg.advance_seed(_coord.x * 5 + 97534 + (_coord.y + 3) * 2, -63973 + (_coord.z - 9) * 17);
								Vector3 offset;
								if (deformVector.is_set())
								{
									offset = deformVector.get() * (rg.get_bool() ? -1.0f : 1.0f);
								}
								else
								{
									offset.x = rg.get_float(-1.0f, 1.0f);
									offset.y = rg.get_float(-1.0f, 1.0f);
									offset.z = rg.get_float(-1.0f, 1.0f);
									if (deformNormal.is_set())
									{
										offset = offset.drop_using(deformNormal.get());
									}
								}
								offset = offset.normal();
								offset *= deformDist * rg.get_float(0.0f, 1.0f);
								return offset;
							}
						};
						// we need 8 deformation offsets to interpolate between
						Vector3 deformOffset[8];
						deformOffset[0] = DeformOffset::calculate(VectorInt3(loIdx.x, loIdx.y, loIdx.z), useDeformDist, deformVector, deformNormal, rgBase);
						deformOffset[1] = DeformOffset::calculate(VectorInt3(hiIdx.x, loIdx.y, loIdx.z), useDeformDist, deformVector, deformNormal, rgBase);
						deformOffset[2] = DeformOffset::calculate(VectorInt3(loIdx.x, hiIdx.y, loIdx.z), useDeformDist, deformVector, deformNormal, rgBase);
						deformOffset[3] = DeformOffset::calculate(VectorInt3(hiIdx.x, hiIdx.y, loIdx.z), useDeformDist, deformVector, deformNormal, rgBase);
						deformOffset[4] = DeformOffset::calculate(VectorInt3(loIdx.x, loIdx.y, hiIdx.z), useDeformDist, deformVector, deformNormal, rgBase);
						deformOffset[5] = DeformOffset::calculate(VectorInt3(hiIdx.x, loIdx.y, hiIdx.z), useDeformDist, deformVector, deformNormal, rgBase);
						deformOffset[6] = DeformOffset::calculate(VectorInt3(loIdx.x, hiIdx.y, hiIdx.z), useDeformDist, deformVector, deformNormal, rgBase);
						deformOffset[7] = DeformOffset::calculate(VectorInt3(hiIdx.x, hiIdx.y, hiIdx.z), useDeformDist, deformVector, deformNormal, rgBase);

						Vector3 locVoxPt = locVox - loIdx.to_vector3();
						locVoxPt.x = clamp(locVoxPt.x, 0.0f, 1.0f);
						locVoxPt.y = clamp(locVoxPt.y, 0.0f, 1.0f);
						locVoxPt.z = clamp(locVoxPt.z, 0.0f, 1.0f);
					
						Vector3 deformationOffset =
							lerp(locVoxPt.z, lerp(locVoxPt.y, lerp(locVoxPt.x, deformOffset[0], deformOffset[1]),
															  lerp(locVoxPt.x, deformOffset[2], deformOffset[3])),
											 lerp(locVoxPt.y, lerp(locVoxPt.x, deformOffset[4], deformOffset[5]),
															  lerp(locVoxPt.x, deformOffset[6], deformOffset[7])));

						::System::VertexFormatUtils::store_location(vertexData, vIdx, vertexFormat, loc + deformationOffset);
					}
				}
			}
		}
	}

	return result;
}
