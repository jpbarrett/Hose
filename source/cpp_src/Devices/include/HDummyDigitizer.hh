#ifndef HDummyDigitizer_HH__
#define HDummyDigitizer_HH__

#include <stdint.h>

#include <limits>
#include <queue>
#include <utility>

#include "HDummyUniformRawArrayFiller.hh"

#include "HDigitizer.hh"
#include "HProducer.hh"

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
class HDummyDigitizer: public HDigitizer< XSampleType, HDummyDigitizer >, public HProducer< XSampleType, HProducerBufferHander_Wait >
{
    public:

        HDummyDigitizer():
            HDigitizer< uint16_t, HDummyDigitizer >(),
            fLowerLimit( std::numeric_limits<XSampleType>::min() ),
            fUpperLimit( std::numeric_limits<XSampleType>::max() ),
            fCounter(0),
            fSleepDurationNanoSeconds(500)
        {};

        virtual ~HDummyDigitizer(){};

        void SetUpperLimit(XSampleType upper_limit){fUpperLimit = upper_limit;};
        void SetLowerLimit(XSampleType lower_limit){fLowerLimit = lower_limit;};

        void SetSleepDurationNanoSeconds(unsigned int ns){fSleepDurationNanoSeconds = ns;};
        unsigned int GetSleepDurationNanoSeconds() const {return fSleepDurationNanoSeconds};

    protected:

        friend class HDigitizer< XSampleType, HDummyDigitizer >;

        //function to actually fill a buffer
        void fill(XSampleType* array, size_t sz);

        //limits on array values
        XSampleType fLowerLimit;
        XSampleType fUpperLimit;

        //'samples' counter and aquire start time stamp
        uint64_t fCounter;
        std::time_t fAcquisitionStartTime;
        unsigned int fSleepDurationNanoSeconds;

        //work queue for the thread pool
        mutable std::mutex fWorkQueueMutex;
        std::queue< std::pair<XSampleType*, size_t> > fWorkArgQueue; //location to write to and length

        //required by digitizer interface
        bool InitializeImpl(); //initialize the digitizer
        void AcquireImpl(); //start acquisition (arm trigger, etc)
        void TransferImpl(); //transfer buffer data
        int FinalizeImpl(); //finalize a buffer, check for errors, etc
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

};




template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::fill(XSampleType* array, size_t sz)
{
    HDummyUniformRawArrayFiller< XSampleType > filler;
    filler(array, fLowerLimit, fUpperLimit, sz);
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
}

template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::TransferImpl()
{
    //make a chunk of work for each thread
    size_t buff_size = this->fBuffer->GetArrayDimension(0);
    XSampleType* raw_ptr = this->fBuffer->GetData();
    size_t chunk_size = buff_size / fNThreads;
    size_t remainder = buff_size % fNThreads 

    //pile some work into the work queue
    std::lock_guard< std::mutex > lock(this->fQueueMutex);
    for(unsigned int i=0; i<fNThreads-1; i++)
    {
        fWorkArgQueue.push( std::make_pair( &(raw_ptr[i*chunk_size]), chunk_size);
    }
    //last chunk might have a slightly different size
    fWorkArgQueue.push( std::make_pair( &(raw_ptr[(fNThreads-1)*chunk_size]), remainder+chunk_size);
}

template< typename XSampleType >
int
HDummyDigitizer< XSampleType >::FinalizeImpl()
{
    //wait until all the threads are idle
    while( !AllThreadsAreIdle() || fWorkArgQueue.size() != 0 )
    {
        std::this_thread::sleep_for(std::chrono::nanoseconds(fSleepDurationNanoSeconds));
    }

    //increment the sample counter
    this->fBuffer->GetMetaData()->SetLeadingSampleIndex(fCounter);
    fCounter += this->fBuffer->GetArrayDimension(0);
    return 0;
}





////////////////////////////////////////////////////////////////////////////////
//Producer Interface


template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::ExecutePreWorkTasks()
{
    //configure the buffer meta data
    buffer->GetMetaData()->SetAquisitionStartSecond( (uint64_t) fAcquisitionStartTime );
    buffer->fBuffer->GetMetaData()->SetSampleRate(2500000000);
}


//needed by the producer interface
template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::GenerateWork()
{
    if(status_code == HProducerBufferPolicyCode::success)
    {
        Transfer();
    }
}

template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::ExecutePostWorkTasks()
{
    int finalize_code = Finalize();

    if(status_code == ProducerBufferPolicyCode::success)
    {
        fBufferHandler
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

    if( fWorkArgQueue.size() != 0 )
    {
        //get lock on mutex for queue modification
        std::lock_guard< std::mutex > lock(this->fQueueMutex);
        if( fWorkArgQueue.size() != 0 )
        {
            //grab the location in the queue we are going to generate output for
            auto dest_len_pair = fWorkArgQueue.front();
            dest = dest_len_pair.first;
            sz = dest_len_pair.second;
            fWorkArgQueue.pop();
        }
    }

    if( dest != nullptr && sz != 0)
    {
        //call the fill function for this chunk
        fill(dest, sz);
    }

}


template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::WorkPresent()
{
    return ( fWorkArgQueue.size() != 0 );
}




} //end of namespace

#endif /* end of include guard: HDummyDigitizer */
