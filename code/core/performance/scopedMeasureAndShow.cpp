#include "scopedMeasureAndShow.h"

using namespace Performance;

ScopedMeasureAndShow::ScopedMeasureAndShow(tchar const * _name, ...)
: minTime(-1.0f)
{
	va_list list;
	va_start(list, _name);
#ifndef AN_CLANG
	int textsize = (int)(tstrlen(_name) + 1);
	int memsize = textsize * sizeof(tchar);
	allocate_stack_var(tchar, format, textsize);
	memory_copy(format, _name, memsize);
	sanitise_string_format(format);
#else
	tchar const* format = _name;
#endif
	tchar buf[4096];
	tvsprintf(buf, 4096-1, format, list);
	name = String(buf);
	timer.start();
}

ScopedMeasureAndShow::ScopedMeasureAndShow(float _minTime, tchar const * _name, ...)
: minTime(_minTime)
{
	va_list list;
	va_start(list, _name);
#ifndef AN_CLANG
	int textsize = (int)(tstrlen(_name) + 1);
	int memsize = textsize * sizeof(tchar);
	allocate_stack_var(tchar, format, textsize);
	memory_copy(format, _name, memsize);
	sanitise_string_format(format);
#else
	tchar const* format = _name;
#endif
	tchar buf[4096];
	tvsprintf(buf, 4096-1, format, list);
	name = String(buf);
	timer.start();
}

ScopedMeasureAndShow::~ScopedMeasureAndShow()
{
	timer.stop();
	if (timer.get_time_ms() >= minTime)
	{
		output(TXT("%8.3fms %10.3fus %15.3fns : %S"), timer.get_time_ms(), timer.get_time_ms() * 1000.0f, timer.get_time_ms() * 1000000.0f, name.to_char());
	}
}
