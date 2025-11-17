#pragma once

#include "navDeclarations.h"

#include "..\..\core\fastCast.h"
#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\other\registeredType.h"

namespace Concurrency
{
	class JobExecutionContext;
};

#define CANCEL_NAV_TASK_ON_REQUEST() \
	if (is_cancel_requested()) \
	{ \
		end_cancelled(); \
		return; \
	}

struct LogInfoContext;

namespace Framework
{
	class Game;
	class Room;

	namespace Nav
	{
		class Mesh;

		namespace TaskState
		{
			enum Type
			{
				Pending,
				//
				QueuedJob,
				Running,
				//
				Done,
				//
				Success,
				Cancelled,
				Failed,
				// etc
			};

			inline tchar const* to_char(Type _task)
			{
				if (_task == Pending) return TXT("pending");
				//
				if (_task == QueuedJob) return TXT("queued job");
				if (_task == Running) return TXT("running");
				//
				if (_task == Done) return TXT("done");
				//
				if (_task == Success) return TXT("success");
				if (_task == Cancelled) return TXT("cancelled");
				if (_task == Failed) return TXT("failed");
				//
				return TXT("??");
			}
		};

		/**
		 *	This is base for all soft pooled (or not!) tasks
		 */
		class Task
		: public RefCountObject
		, public Concurrency::AsynchronousJobData
		{
			FAST_CAST_DECLARE(Task);
			FAST_CAST_END();

		public:
			Task(bool _writer);
			virtual ~Task();

			virtual void cancel() { cancelRequested = true; }
			bool is_cancel_requested() const { return cancelRequested; }

			bool is_pending() const { return state == TaskState::Pending; }
			bool is_active() const { return state == TaskState::QueuedJob || state == TaskState::Running; }
			bool is_queued_job() const { return state == TaskState::QueuedJob; }
			bool is_running() const { return state == TaskState::Running; }
			bool is_done() const { return state >= TaskState::Done; }

			bool has_succeed() const { return state == TaskState::Success; }

			bool is_writer() const { return writer; }
			bool is_reader() const { return ! writer; }

#ifdef AN_LOG_NAV_TASKS
			bool log_to_output(tchar const * _info) const;
			virtual bool should_log() const { return false; }
			virtual void log_internal(LogInfoContext & _log) const;
			virtual void destroy_ref_count_object();
#endif
			virtual tchar const* get_log_name() const { return TXT("??"); }
			virtual void log_task(LogInfoContext & _log) const;

		public:
			virtual void cancel_if_related_to(Room* _room) {}
			virtual void cancel_if_related_to(Nav::Mesh* _navMesh) {}

		public: // Concurrency::AsynchronousJobData
			override_ void release_job_data() { release_ref(); /* this way we will release it when we're done with it, otherway we could end up with releasing it, then having dangling pointer with vtable pointing at AsynchronousJobData and trying to delete this */ }

		protected:
			void reset_to_new(); // to allow reinitialisation - useful for soft pooled

			void end_cancelled();
			void end_failed();
			void end_success();
		public:
			void queue_async_job(Game* _game);
			static void perform_job(Concurrency::JobExecutionContext const * _executionContext, void* _data);

		protected:
			virtual void perform() = 0;

		protected:
			bool const writer = false;
			bool cancelRequested = false;

		private:
			TaskState::Type state = TaskState::Pending;

			friend class System;
		};
	}
}

DECLARE_REGISTERED_TYPE(RefCountObjectPtr<Framework::Nav::Task>);

//TYPE_AS_CHAR(Framework::Nav::Task);
