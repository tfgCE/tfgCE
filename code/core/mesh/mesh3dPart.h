#pragma once

#include "..\memory\refCountObject.h"

#include "..\system\video\video.h"
#include "..\system\video\video3d.h"
#include "..\system\video\vertexFormat.h"
#include "..\system\video\indexFormat.h"

#include "iMesh3d.h"
#include "primitiveType.h"
#include "remapBoneArray.h"
#include "usage.h"

class Serialiser;

namespace System
{
	class Material;
	struct MaterialInstance;
	struct ShaderProgramInstance;
};

namespace Meshes
{
	class Mesh3DBuilder;
	class Skeleton;
	class Mesh3D;
	class CombinedMesh3DInstanceSet;
	struct VertexSkinningInfo;
	
	struct Mesh3DPart
	: public SafeObject<Mesh3DPart> // to have a safe pointer to the object
	, public RefCountObject
	{
	public:
		// this is used to allow loading one par per frame/rendering instead of loading them in bunches when suddenly 20 are required to be loaded
		static void load_one_part_to_gpu(); // removes from the list
		// use with mark_to_load_to_gpu

		static void initialise_static();
		static void close_static();

	private:
		struct StaticData
		{
			Concurrency::SpinLock partsToLoadToGPULock = Concurrency::SpinLock(TXT("Meshes.Mesh3DPart.partsToLoadToGPULock"));
			List<::SafePtr<Mesh3DPart>> partsToLoadToGPU;
		};
		static StaticData* staticData;

	public:
		Mesh3DPart(bool _canBeCombined, bool _updatedDynamicaly);
		~Mesh3DPart();

		Mesh3DPart* create_copy() const;

		void clear();
		void close();

		bool is_empty() const { return emptyPart || (numberOfVertices == 0 && numberOfElements == 0);  }
		void be_empty_part(bool _emptyPart = true) { emptyPart = _emptyPart;  }

		Primitive::Type get_primitive_type() const { return primitiveType; }

		int get_number_of_vertices() const { return numberOfVertices; }
		int get_number_of_elements() const { return numberOfElements; }
		int get_number_of_tris() const;
		Array<int8> & access_vertex_data() { return vertexData; }
		Array<int8> & access_element_data() { return elementData; }
		Array<int8> const & get_vertex_data() const { return vertexData; }
		Array<int8> const & get_element_data() const { return elementData; }

		void convert_vertex_data_to(::System::VertexFormat const& _newFormat);

		void update_data(void const * _vertexData, Primitive::Type _primitiveType, int32 _numberOfPrimitives, ::System::VertexFormat const & _vertexFormat, bool _fillElementArray = true);
		void update_data(void const * _vertexData, void const * _elementData, Primitive::Type _primitiveType, int32 _numberOfVertices, int32 _numberOfElements, ::System::VertexFormat const & _vertexFormat, ::System::IndexFormat const & _indexFormat);
		void load_data(void const * _vertexData, Primitive::Type _primitiveType, int32 _numberOfPrimitives, ::System::VertexFormat const & _vertexFormat, bool _fillElementArray = true);
		void load_data(void const * _vertexData, void const * _elementData, Primitive::Type _primitiveType, int32 _numberOfVertices, int32 _numberOfElements, ::System::VertexFormat const & _vertexFormat, ::System::IndexFormat const & _indexFormat);
		
		void mark_to_load_to_gpu() const;

		::System::VertexFormat const & get_vertex_format() const { return vertexFormat; }
		::System::IndexFormat const & get_index_format() const { return indexFormat; }

		void add_from_part(Mesh3DPart const * _fromPart, RemapBoneArray const * _remapBones = nullptr, OUT_ int * _newAddedVertexDataSize = nullptr, OUT_ void ** _newVerticesGoHere = nullptr);

		void prune_vertices(); // remove doubled and not used

		void optimise_vertices();

		void build_buffer();

		void render(::System::Video3D * _v3d, ::System::ShaderProgramInstance const * _shaderProgramInstance, System::Material const * _usingMaterial, Mesh3DRenderingContext const & _renderingContext);

		bool serialise(Serialiser & _serialiser, int _version);

		bool check_if_has_valid_skinning_data() const;

		Range3 calculate_bounding_box() const;

		void log(LogInfoContext& _log) const;

	public:
		void for_every_vertex(std::function<void(Vector3 const & _a)> _do) const;
		void for_every_triangle(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c)> _do) const;
		void for_every_triangle_and_simple_skinning(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx)> _do) const;
		void for_every_triangle_and_u(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, float _u)> _do) const;
		void for_every_triangle_simple_skinning_and_u(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx, float _u)> _do) const;
		void for_every_triangle_and_full_skinning(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning)> _do) const;
		void for_every_triangle_full_skinning_and_u(std::function<void(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, float _u)> _do) const;
		void for_every_triangle_indices(std::function<void(int _aIdx, int _bIdx, int _cIdx)> _do) const;
		void for_every_triangle_indices_and_u(std::function<void(int _aIdx, int _bIdx, int _cIdx, float _u)> _do) const;

	private:
		void for_every_triangle_worker(bool _getSimpleSkinning, bool _getFullSkinning, bool _getU, std::function<void(int _aIdx, int _bIdx, int _cIdx, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, VertexSkinningInfo const & _aSkinning, VertexSkinningInfo const & _bSkinning, VertexSkinningInfo const & _cSkinning, int _skinToBoneIdx, float _u)> _do) const;

	private:
		// setup
		bool emptyPart = false; // if empty, is no rendered, no data etc
		Primitive::Type primitiveType;
		::System::VertexFormat vertexFormat;
		::System::IndexFormat indexFormat;

		// extra info
		bool canBeCombined;
		bool updatedDynamicaly;

		// data
		bool dataAvailableToBuildBufferObjects; // data available to build buffer objects
		bool newVertexDataAvailable;
		bool newElementDataAvailable;
		int32 numberOfVertices;
		int32 numberOfElements;
		Array<int8> vertexData;
		Array<int8> elementData;

		// built buffers
		::System::BufferObjectID vertexBufferObject;
		::System::BufferObjectID elementBufferObject;

		void update_buffer_if_required();

		void load_vertex_data_into_buffer_object(bool _update);
		void load_element_data_into_buffer_object(bool _update);

		void remap_indices(Array<int> const & _remap);

		// private so any can use it with methods above, but only friends can modify that
		friend class CombinedMesh3DInstanceSet;

	public:
#ifdef AN_DEVELOPMENT
		String debugInfo;
#endif
	};

};
