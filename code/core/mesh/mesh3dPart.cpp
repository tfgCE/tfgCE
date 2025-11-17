#include "mesh3dPart.h"

#include "mesh3dBuilder.h"
#include "iMesh3dRenderBonesProvider.h"
#include "vertexSkinningInfo.h"
#include "..\concurrency\scopedSpinLock.h"
#include "..\containers\arrayStack.h"
#include "..\system\systemInfo.h"
#include "..\system\timeStamp.h"
#include "..\system\video\iMaterialInstanceProvider.h"
#include "..\system\video\indexFormatUtils.h"
#include "..\system\video\materialInstance.h"
#include "..\system\video\vertexFormatUtils.h"
#include "..\system\video\viewFrustum.h"
#include "..\system\video\videoClipPlaneStackImpl.inl"
#include "..\types\vectorPacked.h"

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

#ifdef AN_DEVELOPMENT
#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define SHOW_PRUNE_VERTICES_STATS
//#define SHOW_OPTIMISE_VERTICES_STATS
//#define CHECK_VERTEX_COMPARISONS
//#define DEBUG_OPTIMISE_VERTICES
#endif
#endif

#ifndef BUILD_PUBLIC_RELEASE
#define OUTPUT_INVALID_SKIN
//#define OUTPUT_LOADING_BUFFER
//#define OUTPUT_LOADING_BUFFER_PERFORMANCE
#endif

//

using namespace ::System;
using namespace Meshes;

//

#define USE_OFFSET(_pointer, _offset) (void*)(((int8*)_pointer) + _offset)

Mesh3DPart::StaticData* Mesh3DPart::staticData = nullptr;

void Mesh3DPart::initialise_static()
{
	an_assert(!staticData);
	staticData = new StaticData();
}

void Mesh3DPart::close_static()
{
	an_assert(staticData);
	delete_and_clear(staticData);
}

Mesh3DPart::Mesh3DPart(bool _canBeCombined, bool _updatedDynamicaly)
: SafeObject<Mesh3DPart>(this)
, canBeCombined(_canBeCombined)
, updatedDynamicaly(_updatedDynamicaly)
, dataAvailableToBuildBufferObjects(false)
, newVertexDataAvailable(false)
, newElementDataAvailable(false)
, numberOfVertices(0)
, numberOfElements(0)
, vertexBufferObject(0)
, elementBufferObject(0)
{
}

Mesh3DPart::~Mesh3DPart()
{
	make_safe_object_unavailable();

	close();
}

void Mesh3DPart::load_one_part_to_gpu()
{
	assert_rendering_thread();

	an_assert(staticData);
	if (staticData->partsToLoadToGPU.is_empty())
	{
		return;
	}

	if (staticData->partsToLoadToGPULock.acquire_if_not_locked())
	{
		{
			MEASURE_PERFORMANCE(getOneMesh3DPartToLoad);

			while (!staticData->partsToLoadToGPU.is_empty())
			{
				RefCountObjectPtr<Mesh3DPart> part; // hold it while we load it
				part = staticData->partsToLoadToGPU.get_first().get();
				staticData->partsToLoadToGPU.pop_front();
				if (part.get())
				{
					if (part->dataAvailableToBuildBufferObjects &&
						(part->vertexBufferObject == 0 ||
						 part->newVertexDataAvailable ||
						 part->newElementDataAvailable))
					{
						{
							MEASURE_PERFORMANCE(loadMesh3DPart);
#ifdef OUTPUT_LOADING_BUFFER
							output(TXT("[Mesh3DPart] early load (v:%i e:%i)"), part->numberOfVertices, part->numberOfElements);
#endif
#ifdef OUTPUT_LOADING_BUFFER_PERFORMANCE
							::System::TimeStamp ts;
#endif
							part->update_buffer_if_required();
#ifdef OUTPUT_LOADING_BUFFER_PERFORMANCE
							float timeTaken = ts.get_time_since();
							output(TXT("[Mesh3DPart] time taken to load %.3fms (v:%i e:%i)"), timeTaken * 1000.0f, part->numberOfVertices, part->numberOfElements);
#endif
						}
						break;
					}
				}
			}
		}
		staticData->partsToLoadToGPULock.release();
	}
}

void Mesh3DPart::mark_to_load_to_gpu() const
{
	an_assert(!updatedDynamicaly, TXT("don't push to load in the background if updated dynamically"));
	an_assert(staticData);
	Concurrency::ScopedSpinLock lock(staticData->partsToLoadToGPULock);
	staticData->partsToLoadToGPU.push_back(::SafePtr<Mesh3DPart>(this));
}

Mesh3DPart* Mesh3DPart::create_copy() const
{
	Mesh3DPart* copy = new Mesh3DPart(canBeCombined, updatedDynamicaly);

	if (emptyPart)
	{
		copy->load_data(nullptr, primitiveType, 0, vertexFormat);
		copy->be_empty_part(emptyPart);
	}
	else
	{
		if (indexFormat.get_element_size() != ::System::IndexElementSize::None)
		{
			copy->load_data(vertexData.get_data(), elementData.get_data(), primitiveType, numberOfVertices, numberOfElements, vertexFormat, indexFormat);
		}
		else
		{
			copy->load_data(vertexData.get_data(), primitiveType, Primitive::vertex_count_to_primitive_count(primitiveType, numberOfVertices), vertexFormat);
		}
	}

	return copy;
}

void Mesh3DPart::clear()
{
	vertexData.clear();
	elementData.clear();
	numberOfVertices = 0;
	numberOfElements = 0;
}

void Mesh3DPart::close()
{
	if (is_rendering_thread())
	{
		if (vertexBufferObject != 0)
		{
			assert_rendering_thread();
			DIRECT_GL_CALL_ glDeleteBuffers(1, &vertexBufferObject); AN_GL_CHECK_FOR_ERROR
			gpu_memory_info_freed(GPU_MEMORY_INFO_TYPE_BUFFER, vertexBufferObject);
			if (auto* v3d = Video3D::get())
			{
				v3d->on_vertex_buffer_destroyed(vertexBufferObject);
			}
#ifdef AN_SYSTEM_INFO
			System::SystemInfo::destroyed_vertex_buffer();
#endif
			vertexBufferObject = 0;
		}
		if (elementBufferObject != 0)
		{
			assert_rendering_thread();
			DIRECT_GL_CALL_ glDeleteBuffers(1, &elementBufferObject); AN_GL_CHECK_FOR_ERROR
			gpu_memory_info_freed(GPU_MEMORY_INFO_TYPE_BUFFER, elementBufferObject);
#ifdef AN_SYSTEM_INFO
			System::SystemInfo::destroyed_element_buffer();
#endif
			elementBufferObject = 0;
		}
	}
	else
	{
		if (auto* v3d = Video3D::get())
		{
			if (vertexBufferObject != 0)
			{
				v3d->defer_buffer_to_destroy(vertexBufferObject);
				v3d->on_vertex_buffer_destroyed(vertexBufferObject);
#ifdef AN_SYSTEM_INFO
				System::SystemInfo::destroyed_vertex_buffer();
#endif
			}
			vertexBufferObject = 0;
			if (elementBufferObject != 0)
			{
				v3d->defer_buffer_to_destroy(elementBufferObject);
#ifdef AN_SYSTEM_INFO
				System::SystemInfo::destroyed_element_buffer();
#endif
			}
			elementBufferObject = 0;
		}
	}
}

void Mesh3DPart::load_vertex_data_into_buffer_object(bool _update)
{
	MEASURE_PERFORMANCE_COLOURED(load_vertex_data_into_buffer_object, Colour::white);

#ifdef OUTPUT_LOADING_BUFFER
	output(TXT("[Mesh3DPart] load vertex data (v:%i)"), numberOfVertices);
#endif

	assert_rendering_thread();

#ifdef AN_SYSTEM_INFO
	::System::TimeStamp ts;
#endif
	Video3D::get()->unbind_and_send_buffers_and_vertex_format();
	Video3D::get()->bind_and_send_vertex_buffer_to_load_data(vertexBufferObject);
	// load data
	if (_update)
	{
		DIRECT_GL_CALL_ glBufferSubData(GL_ARRAY_BUFFER, 0, vertexData.get_size(), vertexData.get_data()); AN_GL_CHECK_FOR_ERROR
	}
	else
	{
		DIRECT_GL_CALL_ glBufferData(GL_ARRAY_BUFFER, vertexData.get_size(), vertexData.get_data(), updatedDynamicaly ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW); AN_GL_CHECK_FOR_ERROR
		gpu_memory_info_allocated(GPU_MEMORY_INFO_TYPE_BUFFER, vertexBufferObject, vertexData.get_size());
	}
	// unbind
	Video3D::get()->bind_and_send_vertex_buffer_to_load_data(0);
#ifdef AN_SYSTEM_INFO
	System::SystemInfo::loaded_vertex_buffer(vertexData.get_size(), ts.get_time_since());
#endif

	newVertexDataAvailable = false;

#ifdef OUTPUT_LOADING_BUFFER
	output(TXT("[Mesh3DPart] done"));
#endif
}

void Mesh3DPart::load_element_data_into_buffer_object(bool _update)
{
	MEASURE_PERFORMANCE_COLOURED(load_element_data_into_buffer_object, Colour::white);

#ifdef OUTPUT_LOADING_BUFFER
	output(TXT("[Mesh3DPart] load element data (e:%i)"), numberOfElements);
#endif

	assert_rendering_thread();

#ifdef AN_SYSTEM_INFO
	::System::TimeStamp ts;
#endif
	Video3D::get()->unbind_and_send_buffers_and_vertex_format();
	Video3D::get()->bind_and_send_element_buffer_to_load_data(elementBufferObject);
	// load data
	if (_update)
	{
		DIRECT_GL_CALL_ glBufferSubData(GL_ARRAY_BUFFER, 0, elementData.get_size(), elementData.get_data()); AN_GL_CHECK_FOR_ERROR
	}
	else
	{
		DIRECT_GL_CALL_ glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementData.get_size(), elementData.get_data(), updatedDynamicaly ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW); AN_GL_CHECK_FOR_ERROR
		gpu_memory_info_allocated(GPU_MEMORY_INFO_TYPE_BUFFER, elementBufferObject, elementData.get_size());
	}
	// unbind
	Video3D::get()->bind_and_send_element_buffer_to_load_data(0);
#ifdef AN_SYSTEM_INFO
	System::SystemInfo::loaded_element_buffer(vertexData.get_size(), ts.get_time_since());
#endif

	newElementDataAvailable = false;

#ifdef OUTPUT_LOADING_BUFFER
	output(TXT("[Mesh3DPart] done"));
#endif
}

void Mesh3DPart::build_buffer()
{
	MEASURE_PERFORMANCE_COLOURED(build_buffer, Colour::white);

#ifdef OUTPUT_LOADING_BUFFER
	output(TXT("[Mesh3DPart] build buffer (v:%i e:%i)"), numberOfVertices, numberOfElements);
#endif

	an_assert(dataAvailableToBuildBufferObjects);

	assert_rendering_thread();
	
	// setup stride and offsets
	vertexFormat.calculate_stride_and_offsets();
	indexFormat.calculate_stride();

	close();

	// make sure vertex format is valid
	an_assert(vertexFormat.is_ok_to_be_used(), TXT("stride and offsets not calculated"));
	an_assert(elementData.is_empty() || indexFormat.is_ok_to_be_used(), TXT("stride not calculated"));

	// generate buffer and bind
	DIRECT_GL_CALL_ glGenBuffers(1, &vertexBufferObject); AN_GL_CHECK_FOR_ERROR
#ifdef AN_SYSTEM_INFO
	System::SystemInfo::created_vertex_buffer();
#endif
	load_vertex_data_into_buffer_object(false);

	if (!elementData.is_empty())
	{
		// generate buffer and bind
		DIRECT_GL_CALL_ glGenBuffers(1, &elementBufferObject); AN_GL_CHECK_FOR_ERROR
#ifdef AN_SYSTEM_INFO
		System::SystemInfo::created_element_buffer();
#endif
		load_element_data_into_buffer_object(false);
	}

	if (MainSettings::global().should_discard_data_after_building_buffer_objects() && !canBeCombined && !updatedDynamicaly)
	{
		vertexData.clear_and_prune();
		elementData.clear_and_prune();
		dataAvailableToBuildBufferObjects = false;
	}
	else
	{
		// prune data - we don't want to waste space
		vertexData.prune();
		elementData.prune();
	}

#ifdef OUTPUT_LOADING_BUFFER
	output(TXT("[Mesh3DPart] done"));
#endif
}

void Mesh3DPart::update_buffer_if_required()
{
	assert_rendering_thread();

	if (vertexBufferObject == 0 && vertexData.is_empty())
	{
		warn_dev_ignore(TXT("not an empty part but should be!"));
		be_empty_part();
	}
		

	if (emptyPart)
	{
		return;
	}

	// fail-safe to build buffers (to build them on main thread)
	if (vertexBufferObject == 0)
	{
		//vertexFormat.debug_vertex_data(numberOfVertices, vertexData.get_data());
		build_buffer();
	}

	if (vertexBufferObject != 0 && newVertexDataAvailable && updatedDynamicaly)
	{
		load_vertex_data_into_buffer_object(true);
	}

	if (elementBufferObject != 0 && newElementDataAvailable && updatedDynamicaly)
	{
		load_element_data_into_buffer_object(true);
	}
}

void Mesh3DPart::render(Video3D * _v3d, ShaderProgramInstance const * _shaderProgramInstance, System::Material const * _usingMaterial, Mesh3DRenderingContext const & _renderingContext)
{
	if (emptyPart)
	{
		return;
	}

	assert_rendering_thread();
	
	if (! _renderingContext.useExistingBlend && // maybe we forced blend
		_renderingContext.renderBlending.is_set() && _usingMaterial &&
		(_renderingContext.renderBlending.get() ^ _usingMaterial->does_any_blending()))
	{
		// no reason to render this one
		return;
	}

#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
	MEASURE_PERFORMANCE(renderPart);
#endif

	update_buffer_if_required();

	Video3DStateStore stateStore;

	Mesh3DRenderingUtils::render_worker_begin(_v3d, stateStore, _shaderProgramInstance, _usingMaterial, _renderingContext, (void*)0, primitiveType, numberOfVertices, vertexFormat, &vertexBufferObject, &elementBufferObject);
#ifdef AN_N_RENDERING
	if (::System::Input::get()->has_key_been_pressed(System::Key::N))
	{
		LogInfoContext log;
		log.log(TXT("render!"));
		log.output_to_output();
	}
#endif
	if (elementBufferObject != 0)
	{
		Mesh3DRenderingUtils::render_worker_render_indexed(_v3d, primitiveType, numberOfElements, indexFormat);
	}
	else
	{
		Mesh3DRenderingUtils::render_worker_render_non_indexed(_v3d, primitiveType, numberOfVertices);
	}
	Mesh3DRenderingUtils::render_worker_end(_v3d, stateStore, _shaderProgramInstance, _usingMaterial, _renderingContext, (void*)0, primitiveType, numberOfVertices, vertexFormat, &vertexBufferObject, &elementBufferObject);
}

// for very small objects it doesn't make sense to build elements array
#define MIN_PRIMITIVES_TO_BUILD_ELEMENT_ARRAY 6

void Mesh3DPart::update_data(void const * _vertexData, Primitive::Type _primitiveType, int32 _numberOfPrimitives, ::System::VertexFormat const & _vertexFormat, bool _fillElementArray)
{
	todo_note(TXT("update in place if possible, for now just always close and load_data"));
	close();
	load_data(_vertexData, _primitiveType, _numberOfPrimitives, _vertexFormat, _fillElementArray);
}

void Mesh3DPart::update_data(void const * _vertexData, void const * _elementData, Primitive::Type _primitiveType, int32 _numberOfVertices, int32 _numberOfElements, ::System::VertexFormat const & _vertexFormat, ::System::IndexFormat const & _indexFormat)
{
	an_assert(!emptyPart);

	if (updatedDynamicaly &&
		vertexBufferObject != 0 &&
		elementBufferObject != 0)
	{
		primitiveType = _primitiveType;
		vertexFormat = _vertexFormat;
		vertexFormat.calculate_stride_and_offsets();
		indexFormat = _indexFormat;
		indexFormat.calculate_stride();
		numberOfVertices = _numberOfVertices;
		numberOfElements = _numberOfElements;
		int vertexDataSize = numberOfVertices * vertexFormat.get_stride();
		int elementDataSize = numberOfElements * indexFormat.get_stride();
		memory_leak_suspect;
		vertexData.set_size(vertexDataSize);
		elementData.set_size(elementDataSize);
		forget_memory_leak_suspect;
		memory_copy(vertexData.get_data(), _vertexData, vertexDataSize);
		memory_copy(elementData.get_data(), _elementData, elementDataSize);

		dataAvailableToBuildBufferObjects = true; // just to mark that we have it
		newVertexDataAvailable = true;
		newElementDataAvailable = true;

		return;
	}
	close();
	load_data(_vertexData, _elementData, _primitiveType, _numberOfVertices, _numberOfElements, _vertexFormat, _indexFormat);
}

void Mesh3DPart::load_data(void const * _vertexData, Primitive::Type _primitiveType, int32 _numberOfPrimitives, VertexFormat const & _vertexFormat, bool _fillElementArray)
{
	an_assert(!emptyPart);

	primitiveType = _primitiveType;

	vertexFormat = _vertexFormat;
	vertexFormat.calculate_stride_and_offsets();
#ifdef AN_DEBUG
	if (MainSettings::global().should_force_reimporting() && _numberOfPrimitives > MIN_PRIMITIVES_TO_BUILD_ELEMENT_ARRAY)
	{
		indexFormat = index_format().with_element_size(::System::IndexElementSize::FourBytes); // because we're going to save that to disk
	}
	else
	{
		indexFormat = index_format().not_used(); // building is slow in debug (to gain performance on objects that are built every run - door holes for example)
	}
#else
	if (_numberOfPrimitives > MIN_PRIMITIVES_TO_BUILD_ELEMENT_ARRAY)
	{
		indexFormat = index_format().not_used();
	}
	else
	{
		indexFormat = index_format().with_element_size(::System::IndexElementSize::FourBytes);
	}
#endif
	indexFormat.calculate_stride();

	an_assert(vertexBufferObject == 0, TXT("vertex buffer already created, close first"));
	an_assert(elementBufferObject == 0, TXT("element buffer already created, close first"));

	numberOfVertices = Primitive::primitive_count_to_vertex_count(primitiveType, _numberOfPrimitives);
	uint32 vertexSize = vertexFormat.get_stride();
	uint32 indexSize = indexFormat.get_stride();

	bool dropElementArray = false;

	_fillElementArray &= indexSize != 0; // if there is no index data, don't create element array

	if (_fillElementArray && _vertexData)
	{
		int8 const * data8 = (int8 const *)_vertexData;

		// make enough place
		uint32 vertexCount = 0;
		memory_leak_suspect;
		vertexData.set_size(numberOfVertices * vertexSize);
		elementData.set_size(numberOfVertices * indexSize);
		forget_memory_leak_suspect;
		uint32* element = (uint32*)(elementData.get_data());
		int8 const * inVertexData = data8;
		for (int inVertex = 0; inVertex < numberOfVertices; ++inVertex, ++element, inVertexData += vertexSize)
		{
			bool found = false;
			// check if such vertex already exists in our array
			int8 const * existingVertexData = vertexData.get_data();
			for (uint32 existingVertex = 0; existingVertex < vertexCount; ++existingVertex, existingVertexData += vertexSize)
			{
				if (memory_compare(existingVertexData, inVertexData, vertexSize))
				{
					*element = existingVertex;
					found = true;
					break;
				}
			}
			if (!found)
			{
				*element = vertexCount;
				// add vertex
				memory_copy(&vertexData[vertexCount * vertexSize], inVertexData, vertexSize);
				++vertexCount;
			}
		}

		// make vertex data smaller
		memory_leak_suspect;
		vertexData.set_size(vertexCount * vertexSize);
		vertexData.prune();
		forget_memory_leak_suspect;

		numberOfElements = numberOfVertices;
		if (numberOfVertices == vertexCount)
		{
			// because we didn't gain that much
			dropElementArray = true;
		}
		else
		{
			numberOfVertices = vertexCount;
		}
	}
	else
	{
		dropElementArray = true;
	}
	
	if (dropElementArray)
	{
		memory_leak_suspect;
		vertexData.set_size(numberOfVertices * vertexSize);
		forget_memory_leak_suspect;
		if (_vertexData) memory_copy(vertexData.get_data(), _vertexData, vertexData.get_data_size());
		elementData.clear_and_prune();
		numberOfElements = 0;
		indexFormat.not_used();
	}

	dataAvailableToBuildBufferObjects = true;
	newVertexDataAvailable = true;
	newElementDataAvailable = true;
}

#undef MIN_PRIMITIVES_TO_BUILD_ELEMENT_ARRAY

void Mesh3DPart::load_data(void const * _vertexData, void const * _elementData, Primitive::Type _primitiveType, int32 _numberOfVertices, int32 _numberOfElements, VertexFormat const & _vertexFormat, IndexFormat const & _indexFormat)
{
	an_assert(!emptyPart);

	primitiveType = _primitiveType;

	vertexFormat = _vertexFormat;
	vertexFormat.calculate_stride_and_offsets();

	indexFormat = _indexFormat;
	indexFormat.calculate_stride();

	an_assert(vertexBufferObject == 0, TXT("vertex buffer already created, close first"));
	an_assert(elementBufferObject == 0, TXT("element buffer already created, close first"));

	numberOfVertices = _numberOfVertices;
	numberOfElements = _numberOfElements;
	uint32 vertexSize = vertexFormat.get_stride();
	uint32 elementSize = indexFormat.get_stride();

	memory_leak_suspect;
	vertexData.set_size(numberOfVertices * vertexSize);
	if (_vertexData) memory_copy(vertexData.get_data(), _vertexData, vertexData.get_data_size());
	elementData.set_size(numberOfElements * elementSize);
	if (_elementData) memory_copy(elementData.get_data(), _elementData, elementData.get_data_size());
	forget_memory_leak_suspect;

	dataAvailableToBuildBufferObjects = true;
	newVertexDataAvailable = true;
	newElementDataAvailable = true;
}

bool Mesh3DPart::serialise(Serialiser & _serialiser, int _version)
{
	bool result = true;

	serialise_data(_serialiser, canBeCombined);
	serialise_data(_serialiser, updatedDynamicaly);

	serialise_data(_serialiser, emptyPart);
	serialise_data(_serialiser, primitiveType);
	serialise_data(_serialiser, vertexFormat);
	serialise_data(_serialiser, indexFormat);

	serialise_data(_serialiser, numberOfVertices);
	serialise_data(_serialiser, numberOfElements);

	todo_future(TXT("assumed vertex format is composed of 4 bytes"));
	_serialiser.pre_serialise_adjust_endian_series(vertexData.get_data(), 4, vertexData.get_usize());
	_serialiser.pre_serialise_adjust_endian_series(elementData.get_data(), indexFormat.get_element_size(), elementData.get_usize());
	serialise_plain_data_array(_serialiser, vertexData);
	serialise_plain_data_array(_serialiser, elementData);
	_serialiser.post_serialise_adjust_endian_series(vertexData.get_data(), 4, vertexData.get_usize());
	_serialiser.post_serialise_adjust_endian_series(elementData.get_data(), indexFormat.get_element_size(), elementData.get_usize());

	if (_serialiser.is_reading())
	{
		dataAvailableToBuildBufferObjects = true; // data has been just loaded
		newVertexDataAvailable = ! vertexData.is_empty();
		newElementDataAvailable = ! elementData.is_empty();
	}

	return result;
}

void Mesh3DPart::add_from_part(Mesh3DPart const * _fromPart, RemapBoneArray const * _remapBones, OUT_ int * _newAddedVertexDataSize, OUT_ void ** _newVerticesGoHere)
{
	if (_fromPart->is_empty() ||
		(_fromPart->numberOfVertices == 0 && _fromPart->numberOfElements == 0))
	{
		// nothing to add?
		return;
	}

	if (is_empty())
	{
		// resetup part so it doesn't appear empty
		emptyPart = false;
		vertexFormat = _fromPart->vertexFormat;
		indexFormat = _fromPart->indexFormat;
		primitiveType = _fromPart->primitiveType;
	}
	else
	{
		an_assert(dataAvailableToBuildBufferObjects, TXT("we should hold old data, so we can use it before building a buffer"));
	}

	Mesh3DPart * const intoPart = this;
	// make space for new elements and store pointers where new data will be held
	int prevVertexDataSize = intoPart->vertexData.get_size();
	int prevElementDataSize = intoPart->elementData.get_size();
	int newAddedVertexDataSize = intoPart->vertexFormat.get_stride() * _fromPart->numberOfVertices;
	int newAddedElementDataSize = intoPart->indexFormat.get_stride() * _fromPart->numberOfElements;
	intoPart->vertexData.grow_size(newAddedVertexDataSize);
	intoPart->elementData.grow_size(newAddedElementDataSize);
	void* newVerticesGoHere = &intoPart->vertexData[prevVertexDataSize];
	void* newElementsGoHere = newAddedElementDataSize != 0 ? &intoPart->elementData[prevElementDataSize] : nullptr;

	// just copy indices/elements
	if (newAddedElementDataSize != 0)
	{
		if (::System::IndexFormatUtils::do_match(_fromPart->indexFormat, intoPart->indexFormat))
		{
			// just copy vertices
			memory_copy(newElementsGoHere, _fromPart->elementData.get_data(), newAddedElementDataSize);
		}
		else
		{
			// covert vertices
			::System::IndexFormatUtils::convert_data(_fromPart->indexFormat, _fromPart->elementData.get_data(), _fromPart->elementData.get_size(), intoPart->indexFormat, newElementsGoHere);
		}
		// and increase them by existing vertex count
		::System::IndexFormatUtils::increase_elements_by(intoPart->indexFormat, newElementsGoHere, newAddedElementDataSize, intoPart->numberOfVertices);
	}

	// we have to have some data
	an_assert(newAddedVertexDataSize != 0);

	// covert vertices (actually we should just copy that)
	::System::VertexFormatUtils::convert_data(_fromPart->vertexFormat, _fromPart->vertexData.get_data(), _fromPart->vertexData.get_size(), intoPart->vertexFormat, newVerticesGoHere);

	if (_remapBones)
	{
		::System::VertexFormatUtils::remap_bones(intoPart->vertexFormat, newVerticesGoHere, _fromPart->numberOfVertices * intoPart->vertexFormat.get_stride(), *_remapBones);
	}

	// increase size
	intoPart->numberOfVertices += _fromPart->numberOfVertices;
	intoPart->numberOfElements += _fromPart->numberOfElements;

	// fill output params
	assign_optional_out_param(_newAddedVertexDataSize, newAddedVertexDataSize);
	assign_optional_out_param(_newVerticesGoHere, newVerticesGoHere);

	dataAvailableToBuildBufferObjects = true; // just to mark that we have it
}

int Mesh3DPart::get_number_of_tris() const
{
	if (primitiveType == Primitive::Triangle)
	{
		if (indexFormat.get_element_size() != IndexElementSize::None)
		{
			return numberOfElements / 3;
		}
		else
		{
			return numberOfVertices / 3;
		}
	}
	else if (primitiveType == Primitive::TriangleFan)
	{
		if (indexFormat.get_element_size() != IndexElementSize::None)
		{
			return numberOfElements - 2;
		}
		else
		{
			return numberOfVertices - 2;
		}
	}
	return 0;
}

void Mesh3DPart::for_every_vertex(std::function<void(Vector3 const & _a)> _do) const
{
	if (vertexData.get_data())
	{
		if (primitiveType == Primitive::Triangle)
		{
			auto _vf = vertexFormat;
			_vf.calculate_stride_and_offsets();
			int8 const* locData = vertexData.get_data() + _vf.get_location_offset();
			for_count(int, vIdx, numberOfVertices)
			{
				Vector3 p = *((Vector3*)(locData));

				_do(p);

				locData += _vf.get_stride();
			}
		}
		else
		{
			todo_important(TXT("implement_ for other kinds"));
		}
	}
}

void Mesh3DPart::for_every_triangle(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c)> _do) const
{
	for_every_triangle_worker(false, false, false,
		[_do](int _aIdx, int _bIdx, int _cIdx, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)
	{
		_do(_a, _b, _c);
	});
}

void Mesh3DPart::for_every_triangle_and_simple_skinning(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx)> _do) const
{
	for_every_triangle_worker(true, false, false,
		[_do](int _aIdx, int _bIdx, int _cIdx, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)
	{
		_do(_a, _b, _c, _skinToBoneIdx);
	});
}

void Mesh3DPart::for_every_triangle_and_u(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, float _u)> _do) const
{
	for_every_triangle_worker(false, false, true,
		[_do](int _aIdx, int _bIdx, int _cIdx, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)
	{
		_do(_a, _b, _c, _u);
	});
}

void Mesh3DPart::for_every_triangle_simple_skinning_and_u(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx, float _u)> _do) const
{
	for_every_triangle_worker(true, false, true,
		[_do](int _aIdx, int _bIdx, int _cIdx, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)
	{
		_do(_a, _b, _c, _skinToBoneIdx, _u);
	});
}

void Mesh3DPart::for_every_triangle_and_full_skinning(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning)> _do) const
{
	for_every_triangle_worker(false, true, false,
		[_do](int _aIdx, int _bIdx, int _cIdx, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)
	{
		_do(_a, _b, _c, _aSkinning, _bSkinning, _cSkinning);
	});
}

void Mesh3DPart::for_every_triangle_full_skinning_and_u(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, float _u)> _do) const
{
	for_every_triangle_worker(false, true, true,
		[_do](int _aIdx, int _bIdx, int _cIdx, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)
	{
		_do(_a, _b, _c, _aSkinning, _bSkinning, _cSkinning, _u);
	});
}

void Mesh3DPart::for_every_triangle_indices(std::function<void(int _aIdx, int _bIdx, int _cIdx)> _do) const
{
	for_every_triangle_worker(false, false, false,
		[_do](int _aIdx, int _bIdx, int _cIdx, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)
	{
		_do(_aIdx, _bIdx, _cIdx);
	});
}

void Mesh3DPart::for_every_triangle_indices_and_u(std::function<void(int _aIdx, int _bIdx, int _cIdx, float _u)> _do) const
{
	for_every_triangle_worker(false, false, true,
		[_do](int _aIdx, int _bIdx, int _cIdx, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)
	{
		_do(_aIdx, _bIdx, _cIdx, _u);
	});
}

void Mesh3DPart::for_every_triangle_worker(bool _getSimpleSkinning, bool _getFullSkinning, bool _getU, std::function<void(int _aIdx, int _bIdx, int _cIdx, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)> _do) const
{	
	if (indexFormat.get_element_size() != System::IndexElementSize::None)
	{
		if (elementData.get_data())
		{
			if (primitiveType == Primitive::Triangle)
			{
				auto _vf = vertexFormat;
				_vf.calculate_stride_and_offsets();
				auto _if = indexFormat;
				_if.calculate_stride();
				int8 const * element = elementData.get_data();
				int skinningElementCount = _vf.has_skinning_data() ? _vf.get_skinning_element_count() : 0;
				int skinningOffset = _vf.has_skinning_data() ? (_vf.get_skinning_element_count() > 1 ? _vf.get_skinning_indices_offset() : _vf.get_skinning_index_offset()) : NONE;
				int uOffset = _vf.get_texture_uv() != ::System::VertexFormatTextureUV::None? _vf.get_texture_uv_offset() : NONE;
				for_count(int, triIdx, numberOfElements / 3)
				{
					int idx[3];
					Vector3 p[3];
					p[2] = Vector3::zero;
					int pCount = 3;
					int skinToBoneIdx = NONE;
					VertexSkinningInfo skinningInfo[3];
					Optional<float> u;
					for_count(int, i, pCount)
					{
						int e = IndexFormatUtils::get_value(_if, element); element += _if.get_stride();
						int8 const * vertexAt = &vertexData[e * _vf.get_stride()];
						idx[i] = e;
						p[i] = *((Vector3*)(vertexAt + _vf.get_location_offset()));
						if (_getSimpleSkinning && skinToBoneIdx == NONE && skinningOffset != NONE)
						{
							if (_vf.get_skinning_index_type() == ::System::DataFormatValueType::Uint8)
							{
								skinToBoneIdx = (int)*(uint8*)(vertexAt + skinningOffset);
								if (skinToBoneIdx == (uint8)NONE)
								{
									skinToBoneIdx = NONE;
								}
							}
							else if (_vf.get_skinning_index_type() == ::System::DataFormatValueType::Uint16)
							{
								skinToBoneIdx = (int)*(uint16*)(vertexAt + skinningOffset);
								if (skinToBoneIdx == (uint16)NONE)
								{
									skinToBoneIdx = NONE;
								}
							}
							else if (_vf.get_skinning_index_type() == ::System::DataFormatValueType::Uint32)
							{
								skinToBoneIdx = (int)*(uint32*)(vertexAt + skinningOffset);
								if (skinToBoneIdx == (uint32)NONE)
								{
									skinToBoneIdx = NONE;
								}
							}
							else
							{
								todo_important(TXT("implement_ other skinning index type"));
							}
						}
						if (_getFullSkinning && skinningElementCount > 0)
						{
							int s = 0;
							memory_leak_suspect;
							skinningInfo[i].bones.set_size(skinningElementCount);
							forget_memory_leak_suspect;
							for (; s < skinningElementCount; ++s)
							{
								uint index;
								float weight;
								VertexFormatUtils::get_skinning_info(_vf, vertexAt, s, index, weight);
								skinningInfo[i].bones[s].bone = index;
								skinningInfo[i].bones[s].weight = weight;
							}
						}
						if (_getU && !u.is_set() && uOffset != NONE)
						{
							u = ::System::VertexFormatUtils::restore_as_float(vertexAt + uOffset, _vf.get_texture_uv_type());
						}
					}

					_do(idx[0], idx[1], idx[2], p[0], p[1], p[2], skinningInfo[0], skinningInfo[1], skinningInfo[2], skinToBoneIdx, u.get(0.0f));
				}
			}
			else
			{
				todo_important(TXT("implement_ for other kinds"));
			}
		}
	}
	else
	{
		if (vertexData.get_data())
		{
			if (primitiveType == Primitive::Triangle)
			{
				auto _vf = vertexFormat;
				_vf.calculate_stride_and_offsets();
				int skinningElementCount = _vf.has_skinning_data() ? _vf.get_skinning_element_count() : 0;
				int skinningOffset = _vf.has_skinning_data() ? (_vf.get_skinning_element_count() > 1 ? _vf.get_skinning_indices_offset() : _vf.get_skinning_index_offset()) : NONE;
				int uOffset = _vf.get_texture_uv() != ::System::VertexFormatTextureUV::None ? _vf.get_texture_uv_offset() : NONE;
				int e = 0;
				for_count(int, triIdx, numberOfVertices / 3)
				{
					int idx[3];
					Vector3 p[3];
					p[2] = Vector3::zero;
					int pCount = 3;
					int skinToBoneIdx = NONE;
					VertexSkinningInfo skinningInfo[3];
					Optional<float> u;
					for_count(int, i, pCount)
					{
						int8 const * vertexAt = &vertexData[e * _vf.get_stride()];
						idx[i] = e;
						p[i] = *((Vector3*)(vertexAt + _vf.get_location_offset()));
						++e;
						if (_getSimpleSkinning && skinToBoneIdx == NONE && skinningOffset != NONE)
						{
							if (_vf.get_skinning_index_type() == ::System::DataFormatValueType::Uint8)
							{
								skinToBoneIdx = (int)*(uint8*)(vertexAt + skinningOffset);
								if (skinToBoneIdx == (uint8)NONE)
								{
									skinToBoneIdx = NONE;
								}
							}
							else if (_vf.get_skinning_index_type() == ::System::DataFormatValueType::Uint16)
							{
								skinToBoneIdx = (int)*(uint16*)(vertexAt + skinningOffset);
								if (skinToBoneIdx == (uint16)NONE)
								{
									skinToBoneIdx = NONE;
								}
							}
							else if (_vf.get_skinning_index_type() == ::System::DataFormatValueType::Uint32)
							{
								skinToBoneIdx = (int)*(uint32*)(vertexAt + skinningOffset);
								if (skinToBoneIdx == (uint32)NONE)
								{
									skinToBoneIdx = NONE;
								}
							}
							else
							{
								todo_important(TXT("implement_ other skinning index type"));
							}
						}
						if (_getFullSkinning && skinningElementCount > 0)
						{
							int s = 0;
							memory_leak_suspect;
							skinningInfo[i].bones.set_size(skinningElementCount);
							forget_memory_leak_suspect;
							for (; s < skinningElementCount; ++s)
							{
								uint index;
								float weight;
								VertexFormatUtils::get_skinning_info(_vf, vertexAt, s, index, weight);
								skinningInfo[i].bones[s].bone = index;
								skinningInfo[i].bones[s].weight = weight;
							}
						}
						if (_getU && !u.is_set() && uOffset != NONE)
						{
							u = ::System::VertexFormatUtils::restore_as_float(vertexAt + uOffset, _vf.get_texture_uv_type());
						}
					}

					_do(idx[0], idx[1], idx[2], p[0], p[1], p[2], skinningInfo[0], skinningInfo[1], skinningInfo[2], skinToBoneIdx, u.get(0.0f));
				}
			}
			else
			{
				todo_important(TXT("implement_ for other kinds"));
			}
		}
	}
}

bool Mesh3DPart::check_if_has_valid_skinning_data() const
{
	if (emptyPart)
	{
		return true;
	}
	auto _vf = vertexFormat;
	_vf.calculate_stride_and_offsets();
	if (!_vf.has_skinning_data())
	{
		warn(TXT("no skinning data"));
		return false;
	}
	int skinningOffset = _vf.get_skinning_element_count() > 1 ? _vf.get_skinning_indices_offset() : _vf.get_skinning_index_offset();
	int skinningWeightOffset = _vf.get_skinning_element_count() > 1 ? _vf.get_skinning_weights_offset() : NONE;
	for_count(int, pIdx, numberOfVertices)
	{
		int8 const * vertexAt = &vertexData[pIdx * _vf.get_stride()];
		float weightSum = 0.0f;
		bool invalid = false;
		for_count(int, i, _vf.get_skinning_element_count())
		{
			int skinToBoneIdx = NONE;
			if (_vf.get_skinning_index_type() == ::System::DataFormatValueType::Uint8)
			{
				skinToBoneIdx = (int)*(uint8*)(vertexAt + skinningOffset + sizeof(uint8) * i);
			}
			else if (_vf.get_skinning_index_type() == ::System::DataFormatValueType::Uint16)
			{
				skinToBoneIdx = (int)*(uint16*)(vertexAt + skinningOffset + sizeof(uint16) * i);
			}
			else if (_vf.get_skinning_index_type() == ::System::DataFormatValueType::Uint32)
			{
				skinToBoneIdx = (int)*(uint32*)(vertexAt + skinningOffset + sizeof(uint32) * i);
			}
			else
			{
				todo_important(TXT("implement_ other skinning index type"));
			}
			if (_vf.get_skinning_element_count() > 1)
			{
				float weight = *(float*)(vertexAt + skinningWeightOffset + sizeof(float) * i);
				if (weight > 0.0f && skinToBoneIdx == NONE)
				{
					warn(TXT("not skinned to a bone and skinning weight is non-zero"));
					invalid = true;
				}
				weightSum += weight;
			}
			else
			{
				if (skinToBoneIdx == NONE)
				{
					warn(TXT("should be skinned to a single bone but is skinned to none"));
					invalid = true;
				}
				weightSum = 1.0f;
			}
		}
		if (abs(weightSum - 1.0f) > 0.01f)
		{
			warn(TXT("sum of skinning weights is not 1.0 (%.3f)"), weightSum);
			invalid = true;
		}
		if (invalid)
		{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
			output(TXT("vertex %i"), pIdx);
#endif
			for_count(int, i, _vf.get_skinning_element_count())
			{
				int skinToBoneIdx = NONE;
				if (_vf.get_skinning_index_type() == ::System::DataFormatValueType::Uint8)
				{
					skinToBoneIdx = (int)*(uint8*)(vertexAt + skinningOffset + sizeof(uint8) * i);
				}
				else if (_vf.get_skinning_index_type() == ::System::DataFormatValueType::Uint16)
				{
					skinToBoneIdx = (int)*(uint16*)(vertexAt + skinningOffset + sizeof(uint16) * i);
				}
				else if (_vf.get_skinning_index_type() == ::System::DataFormatValueType::Uint32)
				{
					skinToBoneIdx = (int)*(uint32*)(vertexAt + skinningOffset + sizeof(uint32) * i);
				}
				else
				{
					todo_important(TXT("implement_ other skinning index type"));
				}
				float weight = 1.0f;
				if (_vf.get_skinning_element_count() > 1)
				{
					weight = *(float*)(vertexAt + skinningWeightOffset + sizeof(float) * i);
				}
#ifdef AN_ALLOW_EXTENSIVE_LOGS
				output(TXT("  %i. ->%i : %.3f"), i, skinToBoneIdx, weight);
#endif
			}
			return false;
		}
	}

	return true;
}

void Mesh3DPart::optimise_vertices()
{
	if (primitiveType != Primitive::Triangle ||
		vertexFormat.get_location() != VertexFormatLocation::XYZ ||
		indexFormat.get_element_size() == System::IndexElementSize::None)
	{
		// not supported yet
		return;
	}

	if (numberOfElements > 2000)
	{
		// doesn't work really well with larger meshes
		prune_vertices();
		return;
	}

#ifdef SHOW_OPTIMISE_VERTICES_STATS
	::System::TimeStamp optimiseTS;
#endif

	// used structures and functions
	struct TriangleUsage
	{
		ArrayStatic<int, 3> v;
		bool processed = false;
		bool inUse = false;
		TriangleUsage()
		{
			SET_EXTRA_DEBUG_INFO(v, TXT("Mesh3DPart::optimise_vertices::TriangleUsage.v"));
		}
	};
	struct Edge
	{
		int a, b;
		bool used;
	};
	struct Vertex
	{
		int v;
		Vector3 loc;
		Vertex(int _v) :v(_v) {}

		static int find_org_v(Vector2 const& _flatLoc, Array<Vector2> const& _flatVertices, Array<Vertex> const& _vertices)
		{
			int bestIdx = NONE;
			float bestDistSq = 0.0f;
			for_every(fv, _flatVertices)
			{
				float distSq = (*fv - _flatLoc).length_squared();
				if (bestIdx == NONE || distSq < bestDistSq)
				{
					bestIdx = for_everys_index(fv);
					bestDistSq = distSq;
				}
			}
			an_assert(bestIdx != NONE)
			return _vertices[bestIdx].v;
		}
	};
	struct Utils
	{
		static bool do_vertex_match(VertexFormat const& vertexFormat, Array<int8> const& vertexData, int _a, int _b)
		{
			int vertexStride = vertexFormat.get_stride();
			if (VertexFormatUtils::compare(vertexFormat, &vertexData[_a * vertexStride], &vertexData[_b * vertexStride], VertexFormatCompare::All & (~VertexFormatCompare::Flag::Location) & (~VertexFormatCompare::Normal)))
			{
				Vector3 normalA = get_normal(vertexFormat, vertexData, _a);
				Vector3 normalB = get_normal(vertexFormat, vertexData, _b);
				return (normalA - normalB).is_almost_zero(0.0001f);
			}
			else
			{
				return false;
			}
		}
		static bool do_vertex_match(VertexFormat const& vertexFormat, Array<int8> const& vertexData, int _a, int _b, int _c)
		{
			return do_vertex_match(vertexFormat, vertexData, _a, _b) && do_vertex_match(vertexFormat, vertexData, _a, _c);
		}

		static bool do_vertex_match_other_attribs(VertexFormat const& vertexFormat, Array<int8> const& vertexData, int _a, int _b)
		{
			int vertexStride = vertexFormat.get_stride();
			return VertexFormatUtils::compare(vertexFormat, &vertexData[_a * vertexStride], &vertexData[_b * vertexStride], VertexFormatCompare::All & (~VertexFormatCompare::Flag::Location) & (~VertexFormatCompare::Normal));
		}
		static bool do_vertex_match_other_attribs(VertexFormat const& vertexFormat, Array<int8> const& vertexData, int _a, int _b, int _c)
		{
			return do_vertex_match_other_attribs(vertexFormat, vertexData, _a, _b) && do_vertex_match_other_attribs(vertexFormat, vertexData, _a, _c);
		}

		static Vector3 get_location(VertexFormat const& vertexFormat, Array<int8> const& vertexData, int vIdx)
		{
			int vertexStride = vertexFormat.get_stride();
			int locationOffset = vertexFormat.get_location_offset();
			int8 const * currentVertexData = &(vertexData[vIdx * vertexStride + locationOffset]);
			Vector3 location = *(Vector3*)(currentVertexData);
			return location;
		}

		static Vector3 get_normal(VertexFormat const& vertexFormat, Array<int8> const& vertexData, int vIdx)
		{
			if (vertexFormat.get_normal() == VertexFormatNormal::No)
			{
				return Vector3::zero;
			}
			int vertexStride = vertexFormat.get_stride();
			int normalOffset = vertexFormat.get_normal_offset();
			int8 const * currentVertexData = (&vertexData[vIdx * vertexStride + normalOffset]);
			Vector3 normal;
			if (vertexFormat.is_normal_packed())
			{
				normal = ((VectorPacked*)(currentVertexData))->get_as_vertex_normal();
			}
			else
			{
				normal = *(Vector3*)(currentVertexData);
			}
			return normal;
		}
	};

	// data used
	Array<TriangleUsage> triangles;
	Array<int> triangleSet;
	Array<Edge> edges;
	Array<Vertex> vertices;
	Array<Vector2> flatVertices;
	Array<PolygonUtils::Triangle> newTriangles;

	int const predictedNumberOfTriangles = 32;
	int const triangleSetLimit = 32; // if we get more, we're likely to end up with a hole, also optimising a set of 16 might be enough

	triangles.set_size(numberOfElements / 3);
	triangleSet.make_space_for(predictedNumberOfTriangles * 3);
	edges.make_space_for(predictedNumberOfTriangles * 3);
	vertices.make_space_for(predictedNumberOfTriangles * 3);
	flatVertices.make_space_for(predictedNumberOfTriangles * 3);

	// to remove doubled
	prune_vertices();

	// 1. build list of triangles used or not
	{
		an_assert(indexFormat.is_ok_to_be_used());
		int elementStride = indexFormat.get_stride();

		int8* e = elementData.begin();
		for_every(triangle, triangles)
		{
			triangle->v.set_size(3);
			for_every(v, triangle->v)
			{
				*v = ::System::IndexFormatUtils::get_value(indexFormat, (void const*)e);
				e += elementStride;
			}
			triangle->processed = false;
			triangle->inUse = true;
			Vector3 locv0 = Utils::get_location(vertexFormat, vertexData, triangle->v[0]);
			Vector3 locv1 = Utils::get_location(vertexFormat, vertexData, triangle->v[1]);
			Vector3 locv2 = Utils::get_location(vertexFormat, vertexData, triangle->v[2]);
			if (triangle->v[0] == triangle->v[1] ||
				triangle->v[0] == triangle->v[2] ||
				triangle->v[1] == triangle->v[2])
			{
				// skip degenerate (and remove them!)
				triangle->inUse = false;
				triangle->processed = true;
			}
			else if (locv0 == locv1 ||
					 locv0 == locv2 ||
					 locv1 == locv2)
			{
				// skip degenerate (and remove them!)
				triangle->inUse = false;
				triangle->processed = true;
			}
			else if (! Utils::do_vertex_match_other_attribs(vertexFormat, vertexData, triangle->v[0], triangle->v[1], triangle->v[2]))
			{
				// we're going to leave it as it is
				// we want to have a triangle that has the same values for all vertices
				triangle->processed = true;
			}
		}
	}

	// 2. go through triangles and for each that hasn't been checked:
	//		a. find triangles sharing edges who have same normal, same u, skinning etc
	//		b. get all points along the edge
	//		c. check if there are any points that might be removed (same params, lie on line)
	//		d. if any points are to be removed, rebuild triangles and reuse existing triangles
	{
		for_every(t, triangles)
		{
			if (t->processed)
			{
				continue;
			}

			Plane tPlane;
			{
				tPlane = Plane(Utils::get_location(vertexFormat, vertexData, t->v[0]),
							   Utils::get_location(vertexFormat, vertexData, t->v[1]),
							   Utils::get_location(vertexFormat, vertexData, t->v[2]));
			}

			// a. find triangles sharing edges who have same normal, same u, skinning etc
			{
				triangleSet.clear();
				triangleSet.push_back(for_everys_index(t));
				bool keepAdding = true;
				while (keepAdding && triangleSet.get_size() < triangleSetLimit)
				{
					keepAdding = false;
					for_every_continue_after(ot, t)
					{
						if (ot->processed)
						{
							continue;
						}

						{
							int otIdx = for_every_continues_index(ot, t);
							if (triangleSet.does_contain(otIdx))
							{
								continue;
							}
						}

						// check if they share edge
						bool shareEdge = false;
						for_every(ts, triangleSet)
						{
							auto& t = triangles[*ts];
							for_count(int, ti, 3)
							{
								int v0 = t.v[(ti) % 3];
								int v1 = t.v[(ti + 1) % 3];
								for_count(int, oti, 3)
								{
									int ov0 = ot->v[(oti) % 3];
									int ov1 = ot->v[(oti + 1) % 3];
									if (v0 == ov1 && v1 == ov0)
									{
										shareEdge = true;
										break;
									}
								}
								if (shareEdge)
								{
									break;
								}
							}
							if (shareEdge)
							{
								break;
							}
						}
						if (shareEdge)
						{
							if (Utils::do_vertex_match_other_attribs(vertexFormat, vertexData, t->v[0], ot->v[0]))
							{
								Vector3 ot0 = Utils::get_location(vertexFormat, vertexData, ot->v[0]);
								Vector3 ot1 = Utils::get_location(vertexFormat, vertexData, ot->v[1]);
								Vector3 ot2 = Utils::get_location(vertexFormat, vertexData, ot->v[2]);
								float ot0if = tPlane.get_in_front(ot0);
								float ot1if = tPlane.get_in_front(ot1);
								float ot2if = tPlane.get_in_front(ot2);
								float threshold = 0.00001f;
								if (abs(ot0if) < threshold &&
									abs(ot1if) < threshold &&
									abs(ot2if) < threshold)
								{
									triangleSet.push_back(for_every_continues_index(ot, t));
									keepAdding = true;
									if (triangleSet.get_size() >= triangleSetLimit)
									{
										break;
									}
								}
							}
						}
					}
				}
			}

			for_every(ts, triangleSet)
			{
				triangles[*ts].processed = true;
			}

			if (triangleSet.get_size() <= 2)
			{
				// not much we can really do
				continue;
			}

			// calculate normal from the set
			Vector3 grandNormal = Vector3::zero;
			{
				for_every(ts, triangleSet)
				{
					auto& t = triangles[*ts];
					{
						Vector3 v0 = Utils::get_location(vertexFormat, vertexData, t.v[0]);
						Vector3 v1 = Utils::get_location(vertexFormat, vertexData, t.v[1]);
						Vector3 v2 = Utils::get_location(vertexFormat, vertexData, t.v[2]);
						Vector3 normal = Vector3::cross(v2 - v0, v1 - v0);
						grandNormal += normal;
					}
				}
				grandNormal.normalise();
			}

			// b. get all points along the edge
			{
				vertices.clear();
				while (triangleSet.get_size() > 2 && vertices.is_empty())
				{
					// build edges list, if reversed exists, remove the existing one
					{
						edges.clear();
#ifdef DEBUG_OPTIMISE_VERTICES
						{
							lock_output();
							output(TXT("triangleSet"));
							for_every(ts, triangleSet)
							{
								auto& t = triangles[*ts];
								output(TXT(" %i -> %i -> %i"), t.v[0], t.v[1], t.v[2]);
							}
							unlock_output();
						}
#endif
						for_every(ts, triangleSet)
						{
							auto& t = triangles[*ts];
							for_count(int, ti, 3)
							{
								int v0 = t.v[(ti) % 3];
								int v1 = t.v[(ti + 1) % 3];
								bool addIt = true;
								for (int ie = 0; ie < edges.get_size(); ++ie)
								{
									auto& e = edges[ie];
									if (e.a == v1 && e.b == v0)
									{
										addIt = !addIt;
										an_assert_info(!addIt, TXT("maybe it really doesn't matter?"));
										edges.remove_fast_at(ie);
										--ie;
									}
								}
								if (addIt)
								{
									Edge e;
									e.a = v0;
									e.b = v1;
									e.used = false;
									edges.push_back(e);
								}
							}
						}

						//an_assert(!edges.is_empty());
					}

#ifdef DEBUG_OPTIMISE_VERTICES
					{
						lock_output();
						output(TXT("edges"));
						for_every(e, edges)
						{
							output(TXT(" %i -> %i"), e->a, e->b);
						}
						unlock_output();
					}
#endif

					// out of edges that left, build a list of vertices
					if (! edges.is_empty())
					{
						vertices.clear();
						vertices.push_back(Vertex(edges[0].a));
						vertices.push_back(Vertex(edges[0].b));
						edges[0].used = true;
						//int firstVertex = vertices.get_first().v;
						int lastVertex = vertices.get_last().v;

						int edgesLeft = edges.get_size() - 1;
						for_every(e, edges)
						{
							if (e->a == e->b) // skip degenerated 
							{
								--edgesLeft;
								e->used = true;
							}
						}
						while (edgesLeft > 0)
						{
							bool addedSomething = false;
							for_every(e, edges)
							{
								if (!e->used &&
									e->a == lastVertex)
								{
									vertices.push_back(Vertex(e->b));
									lastVertex = e->b;
									e->used = true;
									addedSomething = true;
									--edgesLeft;
									if (!edgesLeft)
									{
										break;
									}
								}
							}
							if (!addedSomething)
							{
								// if we haven't added something and we still have edges left, it means that we have a hole inside, skip this
								vertices.clear();
								break;
							}
						}
					}

					// if we managed to build a vertices list, go with it, if not, remove one triangle and try again
					{
						if (vertices.is_empty())
						{
							triangles[triangleSet.get_last()].processed = false; // allow to be processed again
							triangleSet.pop_back();
						}
						else
						{
#ifdef AN_DEVELOPMENT
							for_every(e, edges)
							{
								an_assert(e->used);
							}
#endif
							an_assert(vertices.get_first().v == vertices.get_last().v);
							vertices.remove_fast_at(vertices.get_size() - 1);

							// fill locations
							{
								for_every(v, vertices)
								{
									v->loc = Utils::get_location(vertexFormat, vertexData, v->v);
								}
							}
						}
					}
				}
			}

			if (vertices.is_empty())
			{
				continue;
			}

#ifdef DEBUG_OPTIMISE_VERTICES
			{
				lock_output();
				output(TXT("into vertices"));
				for_every(v, vertices)
				{
					output(TXT(" %i"), v->v);
				}
				unlock_output();
			}
#endif

			bool rebuildTriangles = false;

			// c. check if there are any points that might be removed (same params, lie on line)
			{
				// check vertices list and remove those colinear
				for (int i = 0; i < vertices.get_size(); ++i)
				{
					int verticesNum = vertices.get_size();
					Vertex const& prev = vertices[mod(i - 1, verticesNum)];
					Vertex const& curr = vertices[mod(i, verticesNum)];
					Vertex const& next = vertices[mod(i + 1, verticesNum)];

					Vector3 p2c = (curr.loc - prev.loc).normal();
					Vector3 p2n = (next.loc - prev.loc).normal();

					if ((p2n - p2c).is_almost_zero(0.000001f))
					{
						rebuildTriangles = true;
						vertices.remove_at(i);
						--i;
					}
				}
			}

#ifdef DEBUG_OPTIMISE_VERTICES
			{
				lock_output();
				output(TXT("vertices after removal"));
				for_every(v, vertices)
				{
					output(TXT(" %i"), v->v);
				}
				unlock_output();
			}
#endif

			// d. if any points are to be removed, rebuild triangles and reuse existing triangles
			if (rebuildTriangles)
			{
				flatVertices.clear();
				Transform placement;
				{
					// use grand normal from all triangles used as it should mark the true up dir
					Vector3 v0 = Utils::get_location(vertexFormat, vertexData, t->v[0]);
					placement = matrix_from_up(v0, grandNormal).to_transform();
				}
				for_every(v, vertices)
				{
					flatVertices.push_back(placement.location_to_local(v->loc).to_vector2());
				}
				newTriangles.clear();
				// build triangles
				PolygonUtils::triangulate(flatVertices, newTriangles);
				// if there are fewer than we had, mark all existing triangles as not used and fill
				if (newTriangles.get_size() < triangleSet.get_size())
				{
#ifdef DEBUG_OPTIMISE_VERTICES
					{
						lock_output();
						output(TXT("new triangles"));
						for_every(nt, newTriangles)
						{
							output(TXT(" %i -> %i -> %i"), 
								Vertex::find_org_v(nt->a, flatVertices, vertices),
								Vertex::find_org_v(nt->b, flatVertices, vertices),
								Vertex::find_org_v(nt->c, flatVertices, vertices));
						}
						unlock_output();
					}
#endif

					// clear
					for_every(ts, triangleSet)
					{
						triangles[*ts].inUse = false;
					}
					// find vertices
					for_every(nt, newTriangles)
					{
						auto& t = triangles[triangleSet[for_everys_index(nt)]];
						t.v[0] = Vertex::find_org_v(nt->a, flatVertices, vertices);
						t.v[1] = Vertex::find_org_v(nt->b, flatVertices, vertices);
						t.v[2] = Vertex::find_org_v(nt->c, flatVertices, vertices);
						t.inUse = true;
					}
					// modify data
					{
						int elementStride = indexFormat.get_stride();
						for_every(ts, triangleSet)
						{
							int8* e = &elementData[*ts * elementStride * 3];
							auto& t = triangles[*ts];
							for_count(int, i, 3)
							{
								::System::IndexFormatUtils::set_value(indexFormat, e, t.v[i]);
								e += elementStride;
							}
						}
					}
				}
			}
		}
	}

	// 3. when all triangles are done, remove not used triangles
	{
		int removedTriangles = 0;
		int elementStride = indexFormat.get_stride();
		int triangleSize = elementStride * 3;
		int8* eBegin = elementData.begin();
		int8* e = eBegin;
		for_every(t, triangles)
		{
			if (t->inUse)
			{
				e += triangleSize;
			}
			else
			{
				elementData.remove_at((int)(e - eBegin), triangleSize);
				++removedTriangles;
			}
		}
		numberOfElements -= removedTriangles * 3;
	}

	// 4. find which points are no longer used (prune_vertices)
	{
		prune_vertices();
	}

#ifdef SHOW_OPTIMISE_VERTICES_STATS
	output(TXT(" + optimised vertices : %.3f"), optimiseTS.get_time_since());
#endif
}

void Mesh3DPart::prune_vertices()
{
	if (!get_number_of_elements())
	{
		// do not remove anything, these are just plain vertices
		return;
	}

#ifdef SHOW_PRUNE_VERTICES_STATS
	int verticesToBeRemoved = 0;
#endif
	bool anyVertexToRemove = false;
	ARRAY_PREALLOC_SIZE(bool, verticesToRemove, get_number_of_vertices());
	ARRAY_PREALLOC_SIZE(int, remapIndices, get_number_of_vertices());
	memory_leak_suspect;
	verticesToRemove.set_size(get_number_of_vertices());
	remapIndices.set_size(get_number_of_vertices());
	forget_memory_leak_suspect;
	memory_zero(verticesToRemove.get_data(), sizeof(bool) * get_number_of_vertices());

	an_assert(vertexFormat.is_ok_to_be_used());
	int vertexStride = vertexFormat.get_stride();

	{	// remove doubled
#ifdef SHOW_PRUNE_VERTICES_STATS
		::System::TimeStamp removeDoubledTS;
#endif
		bool alteredAnything = false;

		// rebuild remap indices
		for_count(int, a, get_number_of_vertices())
		{
			remapIndices[a] = a;
		}

		int8 const * va = vertexData.get_data();
		for_count(int, a, get_number_of_vertices())
		{
			int8 const * vb = vertexData.get_data();
			for (int b = 0; b < a; ++b)
			{
				if (VertexFormatUtils::compare(vertexFormat, va, vb))
				{
#ifdef CHECK_VERTEX_COMPARISONS
					assert_slow(::System::VertexFormat::compare(vertexFormat, va, vb));
#endif
#ifdef SHOW_PRUNE_VERTICES_STATS
					if (!verticesToRemove[a])
					{
						++verticesToBeRemoved;
					}
#endif

					verticesToRemove[a] = true;
					anyVertexToRemove = true;
					remapIndices[a] = b;
					alteredAnything = true;
					break;
				}
				else
				{
#ifdef CHECK_VERTEX_COMPARISONS
					assert_slow(!::System::VertexFormat::compare(vertexFormat, va, vb));
#endif
				}
				vb += vertexStride;
			}
			va += vertexStride;
		}

#ifdef SHOW_PRUNE_VERTICES_STATS
		output(TXT(" + mark to remove doubled : %.3f"), removeDoubledTS.get_time_since());
#endif

		if (alteredAnything)
		{
			// remap indices so we don't have doubled points
			remap_indices(remapIndices);
		}
	}

	if (get_number_of_elements())
	{	// remove unused
#ifdef SHOW_PRUNE_VERTICES_STATS
		::System::TimeStamp removeUnusedTS;
#endif
		
		ARRAY_PREALLOC_SIZE(bool, verticesUsed, get_number_of_vertices());
		memory_leak_suspect;
		verticesUsed.set_size(get_number_of_vertices());
		forget_memory_leak_suspect;
		memory_zero(verticesUsed.get_data(), sizeof(bool) * get_number_of_vertices());

		an_assert(indexFormat.is_ok_to_be_used());
		int elementStride = indexFormat.get_stride();

		int8 * e = elementData.get_data();
		for_count(int, eIdx, get_number_of_elements())
		{
			uint i = ::System::IndexFormatUtils::get_value(indexFormat, (void const *)e);
			verticesUsed[i] = true;
			e += elementStride;
		}

		for_every(vertexUsed, verticesUsed)
		{
			if (!(*vertexUsed))
			{
#ifdef SHOW_PRUNE_VERTICES_STATS
				if (!verticesToRemove[for_everys_index(vertexUsed)])
				{
					++verticesToBeRemoved;
				}
#endif
				verticesToRemove[for_everys_index(vertexUsed)] = true;
				anyVertexToRemove = true;
			}
		}
#ifdef SHOW_PRUNE_VERTICES_STATS
		output(TXT(" + mark to remove unused : %.3f"), removeUnusedTS.get_time_since());
#endif
	}

	if (anyVertexToRemove)
	{
#ifdef SHOW_PRUNE_VERTICES_STATS
		::System::TimeStamp removeTS;
#endif
		// rebuild remap indices
		for_count(int, a, get_number_of_vertices())
		{
			remapIndices[a] = a;
		}

		// use copy to instead of removing, copying just what we need
		Array<int8> vertexDataCopy = vertexData;

		// remap those that stay, we will have full list of vertices afterwards
		// if it is removed, we may safely use new index
		// and remove actual points here!
		int removedCount = 0;
		int8 * src = vertexDataCopy.get_data();
		int8 * dest = vertexData.get_data();
		for (int oldIdx = 0; oldIdx < remapIndices.get_size(); ++oldIdx)
		{
			if (verticesToRemove[oldIdx])
			{
				// and remove actual point
				--numberOfVertices;
				++removedCount;
			}
			else
			{
				memory_copy(dest, src, vertexStride);
				dest += vertexStride;
			}
			remapIndices[oldIdx] -= removedCount;
			src += vertexStride;
		}
		memory_leak_suspect;
		vertexData.set_size((int)(dest - vertexData.get_data()));
		forget_memory_leak_suspect;

#ifdef SHOW_PRUNE_VERTICES_STATS
		output(TXT(" + actual removal : %.3f"), removeTS.get_time_since());
#endif
		remap_indices(remapIndices);
	}
}

void Mesh3DPart::remap_indices(Array<int> const & _remap)
{
	if (get_number_of_elements() == 0)
	{
		return;
	}

#ifdef SHOW_PRUNE_VERTICES_STATS
	::System::TimeStamp remapTS;
#endif
	an_assert(indexFormat.is_ok_to_be_used());

	int elementStride = indexFormat.get_stride();
	int8 * e = elementData.get_data();
	for_count(int, ie, get_number_of_elements())
	{
		uint i = ::System::IndexFormatUtils::get_value(indexFormat, (void const *)e);
		i = (uint)_remap[i];
		::System::IndexFormatUtils::set_value(indexFormat, (void *)e, i);
		e += elementStride;
	}

#ifdef SHOW_PRUNE_VERTICES_STATS
	output(TXT(" + remap indices : %.3f"), remapTS.get_time_since());
#endif
}

Range3 Mesh3DPart::calculate_bounding_box() const
{
	Range3 result = Range3::empty;

	if (get_number_of_vertices() > 0)
	{
		an_assert(vertexFormat.is_ok_to_be_used());
		int vertexStride = vertexFormat.get_stride();
		int8 const* vertexAt = vertexData.get_data() + vertexFormat.get_location_offset();
		scoped_call_stack_info_str_printf(TXT("calc bound box vertices:%i stride:%i data:%i->%i"), get_number_of_vertices(), vertexStride, vertexData.get_size(), vertexData.get_size() / vertexStride);
		for_count(int, a, get_number_of_vertices())
		{
			scoped_call_stack_info_str_printf(TXT(" vert %i"), a);
			Vector3 loc = *((Vector3*)(vertexAt));
			result.include(loc);
			vertexAt += vertexStride;
		}
	}

	return result;
}

void Mesh3DPart::log(LogInfoContext& _log) const
{
	_log.log(TXT("vertices          : %i"), numberOfVertices);
	_log.log(TXT("elements          : %i"), numberOfElements);
	_log.log(TXT("primitives        : %S"), Primitive::to_char(primitiveType));
	_log.log(TXT("canBeCombined     : %S"), canBeCombined ? TXT("yes") : TXT("no"));
	_log.log(TXT("updatedDynamicaly : %S"), updatedDynamicaly ? TXT("yes") : TXT("no"));

	todo_note(TXT("log vertexFormat"));
	todo_note(TXT("log indexFormat"));
	
	{
		_log.log(TXT("triangles"));
		LOG_INDENT(_log);
		{
			if (indexFormat.get_element_size() != System::IndexElementSize::None)
			{
				if (primitiveType == Primitive::Triangle)
				{
					an_assert(!elementData.is_empty());
					auto _vf = vertexFormat;
					_vf.calculate_stride_and_offsets();
					auto _if = indexFormat;
					_if.calculate_stride();
					int8 const* element = elementData.get_data();
					for_count(int, triIdx, numberOfElements / 3)
					{
						int ind[3];
						Vector3 p[3];
						p[2] = Vector3::zero;
						int pCount = 3;
						VertexSkinningInfo skinningInfo[3];
						Optional<float> u;
						for_count(int, i, pCount)
						{
							int e = IndexFormatUtils::get_value(_if, element); element += _if.get_stride();
							int8 const* vertexAt = &vertexData[e * _vf.get_stride()];
							ind[i] = e;
							p[i] = *((Vector3*)(vertexAt + _vf.get_location_offset()));
						}

						_log.log(TXT("[%i] %i+%i+%i  %S  %S  %S"),
							triIdx, ind[0], ind[1], ind[2],
							p[0].to_string().to_char(),
							p[1].to_string().to_char(),
							p[2].to_string().to_char());
					}
				}
				else
				{
					todo_important(TXT("implement_ for other kinds"));
				}
			}
			else
			{
				if (primitiveType == Primitive::Triangle)
				{
					auto _vf = vertexFormat;
					_vf.calculate_stride_and_offsets();
					int e = 0;
					for_count(int, triIdx, numberOfVertices / 3)
					{
						int ind[3];
						Vector3 p[3];
						p[2] = Vector3::zero;
						int pCount = 3;
						for_count(int, i, pCount)
						{
							int8 const* vertexAt = &vertexData[e * _vf.get_stride()];
							++e;
							ind[i] = e;
							p[i] = *((Vector3*)(vertexAt + _vf.get_location_offset()));
						}

						_log.log(TXT("[%i] %i+%i+%i  %S  %S  %S"),
							triIdx, ind[0], ind[1], ind[2],
							p[0].to_string().to_char(),
							p[1].to_string().to_char(),
							p[2].to_string().to_char());
					}
				}
				else
				{
					todo_important(TXT("implement_ for other kinds"));
				}
			}
		}
	}
}

void Mesh3DPart::convert_vertex_data_to(::System::VertexFormat const& _newFormat)
{
	Array<int8> srcVertexData;
	srcVertexData = vertexData;

	vertexData.set_size(numberOfVertices * _newFormat.get_stride());

	auto srcVertexFormat = vertexFormat;
	vertexFormat = _newFormat;

	::System::VertexFormatUtils::convert_data(srcVertexFormat, srcVertexData.get_data(), srcVertexData.get_size(), vertexFormat, vertexData.get_data());
}

#undef USE_OFFSET
