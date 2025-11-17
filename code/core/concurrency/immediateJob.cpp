#include "immediateJob.h"

#include "..\performance\performanceUtils.h"

using namespace Concurrency;

ImmediateJob::ImmediateJob(ImmediateJobFunc _func, void* _data)
:	execute_func( _func )
,	data( _data )
{
	an_assert(execute_func != nullptr, TXT("execute function not provided"));
}

ImmediateJob::~ImmediateJob()
{
}

void ImmediateJob::execute(JobExecutionContext const * _executionContext)
{
	scoped_call_stack_info(TXT("immediate job"));
	//MEASURE_PERFORMANCE_COLOURED(immediateJob, Colour::green.with_alpha(0.2f));
	execute_func(_executionContext, &data, 1);
}
