#include "HSpectrometerCUDA.hh"
#include "HSpectrumObject.hh"

#include <fstream>
#include <pthread.h>

namespace hose
{

HSpectrometerCUDA::HSpectrometerCUDA(size_t spectrum_length, size_t n_averages):
    fSpectrumLength(spectrum_length),
    fNAverages(n_averages)
    {};


HSpectrometerCUDA::~HSpectrometerCUDA(){};


bool 
HSpectrometerCUDA::WorkPresent()
{
    // if(this->fSignalTerminate)
    // {
    //     std::cout<<"work size = "<<fSourceBufferPool->GetConsumerPoolSize()<<std::endl;
    // }
    return (fSourceBufferPool->GetConsumerPoolSize() != 0);
}


void 
HSpectrometerCUDA::ExecuteThreadTask()
{
    //have to assume that the data buffers are constructed with the correct sizes right now
    //TODO: add a 'get allocator function to return an allocator for appropriately size spectrum_data objects'

    //initialize the thread workspace
    spectrometer_data* sdata = nullptr;

    //buffers
    HLinearBuffer< spectrometer_data >* sink = nullptr;
    HLinearBuffer< uint16_t>* source = nullptr;

    //std::cout<<"executing thread task"<<std::endl;
    // 
    // std::cout<<"sink (out to writer) pro buff pool size = "<<fSinkBufferPool->GetProducerPoolSize()<<std::endl;
    // std::cout<<"source (in from digi) cons buff pool size = "<<fSourceBufferPool->GetConsumerPoolSize()<<std::endl;

    if( fSourceBufferPool->GetConsumerPoolSize() != 0 ) //only do work if there is stuff to process
    {
        //first get a sink buffer from the buffer handler
        HProducerBufferPolicyCode sink_code = this->fSinkBufferHandler.ReserveBuffer(this->fSinkBufferPool, sink);
        HConsumerBufferPolicyCode source_code = this->fSourceBufferHandler.ReserveBuffer(this->fSourceBufferPool, source);

        if( (sink_code & HProducerBufferPolicyCode::success) && (source_code & HConsumerBufferPolicyCode::success) && sink != nullptr && source !=nullptr)
        {
            std::unique_lock<std::mutex> sink_lock(sink->fMutex, std::defer_lock);
            std::unique_lock<std::mutex> source_lock(source->fMutex, std::defer_lock);

            int lock_code = std::try_lock(sink_lock, source_lock);

            if(lock_code == -1)
            {
                // std::cout<<"got a source and sink buffer"<<std::endl;

                // //calculate the noise rms (may eventually need to move this calculation to the GPU)
                fPowerCalc.SetBuffer(source);
                fPowerCalc.Calculate();

                //point the sdata to the buffer object (this is a horrible hack)
                sdata = &( (sink->GetData())[0] ); //should have buffer size of 1

                //set meta data
                *( sink->GetMetaData() ) = *( source->GetMetaData() );
                sdata->sample_rate = source->GetMetaData()->GetSampleRate();
                sdata->acquistion_start_second = source->GetMetaData()->GetAcquisitionStartSecond();
                sdata->leading_sample_index = source->GetMetaData()->GetLeadingSampleIndex();
                sdata->data_length = source->GetArrayDimension(0); //also equal to fSpectrumLength*fNAverages;
                sdata->spectrum_length = fSpectrumLength;
                sdata->n_spectra = fNAverages;

                //call Juha's process_vector routine
                process_vector_no_output(source->GetData(), sdata);
                // std::cout<<"processed on gpu"<<std::endl;

                source_lock.unlock();
                sink_lock.unlock();

                //release the buffers
                this->fSourceBufferHandler.ReleaseBufferToProducer(this->fSourceBufferPool, source);
                this->fSinkBufferHandler.ReleaseBufferToConsumer(this->fSinkBufferPool, sink);
            }
            // else
            // {
            //     std::cout<<"lock code = "<<lock_code<<std::endl;
            //     this->fSourceBufferHandler.ReleaseBufferToConsumer(this->fSourceBufferPool, source);
            //     this->fSinkBufferHandler.ReleaseBufferToProducer(this->fSinkBufferPool, sink);
            // }
        }
        else
        {
            if(source != nullptr)
            {
                this->fSourceBufferHandler.ReleaseBufferToConsumer(this->fSourceBufferPool, source);
            }

            if(sink !=nullptr)
            {
                this->fSinkBufferHandler.ReleaseBufferToProducer(this->fSinkBufferPool, sink);
            }
        }

    }

}



}
