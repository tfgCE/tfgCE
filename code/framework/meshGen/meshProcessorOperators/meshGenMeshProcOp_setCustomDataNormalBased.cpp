#include "meshGenMeshProcOp_setCustomDataNormalBased.h"

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

REGISTER_FOR_FAST_CAST(SetCustomDataNormalBased);

bool SetCustomDataNormalBased::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= name.load_from_xml(_node, TXT("name"));

	normal.load_from_xml(_node, TXT("normal"));

	for_every(n, _node->children_named(TXT("sameDir")))
	{
		result &= valueSameDir.load_from_xml(n);
	}
	for_every(n, _node->children_named(TXT("oppositeDir")))
	{
		result &= valueOppositeDir.load_from_xml(n);
	}

	if (auto* attr = _node->get_attribute(TXT("method")))
	{
		if (attr->get_as_string() == TXT("angle")) method = Method::UseAngle;
		if (attr->get_as_string() == TXT("dot product") ||
			attr->get_as_string() == TXT("dotProduct") || 
			attr->get_as_string() == TXT("dot")) method = Method::UseDotProduct;
	}

	error_loading_xml_on_assert(normal.is_set(), result, _node, TXT("\"normal\" not provided"));

	return result;
}

template <typename Value>
static Optional<Value> prepare_value_for_set_custom_data_normal_based(GenerationContext& _context, ValueDef<Value> const & _vSame, ValueDef<Value> const & _vOp, float useSame)
{
	if (_vSame.is_set() || _vOp.is_set())
	{
		Optional<Value> vSame;
		Optional<Value> vOp;
		if (_vSame.is_set()) vSame = _vSame.get(_context);
		if (_vOp.is_set())   vOp = _vOp.get(_context);
		if (!vSame.is_set()) { vSame = vOp.get(); }
		if (!vOp.is_set())   { vOp = vSame.get(); }
		return lerp<Value>(useSame, vOp.get(), vSame.get());
	}
	return NP;
}

bool SetCustomDataNormalBased::process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const
{
	bool result = base::process(_context, _instance, _actOnMeshPartIndices, _newMeshPartIndices);

	Name cdName = this->name.get_with_default(_context, Name::invalid());

	struct Utils
	{
		static void fill(GenerationContext& _context, System::VertexFormatUtils::FillCustomDataParams& params, Value const& pointA, Value const& pointB, float useSame)
		{
			Optional<float> asFloat = prepare_value_for_set_custom_data_normal_based<float>(_context, pointA.asFloat, pointB.asFloat, useSame);
			if (asFloat.is_set())
			{
				params.with_float(asFloat.get());
			}
			Optional<int> asInt = prepare_value_for_set_custom_data_normal_based<int>(_context, pointA.asInt, pointB.asInt, useSame);
			if (asInt.is_set())
			{
				params.with_int(asInt.get());
			}
			Optional<Vector3> asVector3 = prepare_value_for_set_custom_data_normal_based<Vector3>(_context, pointA.asVector3, pointB.asVector3, useSame);
			if (asVector3.is_set())
			{
				params.with_vector3(asVector3.get());
			}
			Optional<Vector4> asVector4 = prepare_value_for_set_custom_data_normal_based<Vector4>(_context, pointA.asVector4, pointB.asVector4, useSame);
			if (asVector4.is_set())
			{
				params.with_vector4(asVector4.get());
			}
		}
	};

	Vector3 useNormal = normal.get(_context);
	useNormal.normalise();

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
				int norOffset = vertexFormat.get_normal_offset();
				for (int vrtIdx = 0; vrtIdx < numberOfVertices; ++vrtIdx, currentVertexData += stride)
				{
					Vector3 vNormal = System::VertexFormatUtils::restore_normal(currentVertexData + norOffset, vertexFormat);
					vNormal.normalise();
					
					float dot = clamp(Vector3::dot(useNormal, vNormal), -1.0f, 1.0f);

					float useSame = 0.0f;
					if (method == Method::UseAngle)
					{
						float angleMatch = acos_deg(dot);
						useSame = remap_value(angleMatch, 0.0f, 180.0f, 1.0f, 0.0f, true);
					}
					else
					{
						an_assert(method == Method::UseDotProduct);
						useSame = remap_value(dot, 1.0f, -1.0f, 1.0f, 0.0f, true);
					}

					System::VertexFormatUtils::FillCustomDataParams params;
					Utils::fill(_context, params, valueSameDir, valueOppositeDir, useSame);
					System::VertexFormatUtils::fill_custom_data(vertexFormat, currentVertexData, stride, *cd, params);
				}
			}
		}
	}

	return result;
}

//--

bool SetCustomDataNormalBased::Value::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	result &= asFloat.load_from_xml(_node, TXT("float"));
	result &= asInt.load_from_xml(_node, TXT("int"));
	result &= asVector3.load_from_xml(_node, TXT("vector3"));
	result &= asVector4.load_from_xml(_node, TXT("vector4"));

	return result;
}

