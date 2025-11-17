#pragma once

#include "..\library\libraryStored.h"
#include "..\video\material.h"

#include "..\..\core\mesh\mesh3dInstance.h"

namespace Framework
{
	class Mesh;

	/**
	 *	This is an instance of a mesh in the lib.
	 *	It has own material/shader setup.
	 *
	 *	It's best to use as UI assets.
	 */
	class MeshLibInstance
	: public LibraryStored
	{
		FAST_CAST_DECLARE(MeshLibInstance);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		MeshLibInstance(Library * _library, LibraryName const & _name);
		virtual ~MeshLibInstance();

		Meshes::Mesh3DInstance & access_mesh_instance() { return meshInstance; }
		Meshes::Mesh3DInstance const& get_mesh_instance() const { return meshInstance; }

		Transform const& get_placement() const { return placement; }

	public: // material setups
		Array<MaterialSetup> const& get_material_setups() const { return materialSetups; }
		void set_material(int _idx, Material* _material) { materialSetups.set_size(max(materialSetups.get_size(), _idx + 1)); materialSetups[_idx].set_material(_material); }
		void set_material_setup(int _idx, MaterialSetup const& _materialSetup) { materialSetups.set_size(max(materialSetups.get_size(), _idx + 1)); materialSetups[_idx] = _materialSetup; }
		Material const* get_material(int _idx) const { if (_idx < materialSetups.get_size()) return materialSetups[_idx].get_material(); else return nullptr; }

		void set_missing_materials(Library* _library);

	public: // LibraryStored
		override_ void prepare_to_unload();
#ifdef AN_DEVELOPMENT
		virtual void ready_for_reload();
#endif
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		override_ void debug_draw() const;
		override_ void log(LogInfoContext& _context) const;

	protected:
		UsedLibraryStored<Mesh> mesh;
		Meshes::Mesh3DInstance meshInstance;

		Array<MaterialSetup> materialSetups;

		Transform placement = Transform::identity;
	};
};
