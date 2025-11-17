#pragma once

#define MAX_IMMEDIATE_JOB_BATCH_SIZE 10

#define IMMEDIATE_JOB_PARAMS Concurrency::JobExecutionContext const * _executionContext, void** _data, int _count

// requires (void ** _data, int _count)
#define FOR_EVERY_IMMEDIATE_JOB_DATA(Class, variable) \
	ArrayStatic<Class*, MAX_IMMEDIATE_JOB_BATCH_SIZE> elements; SET_EXTRA_DEBUG_INFO(elements, TXT("ImmediateJob.ForEvery.elements")); \
	for (int jobIdx = 0; jobIdx < _count; ++jobIdx, ++_data) \
	{ \
		elements.push_back(plain_cast<Class>(*_data)); \
	} \
	for_every_ptr_prefetch(variable, elements)
