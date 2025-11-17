#pragma once

#include "aiITaskOwner.h"
#include "aiLatentTask.h"
#include "aiParam.h"
#include "..\..\core\latent\latent.h"

namespace Framework
{
	namespace AI
	{
		class LatentTask;
		class Logic;
		class MindInstance;
		struct LatentTaskHandle;
		struct RegisteredLatentTaskHandles;

		/**
		 *	If last task owner removes handle, task is dropped automaticaly
		 */
		struct TaskHandle
		: public ITaskOwner
		{
			FAST_CAST_DECLARE(TaskHandle);
			FAST_CAST_BASE(ITaskOwner);
			FAST_CAST_END();
		public:
			TaskHandle();
			TaskHandle(TaskHandle const & _other);
			TaskHandle& operator=(TaskHandle const & _other);
			virtual ~TaskHandle();

			bool is_running(LATENT_FUNCTION_VARIABLE(_taskFunction)) const;
			bool is_running(LatentFunctionInfo const& _taskFunctionInfo) const;
			bool is_running() const { return task != nullptr; }

			LatentTask* start_latent_task(MindInstance* _mindInstance, LatentTaskHandle* _latentTaskHandle, LatentFunctionInfo const& _taskFunctionInfo); // returns new task to allow adding params
			LatentTask* switch_latent_task(MindInstance* _mindInstance, LatentTaskHandle* _latentTaskHandle, LatentFunctionInfo const& _taskFunctionInfo); // returns new task to allow adding params
			void start(MindInstance* _mindInstance, ITask* _task);
			void switch_to(MindInstance* _mindInstance, ITask* _task);
			void stop();

		public:
			void log(LogInfoContext& _context) const;
			void log(LogInfoContext & _log, ::Latent::FramesInspection & _framesInspection) const;

		public: // ITaskOwner
			implement_ void on_task_release(ITask* _task);

		protected:
			virtual void drop_task();

		protected:
			MindInstance* mindInstance = nullptr;
			ITask* task = nullptr;

			void start_new(MindInstance* _mindInstance, ITask* _task);
		};

		/**
		 *	For latent tasks it is better to use this, provides feedback through result
		 */
		struct LatentTaskHandle
		: protected TaskHandle
		{
			typedef TaskHandle base;
			typedef Framework::AI::Params ResultType; // try if it is enough

		public:
			LatentTaskHandle();
			LatentTaskHandle(LatentTaskHandle const & _other);
			LatentTaskHandle& operator=(LatentTaskHandle const & _other);

			bool is_running(LATENT_FUNCTION_VARIABLE(_taskFunction)) const { return base::is_running(_taskFunction); }
			bool is_running(LatentFunctionInfo const& _taskFunctionInfo) const { return base::is_running(_taskFunctionInfo); }
			bool is_running() const { return base::is_running(); }
			void stop() { base::stop(); info = LatentTaskInfo(); }

			void allow_to_interrupt(bool _allowToInterrupt) { info.allowToInterrupt = _allowToInterrupt; }
			bool can_be_interrupted() const { return info.allowToInterrupt; }

			bool can_start_new() const { return !is_running() || info.allowToInterrupt; } // can consider trying new task
			bool can_start(LatentTaskInfo const & _info) const
				{ return _info.taskFunctionInfo.taskFunction &&
					(!is_running() ||		((info.allowToInterrupt || _info.startImmediately) &&
											 info.taskFunctionInfo.taskFunction != _info.taskFunctionInfo.taskFunction &&
											 ((! _info.onlyIfPriorityIsHigher && info.priority <= _info.priority) || (info.priority < _info.priority)))); }
			LatentTask* start_latent_task(MindInstance* _mindInstance, LatentTaskInfo const & _info);
			LatentTask* start_latent_task(MindInstance* _mindInstance, LatentTaskInfoWithParams const & _info);
			LatentTask* switch_latent_task(MindInstance* _mindInstance, LatentTaskInfo const & _info);
			LatentTask* switch_latent_task(MindInstance* _mindInstance, LatentTaskInfoWithParams const & _info);

			ResultType & access_result() { return result; }
			ResultType const & get_result() const { return result; }

		public:
			void log(LogInfoContext& _context) const;
			void log(LogInfoContext & _log, ::Latent::FramesInspection & _framesInspection) const { return base::log(_log, _framesInspection); }

		public: // ITaskOwner
			override_ void on_task_release(ITask* _task);

		protected: // TaskHandle
			override_ void drop_task();

		private:
			LatentTaskInfo info;
			ResultType result;
		};

		/**
		 *	Loadable from xml
		 */
		struct RegisteredLatentTasksInfo
		{
			bool load_from_xml(IO::XML::Node const * _node, tchar const * _childName);

		private:
			Array<Name> tasks;

			friend struct RegisteredLatentTaskHandles;
		};

		/**
		 *	Creates as many latent tasks as we need
		 */
		struct RegisteredLatentTaskHandles
		{
		public:
			RegisteredLatentTaskHandles();
			~RegisteredLatentTaskHandles();

			void add(MindInstance* _mindInstance, Name const & _latentTaskFunction);
			void add(MindInstance* _mindInstance, RegisteredLatentTasksInfo const & _info);

		public:
			void log(LogInfoContext & _log, ::Latent::FramesInspection & _framesInspection) const;

		private:
			Array<LatentTaskHandle> tasks;
		};
	};
};

DECLARE_REGISTERED_TYPE(Framework::AI::LatentTaskHandle);
DECLARE_REGISTERED_TYPE(Framework::AI::LatentTaskHandle*);
DECLARE_REGISTERED_TYPE(Framework::AI::RegisteredLatentTaskHandles);
