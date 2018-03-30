#ifndef HThreadPool_HH__
#define HThreadPool_HH__

#include <string>
#include <stdint.h>
#include <tuple>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <map>
#include <utility>


/*
*File: HThreadPool.hh
*Class: HThreadPool
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

class HThreadPool
{
    public:
        HThreadPool();
        virtual ~HThreadPool();

        void Launch();

        void SignalTerminateOnComplete();
        void ForceTermination();

        void Join();


    private:

        virtual void ProcessLoop() = 0;







};

#endif /* end of include guard: HThreadPool */




////////////////////////////////////////////////////////////////////////////////

        //thread management
        void LaunchThreads();
        void JoinThreads();
        void ReadLoop();
        void InsertIdleIndicator();
        void SetIdleIndicatorFalse();
        void SetIdleIndicatorTrue();
        bool AllThreadsAreIdle();

        //ADQ API related data
        bool fBoardInterfaceInitialized;
        void* fADQControlUnit;
        unsigned int fADQDeviceNumber;
        std::string fSerialNumber;
        int fADQAPIRevision;

        //board data
        unsigned int fDecimationFactor;
        double fSampleRate; //sample rate is given in hertz
        double fSampleRateMHz;
        unsigned int fNChannels;

        //aquisition configuration, and time stamp
        unsigned int fNRecords;
        unsigned int fNSamplesPerRecord;
	    //global sample counter, zeroed at the start of an aquisition
    	uint64_t fCounter;
        std::time_t fAcquisitionStartTime;
        int fEnableA;
        int fEnableB;
        unsigned int fTestPattern;


        //thread pool stuff for read-out
        unsigned int fNThreads;
        bool fSignalTerminate;
        bool fForceTerminate;
        unsigned int fSleepTime;
        mutable std::mutex fQueueMutex;
        std::queue< std::tuple<void*, void*, size_t> > fMemcpyArgQueue;
        std::vector< std::thread > fThreads;
        std::mutex fIdleMutex;
        std::map< std::thread::id, bool > fThreadIdleMap;

        //internal error code, cleared on stop/acquire, indicates board buffer overflow
        int fErrorCode;
