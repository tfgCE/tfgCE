#pragma once

//#include "..\globalDefinitions.h"

#define AsynchronousJobQualifier volatile 
#define AsynchronousJobPointer AsynchronousJobQualifier AsynchronousJob*

#include "asynchronousJobFunc.h"

#include <functional>

namespace Concurrency
{
	class JobExecutionContext;

	class AsynchronousJobData
	{
	public:
		virtual ~AsynchronousJobData() {}
		virtual void release_job_data() { delete this; }
	};

	typedef void(*AsynchronousJobFunc)(JobExecutionContext const * _executionContext, void* _data);

	class AsynchronousJob
	{
		friend class AsynchronousJobList;

	protected:
		AsynchronousJob();
		~AsynchronousJob();

		void setup(AsynchronousJobFunc _func, AsynchronousJobData* _jobData);
		void setup(AsynchronousJobFunc _func, void* _data = nullptr);
		void setup(std::function<void()> _func);
		void execute(JobExecutionContext const * _executionContext);
		void clean();

	private:
		AsynchronousJobFunc execute_func = nullptr;
		std::function<void()> execute_func_alt;
		AsynchronousJobData* jobData = nullptr;
		void* data = nullptr;
		AsynchronousJobPointer next = nullptr;
	};

};
