#pragma once

#include "performanceSystem.h"

namespace Performance
{
	class ScopedMeasureLongJob
	{
	public:
		ScopedMeasureLongJob(tchar const * _name);
		~ScopedMeasureLongJob();

	private:
		String name;
		Timer timer;
	};

};
