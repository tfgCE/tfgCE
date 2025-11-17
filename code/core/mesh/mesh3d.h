#pragma once

#include "..\types\string.h"
#include "..\math\math.h"

#include "..\system\video\vertexFormat.h"
#include "..\system\video\indexFormat.h"

#include "iMesh3d.h"
#include "primitiveType.h"
#include "remapBoneArray.h"
#include "usage.h"

#include "..\serialisation\serialisableResource.h"
#include "..\serialisation\importer.h"

#include "mesh3dPart.h"

class Serialiser;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace System
{
	struct MaterialInstance;
	struct ShaderProgramInstance;
};

namespace Meshes
{
	class Mesh3DBuilder;
	class Skeleton;

	struct Mesh3DImportCombinePart
	{
		String byName;
		int byIndex;
		String ifNameContains;
		
		Mesh3DImportCombinePart() : byIndex(NONE) {}
		bool load_from_xml(IO::XML::Node const * _node);
	};

	struct Mesh3DImportOptions
	{
		String importFromNode;
		Array<String> ignoreImportingFromNodesWithPrefix;
		Skeleton* skeleton; // use skeleton to get skinning data, without skeleton there won't be skinning data extracted
		bool skipUV;
		bool skipColour;
		bool skipSkinning;
		bool floatUV;
		Optional<float> setU;
		Optional<Transform> importFromPlacement;
		Optional<Vector3> importFromAutoCentre;
		Optional<Vector3> importFromAutoPt;
		Optional<Transform> placeAfterImport;
		Vector3 importScale;
		Optional<Vector3> importScaleToFit;
		List<List<Mesh3DImportCombinePart> > combineParts; // first index is resulting part index,
			// we may combine parts 0..5 like this 0:0,2 1:1,4 rest (3,5) will be added at the end,
			// so we will have 0+2,1+4,3,5
		bool combineAllParts;
		
		Mesh3DImportOptions();

		bool load_from_xml(IO::XML::Node const * _node);
	};

	/**
	 *	Data to load or to render can be prepared by hand although it is advised to create it with builder
	 *
	 *	Usage:
	 *		load_part_data or load_builder
	 *		build
	 *
	 *	Can be used for static and skinned meshes
	 *
	 *
	 *	It can be used with materials or without them (for UI)
	 *
	 *		_v3d->set_fallback_colour(colourForMesh);
	 *		modelViewStack.push_set(Matrix44::identity);
	 *		Meshes::Mesh3D::render_data(_v3d, nullptr, nullptr, ::Meshes::Mesh3DRenderingContext::none(), completeFrontPlanePoints, ::Meshes::Primitive::TriangleFan, 2, ::System::vertex_format().no_padding().calculate_stride_and_offsets());
	 *		modelViewStack.pop();
	 *		_v3d->set_fallback_colour();
	 *
	 *	In this example vertex format is defined on the run, it is advised to define it earlier.
	 */
	class Mesh3D
	: public IMesh3D
	, public SerialisableResource
	{
		FAST_CAST_DECLARE(Mesh3D);
		FAST_CAST_BASE(IMesh3D);
		FAST_CAST_END();

		typedef IMesh3D base;
	public:
		static void initialise_static();
		static void close_static();

		static bool do_immovable_match(bool _aImmovable, bool _bImmovable) { return (_aImmovable && _bImmovable) || (!_aImmovable && !_bImmovable); }

		static Mesh3D* create_new(); // workaround for importers

		Mesh3D();
		virtual ~Mesh3D();

		static Importer<Mesh3D, Mesh3DImportOptions> & importer() { return s_importer; }

#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
		void set_debug_mesh_name(String const& _debugMeshName) { debugMeshName = _debugMeshName; }
		void set_debug_mesh_name(tchar const * _debugMeshName) { debugMeshName = _debugMeshName; }
		tchar const * get_debug_mesh_name() const { return debugMeshName.to_char(); }
#else
		tchar const* get_debug_mesh_name() const { return TXT("??"); }
#endif

		int get_parts_num() const { return parts.get_size(); }
		bool is_part_valid(int _idx) const { return parts.is_index_valid(_idx) && ! parts[_idx]->is_empty(); }
		Mesh3DPart const * get_part(int _partIndex) const { return parts.is_index_valid(_partIndex) ? parts[_partIndex].get() : nullptr; }
		Mesh3DPart * access_part(int _partIndex) const { return parts.is_index_valid(_partIndex) ? parts[_partIndex].get() : nullptr; }
		void add_part(Mesh3DPart* part) { parts.push_back(RefCountObjectPtr<Mesh3DPart>(part)); }
		void remove_all_parts();

		void updated_dynamicaly(bool _updatedDynamicaly = true) { updatedDynamicaly = _updatedDynamicaly; }

		bool can_be_combined() const { return canBeCombined; }
		bool can_be_combined_as_immovable() const { return canBeCombinedAsImmovable; }
		bool can_be_updated_dynamicaly() const { return updatedDynamicaly; }

		void close();

		Mesh3DPart* update_part_data(int _partIdx, void const * _vertexData, Primitive::Type _primitiveType, int32 _numberOfPrimitives, ::System::VertexFormat const & _vertexFormat, bool _fillElementArray = true);
		Mesh3DPart* update_part_data(int _partIdx, void const * _vertexData, void const * _elementData, Primitive::Type _primitiveType, int32 _numberOfVertices, int32 _numberOfElements, ::System::VertexFormat const & _vertexFormat, ::System::IndexFormat const & _indexFormat);
		Mesh3DPart* load_copy_part_data(Mesh3DPart const * _part);
		Mesh3DPart* load_part_data(void const * _vertexData, Primitive::Type _primitiveType, int32 _numberOfPrimitives, ::System::VertexFormat const & _vertexFormat, bool _fillElementArray = true);
		Mesh3DPart* load_part_data(void const * _vertexData, void const * _elementData, Primitive::Type _primitiveType, int32 _numberOfVertices, int32 _numberOfElements, ::System::VertexFormat const & _vertexFormat, ::System::IndexFormat const & _indexFormat);
		void load_builder(Mesh3DBuilder const * _builder);

		void prune_vertices(); // remove doubled and not used

		void optimise_vertices();

		void build_buffers();

		static void render_data(::System::Video3D * _v3d, ::System::ShaderProgramInstance const * _shaderProgramInstance, System::Material const * _usingMaterial, Mesh3DRenderingContext const & _renderingContext, void const * _data, Primitive::Type _primitiveType, int32 _numberOfPrimitives, ::System::VertexFormat const & _vertexFormat);
		static void render_builder(::System::Video3D * _v3d, Mesh3DBuilder const * _builder, ::System::IMaterialInstanceProvider const * _materialProvider, Mesh3DRenderingContext const & _renderingContext); // wrapper for render_data

		inline Usage::Type get_usage() const { return usage; }
		inline void set_usage(Usage::Type _usage) { usage = _usage; canBeCombined = usage == Usage::Static || usage == Usage::Skinned; canBeCombinedAsImmovable = usage == Usage::Static; }

		IO::Digested const & get_digested_skeleton() const { return digestedSkeleton; }
		void set_digested_skeleton(IO::Digested const & _digestedSkeleton) { digestedSkeleton = _digestedSkeleton; }

		struct FuseMesh3DPart
		{
			RefCountObjectPtr<Mesh3DPart> part;
			Optional<Transform> placement;
			bool own = false;
			FuseMesh3DPart() {}
			explicit FuseMesh3DPart(Mesh3DPart* _part, Optional<Transform> const& _placement = NP) : part(_part), placement(_placement) {}
			inline static bool does_contain(Array<FuseMesh3DPart> const& _parts, Mesh3DPart const * _part)
			{
				for_every(f, _parts) { if (f->part.get() == _part) return true; }
				return false;
			}
		};
		// this is something else than being combined - being combined is about going into combined mesh instance set
		// and this is just about combining parts together - it can be used to combine parts coming from different meshes
		// first index is the target part, inner array/second index are parts to be fused together
		void fuse_parts(Array<Array<FuseMesh3DPart> > const & _fromParts, Array<Array<RemapBoneArray const *>>* _remapBones, bool _keepPartsThatAreNotMentioned = false);

		void mark_to_load_to_gpu() const;
		Mesh3D* create_copy() const;

		bool check_if_has_valid_skinning_data() const;

		Range3 get_bounding_box() const { an_assert(boundingBox.is_set()); return boundingBox.get(); }
		void update_bounding_box();
		float get_size_for_lod() const { an_assert(sizeForLOD.is_set()); return sizeForLOD.get(); }

	public: // IMesh3D
		implement_ void render(::System::Video3D * _v3d, ::System::IMaterialInstanceProvider const * _materialProvider = nullptr, Mesh3DRenderingContext const & _renderingContext = Mesh3DRenderingContext::none()) const;
		implement_ ::System::MaterialShaderUsage::Type get_material_usage() const { return Usage::usage_to_material_usage(usage); }
		implement_ int calculate_triangles() const;
		implement_ void log(LogInfoContext & _log) const;

	public: // SerialisableResource
		virtual bool serialise(Serialiser & _serialiser);

	private:
		static Importer<Mesh3D, Mesh3DImportOptions> s_importer;

#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
		String debugMeshName;
#endif

		Usage::Type usage;
		IO::Digested digestedSkeleton; // if present, for reference when importing
		bool canBeCombined; // this also means that we're not removing original data
		bool canBeCombinedAsImmovable;
		bool updatedDynamicaly;

		friend struct Mesh3DPart;

		Array<RefCountObjectPtr<Mesh3DPart>> parts;

		Optional<Range3> boundingBox;
		Optional<float> sizeForLOD;

		Mesh3D(Mesh3D const & _source); // inaccessible
		Mesh3D& operator= (Mesh3D const & _source); // inaccessible
	};

};
