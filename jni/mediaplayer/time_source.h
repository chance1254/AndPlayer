#ifndef _TIME_SOURCE_H_
#define _TIME_SOURCE_H_

namespace ffmpeg {

class TimeSource {
public:
    TimeSource() {}
    virtual ~TimeSource() {}

    virtual double getRealTime() = 0;

private:
    TimeSource(const TimeSource &);
    TimeSource &operator=(const TimeSource &);
};

class SystemTimeSource : public TimeSource {
public:
    SystemTimeSource();

    virtual double getRealTime();

private:
    static double GetSystemTime();

    double mStartTime;
};

}

#endif //_TIME_SOURCE_H_
