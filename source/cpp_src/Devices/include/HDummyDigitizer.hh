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


#include "HDummyUniformRawArrayFiller.hh"

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
            fLowerLimit( std::numeric_limits<XSampleType>::min() ),
            fUpperLimit( std::numeric_limits<XSampleType>::max() ),
            fCounter(0),
            fAcquireActive(false),
            fBufferCode(HProducerBufferPolicyCode::unset),
            fSleepDurationNanoSeconds(500)
        {
            this->fAllocator = new HBufferAllocatorNew< XSampleType >();
        };

        virtual ~HDummyDigitizer()
        {
            delete this->fAllocator;
        };

        void SetUpperLimit(XSampleType upper_limit){fUpperLimit = upper_limit;};
        void SetLowerLimit(XSampleType lower_limit){fLowerLimit = lower_limit;};

        void SetSleepDurationNanoSeconds(unsigned int ns){fSleepDurationNanoSeconds = ns;};
        unsigned int GetSleepDurationNanoSeconds() const {return fSleepDurationNanoSeconds;};

    protected:

        friend class HDigitizer< XSampleType, HDummyDigitizer >;

        //function to actually fill a buffer
        void fill(XSampleType* array, size_t sz);

        //limits on array values
        XSampleType fLowerLimit;
        XSampleType fUpperLimit;

        //'samples' counter and aquire start time stamp
        uint64_t fCounter;
        bool fAcquireActive;
        HProducerBufferPolicyCode fBufferCode;
        std::time_t fAcquisitionStartTime;
        unsigned int fSleepDurationNanoSeconds;

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
        virtual void GenerateWork() override; //execute a transfer into buffer
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
    HDummyUniformRawArrayFiller< XSampleType > filler;
    filler.Fill(array, fLowerLimit, fUpperLimit, sz);
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

    //create data output directory, need to make this configurable and move it elsewhere
    std::stringstream ss;
    ss << DATA_INSTALL_DIR;
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
    std::cout<<"in x-fer impl"<<std::endl;
    //make a chunk of work for each thread
    size_t buff_size = this->fBuffer->GetArrayDimension(0);
    XSampleType* raw_ptr = this->fBuffer->GetData();
    size_t chunk_size = buff_size / this->fNThreads;
    size_t remainder = buff_size % this->fNThreads; 

    //pile some work into the work queue
    std::lock_guard< std::mutex > lock(this->fWorkQueueMutex);
    for(unsigned int i=0; i<this->fNThreads-1; i++)
    {
        std::cout<<"thread "<<i<<" work added"<<std::endl;
        fWorkArgQueue.push( std::make_pair( &(raw_ptr[i*chunk_size]), chunk_size) );
    }
    //last chunk might have a slightly different size
    fWorkArgQueue.push( std::make_pair( &(raw_ptr[(this->fNThreads-1)*chunk_size]), remainder+chunk_size) );

    std::cout<<"done xfer"<<std::endl;
    

}

template< typename XSampleType >
HDigitizerErrorCode
HDummyDigitizer< XSampleType >::FinalizeImpl()
{

    std::cout<<"in finalize"<<std::endl;
    //wait until all the threads are idle
    while( !( this->AllThreadsAreIdle() ) && !(this->fForceTerminate) )
    {
        //std::cout<<"thread idle? "<<this->AllThreadsAreIdle()<<std::endl;
        //std::cout<<"work remaining: "<<fWorkArgQueue.size()<<std::endl;
        std::this_thread::sleep_for(std::chrono::nanoseconds(fSleepDurationNanoSeconds));
    }

    //increment the sample counter
    this->fBuffer->GetMetaData()->SetLeadingSampleIndex(fCounter);
    fCounter += this->fBuffer->GetArrayDimension(0);
    std::cout<<"finalize impl"<<std::endl;
    std::cout<<"counter = "<<fCounter<<std::endl;
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
        this->fBuffer->GetMetaData()->SetAquisitionStartSecond( (uint64_t) fAcquisitionStartTime );
        this->fBuffer->GetMetaData()->SetSampleRate(2500000000);
    }
}


//needed by the producer interface
template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::GenerateWork()
{
    //we have an active buffer, transfer the data
    if(fBufferCode == HProducerBufferPolicyCode::success)
    {
        std::cout<<"transferring..."<<std::endl;
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

    std::cout<<"doing thread task"<<std::endl;

    std::lock_guard< std::mutex > lock(this->fWorkQueueMutex);
    if( fWorkArgQueue.size() != 0 )
    {
        //grab the location in the queue we are going to generate output for
        auto dest_len_pair = fWorkArgQueue.front();
        dest = dest_len_pair.first;
        sz = dest_len_pair.second;
        fWorkArgQueue.pop();

        std::cout<<" popping work : size = "<<fWorkArgQueue.size()<<std::endl;
    }
    else
    {
        std::cout<<" no work to do: arg size = "<<fWorkArgQueue.size()<<std::endl;
    }

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
    std::cout<<"executing post production tasks"<<std::endl;
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
