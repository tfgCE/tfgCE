#include "navSystem.h"

#include "navNode.h"
#include "navPlacementAtNode.h"
#include "navTask.h"

#include "tasks\navTask_BuildNavMesh.h"

#include "..\game\game.h"
#include "..\world\room.h"

#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\memory\memory.h"
#include "..\..\core\system\core.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace Nav;

//

Nav::System::System(Game* _game)
: game(_game)
{
}

Nav::System::~System()
{
}

void Nav::System::add(Task* _task)
{
	Concurrency::ScopedSpinLock lock(tasksLock, true, TXT("Nav::System::add"));

	if (fast_cast<Nav::Tasks::BuildNavMesh>(_task))
	{
		++pendingBuildTasksCount;
	}
	
	// prefer writers as we may modify navmesh and reader result may be not that useful
	if (_task->is_writer())
	{
		// add writer before any readers
		for_every_ref(task, tasks)
		{
			if (task->is_reader())
			{
				tasks.insert(for_everys_iterator(task).to_non_const(), TaskPtr(_task));
				if (!game->is_running_single_threaded())
				{
					trigger_next_task_internal();
				}
				return;
			}
		}
	}

	// no readers or another reader
#ifdef AN_DEVELOPMENT
	for_every_ref(task, tasks)
	{
		an_assert(task != _task);
	}
#endif
	tasks.push_back(TaskPtr(_task));
	if (!game->is_running_single_threaded())
	{
		trigger_next_task_internal();
	}
}

void Nav::System::cancel_all()
{
	Concurrency::ScopedSpinLock lock(tasksLock, true, TXT("Nav::System::cancel_all"));

	for_every_ref(task, tasks)
	{
		if (!task->is_done() && ! task->is_cancel_requested())
		{
#ifdef AN_OUTPUT_NAV_GENERATION
			output_colour_nav();
			output(TXT("cancelling nav task"));
			output_colour();
#endif
			task->cancel();
		}
	}
}

void Nav::System::cancel_related_to(Room* _room)
{
	Concurrency::ScopedSpinLock lock(tasksLock, true, TXT("Nav::System::cancel_related_to"));

	for_every_ref(task, tasks)
	{
		if (!task->is_done())
		{
			task->cancel_if_related_to(_room);
		}
	}
}

void Nav::System::cancel_related_to(Nav::Mesh* _navMesh)
{
	Concurrency::ScopedSpinLock lock(tasksLock, true, TXT("Nav::System::cancel_related_to"));

	for_every_ref(task, tasks)
	{
		if (!task->is_done())
		{
			task->cancel_if_related_to(_navMesh);
		}
	}
}

void Nav::System::async_perform_all()
{
	ASSERT_ASYNC;

	Concurrency::ScopedSpinLock lock(tasksLock, TXT("Nav::System::async_perform_all"));

	while (!roomsRequiringNewNavMesh.is_empty() || ! tasks.is_empty())
	{
		while (! tasks.is_empty())
		{
			auto* firstTask = tasks.get_first().get();

			if (firstTask->is_done())
			{
				tasks.pop_front();
			}
			else if (firstTask->is_active())
			{
				// shouldn't be happening right now! leave to be performed in a normal way
				return;
			}
			else
			{
				firstTask->state = TaskState::Running;

#ifdef AN_LOG_NAV_TASKS
				firstTask->log_to_output(TXT("perform (all)"));
#endif

				firstTask->perform();

				an_assert(firstTask->is_done(), TXT("use end_* function"));

				tasks.pop_front();
			}
		}

		{
			Room* room = roomsRequiringNewNavMesh.get_first();
			roomsRequiringNewNavMesh.pop_front();
			room->queue_build_nav_mesh_task();
			pendingRoomsCount = roomsRequiringNewNavMesh.get_size();
		}
	}
}

void Nav::System::advance()
{
	scoped_call_stack_info(TXT("Nav::System::advance"));
	{
		Concurrency::ScopedSpinLock lock(roomsLock);

		if (!roomsRequiringNewNavMesh.is_empty())
		{
			// have only one build nav mesh task queued
			bool queuedBuildNavMeshTask = false;

			{
				Concurrency::ScopedSpinLock lock(tasksLock, TXT("Nav::System::advance"));
				for_every_ref(task, tasks)
				{
					if (fast_cast<Nav::Tasks::BuildNavMesh>(task))
					{
						queuedBuildNavMeshTask = true;
						break;
					}
				}
			}

			if (!queuedBuildNavMeshTask)
			{
				Room* room = roomsRequiringNewNavMesh.get_first();
				roomsRequiringNewNavMesh.pop_front();
				room->queue_build_nav_mesh_task();
				pendingRoomsCount = roomsRequiringNewNavMesh.get_size();

			}
		}
	}

	trigger_next_task();
}

void Nav::System::trigger_next_task()
{
	if (!game->is_running_single_threaded())
	{
		Concurrency::ScopedSpinLock lock(tasksLock, true, TXT("Nav::System::trigger_next_task"));

		trigger_next_task_internal();
	}
	else
	{
		// we're single threaded anyway, right?
		trigger_next_task_internal();
	}
}

void Nav::System::trigger_next_task_internal()
{
	if (tasks.is_empty())
	{
		performingTaskOrQueued = false;
		pendingBuildTasksCount = 0;
		return;
	}

	todo_note(TXT("remove tasks that waited for too long"));

	// remove already done (we should update variables in the next go)
	while (auto* firstTask = tasks.get_first().get())
	{
		if (firstTask->is_done())
		{
			tasks.pop_front();
			if (tasks.is_empty())
			{
				// no more tasks!
				return;
			}
		}
		else if (firstTask->is_active())
		{
			// no need to trigger next task
			return;
		}
		else
		{
			break;
		}
	}

	int newPendingBuildTasksCount = 0;

	int activeTasks = 0;
	for_every_ref(task, tasks)
	{
		if (task->is_queued_job() || task->is_running())
		{
			++activeTasks;
		}
		if (fast_cast<Tasks::BuildNavMesh>(task))
		{
			++newPendingBuildTasksCount;
		}
	}
	performingTaskOrQueued = activeTasks != 0;
	pendingBuildTasksCount = newPendingBuildTasksCount;

#ifdef AN_DEVELOPMENT
	inWritingMode = false;
#endif

	if (!game->does_allow_async_jobs())
	{
		return;
	}

	if (activeTasks > 0)
	{
		// wait for all to finish
		return;
	}

	auto* firstTask = tasks.get_first().get();

	if (firstTask->is_writer() && firstTask->is_pending())
	{
#ifdef AN_DEVELOPMENT
		inWritingMode = true;
#endif
		firstTask->queue_async_job(game);
		performingTaskOrQueued = true;
	}
	else
	{
		for_every_ref(task, tasks)
		{
			// allow multiple reading tasks, as long as they are in order
			if (task->is_reader() && task->is_pending())
			{
				task->queue_async_job(game);
				performingTaskOrQueued = true;
			}
			else
			{
				break;
			}
		}
	}
}

void Nav::System::end()
{
	ending = true;
}

bool Nav::System::advance_till_done()
{
	trigger_next_task();

	{
		Concurrency::ScopedSpinLock lock(tasksLock, TXT("Nav::System::advance_till_done"));
		if (tasks.is_empty())
		{
			return true;
		}
	}

	return false;
}

void Nav::System::mark_new_nav_mesh_for_room_required(Room* _room)
{
	Concurrency::ScopedSpinLock lock(roomsLock);

	roomsRequiringNewNavMesh.push_back_unique(_room);
	pendingRoomsCount = roomsRequiringNewNavMesh.get_size();
}

void Nav::System::new_nav_mesh_for_room_no_longer_needed(Room* _room)
{
	Concurrency::ScopedSpinLock lock(roomsLock);

	roomsRequiringNewNavMesh.remove(_room);
	pendingRoomsCount = roomsRequiringNewNavMesh.get_size();
}

void Nav::System::log_tasks(LogInfoContext& _log)
{
	Concurrency::ScopedSpinLock lock(tasksLock, TXT("Nav::System::log_tasks"));

	for_every_ref(task, tasks)
	{
		_log.log(TXT("task %02i %S [%S]"), for_everys_index(task), task->get_log_name(), TaskState::to_char(task->state));
		{
			LOG_INDENT(_log);
			task->log_task(_log);
		}
	}
}
