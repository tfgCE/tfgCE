#pragma once

namespace Concurrency
{
	class JobExecutionContext;

	typedef void(*AsynchronousJobFunc)(JobExecutionContext const * _executionContext, void* _data);
};
