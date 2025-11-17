#pragma once

#include "..\globalDefinitions.h"
#include "..\memory\pooledObject.h"
#include "jobExecutionContext.h"
#include "immediateJobFunc.h"

namespace Concurrency
{

	class ImmediateJob
	{
	public:
		ImmediateJob(ImmediateJobFunc _func, void* _data = nullptr);
		~ImmediateJob();

		void execute(JobExecutionContext const * _executionContext);

	private:
		ImmediateJobFunc execute_func;
		void* data;
	};

};
