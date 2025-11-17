#pragma once

#include "performanceSystem.h"

namespace Performance
{
	class ScopedMeasureAndShow
	{
	public:
		ScopedMeasureAndShow(tchar const * _name = TXT(""), ...);
		ScopedMeasureAndShow(float _minTime, tchar const * _name = TXT(""), ...);
		~ScopedMeasureAndShow();

	private:
		String name;
		Timer timer;
		float minTime;
	};

};
