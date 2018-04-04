#ifndef HDummyDigitizer_HH__
#define HDummyDigitizer_HH__

#include <random>
#include <limits>
#include <queue>
#include <stdint.h>

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
        HDummyDigitizer();
        virtual ~HDummyDigitizer();

    protected:

        friend class HDigitizer< XSampleType, HDummyDigitizer >;

        //required by digitizer interface
        bool InitializeImpl();
        void AcquireImpl();
        void TransferImpl();
        int FinalizeImpl();
        void StopImpl(){};
        void TearDownImpl(){};

        //needed by the producer interface (organizes calls to digitizer interface)
        virtual void ExecutePreWorkTasks(ProducerBufferPolicyCode status_code, HLinearBuffer<XBufferItemType>* buffer) override;
        virtual void GenerateWork(ProducerBufferPolicyCode status_code, HLinearBuffer<XBufferItemType>* buffer) override;
        virtual void ExecutePostWorkTasks(ProducerBufferPolicyCode status_code, HLinearBuffer<XBufferItemType>* buffer) override ;

        //needed by the thread pool interface
        virtual void ExecuteThreadTask() override;
        virtual bool WorkPresent() override;

        //function to actually fill the buffer
        void fill_function();

        //random number generator
        std::random_device* fRandom;
        std::mt19937* fGenerator;
        std::uniform_int_distribution<int16_t>* fUniformDistribution;

        //'samples' counter and aquire time stamp
        uint64_t fCounter;
        std::time_t fAcquisitionStartTime;
};




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
    fill_function();
    this->fBuffer->GetMetaData()->SetAquisitionStartSecond( (uint64_t) fAcquisitionStartTime );
    //set the sample rate
    this->fBuffer->GetMetaData()->SetSampleRate(2500000000);
}

template< typename XSampleType >
int
HDummyDigitizer< XSampleType >::FinalizeImpl()
{
    //increment the sample counter
    this->fBuffer->GetMetaData()->SetLeadingSampleIndex(fCounter);
    fCounter += this->fBuffer->GetArrayDimension(0);
    return 0;
}

//needed by the producer interface
template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::GenerateWork(ProducerBufferPolicyCode status_code, HLinearBuffer<XBufferItemType>* buffer)
{
    //






































    //configure buffer information, cat time to uint64_t and set, then set the sample rate
    this->fBuffer->GetMetaData()->SetAquisitionStartSecond( (uint64_t) fAcquisitionStartTime );
    this->fBuffer->GetMetaData()->SetSampleRate(fSampleRate); //check that double to uint64_t conversion is OK here

    unsigned int n_samples_collect  = this->fBuffer->GetArrayDimension(0);
    unsigned int samples_to_collect = this->fBuffer->GetArrayDimension(0);
    unsigned int buffers_filled = 0;
    int collect_result = 0;

    #ifdef TIMEOUT_WHEN_POLLING
        unsigned int timeout = 0;
    #endif

    //start timer
    #ifdef DEBUG_TIMER
        timespec start;
        timespec end;
    	clock_gettime(CLOCK_REALTIME, &start);
    #endif

    while (samples_to_collect > 0)
    {
        unsigned int samples_in_buffer;
        timeout = 0;
        do
        {
            collect_result = ADQ_GetTransferBufferStatus(fADQControlUnit, fADQDeviceNumber, &buffers_filled);
            #ifdef TIMEOUT_WHEN_POLLING
                if( (buffers_filled == 0) && (collect_result) )
                {
                    timeout++;
                    if(timeout > 1000000)
                    {
                        std::cout<<"Error: Time out during data aquisition!"<<std::endl;
                        //stop the card aquisition and bail out
                        fErrorCode = 1;
                        samples_to_collect = 0;
                        return;
                    }
                    usleep(1);
                }
            #endif
        }
        while( (buffers_filled == 0) && (collect_result) && (!fErrorCode));

        collect_result = ADQ_CollectDataNextPage(fADQControlUnit, fADQDeviceNumber);
        samples_in_buffer = MIN(ADQ_GetSamplesPerPage(fADQControlUnit, fADQDeviceNumber), samples_to_collect);

        if(ADQ_GetStreamOverflow(fADQControlUnit, fADQDeviceNumber))
        {
            fErrorCode = 1;
            std::cout<<"Warning: Card streaming overflow!"<<std::endl;
            collect_result = 0;
            samples_to_collect = 0;
        }

        if(collect_result)
        {
            //push the mempy arguments to the thread pool queue
            void* dest = (void*) &( (this->fBuffer->GetData())[n_samples_collect-samples_to_collect]);
            void* src = ADQ_GetPtrStream(fADQControlUnit, fADQDeviceNumber);
            size_t sz = samples_in_buffer*sizeof(signed short);
            samples_to_collect -= samples_in_buffer;
            std::lock_guard< std::mutex > lock(fQueueMutex);
            fMemcpyArgQueue.push( std::make_tuple(dest, src, sz) );
            //std::cout<<"push, sz = "<<sz<<std::endl;
        }
        else
        {
            std::cout<<"Warning: Collect next data page failed!"<<std::endl;
            fErrorCode = 2;
            samples_to_collect = 0;
        }
    }

    #ifdef DEBUG_TIMER
        //stop timer and print
        clock_gettime(CLOCK_REALTIME, &end);
        
        timespec temp;
        if( (end.tv_nsec-start.tv_nsec) < 0)
        {
            temp.tv_sec = end.tv_sec-start.tv_sec-1;
            temp.tv_nsec = (1000000000+end.tv_nsec)-start.tv_nsec;
        }
        else
        {
            temp.tv_sec = end.tv_sec-start.tv_sec;
            temp.tv_nsec = end.tv_nsec-start.tv_nsec;
        }
    	std::cout << temp.tv_sec << "." << temp.tv_nsec << " sec for xfer "<<std::endl;
        std::cout<<"to collect: "<<this->fBuffer->GetArrayDimension(0)<<" samples."<<std::endl;
    #endif
}

template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::ExecutePostWorkTasks(ProducerBufferPolicyCode status_code, HLinearBuffer<XBufferItemType>* buffer)
{
    //put the buffer on the consuemer/producer queue depending on the status code
}


template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::ExecuteThreadTask()
{
    void* dest = nullptr;
    void* src = nullptr;
    size_t sz = 0;

    auto args = std::make_tuple(dest, src, sz);
    if( fMemcpyArgQueue.size() != 0 )
    {
        //get lock on mutex for queue modification
        std::lock_guard< std::mutex > lock(this->fQueueMutex);
        if( fMemcpyArgQueue.size() != 0 )
        {
            //grab the locations in the queue we need to perform memcpy on
            args = fMemcpyArgQueue.front();
            dest = std::get<0>(args);
            src = std::get<1>(args);
            sz = std::get<2>(args);
            fMemcpyArgQueue.pop();
            //std::cout<<"pop"<<std::endl;
        }
    }

    if(  dest != nullptr &&  src != nullptr && sz != 0)
    {
        //do the memcpy
        SetIdleIndicatorFalse();
        memcpy(dest, src, sz);
        // char v = 0X7F;
        // memset(dest,v,sz);
        SetIdleIndicatorTrue();
    }

    SetIdleIndicatorTrue();

}


template< typename XSampleType >
void
HDummyDigitizer< XSampleType >::WorkPresent()
{
    return ( fMemcpyArgQueue.size() != 0 );
}



} //end of namespace

#endif /* end of include guard: HDummyDigitizer */
