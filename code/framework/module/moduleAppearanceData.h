#pragma once

#include "moduleData.h"

#include "..\appearance\mesh.h"
#include "..\appearance\skeleton.h"

#include "..\..\core\containers\list.h"

namespace Framework
{
	class AnimationSet;
	class AppearanceControllerData;
	class MeshDepot;

	class ModuleAppearanceData
	: public ModuleData
	{
		FAST_CAST_DECLARE(ModuleAppearanceData);
		FAST_CAST_BASE(ModuleData);
		FAST_CAST_END();
		typedef ModuleData base;
	public:
		ModuleAppearanceData(LibraryStored* _inLibraryStored);
		virtual ~ModuleAppearanceData();

		Array<AppearanceControllerDataPtr> const & get_controllers() const { return controllers; }

		MeshDepot* get_mesh_depot() const { return meshDepot.get(); }
		MeshGenerator* get_mesh_generator() const { return meshGenerator.get(); }

		//Mesh* get_mesh(Random::G) const { return mesh.get(); }

	public: // ModuleData
		override_ bool read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc);
		override_ bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected:	friend class ModuleAppearance;
		Tags tags;
		Name name;
		Name syncTo;
		Array<UsedLibraryStored<Mesh>> meshes;
		Array<MaterialSetup> materialSetups;
		UsedLibraryStored<Skeleton> skeleton;
		Array<UsedLibraryStored<AnimationSet>> animationSets;

		// to allow more individual appearances
		UsedLibraryStored<MeshDepot> meshDepot;
		UsedLibraryStored<MeshGenerator> meshGenerator;
		UsedLibraryStored<MeshGenerator> skelGenerator;
		SimpleVariableStorage generatorVariables; // used for various modules (eg. when creating mesh via generator)

		Array<Name> copyMaterialParameters;
		bool copyMaterialParametersOnce = false;

		bool useControllers = true; // useful to switch off if we just sync to other appearance (then has to be explicitly set)
		Array<AppearanceControllerDataPtr> controllers;
	};
};


