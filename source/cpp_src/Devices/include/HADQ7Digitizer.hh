#ifndef HADQ7Digitizer_HH__
#define HADQ7Digitizer_HH__

#define LINUX

#ifdef LINUX
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #define Sleep(interval) usleep(1000*interval)
    #define POLLING_FOR_DATA //Must be defined for PCIe devices under Linux
#endif

#include "ADQAPI.h"

#include <string>
#include <stdint.h>
#include <tuple>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <map>
#include <utility>
#include <ctime>

#include "HDigitizer.hh"


//following defines are for debugging
//#define TIMEOUT_WHEN_POLLING
//#define DEBUG_TIMER

namespace hose {

/*
*File: HADQ7Digitizer.hh
*Class: HADQ7Digitizer
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

//buffer sample type
typedef signed short ADQ7_SAMPLE_TYPE;

class HADQ7Digitizer: public HDigitizer< ADQ7_SAMPLE_TYPE, HADQ7Digitizer >
{
    public:
        HADQ7Digitizer();
        virtual ~HADQ7Digitizer();

        void EnableTestPattern(){fTestPattern = 2;};
        void DisableTestPattern(){fTestPattern = 0;};

        void EnableADX(){fADXMode = 0;};
        void DisableADX(){fADXMode = 1;};

        void SelectChannelA(){fEnableA = 1; fEnableB = 0;};
        void SelectChannelB(){fEnableA = 0; fEnableB = 1;};

        void SetDecimationFactor(unsigned int factor);

        void SetNThreads(unsigned int n){fNThreads = n;};

        //signal to the threads to terminate on completion of work
        void SignalTerminateOnComplete();

        //force the threads to abandon any remaining work, and terminate immediately
        void ForceTermination();

    protected:

        friend class HDigitizer< ADQ7_SAMPLE_TYPE, HADQ7Digitizer >;

        //required by digitizer interface
        bool InitializeImpl();
        void AcquireImpl();
        void TransferImpl();
        HDigitizerErrorCode FinalizeImpl();
        void StopImpl();
        void TearDownImpl();

        //set up ADQ board
        bool InitializeBoardInterface();

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
        unsigned int fADXMode;


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

};

}

#endif /* end of include guard: HADQ7Digitizer */
