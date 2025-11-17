#pragma once

#include "aiITask.h"
#include "..\..\core\memory\softPooledObject.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\types\name.h"
#include "..\..\core\types\optional.h"

#include "..\..\core\latent\latent.h"

#include "aiObjectVariables.h"

#define ADD_LATENT_TASK_INFO_PARAM(latentTaskInfo, type, value) \
	latentTaskInfo.params.push_param<type>(value)

#define ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(latentTaskInfo) \
	latentTaskInfo.params.push_param_optional_not_provided()

#define AI_LATENT_TASK_FUNCTION(task) Framework::AI::LatentFunctionInfo(task, TXT(#task))

struct LogInfoContext;

namespace Latent
{
	struct FramesInspection;
};

namespace Framework
{
	namespace AI
	{
		class Logic;
		class MindInstance;
		struct LatentTaskHandle;
		struct MindInstanceContext;

		struct LatentFunctionInfo
		{
			LATENT_FUNCTION_VARIABLE(taskFunction) = nullptr;
			tchar const* info = nullptr;

			LatentFunctionInfo() {}
			explicit LatentFunctionInfo(LATENT_FUNCTION_VARIABLE(_taskFunction), tchar const* _info) : taskFunction(_taskFunction), info(_info) {}

			void clear()
			{
				taskFunction = nullptr;
				info = nullptr;
			}

			tchar const* get_info() const { return info ? info : TXT("--"); }
		};

		/**
		 *	Simple information about latent task.
		 *	Will be used as a reference.
		 */
		struct LatentTaskInfo
		{
			int priority = 0;
			bool onlyIfPriorityIsHigher = false; // to allow same priority to end
			LatentFunctionInfo taskFunctionInfo;
			bool allowToInterrupt = true;
			bool startImmediately = false; // only for proposed task info, ignore allowToInterrupt
			Optional<float> timeLimit;

			void clear()
			{
				priority = 0;
				taskFunctionInfo.clear();
				allowToInterrupt = true;
				timeLimit.clear();
			}

			bool is_proposed() const { return taskFunctionInfo.taskFunction != nullptr; }

			bool propose(LatentTaskInfo const & _info)
			{
				if (_info.taskFunctionInfo.taskFunction)
				{
					return propose(_info.taskFunctionInfo, _info.priority, _info.onlyIfPriorityIsHigher, _info.allowToInterrupt, _info.startImmediately, _info.timeLimit);
				}
				return false;
			}
			bool propose(LatentFunctionInfo const & _taskFunctionInfo, Optional<int> _priority = NP, Optional<bool> _onlyIfPriorityIsHigher = NP, Optional<bool> _allowToInterrupt = NP, Optional<bool> _startImmediately = NP, Optional<float> _timeLimit = NP);
			bool propose(Name const & _taskFunction, Optional<int> _priority = NP, Optional<bool> _onlyIfPriorityIsHigher = NP, Optional<bool> _allowToInterrupt = NP, Optional<bool> _startImmediately = NP, Optional<float> _timeLimit = NP);

			void log(LogInfoContext& _context) const;
		};

		/**
		 *	With params. Can be used to start latent function
		 */
		struct LatentTaskInfoWithParams
		: public LatentTaskInfo
		{
			typedef LatentTaskInfo base;

			::Latent::StackVariablesStatic<16> params;

			void clear()
			{
				base::clear();
				params.remove_all_variables();
			}

			bool propose(LatentFunctionInfo const& _taskFunctionInfo, Optional<int> _priority = NP, Optional<bool> _onlyIfPriorityIsHigher = NP, Optional<bool> _allowToInterrupt = NP, Optional<bool> _startImmediately = NP, Optional<float> _timeLimit = NP);
			bool propose(Name const & _taskFunction, Optional<int> _priority = NP, Optional<bool> _onlyIfPriorityIsHigher = NP, Optional<bool> _allowToInterrupt = NP, Optional<bool> _startImmediately = NP, Optional<float> _timeLimit = NP);
		};

		/**
		 *	Latent task is encapsulation of latent function.
		 *	Best to be used with TaskHandle.
		 *	It has one or two latent params by default, mind instance and if provided, latent task handle!
		 *	Check LogicWithLatentTask for further usage examples
		 */
		class LatentTask
		: public SoftPooledObject<LatentTask>
		, public ITask
		{
			FAST_CAST_DECLARE(LatentTask);
			FAST_CAST_BASE(ITask);
			FAST_CAST_END();

			typedef ITask base;
		public:
			void setup(MindInstance* _mindInstance, LatentTaskHandle* _latentTaskHandle, LatentFunctionInfo const& _taskFunctionInfo);
			void set_time_limit(Optional<float> _timeLimit = NP) { timeLimit = _timeLimit; }
			virtual ~LatentTask() {}

			::Latent::Frame & access_latent_frame() { return latentFrame; } // access to add parameters
			LATENT_FUNCTION_TYPE get_task_function() const { return taskFunctionInfo.taskFunction; }
			float get_active_time() const { return activeTime; }

			void log(LogInfoContext & _log, ::Latent::FramesInspection & _framesInspection) const;

		public: // ITask
			override_ bool advance(float _deltaTime);
			override_ void on_break();
			override_ void release_task_internal() { latentFrame.reset(); release(); }

		protected: // SoftPooledObject
			friend class SoftPooledObject<LatentTask>;
			friend class SoftPool<LatentTask>;
			LatentTask();
			override_ void on_get();
			override_ void on_release();

		private:
			float activeTime = 0.0f;
			MindInstance* mindInstance = nullptr;
			Optional<float> timeLimit;
			::Latent::Frame latentFrame;
			LatentFunctionInfo taskFunctionInfo;
		};
	};
};

TYPE_AS_CHAR(Framework::AI::LatentTask);