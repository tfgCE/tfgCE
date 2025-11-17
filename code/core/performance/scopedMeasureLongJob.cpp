#include "scopedMeasureLongJob.h"

using namespace Performance;

ScopedMeasureLongJob::ScopedMeasureLongJob(tchar const * _name)
: name(_name)
{
	timer.start();
}

ScopedMeasureLongJob::~ScopedMeasureLongJob()
{
	timer.stop();
	Performance::System::add_long_job(name.to_char(), timer);
}
