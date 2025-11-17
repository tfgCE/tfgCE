#include "collisionModel.h"

#include "..\..\core\types\names.h"
#include "..\..\core\collision\model.h"
#include "..\..\core\debug\debugRenderer.h"

#include "physicalMaterialStub.h"
#include "physicalMaterialLoadingContext.h"
#include "..\library\library.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(CollisionModel);
LIBRARY_STORED_DEFINE_TYPE(CollisionModel, collisionModel);

CollisionModel::CollisionModel(Library * _library, LibraryName const & _name)
: base(_library, _name)
, model(new Collision::Model())
, loadData(nullptr)
{
}

CollisionModel::~CollisionModel()
{
	delete_and_clear(model);
	delete_and_clear(loadData);
}

void CollisionModel::replace_model(Collision::Model* _model)
{
	an_assert(_model);
	if (_model == model)
	{
		return;
	}
	delete_and_clear(model);
	delete_and_clear(loadData);
	model = _model;
}

void CollisionModel::refresh_materials()
{
	if (model)
	{
		model->refresh_materials();
	}
}

void CollisionModel::cache_for(Skeleton const * _skeleton)
{
	if (model)
	{
		model->cache_for(_skeleton? _skeleton->get_skeleton() : nullptr);
	}
}

#ifdef AN_DEVELOPMENT
void CollisionModel::ready_for_reload()
{
	base::ready_for_reload();

	delete_and_clear(model);
	delete_and_clear(loadData);
	model = new Collision::Model();
}
#endif

bool CollisionModel::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	if (!loadData)
	{
		loadData = new LoadData();
	}

	PhysicalMaterialLoadingContext loadingContext;
	loadingContext.set_create_physical_material([this](RefCountObjectPtr<Collision::PhysicalMaterial>& _pmPtr) { _pmPtr = new PhysicalMaterialStub(); loadData->loadedMaterials.push_back(&_pmPtr); });
	loadingContext.load_material_from_attribute(Names::physicalMaterial);
	loadingContext.set_library_loading_context(&_lc);
	return model->load_from_xml(_node, loadingContext) && result;
}

bool CollisionModel::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		if (loadData)
		{
			bool anyStubsReplaced = false;
			for_every_ptr(loadedMaterialPtr, loadData->loadedMaterials)
			{
				if (PhysicalMaterialStub* stub = fast_cast<PhysicalMaterialStub>(loadedMaterialPtr->get()))
				{
					// replace stub with pointer to actual physical material
					LibraryName physicalMaterialName = stub->get_library_name();
					*loadedMaterialPtr = _library->get_physical_materials().find(physicalMaterialName);
					anyStubsReplaced = true;
				}
			}
			loadData->loadedMaterials.clear();
			delete_and_clear(loadData);

			if (anyStubsReplaced)
			{
				refresh_materials();
			}
		}
	}
	return result;
}

void CollisionModel::debug_draw_coloured(Optional<Colour> const & _colour) const
{
	debug_draw_collision_model(true, false, model, _colour.is_set() ? _colour.get() : Colour::c64Orange, 0.1f, Transform::identity);
}

void CollisionModel::log(LogInfoContext & _context) const
{
	base::log(_context);
	LOG_INDENT(_context);
	model->log(_context);
}

void CollisionModel::fuse(Array<FuseModel> const & _models)
{
	Array<::Collision::Model::FuseModel> models;
	for_every(_model, _models)
	{
		if (_model->model)
		{
			models.push_back(::Collision::Model::FuseModel(_model->model->get_model(), _model->placement));
		}
	}
	model->fuse(models);
}

void CollisionModel::fuse(Array<CollisionModel const *> const & _models)
{
	Array<::Collision::Model const *> models;
	for_every_const_ptr(_model, _models)
	{
		models.push_back(_model->get_model());
	}
	model->fuse(models);
}

CollisionModel* CollisionModel::create_temporary_hard_copy() const
{
	CollisionModel* copy = new CollisionModel(get_library(), LibraryName::invalid());
	copy->be_temporary();
	
	delete_and_clear(copy->model);
	copy->model = model->create_copy();

	return copy;
}
