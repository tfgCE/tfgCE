#pragma once

#include "..\..\core\io\xml.h"
#include "..\..\core\types\colour.h"
#include "..\..\core\types\optional.h"

#include "..\library\libraryStored.h"

namespace Collision
{
	class Model;
	class PhysicalMaterial;
};

namespace Framework
{
	class Skeleton;

	class CollisionModel
	: public LibraryStored
	{
		FAST_CAST_DECLARE(CollisionModel);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		CollisionModel(Library * _library, LibraryName const & _name);

		Collision::Model * get_model() { return model; }
		Collision::Model const * get_model() const { return model; }
		void replace_model(Collision::Model* _model);

		void refresh_materials();

		void cache_for(Skeleton const * _skeleton);

		struct FuseModel
		{
			CollisionModel const* model;
			Optional<Transform> placement;
			FuseModel() {}
			explicit FuseModel(CollisionModel const* _model, Optional<Transform> const& _placement = NP) : model(_model), placement(_placement) {}
		};
		void fuse(Array<FuseModel> const & _models); // fuse other models into this one (will leave other models as they were)
		void fuse(Array<CollisionModel const *> const & _models); // fuse other models into this one (will leave other models as they were)
		CollisionModel* create_temporary_hard_copy() const;

	public: // LibraryStored
#ifdef AN_DEVELOPMENT
		override_ void ready_for_reload();
#endif
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		override_ void debug_draw() const { debug_draw_coloured(); }
		override_ void debug_draw_coloured(Optional<Colour> const& _colour = NP) const;
		override_ void log(LogInfoContext& _context) const;

	protected:
		~CollisionModel();

	private:
		Collision::Model* model;

	private:
		struct LoadData
		{
			Array<RefCountObjectPtr<Collision::PhysicalMaterial>*> loadedMaterials;
		};

		LoadData* loadData;
	};
};
