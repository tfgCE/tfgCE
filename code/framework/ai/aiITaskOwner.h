#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\fastCast.h"

namespace Framework
{
	namespace AI
	{
		interface_class ITask;

		interface_class ITaskOwner
		{
			FAST_CAST_DECLARE(ITaskOwner);
			FAST_CAST_END();
		public:
			virtual ~ITaskOwner() {}

			// notifies about execution state
			virtual void on_task_release(ITask* _task) {}
		};
	};
};
