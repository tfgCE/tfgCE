#pragma once

namespace Concurrency
{
	class JobExecutionContext;
	typedef void(*ImmediateJobFunc)(JobExecutionContext const * _executionContext, void** _data, int _count);
};
	
