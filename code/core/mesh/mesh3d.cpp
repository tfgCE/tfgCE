#include "mesh3d.h"

#include "..\serialisation\importer.h"

#include "mesh3dBuilder.h"
#include "iMesh3dRenderBonesProvider.h"
#include "..\system\video\iMaterialInstanceProvider.h"
#include "..\system\video\indexFormatUtils.h"
#include "..\system\video\materialInstance.h"
#include "..\system\video\vertexFormatUtils.h"

#include "..\performance\performanceUtils.h"

#include "..\mainSettings.h"

#include "..\serialisation\serialiser.h"

#include "mesh3dRenderingUtils.h"

#ifdef AN_DEVELOPMENT
#include "..\system\input.h"
#include "..\debugKeys.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace ::System;
using namespace Meshes;

//

REGISTER_FOR_FAST_CAST(Mesh3D);

#define USE_OFFSET(_pointer, _offset) (void*)(((int8*)_pointer) + _offset)

Importer<Mesh3D, Mesh3DImportOptions> Mesh3D::s_importer;

//

bool Mesh3DImportCombinePart::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	if (_node->get_name() == TXT("by"))
	{
		byName = _node->get_string_attribute(TXT("name"), byName);
		byIndex = _node->get_int_attribute(TXT("index"), byIndex);
		if (byName.is_empty() && byIndex == NONE)
		{
			error_loading_xml(_node, TXT("invalid combine part definition"));
			result = false;
		}
	}
	else if (_node->get_name() == TXT("name"))
	{
		ifNameContains = _node->get_string_attribute(TXT("contains"), ifNameContains);
		if (ifNameContains.is_empty())
		{
			error_loading_xml(_node, TXT("invalid combine part definition"));
			result = false;
		}
	}
	else if (_node->get_type() != IO::XML::NodeType::Comment &&
			 _node->get_type() != IO::XML::NodeType::NodeCommentedOut &&
			 _node->get_type() != IO::XML::NodeType::Text)
	{
		error_loading_xml(_node, TXT("invalid combine part definition"));
		result = false;
	}
	
	return result;
}

//

Mesh3DImportOptions::Mesh3DImportOptions()
: skeleton(nullptr)
, skipUV(false)
, skipColour(! MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes())
, skipSkinning(false)
, floatUV(false)
, importScale(Vector3::one)
, combineAllParts(false)
{
}

bool Mesh3DImportOptions::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("importFromNode")))
	{
		importFromNode = attr->get_as_string();
	}
	skipUV = _node->get_bool_attribute_or_from_child_presence(TXT("skipUV"), skipUV);
	skipColour = _node->get_bool_attribute_or_from_child_presence(TXT("skipColour"), skipColour);
	skipSkinning = _node->get_bool_attribute_or_from_child_presence(TXT("skipSkinning"), skipSkinning);
	floatUV = _node->get_bool_attribute_or_from_child_presence(TXT("floatUV"), floatUV);
	setU.load_from_xml(_node, TXT("setU"));
	if (IO::XML::Node const * childNode = _node->first_child_named(TXT("importFromPlacement")))
	{
		importFromPlacement.set_if_not_set(Transform::identity);
		importFromPlacement.access().load_from_xml(childNode);
	}
	if (IO::XML::Node const * childNode = _node->first_child_named(TXT("importFromAutoCentre")))
	{
		importFromAutoCentre.set_if_not_set(Vector3::one);
		importFromAutoCentre.access().load_from_xml(childNode);
	}
	if (IO::XML::Node const * childNode = _node->first_child_named(TXT("importFromAutoPt")))
	{
		importFromAutoPt.set_if_not_set(Vector3(0.5f, 0.5f, 0.0f));
		importFromAutoPt.access().load_from_xml(childNode);
	}
	if (IO::XML::Node const * childNode = _node->first_child_named(TXT("importScaleToFit")))
	{
		importScaleToFit.set_if_not_set(Vector3::zero);
		importScaleToFit.access().load_from_xml(childNode);
	}

	if (IO::XML::Node const * childNode = _node->first_child_named(TXT("placeAfterImport")))
	{
		placeAfterImport.set_if_not_set(Transform::identity);
		placeAfterImport.access().load_from_xml(childNode);
	}
	{
		float const isDef = -1000.0f;
		float is = _node->get_float_attribute_or_from_child(TXT("importScale"), isDef);
		if (is != isDef)
		{
			importScale = is * Vector3::one;
		}
		importScale.load_from_xml_child_node(_node, TXT("importScale"), TXT("x"), TXT("y"), TXT("z"), TXT("axis"), true);
	}
	if (IO::XML::Node const * node = _node->first_child_named(TXT("importCombine")))
	{
		// by default import all parts if "importCombine" is present
		combineAllParts = node->get_bool_attribute(TXT("all"), true);
		for_every(combineIntoPartNode, node->children_named(TXT("combine")))
		{
			combineParts.push_back(List<Mesh3DImportCombinePart>());
			auto & combineIntoPartList = combineParts.get_last();
			for_every(combinePartNode, combineIntoPartNode->all_children())
			{
				Mesh3DImportCombinePart combinePart;
				if (combinePart.load_from_xml(combinePartNode))
				{
					combineIntoPartList.push_back(combinePart);
				}
				else if (combinePartNode->get_type() != IO::XML::NodeType::Text &&
						 combinePartNode->get_type() != IO::XML::NodeType::Comment &&
						 combinePartNode->get_type() != IO::XML::NodeType::NodeCommentedOut)
				{
					error_loading_xml(combinePartNode, TXT("error loading combine parts"));
					result = false;
				}
			}
		}
	}
	if (auto* attr = _node->get_attribute(TXT("ignoreImportingFromNodesWithPrefix")))
	{
		ignoreImportingFromNodesWithPrefix.push_back(attr->get_as_string());
	}
	for_every(node, _node->children_named(TXT("ignoreImportingFromNodes")))
	{
		if (auto* attr = node->get_attribute(TXT("withPrefix")))
		{
			ignoreImportingFromNodesWithPrefix.push_back(attr->get_as_string());
		}
	}
		
	return result;
}

//

void Mesh3D::initialise_static()
{
	Mesh3D::importer().register_file_type_with_options(String(TXT("obj")),
		[](String const & _fileName, Meshes::Mesh3DImportOptions const & _options)
		{
			Mesh3DBuilder builder;
			if (builder.load_from_file(_fileName, _options))
			{
				Mesh3D* mesh = new Mesh3D();
				mesh->load_builder(&builder);
				return mesh;
			}
			else
			{
				return (Mesh3D*)nullptr;
			}
		}
	);
}

void Mesh3D::close_static()
{
	Mesh3D::importer().unregister(String(TXT("obj")));
}

Mesh3D* Mesh3D::create_new()
{
	return new Mesh3D();
}

Mesh3D::Mesh3D()
: usage(Usage::Static)
, canBeCombined(true)
, canBeCombinedAsImmovable(true)
, updatedDynamicaly(false)
{
}

Mesh3D::~Mesh3D()
{
	remove_all_parts();
}

Mesh3D::Mesh3D(Mesh3D const & _source)
{
	an_assert(false, TXT("not allowed"));
}

Mesh3D& Mesh3D::operator=(Mesh3D const & _source)
{
	an_assert(false, TXT("not allowed"));
	return *this;
}

void Mesh3D::remove_all_parts()
{
	close();
	parts.clear();
}

void Mesh3D::close()
{
	for_every_ref(part, parts)
	{
		part->close();
	}
}

void Mesh3D::render(Video3D * _v3d, IMaterialInstanceProvider const * _materialProvider, Mesh3DRenderingContext const & _renderingContext) const
{
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
	MEASURE_PERFORMANCE_CONTEXT(debugMeshName);
#endif
	DRAW_OBJECT_NAME(debugMeshName.to_char());
#else
	NO_DRAW_OBJECT_NAME();
#endif
#ifdef INSPECT_MESH_RENDERING
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
	output(TXT(" render %S"), debugMeshName.to_char());
#endif
#endif
	for_every_ref(part, parts)
	{
		if (part->is_empty())
		{
			continue;
		}
		int idx = for_everys_index(part);
#ifdef INSPECT_MESH_RENDERING
		output(TXT("  render part %i"), for_everys_index(part));
#endif
#ifdef AN_N_RENDERING
		if (::System::Input::get()->has_key_been_pressed(System::Key::N))
		{
			LogInfoContext log;
			log.log(TXT("rendering %S, part %i"), get_debug_mesh_name(), idx);
			log.output_to_output();
		}
#endif
		ShaderProgramInstance const * shaderProgramInstance = nullptr;
		Material const * usingMaterial = nullptr;
		if (_materialProvider)
		{
			if (auto const * materialInstance = _materialProvider->get_material_instance_for_rendering(idx))
			{
				shaderProgramInstance = materialInstance->get_shader_program_instance();
				usingMaterial = materialInstance->get_material();
			}
			else if (auto const * fallbackShaderProgramInstance = _materialProvider->get_fallback_shader_program_instance())
			{
				shaderProgramInstance = fallbackShaderProgramInstance;
			}
		}
		part->render(_v3d, shaderProgramInstance, usingMaterial, _renderingContext);
	}
	NO_DRAW_OBJECT_NAME();
}

void Mesh3D::prune_vertices()
{
	for_every_ref(part, parts)
	{
		part->prune_vertices();
	}
}

void Mesh3D::optimise_vertices()
{
	for_every_ref(part, parts)
	{
		part->optimise_vertices();
	}
}

void Mesh3D::build_buffers()
{
	for_every_ref(part, parts)
	{
		part->build_buffer();
	}
}

Mesh3DPart* Mesh3D::update_part_data(int _partIdx, void const * _vertexData, Primitive::Type _primitiveType, int32 _numberOfPrimitives, ::System::VertexFormat const & _vertexFormat, bool _fillElementArray)
{
	an_assert(parts.is_index_valid(_partIdx));
	if (parts.is_index_valid(_partIdx))
	{
		Mesh3DPart* part = parts[_partIdx].get();
		part->update_data(_vertexData, _primitiveType, _numberOfPrimitives, _vertexFormat, _fillElementArray);
		return part;
	}
	else
	{
		return nullptr;
	}
}

Mesh3DPart* Mesh3D::update_part_data(int _partIdx, void const * _vertexData, void const * _elementData, Primitive::Type _primitiveType, int32 _numberOfVertices, int32 _numberOfElements, ::System::VertexFormat const & _vertexFormat, ::System::IndexFormat const & _indexFormat)
{
	an_assert(parts.is_index_valid(_partIdx));
	if (parts.is_index_valid(_partIdx))
	{
		Mesh3DPart* part = parts[_partIdx].get();
		part->update_data(_vertexData, _elementData, _primitiveType, _numberOfVertices, _numberOfElements, _vertexFormat, _indexFormat);
		return part;
	}
	else
	{
		return nullptr;
	}
}

Mesh3DPart* Mesh3D::load_copy_part_data(Mesh3DPart const * _part)
{
	RefCountObjectPtr<Mesh3DPart> copy(_part->create_copy());
	parts.push_back(copy);
	boundingBox.clear(); // invalidate
	return copy.get();
}

Mesh3DPart* Mesh3D::load_part_data(void const * _vertexData, Primitive::Type _primitiveType, int32 _numberOfPrimitives, VertexFormat const & _vertexFormat, bool _fillElementArray)
{
	RefCountObjectPtr<Mesh3DPart> part(new Mesh3DPart(canBeCombined, updatedDynamicaly));
	parts.push_back(part);
	part->load_data(_vertexData, _primitiveType, _numberOfPrimitives, _vertexFormat, _fillElementArray);
	boundingBox.clear(); // invalidate
	return part.get();
}

Mesh3DPart* Mesh3D::load_part_data(void const * _vertexData, void const * _elementData, Primitive::Type _primitiveType, int32 _numberOfVertices, int32 _numberOfElements, VertexFormat const & _vertexFormat, IndexFormat const & _indexFormat)
{
	RefCountObjectPtr<Mesh3DPart> part(new Mesh3DPart(canBeCombined, updatedDynamicaly));
	parts.push_back(part);
	part->load_data(_vertexData, _elementData, _primitiveType, _numberOfVertices, _numberOfElements, _vertexFormat, _indexFormat);
	boundingBox.clear(); // invalidate
	return part.get();
}

void Mesh3D::load_builder(Mesh3DBuilder const * _builder)
{
	int partsCount = _builder->get_number_of_parts();
	for_count(int, idx, partsCount)
	{
		load_part_data(_builder->get_data(idx), _builder->get_primitive_type(), _builder->get_number_of_primitives(idx), _builder->get_vertex_format());
	}
}

void Mesh3D::render_data(Video3D * _v3d, ShaderProgramInstance const * _shaderProgramInstance, System::Material const * _usingMaterial, Mesh3DRenderingContext const & _renderingContext, void const * _data, Primitive::Type _primitiveType, int32 _numberOfPrimitives, VertexFormat const & _vertexFormat)
{
	uint32 numberOfVertices = Primitive::primitive_count_to_vertex_count(_primitiveType, _numberOfPrimitives);
	an_assert(_vertexFormat.is_ok_to_be_used(), TXT("calculate stride and offset"));

	Video3DStateStore stateStore;
	Mesh3DRenderingUtils::render_worker_begin(_v3d, stateStore, _shaderProgramInstance, _usingMaterial, _renderingContext, _data, _primitiveType, numberOfVertices, _vertexFormat);
	Mesh3DRenderingUtils::render_worker_render_non_indexed(_v3d, _primitiveType, numberOfVertices);
	Mesh3DRenderingUtils::render_worker_end(_v3d, stateStore, _shaderProgramInstance, _usingMaterial, _renderingContext, _data, _primitiveType, numberOfVertices, _vertexFormat);
}

void Mesh3D::render_builder(Video3D * _v3d, Mesh3DBuilder const * _builder, IMaterialInstanceProvider const * _materialProvider, Mesh3DRenderingContext const & _renderingContext)
{
	int partsCount = _builder->get_number_of_parts();
	for_count(int, idx, partsCount)
	{
		ShaderProgramInstance const * shaderProgramInstance = nullptr;
		Material const * material = nullptr;
		if (_materialProvider)
		{
			if (auto const * materialInstance = _materialProvider->get_material_instance_for_rendering(idx))
			{
				shaderProgramInstance = materialInstance->get_shader_program_instance();
				material = materialInstance->get_material();
			}
			else if (auto const * fallbackShaderProgramInstance = _materialProvider->get_fallback_shader_program_instance())
			{
				shaderProgramInstance = fallbackShaderProgramInstance;
			}
		}
		render_data(_v3d, shaderProgramInstance, material, _renderingContext, _builder->get_data(idx), _builder->get_primitive_type(), _builder->get_number_of_primitives(idx), _builder->get_vertex_format());
	}
}

#define VER_BASE 0
#define CURRENT_VERSION VER_BASE

bool Mesh3D::serialise(Serialiser & _serialiser)
{
	version = CURRENT_VERSION;
	bool result = SerialisableResource::serialise(_serialiser);
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid mesh version"));
		return false;
	}
	serialise_data(_serialiser, usage);
	serialise_data(_serialiser, digestedSkeleton);
	serialise_data(_serialiser, canBeCombined);
	serialise_data(_serialiser, canBeCombinedAsImmovable);
	serialise_data(_serialiser, updatedDynamicaly);
	int partsCount = parts.get_size();
	serialise_data(_serialiser, partsCount);
	if (_serialiser.is_reading())
	{
		remove_all_parts();
		int partsLeft = partsCount;
		while (partsLeft)
		{
			parts.push_back(RefCountObjectPtr<Mesh3DPart>(new Mesh3DPart(canBeCombined, updatedDynamicaly)));
			--partsLeft;
		}
		memory_leak_suspect;
		parts.set_size(partsCount);
		forget_memory_leak_suspect;
	}
	for_every_ref(part, parts)
	{
		result &= part->serialise(_serialiser, version);
	}
	boundingBox.clear(); // invalidate
	return result;
}

void Mesh3D::fuse_parts(Array<Array<FuseMesh3DPart> > const & _fromParts, Array<Array<RemapBoneArray const *>>* _remapBones, bool _keepPartsThatAreNotMentioned)
{
	Array<Array<FuseMesh3DPart>> fromParts = _fromParts;
	//an_assert(!_remapBones, TXT("can't use combining everything else if remapBones is in use"));
	// if parts are not mentioned in fromParts, add them there
	for_every_ref(part, parts)
	{
		int partIdx = for_everys_index(part);
		bool mentioned = false;
		for_every(fromPartsSet, fromParts)
		{
			for_every(fusePart, *fromPartsSet)
			{
				if (fusePart->part == part)
				{
					fusePart->own = true;
					mentioned = true;
					break;
				}
			}
			if (mentioned)
			{
				break;
			}
		}
		if (!mentioned)
		{
			if (! _keepPartsThatAreNotMentioned)
			{
				delete part;
			}
			else
			{
				if (!fromParts.is_index_valid(partIdx))
				{
					fromParts.set_size(partIdx + 1);
				}
				fromParts[partIdx].push_front(FuseMesh3DPart(part));
				fromParts[partIdx].get_first().own = true;
			}
		}
	}
	
	parts.clear();
	
	// go through each set
	for_every(fromPartSet, fromParts)
	{
		// for each set create new part
		RefCountObjectPtr<Mesh3DPart> intoPart;
		for_every(fromFusePart, *fromPartSet)
		{
			RefCountObjectPtr<Mesh3DPart> fromPart = fromFusePart->part;
			if (!fromPart->is_empty()) // ignore fusing empty parts
			{
				// such part still exists
				if (! intoPart.get())
				{
					intoPart = new Mesh3DPart(canBeCombined, updatedDynamicaly);
				}
				{
					// check if formats match
					if (intoPart->is_empty() ||
						(::System::VertexFormatUtils::do_match(intoPart->get_vertex_format(), fromPart->get_vertex_format()) &&
						 ::System::IndexFormatUtils::do_match(intoPart->get_index_format(), fromPart->get_index_format())))
					{
						// add to existing part
						int newAddedVertexDataSize;
						void* newVertices;
						intoPart->add_from_part(fromPart.get(), !fromFusePart->own && _remapBones? (*_remapBones)[for_everys_index(fromPartSet)][for_everys_index(fromFusePart)] : nullptr, &newAddedVertexDataSize, &newVertices);

						if (fromFusePart->placement.is_set())
						{
							VertexFormatUtils::apply_transform_to_data(intoPart->get_vertex_format(), newVertices, newAddedVertexDataSize, fromFusePart->placement.get());
						}
					}
					else
					{
						error(TXT("parts don't match (index/vertex formats), won't be combined"));
					}
				}
			}
			if (fromFusePart->own)
			{
				fromFusePart->part.clear();
			}
		}
		if (intoPart.get())
		{
			while (parts.get_size() < for_everys_index(fromPartSet))
			{
				// empty part
				RefCountObjectPtr<Mesh3DPart> emptyPart(new Mesh3DPart(canBeCombined, updatedDynamicaly));
				emptyPart->be_empty_part();
				parts.push_back(emptyPart);
			}
			parts.push_back(intoPart);
		}
	}
	
	boundingBox.clear(); // invalidate
}

bool Mesh3D::check_if_has_valid_skinning_data() const
{
	for_every_ref(part, parts)
	{
		if (!part->check_if_has_valid_skinning_data())
		{
			warn(TXT("part %i (all %i) is invalid"), for_everys_index(part), parts.get_size());
			return false;
		}
	}
	return true;
}

Mesh3D* Mesh3D::create_copy() const
{
	Mesh3D* copy = new Mesh3D();

	copy->usage = usage;
	// no need to copy digestedSkeleton
	copy->canBeCombined = canBeCombined;
	copy->canBeCombinedAsImmovable = canBeCombinedAsImmovable;
	copy->updatedDynamicaly = updatedDynamicaly;

	for_every_ref(part, parts)
	{
		copy->parts.push_back(RefCountObjectPtr<Mesh3DPart>(part->create_copy()));
	}

	copy->boundingBox.clear(); // invalidate

	return copy;
}

void Mesh3D::update_bounding_box()
{
	if (boundingBox.is_set())
	{
		return;
	}
	boundingBox = Range3::empty;
	for_every_ref(part, parts)
	{
		boundingBox.access().include(part->calculate_bounding_box());
	}

	{
		Range3 bbox = get_bounding_box();
		Vector3 v;
		v.x = max(bbox.x.length(), max(abs(bbox.x.min), abs(bbox.x.max)));
		v.y = max(bbox.y.length(), max(abs(bbox.y.min), abs(bbox.y.max)));
		v.z = max(bbox.z.length(), max(abs(bbox.z.min), abs(bbox.z.max)));
		sizeForLOD = v.length();
	}
}

int Mesh3D::calculate_triangles() const
{
	int triCount = 0;
	for_every_ref(part, parts)
	{
		triCount += part->get_number_of_tris();
	}
	return triCount;
}

void Mesh3D::log(LogInfoContext& _log) const
{
	_log.log(TXT("mesh3d"));
	{
		LOG_INDENT(_log);
		_log.log(TXT("parts                    : %i"), parts.get_size());
		_log.log(TXT("usage                    : %S"), Usage::to_char(usage));
		_log.log(TXT("canBeCombined            : %S"), canBeCombined? TXT("yes") : TXT("no"));
		_log.log(TXT("canBeCombinedAsImmovable : %S"), canBeCombinedAsImmovable ? TXT("yes") : TXT("no"));
		_log.log(TXT("updatedDynamicaly        : %S"), updatedDynamicaly ? TXT("yes") : TXT("no"));
		for_every_ref(part, parts)
		{
			_log.log(TXT("part %i"), for_everys_index(part));
			LOG_INDENT(_log);
			part->log(_log);
		}
	}
}

void Mesh3D::mark_to_load_to_gpu() const
{
	for_every_ref(part, parts)
	{
		part->mark_to_load_to_gpu();
	}
}
