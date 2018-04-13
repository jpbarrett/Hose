#ifndef HSpectrometerCUDASigned_HH__
#define HSpectrometerCUDASigned_HH__

#include <iostream>
#include <ostream>
#include <ios>
#include <fstream>
#include <sstream>
#include <thread>
#include <map>
#include <utility>
#include <stdint.h>

#include "HLinearBuffer.hh"
#include "HBufferPool.hh"

#include "spectrometer.h"


namespace hose
{

/*
*File: HSpectrometerCUDASigned.hh
*Class: HSpectrometerCUDASigned
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

class HSpectrometerCUDASigned
{

    public:
        HSpectrometerCUDASigned();
        virtual ~HSpectrometerCUDASigned();

        //set the buffer pool we are working with
        void SetBufferPool( HBufferPool< int16_t >* buffer_pool ){ fBufferPool = buffer_pool;};

        void SetDataLength(size_t length){fDataLength = length;};

        void SetOutputDirectory(std::string dir){fOutputDirectory = dir;};

        void SetNThreads(unsigned int n_threads){fNThreads = n_threads;};
        unsigned int GetNThreads() const {return fNThreads;};

        void SetSleepMicroSeconds(unsigned int usec){fSleepTime = usec;};

        //create and launch the threads doing the processingroutine
        void LaunchThreads();

        //signal to the threads to terminate on completion of work
        void SignalTerminateOnComplete(){fSignalTerminate = true;}

        //force the threads to abandon any remaining work, and terminate immediately
        void ForceTermination(){fForceTerminate = true;}

        //join and destroy threads
        void JoinThreads();

    private:

        //length of the data, needed for workspace allocation
        size_t fDataLength;

        //the data processing loop for each thread
        void ProcessLoop();

        //thread data
        unsigned int fNThreads;
        bool fSignalTerminate;
        bool fForceTerminate;

        unsigned int fSleepTime;

        HBufferPool< int16_t >* fBufferPool;

        std::vector< std::thread > fThreads;

        //default output directory 
        std::string fOutputDirectory;

};


}

#endif /* end of include guard: HSpectrometerCUDASigned */
