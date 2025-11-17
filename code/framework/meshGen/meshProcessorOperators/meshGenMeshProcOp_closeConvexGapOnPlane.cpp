#include "meshGenMeshProcOp_closeConvexGapOnPlane.h"

#include "..\meshGenGenerationContext.h"

#include "..\modifiers\meshGenModifierUtils.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\types\vectorPacked.h"
#include "..\..\..\core\system\video\indexFormatUtils.h"
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

REGISTER_FOR_FAST_CAST(CloseConvexGapOnPlane);

bool CloseConvexGapOnPlane::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	dir.load_from_xml_child_node(_node, TXT("dir"));
	point.load_from_xml_child_node(_node, TXT("point"));
	toPoint.load_from_xml_child_node(_node, TXT("toPoint"));

	if (dir.is_value_set())
	{
		if (dir.get_value().is_zero())
		{
			dir.set(Vector3::xAxis);
		}
	}

	u.load_from_xml(_node, TXT("u"));

	result &= onClosure.load_from_xml_child_node(_node, _lc, TXT("onClosure"));

	return result;
}

bool CloseConvexGapOnPlane::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= onClosure.prepare_for_game(_library, _pfgContext);

	return result;
}

bool CloseConvexGapOnPlane::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	float threshold = Modifiers::Utils::mesh_threshold();

	Optional<float> u;
	if (this->u.is_set())
	{
		u = this->u.get(_context);
	}

	Vector3 normal = dir.get(_context).normal();
	Vector3 loc = point.get(_context);
	Plane plane = Plane(normal, loc);
	Vector3 perp = normal.get_any_perpendicular();

	Optional<Vector3> toPoint; if (this->toPoint.is_set()) toPoint = this->toPoint.get(_context);

	Array<int> newlyCreatedMPIndices;

	for_every(pIdx, _actOnMeshPartIndices)
	{
		::Meshes::Mesh3DPart* part = _context.get_parts()[*pIdx].get();
		auto& _vertexFormat = part->get_vertex_format();
		if (_vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
		{
			int _numberOfVertices = part->get_number_of_vertices();
			struct VertexInfo
			{
				int idx = 0;
				float angle = 0.0f; // used to sort
				Vector3 loc = Vector3::zero;

				static inline int compare(void const* _a, void const* _b)
				{
					VertexInfo const & a = *plain_cast<VertexInfo>(_a);
					VertexInfo const & b = *plain_cast<VertexInfo>(_b);
					if (a.angle < b.angle) return A_BEFORE_B;
					if (a.angle > b.angle) return B_BEFORE_A;
					return A_AS_B;
				}
			};
			ARRAY_PREALLOC_SIZE(VertexInfo, closeOnVIs, _numberOfVertices);
			Vector3 centre = Vector3::zero;
			auto& _vertexData = part->access_vertex_data();
			int stride = _vertexFormat.get_stride();
			int locOffset = _vertexFormat.get_location_offset();
			for (int mainVrtIdx = 0; mainVrtIdx < _numberOfVertices; ++mainVrtIdx)
			{
				Vector3 vLoc = *(Vector3*)(_vertexData.get_data() + mainVrtIdx * stride + locOffset);
				float inFront = plane.get_in_front(vLoc);
				if (abs(inFront) <= threshold)
				{
					VertexInfo vi;
					vi.idx = mainVrtIdx;
					vi.loc = vLoc;
					closeOnVIs.push_back(vi);
					centre += vLoc;
				}
			}
			if (!closeOnVIs.is_empty())
			{
				centre = centre / (float)closeOnVIs.get_size();

				Transform planeTransform = look_matrix(centre, perp, normal).to_transform();

				for_every(vi, closeOnVIs)
				{
					vi->angle = Rotator3::get_yaw(planeTransform.location_to_local(vi->loc).to_vector2());
				}

				sort(closeOnVIs);

				auto const vFormat = part->get_vertex_format();
				auto const iFormat = part->get_index_format();
				if (iFormat.get_element_size() == ::System::IndexElementSize::None)
				{
					error(TXT("requires indexed part"));
				}
				else if (part->get_primitive_type() != ::Meshes::Primitive::Triangle)
				{
					error(TXT("works only with triangles"));
				}
				else
				{
					int vStride = vFormat.get_stride();
					int iStride = iFormat.get_stride();
					Array<int8> vertexData;
					Array<int8> elementData;
					vertexData.make_space_for(vStride * (closeOnVIs.get_size() + 1));
					elementData.make_space_for(iStride * (closeOnVIs.get_size() - 2 + 1) * 3);

					// copy vertices (if we have toPoint set, use first vertex for rest of the data)
					int vrtCount = 0;
					if (toPoint.is_set())
					{
						vertexData.grow_size(vStride);
						an_assert((part->get_vertex_data().get_size() % vStride) == 0);
						memory_copy(&vertexData[vrtCount * vStride], &part->get_vertex_data()[closeOnVIs.get_first().idx * vStride], vStride);
						::System::VertexFormatUtils::store_location(vertexData.begin(), vrtCount, vFormat, toPoint.get());
						if (u.is_set() && vFormat.get_texture_uv() != ::System::VertexFormatTextureUV::None)
						{
							::System::VertexFormatUtils::store(vertexData.begin() + vrtCount * vStride + vFormat.get_texture_uv_offset(), vFormat.get_texture_uv_type(), u.get());
						}
						if (vFormat.get_normal() == ::System::VertexFormatNormal::Yes)
						{
							::System::VertexFormatUtils::store_normal(vertexData.begin(), vrtCount, vFormat, normal);
						}
						++vrtCount;
					}
					for_every(vi, closeOnVIs)
					{
						vertexData.grow_size(vStride);
						an_assert((part->get_vertex_data().get_size() % vStride) == 0);
						memory_copy(&vertexData[vrtCount * vStride], &part->get_vertex_data()[vi->idx * vStride], vStride);
						if (u.is_set() && vFormat.get_texture_uv() != ::System::VertexFormatTextureUV::None)
						{
							::System::VertexFormatUtils::store(vertexData.begin() + vrtCount * vStride + vFormat.get_texture_uv_offset(), vFormat.get_texture_uv_type(), u.get());
						}
						if (vFormat.get_normal() == ::System::VertexFormatNormal::Yes)
						{
							::System::VertexFormatUtils::store_normal(vertexData.begin(), vrtCount, vFormat, normal);
						}
						++vrtCount;
					}

					int triCount = 0;
					int elCount = 0;

					int firstIdx = 0;
					// for toPoint we require to do fool loop, hence the extra first step that uses last vertex (the very first vertex is the centre
					for (int i = toPoint.is_set()? 1 : 2; i < vrtCount; ++i)
					{
						int secondIdx = i > 1? i - 1 : vrtCount - 1;
						int thirdIdx = i;

						elementData.grow_size(3 * iStride);
						::System::IndexFormatUtils::set_value(iFormat, elementData.get_data(), elCount, firstIdx); ++elCount;
						::System::IndexFormatUtils::set_value(iFormat, elementData.get_data(), elCount, secondIdx); ++elCount;
						::System::IndexFormatUtils::set_value(iFormat, elementData.get_data(), elCount, thirdIdx); ++elCount;
						++triCount;
					}

					RefCountObjectPtr<::Meshes::Mesh3DPart> copyForClosure(part->create_copy());
					an_assert(copyForClosure->get_vertex_format().is_ok_to_be_used());
					int newIdx = _context.store_part_just_as(copyForClosure.get(), part);
					newlyCreatedMPIndices.push_back(newIdx);

					copyForClosure->load_data(vertexData.get_data(), elementData.get_data(), part->get_primitive_type(), vrtCount, elCount, vFormat, iFormat);
				}
			}
		}
	}

	_newMeshPartIndices.add_from(newlyCreatedMPIndices);

	result &= onClosure.process(_context, _instance, newlyCreatedMPIndices, _newMeshPartIndices);
	
	return result;
}
