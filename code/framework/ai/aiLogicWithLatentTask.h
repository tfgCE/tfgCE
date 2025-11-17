#pragma once

#include "aiLogic.h"
#include "aiLatentTask.h"
#include "aiTaskHandle.h"

#define AI_LATENT_STOP_LONG_RARE_ADVANCE() \
	if (auto* imo = mind->get_owner_as_modules_owner()) \
	{ \
		if (auto* ai = imo->get_ai()) \
		{ \
			ai->access_rare_logic_advance().stop_long_wait(); \
		} \
	}

namespace Framework
{
	namespace AI
	{
		class MindInstance;

		/**
		 *	Auto manages adding and removal of executeFunction.
		 *	For more information about ExecuteFunction check aiLatentTask.h should take one latent param - logic
		 *	Execute function will have at least one latent param:
		 *		ADD_LATENT_PARAM(task->access_latent_frame(), Logic*, this);
		 */
		class LogicWithLatentTask
		: public Logic
		{
			FAST_CAST_DECLARE(LogicWithLatentTask);
			FAST_CAST_BASE(Logic);
			FAST_CAST_END();

			typedef Logic base;
		public:
			LogicWithLatentTask(MindInstance* _mind, LogicData const * _logicData, LATENT_FUNCTION_VARIABLE(_executeFunction));
			virtual ~LogicWithLatentTask();

			void log_latent_funcs(LogInfoContext & _log, Latent::FramesInspection & _framesInspection) const;

			void register_latent_task(Name const & _name, LatentFunction _function);
			virtual LatentFunction get_latent_task(Name const & _name);

		public: // Logic
			override_ void advance(float _deltaTime);

			override_ void end();

		protected:
			void stop_currently_running_task();

			virtual void setup_latent_task(LatentTask* _task); // to allow adding latent params

		private:
			LATENT_FUNCTION_VARIABLE(executeFunction);
			TaskHandle currentlyRunningTask;

			Map<Name, LatentFunction> registeredLatentTasks;
		};
	};
};
