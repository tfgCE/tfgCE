#pragma once

/**
 *	Nav(igation) System
 *
 *	This is main class that organises Nav Mesh access.
 *	There should be just one task modifying Nav Mesh at the time, while multiple tasks may access it for different purposes.
 *
 *	Nav Mesh is organised into Nav Nodes (for time being I do not see need to have Nav Tiles, as each room should be really small).
 *
 *	Nav Nodes are basic blocks of Nav Mesh. (deriving classes are soft pooled objects)
 *	They are reference counted and could be outdated when Nav Mesh is rebuilt. In such cases, they are only marked as outdated,
 *	but AI may still use such outdated data.
 *	Outdating does not add or remove anything - just marks stuff as outdated.
 *	Each nav node can be immediately marked as outdated (this should help with cases when nav mesh is actor based and we delete actor).
 *
 *	Nav Tasks should be used as asynchronous jobs, although it should be allowed to use them as blocking (!) code blocks.
 *	Each task is accessible through TaskHandle (reference counted by both job and owner). There is a status in it and
 *	each task should be allowed to be immediately interrupted/stopped/cancelled.
 *	Some tasks might be simple "fire and forget" although I advise (note: I am the only programmer, so I advise myself) to hold all tasks.
 *	This is important for doors when they are put into a room and when destroyed, tasks arranged to add door should be cancelled.
 *	Tasks that block Nav Mesh have higher priority.
 *	All tasks have time out (with appropriate result stated in task handle).
 *	Tasks could be:
 *		build nav mesh
 *		alter nav mesh (cut fragment out, useful for dynamic obstacles)
 *		connect/disconnect nav nodes (door is placed in room etc)
 *		find path from one point to another
 *		find point on nav mesh (closest to our point)
 *
 *	Found path is stored with references to Nav Nodes, each Nav Node can be accessed easily
 *
 *	There should be also kind-of-tasks (queries?) that are non-blocking but may fail if nav mesh is inaccessible.
 *	Those could be used for some trivial checks (are we on navmesh - if multiple times we answer "no", let us die).
 */

#include "..\..\core\concurrency\spinLock.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\containers\list.h"

#include "navDeclarations.h"

struct LogInfoContext;

namespace Framework
{
	class Game;
	class Room;

	namespace Nav
	{
		class Mesh;
		struct PlacementAtNode;

		/**
		 *	All tasks should go through system and system creates jobs for them (using appropriate thread)
		 */
		class System
		{
		public:
			System(Game* _game);
			~System();

			//

			void mark_new_nav_mesh_for_room_required(Room* _room);
			void new_nav_mesh_for_room_no_longer_needed(Room* _room);

		public:

			//

			void add(Task* _task);

			//

			void cancel_all();
			void cancel_related_to(Room* _room);
			void cancel_related_to(Nav::Mesh* _navMesh);

			//
			
			void async_perform_all(); // do all tasks right now - this should be called ONLY when the game is not yet running, ie. is loading. otherwise we may get stuck on a spin lock

			//

			void advance();

			void end(); // mark to be ending
			bool advance_till_done(); // try to do all tasks

			bool is_performing_task_or_queued() const { return performingTaskOrQueued; }

			int get_pending_build_tasks_count() const { return pendingBuildTasksCount + pendingRoomsCount /* as they will be require to get done */; }

#ifdef AN_DEVELOPMENT
			bool is_in_writing_mode() const { return inWritingMode; }
#endif

		public:
			void log_tasks(LogInfoContext& _log);

		private: friend class Task;
			 void trigger_next_task();

		private:
			Game* game = nullptr;
			Concurrency::SpinLock tasksLock = Concurrency::SpinLock(TXT("Framework.Nav.System.tasksLock"));
			List<TaskPtr> tasks;
			bool performingTaskOrQueued = false; // queued as async for background job manager
			int pendingBuildTasksCount = 0; // pending, queued, performing
			int pendingRoomsCount = 0; // pending, queued, performing

			Concurrency::SpinLock roomsLock = Concurrency::SpinLock(TXT("Framework.Nav.System.roomsLock"));
			Array<Room*> roomsRequiringNewNavMesh;

			bool ending = false;

#ifdef AN_DEVELOPMENT
			bool inWritingMode = false;
#endif

			void trigger_next_task_internal();
		};
	};
};
