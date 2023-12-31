#ifndef HDummyDigitizer_HH__
#define HDummyDigitizer_HH__

#include <stdint.h>

//needed for mkdir
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <limits>
#include <sstream>
#include <queue>
#include <utility>

extern "C"
{
    #include "HBasicDefines.h"
}

#include "HDummyUniformRawArrayFiller.hh"
#include "HDummyGaussianRawArrayFiller.hh"

#include "HDigitizer.hh"
#include "HProducer.hh"
#include "HBufferAllocatorNew.hh"

namespace hose {

/*
*File: HDummyDigitizer.hh
*Class: HDummyDigitizer
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: Test harness for digitizer interface, generates random numbers
*/


template< typename XSampleType >
class HDummyDigitizer: public HDigitizer< XSampleType, HDummyDigitizer< XSampleType > >, public HProducer< XSampleType, HProducerBufferHandler_Wait< XSampleType > >
{
    public:

        HDummyDigitizer():
            HDigitizer< XSampleType, HDummyDigitizer >(),
            fUseUniformDistribution(true),
            fLowerLimit( std::numeric_limits<XSampleType>::min() ),
            fUpperLimit( std::numeric_limits<XSampleType>::max() ),
            fMean(0),
            fSigma(0),
            fCounter(0),
            fAcquireActive(false),
            fBufferCode(HProducerBufferPolicyCode::unset),
            fSleepDurationNanoSeconds(500),
            fSamplingFrequency(200e6)
        {
            this->fAllocator = new HBufferAllocatorNew< XSampleType >();
        };

        virtual ~HDummyDigitizer()
        {
            delete this->fAllocator;
        };


        void SetGaussianDistributionMeanSigma(XSampleType mean, XSampleType sigma)
        {
            fMean = mean;
            fSigma = sigma;
            fUseUniformDistribution = false;
        }

        void SetUniformDistributionUpperLowerLimit(XSampleType upper_limit, XSampleType lower_limit)
        {
            fUpperLimit = upper_limit;
            fLowerLimit = lower_limit;
            fUseUniformDistribution = true;
        }

        double GetSamplingFrequency() const {return fSamplingFrequency;};

        void SetSleepDurationNanoSeconds(unsigned int ns){fSleepDurationNanoSeconds = ns;};
        unsigned int GetSleepDurationNanoSeconds() const {return fSleepDurationNanoSeconds;};

    protected:

        friend class HDigitizer< XSampleType, HDummyDigitizer >;

        //function to actually fill a buffer
        void fill(XSampleType* array, size_t sz);

        //configure noise statistics
        bool fUseUniformDistribution;
        XSampleType fLowerLimit;
        XSampleType fUpperLimit;
        XSampleType fMean;
        XSampleType fSigma;

        //'samples' counter and aquire start time stamp
        uint64_t fCounter;
        bool fAcquireActive;
        HProducerBufferPolicyCode fBufferCode;
        std::time_t fAcquisitionStartTime;
        unsigned int fSleepDurationNanoSeconds;
        double fSamplingFrequency;

        //work queue for the thread pool
        mutable std::mutex fWorkQueueMutex;
        std::queue< std::pair<XSampleType*, size_t> > fWorkArgQueue; //location to write to and length

        //required by digitizer interface
        bool InitializeImpl(); //initialize the digitizer
        void AcquireImpl(); //start acquisition (arm trigger, etc)
        void TransferImpl(); //transfer buffer data
        HDigitizerErrorCode FinalizeImpl(); //finalize a buffer, check for errors, etc
        void StopImpl(){}; //temporarily stop card acquisition
        void TearDownImpl(){}; //tear down the digitizer

        //required by the producer interface
        virtual void ExecutePreProductionTasks() override; //'initialize' the digitizer
        virtual void ExecutePostProductionTasks() override; //teardown the digitizer
        virtual void ExecutePreWorkTasks() override; //grab a buffer start acquire if we haven't already
        virtual void DoWork() override; //execute a transfer into buffer
        virtual void ExecutePostWorkTasks() override; //finalize the transfer, release the buffer

        //needed by the thread pool interface
        virtual void ExecuteThreadTask() override; //do thread work assoicated with fill the buffer
        virtual bool WorkPresent() override; //check if we have buffer filling work to do
        virtual void Idle() override;
};




template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::fill(XSampleType* array, size_t sz)
{
    if(fUseUniformDistribution)
    {
        HDummyUniformRawArrayFiller< XSampleType > filler;
        filler.Fill(array, fLowerLimit, fUpperLimit, sz);
    }
    else
    {
        HDummyGaussianRawArrayFiller< XSampleType > filler;
        filler.Fill(array, fMean, fSigma, sz);
    }
}




////////////////////////////////////////////////////////////////////////////////
//Digitzer Interface

template< typename XSampleType >
bool
HDummyDigitizer< XSampleType >::InitializeImpl()
{
    fCounter = 0;
    return true;
}

template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::AcquireImpl()
{
    fCounter = 0;
    fAcquisitionStartTime = std::time(nullptr);

    //create data output directory, TODO need to make this configurable and move it elsewhere
    std::stringstream ss;
    ss << STR2(DATA_INSTALL_DIR);
    ss << "/";
    ss << fAcquisitionStartTime;
    int dirstatus = mkdir(ss.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if(dirstatus)
    {
        std::cout<<"Problem when attempting to create directory: "<< ss.str() << std::endl;
    }
}

template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::TransferImpl()
{
    //make a chunk of work for each thread
    size_t buff_size = this->fBuffer->GetArrayDimension(0);
    XSampleType* raw_ptr = this->fBuffer->GetData();
    size_t chunk_size = buff_size / this->fNThreads;
    size_t remainder = buff_size % this->fNThreads; 

    //pile some work into the work queue
    std::lock_guard< std::mutex > lock(this->fWorkQueueMutex);
    for(unsigned int i=0; i<this->fNThreads-1; i++)
    {
        fWorkArgQueue.push( std::make_pair( &(raw_ptr[i*chunk_size]), chunk_size) );
    }
    //last chunk might have a slightly different size
    fWorkArgQueue.push( std::make_pair( &(raw_ptr[(this->fNThreads-1)*chunk_size]), remainder+chunk_size) );
}

template< typename XSampleType >
HDigitizerErrorCode
HDummyDigitizer< XSampleType >::FinalizeImpl()
{
    //wait until all the threads are idle
    while( ( WorkPresent() || !( this->AllThreadsAreIdle() ) ) && !(this->fForceTerminate)  )
    {
        std::this_thread::sleep_for(std::chrono::nanoseconds(fSleepDurationNanoSeconds));
    }

    //increment the sample counter
    this->fBuffer->GetMetaData()->SetLeadingSampleIndex(fCounter);
    fCounter += this->fBuffer->GetArrayDimension(0);
    return HDigitizerErrorCode::success;
}


////////////////////////////////////////////////////////////////////////////////
//Producer Interface

template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::ExecutePreWorkTasks()
{
    //get a buffer from the buffer handler
    HLinearBuffer< XSampleType >* buffer = nullptr;
    fBufferCode = this->fBufferHandler.ReserveBuffer(this->fBufferPool, buffer);
    
    //set the digitizer buffer if succesful
    if(fBufferCode == HProducerBufferPolicyCode::success)
    {
        //successfully got a buffer, assigned it
        this->SetBuffer(buffer);

        //start aquire if we haven't already
        if( !fAcquireActive )
        {
            this->Acquire();
            fAcquireActive = true;
        }

        //configure the buffer meta data
        this->fBuffer->GetMetaData()->SetAcquisitionStartSecond( (uint64_t) fAcquisitionStartTime );
        this->fBuffer->GetMetaData()->SetSampleRate(fSamplingFrequency);
    }
}


//needed by the producer interface
template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::DoWork()
{
    //we have an active buffer, transfer the data
    if(fBufferCode == HProducerBufferPolicyCode::success)
    {
        this->Transfer();
    }
}

template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::ExecutePostWorkTasks()
{
    if(fBufferCode == HProducerBufferPolicyCode::success)
    {   
        HDigitizerErrorCode finalize_code = this->Finalize(); 
        // TODO handle errors in finalize here
        fBufferCode = this->fBufferHandler.ReleaseBufferToConsumer(this->fBufferPool, this->fBuffer);
    }
}


////////////////////////////////////////////////////////////////////////////////
//Thread Pool

template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::ExecuteThreadTask()
{
    XSampleType* dest = nullptr;
    size_t sz = 0;
    std::lock_guard< std::mutex > lock(this->fWorkQueueMutex);
    if( fWorkArgQueue.size() != 0 )
    {
        //grab the location in the queue we are going to generate output for
        auto dest_len_pair = fWorkArgQueue.front();
        dest = dest_len_pair.first;
        sz = dest_len_pair.second;
        fWorkArgQueue.pop();
    }

    std::cout<<"thread: "<< std::this_thread::get_id()<< " processing: "<<dest<<" and size: "<<sz<<std::endl;

    if( dest != nullptr && sz != 0)
    {
        //call the fill function for this chunk
        fill(dest, sz);
    }

}


template< typename XSampleType >
bool
HDummyDigitizer< XSampleType >::WorkPresent()
{
    std::lock_guard< std::mutex > lock(this->fWorkQueueMutex);
    if(fWorkArgQueue.size() > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}


template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::ExecutePreProductionTasks()
{
    //initialization may need to be done elsewhere (for example if the allocator to be constructed requires an initiaized card)
}


template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::ExecutePostProductionTasks()
{
    this->Stop();
    this->TearDown();
    fAcquireActive = false;
}

template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::Idle()
{
    usleep(1);
}



} //end of namespace

#endif /* end of include guard: HDummyDigitizer */
