#include "meshGenMeshProcOp_setCustomDataBetweenPoints.h"

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

REGISTER_FOR_FAST_CAST(SetCustomDataBetweenPoints);

bool SetCustomDataBetweenPoints::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= name.load_from_xml(_node, TXT("name"));

	for_every(n, _node->children_named(TXT("pointA")))
	{
		result &= pointA.load_from_xml(n);
	}
	for_every(n, _node->children_named(TXT("pointB")))
	{
		result &= pointB.load_from_xml(n);
	}

	return result;
}

template <typename Value>
static Optional<Value> prepare_value_for_set_custom_data_between_points(GenerationContext& _context, ValueDef<Value> const & pointA, ValueDef<Value> const & pointB, float pt)
{
	if (pointA.is_set() || pointB.is_set())
	{
		Optional<Value> valueA;
		Optional<Value> valueB;
		if (pointA.is_set()) valueA = pointA.get(_context);
		if (pointB.is_set()) valueB = pointB.get(_context);
		if (!valueA.is_set()) { valueA = valueB.get(); }
		if (!valueB.is_set()) { valueB = valueA.get(); }
		return lerp<Value>(pt, valueA.get(), valueB.get());
	}
	return NP;
}

bool SetCustomDataBetweenPoints::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	Name cdName = this->name.get_with_default(_context, Name::invalid());

	struct Utils
	{
		static void fill(GenerationContext& _context, System::VertexFormatUtils::FillCustomDataParams& params, Point const& pointA, Point const& pointB, float pt)
		{
			Optional<float> asFloat = prepare_value_for_set_custom_data_between_points<float>(_context, pointA.asFloat, pointB.asFloat, pt);
			if (asFloat.is_set())
			{
				params.with_float(asFloat.get());
			}
			Optional<int> asInt = prepare_value_for_set_custom_data_between_points<int>(_context, pointA.asInt, pointB.asInt, pt);
			if (asInt.is_set())
			{
				params.with_int(asInt.get());
			}
			Optional<Vector3> asVector3 = prepare_value_for_set_custom_data_between_points<Vector3>(_context, pointA.asVector3, pointB.asVector3, pt);
			if (asVector3.is_set())
			{
				params.with_vector3(asVector3.get());
			}
			Optional<Vector4> asVector4 = prepare_value_for_set_custom_data_between_points<Vector4>(_context, pointA.asVector4, pointB.asVector4, pt);
			if (asVector4.is_set())
			{
				params.with_vector4(asVector4.get());
			}
		}
	};

	Vector3 aAt = pointA.at.get(_context);
	Vector3 bAt = pointB.at.get(_context);

	Segment segA2B(aAt, bAt);

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
				int8* currentVertexData = vertexData.get_data();
				int locOffset = vertexFormat.get_location_offset();
				for (int vrtIdx = 0; vrtIdx < numberOfVertices; ++vrtIdx, currentVertexData += stride)
				{
					Vector3 vLoc = *(Vector3*)(currentVertexData + locOffset);

					float pt = segA2B.get_t(vLoc);

					System::VertexFormatUtils::FillCustomDataParams params;
					Utils::fill(_context, params, pointA, pointB, pt);
					System::VertexFormatUtils::fill_custom_data(vertexFormat, currentVertexData, stride, *cd, params);
				}
			}
		}
	}

	return result;
}

//--

bool SetCustomDataBetweenPoints::Point::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	result &= at.load_from_xml(_node, TXT("at"));

	result &= asFloat.load_from_xml(_node, TXT("float"));
	result &= asInt.load_from_xml(_node, TXT("int"));
	result &= asVector3.load_from_xml(_node, TXT("vector3"));
	result &= asVector4.load_from_xml(_node, TXT("vector4"));

	return result;
}

