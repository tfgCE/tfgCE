#pragma once

#include "generatedMeshDeterminants.h"
#include "..\library\libraryStored.h"

//

namespace Framework
{
	class Mesh;
	class MeshGenerator;
	interface_class IModulesOwner;

	class MeshDepot
	: public LibraryStored
	{
		FAST_CAST_DECLARE(MeshDepot);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		MeshDepot(Library * _library, LibraryName const & _name);
		virtual ~MeshDepot();

		Mesh* get_mesh_for(IModulesOwner* _imo, SimpleVariableStorage const * _additionalVariables = nullptr);

	public: // LibraryStored
#ifdef AN_DEVELOPMENT
		virtual void ready_for_reload();
#endif
		override_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

	public:
		void clear();

	private:
		UsedLibraryStored<MeshGenerator> meshGenerator;
		GeneratedMeshDeterminants generatedMeshesDeterminants; // variables by what each generated mesh is identified (and default values)

		bool generateDefaultSet = false;
		int varietyAmount = 10; // how many different meshes should be generated

		// by default, when later-loading default values are used to generate a single sets
		// otherwise, all meshes are generated on the go, until varietyAmount is hit, then they are reused
		struct GeneratedSet
		{
			GeneratedMeshDeterminants generatedMeshesDeterminants; // determinants for this set
			Array<UsedLibraryStored<Mesh>> meshes;
		};
		Concurrency::SpinLock setsLock;
		List<GeneratedSet> sets; // all generated sets

		Mesh* get_mesh(Library* _library, GeneratedMeshDeterminants const& _meshDeterminants, Optional<Random::Generator> const & _rg);
	};
};
