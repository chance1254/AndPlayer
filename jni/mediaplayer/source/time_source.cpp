#include <stddef.h>
#include <sys/time.h>

extern "C" {

#include "libavformat/avformat.h"

} // end of extern C

#include "../include/time_source.h"
#include <libavformat/avformat.h>
#include "libavutil/time.h"
#include <time.h>

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
    time_t systime;
	time(&systime);//TODO maybe av_gettime
}

}  // namespace ffmpeg
