#include "navTask_DestroyNavMesh.h"

#include "..\navMesh.h"
#include "..\navSystem.h"

#include "..\..\game\game.h"

using namespace Framework;
using namespace Nav;
using namespace Tasks;

//

REGISTER_FOR_FAST_CAST(DestroyNavMesh);

Nav::Task* DestroyNavMesh::new_task(Nav::Mesh* _navMesh)
{
	return new DestroyNavMesh(_navMesh);
}

DestroyNavMesh::DestroyNavMesh(Nav::Mesh* _navMesh)
: base(true)
, navMesh(_navMesh)
{
	navMesh->outdate();
}

void DestroyNavMesh::perform()
{
	CANCEL_NAV_TASK_ON_REQUEST();

	// store in case we were cancelled
	MeshPtr navMeshToDestroy = navMesh;
	navMesh.clear();

	if (navMeshToDestroy.get())
	{
		Mesh* navMeshDestroyed = navMeshToDestroy.get();

		// clear nav mesh and pointer to it
		navMeshToDestroy->clear();
		navMeshToDestroy.clear();

		// cancel other tasks related to this navmesh
		Game::get()->get_nav_system()->cancel_related_to(navMeshDestroyed);
	}

	end_success();
}

void DestroyNavMesh::cancel_if_related_to(Room* _room)
{
	if (navMesh.get() && navMesh->inRoom == _room)
	{
		cancel();
	}
}

void DestroyNavMesh::cancel_if_related_to(Nav::Mesh* _navMesh)
{
	if (navMesh.get() && navMesh == _navMesh)
	{
		cancel();
	}
}

void DestroyNavMesh::cancel()
{
	base::cancel();
	navMesh = nullptr;
}
