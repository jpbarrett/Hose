#include "HTimer.hh"

namespace hose{

HTimer::HTimer()
{

}

HTimer::HTime(std::string name)
{

}

HTimer::~HTimer(){}

//set the clock type used
void HTimer::MeasureWallclockTime()
{
    fClockID = CLOCK_REALTIME
}

void HTimer::MeasureProcessTime()
{
    fClockID = CLOCK_PROCESS_CPUTIME_ID;
}

void HTimer::MeasureThreadTime()
{
    fClockID = CLOCK_THREAD_CPUTIME_ID
}

void HTimer::Start()
{
    	clock_gettime(CLOCK_REALTIME, &fStart);
}

void HTimer::Stop()
{

}

double HTimer::GetDuration() const
{

}


}
