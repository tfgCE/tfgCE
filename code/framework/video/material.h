#pragma once

#include "..\..\core\system\video\material.h"
#include "..\..\core\system\video\materialInstance.h"
#include "..\..\core\mesh\mesh3dInstance.h"

#include "..\library\libraryStored.h"

#include "shaderProgram.h"

namespace Framework
{
	class FragmentShader;
	class Material;
	class PhysicalMaterialMap;

	/**
	 *	Material is something that packs shader program as pair of vertex and fragment shader programs.
	 *
	 *	When set to mesh (library mesh), material setup is created there.
	 *
	 *	To modify material params in mesh instance, just access mesh instance's materials.
	 */

	/**
	 *	All material params as used by setup.
	 *	Used to modify system::material / system::materialinstance that helds system::shaderprograminstance
	 */
	struct MaterialParams
	: public ShaderParams
	{
		void apply_to(::System::Material* _material) const;
		void apply_to(::System::MaterialInstance* _materialInstance, ::System::MaterialSetting::Type _settingMaterial = ::System::MaterialSetting::Default) const;
	};

	/**
	 *	This is information about material and all parameters set to this material.
	 */
	struct MaterialSetup
	{
		UsedLibraryStored<Material> material;
		MaterialParams params;
		Optional<int> renderingOrderPriorityOffset;

		MaterialSetup();

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext, int _atLevel);

		void resolve_links();

		void set_material(Material* _material);
		Material const * get_material() const { return material.get();  }

		bool apply_renamer(LibraryStoredRenamer const & _renamer, Library * _library = nullptr);

		static void apply_material_setups(Array<MaterialSetup> const & _materialSetups, Meshes::Mesh3DInstance& _meshInstance, ::System::MaterialSetting::Type _settingMaterial = ::System::MaterialSetting::Default);
		void apply_to(Meshes::Mesh3DInstance& _meshInstance, int _materialIndex, ::System::MaterialSetting::Type _settingMaterial = ::System::MaterialSetting::Default) const;

		bool operator==(MaterialSetup const & _other) const { return material == _other.material && params == _other.params; }
		bool operator!=(MaterialSetup const & _other) const { return material != _other.material || params != _other.params; }
	};

	class Material
	: public LibraryStored
	{
		FAST_CAST_DECLARE(Material);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		Material(Library * _library, LibraryName const & _name);

		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

		::System::Material * get_material() const { return material; }
		MaterialParams const & get_params() const { return params; }

		static void add_defaults(Library* _library);
		static Material * get_default(Library* _library);

		PhysicalMaterialMap * get_physical_material_map() const;

	protected:
		virtual ~Material();

	private:
		UsedLibraryStored<VertexShader> vertexShader; // will ignore "useWith*" settings as it will provide it will choose it on its own
		UsedLibraryStored<VertexShader> vertexShaderStatic;
		UsedLibraryStored<VertexShader> vertexShaderSkinned;
		UsedLibraryStored<VertexShader> vertexShaderSkinnedToSingleBone;
		UsedLibraryStored<FragmentShader> fragmentShader;
		UsedLibraryStored<Material> parentMaterial;
		bool useWithStaticMesh;
		bool useWithSkinnedMesh;
		bool useWithSkinnedToSingleBoneMesh; // by default it allows to use it if defined or for static meshes to combine them

		::System::Material* material;
		bool parentLinked;

		MaterialParams params;

		UsedLibraryStored<PhysicalMaterialMap> physicalMaterialMap;

#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
		Concurrency::Counter loadShaderUsagesIssued;
		Concurrency::Counter loadShaderUsagesFinished;
		float loadShaderTime = 0.0f;
		float loadShaderDetailedTime[4];
#endif

		static void load_shader_into_material(Concurrency::JobExecutionContext const * _executionContext, void* _data);
		static void load_shader_into_material__shader_program__setup_attribs(Concurrency::JobExecutionContext const* _executionContext, void* _data);
		static void load_shader_into_material__shader_program__setup_uniforms(Concurrency::JobExecutionContext const* _executionContext, void* _data);
		static void load_shader_into_material__shader_program__finalise(Concurrency::JobExecutionContext const* _executionContext, void* _data);
	};
};
