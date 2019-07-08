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
#include "HProducer.hh"

typedef int16_t adq_sample_t;


/*
*File: HADQ7Digitizer.hh
*Class: HADQ7Digitizer
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

namespace hose
{


class HADQ7Digitizer:  public HDigitizer< adq_sample_t, HADQ7Digitizer >,  public HProducer< adq_sample_t, HProducerBufferHandler_Steal< adq_sample_t > >
{
    public:
        HADQ7Digitizer();
        virtual ~HADQ7Digitizer();

        //methods to configure board
        void SetDeviceNumber(unsigned int n){fADQDeviceNumber = n;};
        unsigned int GetDeviceNumber() const {return fADQDeviceNumber;};

        void SetSidebandFlag(const char& sflag){fSidebandFlag = sflag;};
        void SetPolarizationFlag(const char& pflag){fPolarizationFlag = pflag;};

        void SetSampleSkipFactor(unsigned int factor);

        double GetSamplingFrequency() const {return fAcquisitionRateMHz*1e6;};

        bool IsInitialized() const {return fInitialized;};
        bool IsArmed() const {return fArmed;};

        void StopAfterNextBuffer(){fStopAfterNextBuffer = true;}

    protected:

        friend class HDigitizer<adq_sample_t, HADQ7Digitizer >;

        //required by digitizer interface
        bool InitializeImpl();
        void AcquireImpl();
        void TransferImpl();
        HDigitizerErrorCode FinalizeImpl();
        void StopImpl();
        void TearDownImpl();

        //required by the producer interface
        virtual void ExecutePreProductionTasks() override; //'initialize' the digitizer
        virtual void ExecutePostProductionTasks() override; //teardown the digitizer
        virtual void ExecutePreWorkTasks() override; //grab a buffer start acquire if we haven't already
        virtual void DoWork() override; //execute a transfer into buffer
        virtual void ExecutePostWorkTasks() override; //finalize the transfer, release the buffer

        //needed by the thread pool interface
        virtual void ExecuteThreadTask() override; //do thread work assoicated with fill the buffer
        virtual bool WorkPresent() override; //check if we have buffer filling work to do

        void ConfigureBoardInterface();

        char fSidebandFlag;
        char fPolarizationFlag;
        unsigned int fClockMode; //0 is internal 10MHz reference, 1 uses external 10MHz reference

        //device data
        void* fADQControlUnit;
        unsigned int fADQDeviceNumber;
        std::string fSerialNumber;
        int fEnableA;
        int fEnableB;
        unsigned int fNChannels;
        double fSampleRate; //raw sample rate
        double fAcquisitionRateMHz; //effective sampling frequency in MHz
        unsigned int fSampleSkipFactor; //number of samples to skip to reduce data rate

        bool fInitialized;
        volatile bool fArmed;
        volatile bool fStopAfterNextBuffer;

        //global sample counter
        volatile uint64_t fCounter;
        HProducerBufferPolicyCode fBufferCode;
        volatile std::time_t fAcquisitionStartTime;
        //internal error code, cleared on stop/acquire, indicates board buffer overflow
        int fErrorCode;

        //internal memory copy queue
        mutable std::mutex fQueueMutex;
        std::queue< std::tuple<void*, void*, size_t> > fMemcpyArgQueue;

};


} //end of namespace

#endif /* end of include guard: HADQ7Digitizer */
