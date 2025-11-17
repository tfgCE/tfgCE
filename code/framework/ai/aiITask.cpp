#include "aiITask.h"

#include "aiITaskOwner.h"

#include "..\..\core\utils.h"
#include "..\..\core\containers\arrayStack.h"

using namespace Framework;
using namespace AI;

REGISTER_FOR_FAST_CAST(ITask);

Concurrency::Counter ITask::s_id;

ITask::ITask()
{
	get_new_id();
}

ITask::~ITask()
{
	an_assert(owners.is_empty());
}

void ITask::get_new_id()
{
	id = s_id.increase();
}

void ITask::release_task()
{
	// create local copy as we might be manipulating task release
	ARRAY_STACK(ITaskOwner*, pOwners, owners.get_size());
	pOwners = owners;
	for_every_ptr(owner, pOwners)
	{
		owner->on_task_release(this);
	}
	release_task_internal();
}
