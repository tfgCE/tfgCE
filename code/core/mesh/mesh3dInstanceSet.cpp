#include "mesh3dInstanceSet.h"

#include "mesh3dRenderingBuffer.h"

using namespace ::System;
using namespace Meshes;

Mesh3DInstanceSet::~Mesh3DInstanceSet()
{
	clear();
}

void Mesh3DInstanceSet::clear()
{
	for_every_ptr(instance, instances)
	{
		delete instance;
	}
	instances.clear();
}

Mesh3DInstance* Mesh3DInstanceSet::add(IMesh3D const * _mesh, Transform const & _placement)
{
	Mesh3DInstance* instance = new Mesh3DInstance(_mesh, nullptr, _placement);
	instances.push_back(instance);
	return instances.get_last();
}

Mesh3DInstance* Mesh3DInstanceSet::add(IMesh3D const * _mesh, Skeleton const * _skeleton, Transform const & _placement)
{
	Mesh3DInstance* instance = new Mesh3DInstance(_mesh, _skeleton, _placement);
	instances.push_back(instance);
	return instances.get_last();
}

void Mesh3DInstanceSet::render(Video3D* _v3d, Mesh3DRenderingContext const & _renderingContext) const
{
	for_every_const_ptr(instance, instances)
	{
		instance->render(_v3d, _renderingContext);
	}
}

void Mesh3DInstanceSet::remove(Mesh3DInstance* _instance)
{
	an_assert(instances.does_contain(_instance));
	instances.remove(_instance);
	delete _instance;
}

void Mesh3DInstanceSet::set_placement(int _index, Transform const & _placement)
{
	if (!instances.is_index_valid(_index))
	{
		return;
	}

	instances[_index]->set_placement(_placement);
}
