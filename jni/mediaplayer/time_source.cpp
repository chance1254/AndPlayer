#include <stddef.h>
#include <sys/time.h>

extern "C" {

#include "libavformat/avformat.h"
#include "libavutil/time.h"
	
} // end of extern C

#include "time_source.h"

namespace ffmpeg {

SystemTimeSource::SystemTimeSource()
    : mStartTime(GetSystemTime())
{

}

double SystemTimeSource::getRealTime() 
{
    return GetSystemTime() - mStartTime;
}

// static
double SystemTimeSource::GetSystemTime()
{
    return av_gettime() / 1000000.0;
}

}  // namespace ffmpeg

