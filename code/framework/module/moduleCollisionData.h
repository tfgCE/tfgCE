#pragma once

#include "moduleData.h"

#include "..\collision\collisionModel.h"
#include "..\collision\physicalMaterial.h"

#include "..\..\core\collision\collisionFlags.h"

namespace Framework
{
	class ModuleCollisionData
	: public ModuleData
	{
		FAST_CAST_DECLARE(ModuleCollisionData);
		FAST_CAST_BASE(ModuleData);
		FAST_CAST_END();
		typedef ModuleData base;
	public:
		ModuleCollisionData(LibraryStored* _inLibraryStored);

		Framework::CollisionModel* get_movement_collision_model() const { return movementCollisionModel.get(); }
		Framework::CollisionModel* get_precise_collision_model() const { return preciseCollisionModel.get(); }
		Framework::PhysicalMaterial* get_movement_collision_material() const { return movementCollisionMaterial.get(); }
		Framework::PhysicalMaterial* get_precise_collision_material() const { return preciseCollisionMaterial.get(); }

	public: // ModuleData
		override_ bool read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc);
		override_ bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected:	friend class ModuleCollision;

	private:
		Framework::UsedLibraryStored<Framework::CollisionModel> movementCollisionModel;
		Framework::UsedLibraryStored<Framework::CollisionModel> preciseCollisionModel;
		// will be used if own model is used or for movement collision to get sounds
		Framework::UsedLibraryStored<Framework::PhysicalMaterial> movementCollisionMaterial;
		Framework::UsedLibraryStored<Framework::PhysicalMaterial> preciseCollisionMaterial;

	};
};


