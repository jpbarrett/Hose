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
    if(this->fSignalTerminate)
    {
        std::cout<<"work size = "<<fSourceBufferPool->GetConsumerPoolSize()<<std::endl;
    }
    if( fSourceBufferPool->GetConsumerPoolSize() == 0)
    {
        return false;
    }
    return true;
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

    //std::cout<<"thread: "<< std::this_thread::get_id()<<" executing thread task"<<std::endl;
    // 
    // std::cout<<"sink (out to writer) pro buff pool size = "<<fSinkBufferPool->GetProducerPoolSize()<<std::endl;
    // std::cout<<"source (in from digi) cons buff pool size = "<<fSourceBufferPool->GetConsumerPoolSize()<<std::endl;

    if( fSourceBufferPool->GetConsumerPoolSize() != 0 ) //only do work if there is stuff to process
    {
        //first get a sink buffer from the buffer handler
        HProducerBufferPolicyCode sink_code = this->fSinkBufferHandler.ReserveBuffer(this->fSinkBufferPool, sink);
        if( (sink_code & HProducerBufferPolicyCode::success) && sink != nullptr)
        {
            std::lock_guard<std::mutex> sink_lock(sink->fMutex);
            //std::cout<<"thread: "<< std::this_thread::get_id()<<" locking sink: "<<sink<< "\n";
        
            HConsumerBufferPolicyCode source_code = this->fSourceBufferHandler.ReserveBuffer(this->fSourceBufferPool, source);

            if( (source_code & HConsumerBufferPolicyCode::success) && source !=nullptr)
            {

                std::lock_guard<std::mutex> source_lock(source->fMutex);
                //std::cout<<"thread: "<< std::this_thread::get_id()<<" locking source: "<<source<< "\n";

                //calculate the noise rms (may eventually need to move this calculation to the GPU)
                HPeriodicPowerCalculator< uint16_t > powerCalc;
                
                powerCalc.SetSamplingFrequency(fSamplingFrequency);
                powerCalc.SetSwitchingFrequency(fSwitchingFrequency);
                powerCalc.SetBlankingPeriod(fBlankingPeriod);
                
                powerCalc.SetBuffer(source);
                powerCalc.Calculate();

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

                // std::cout<<"thread: "<< std::this_thread::get_id()<<"  ptr: "<<source<<" XX!! size of on accumulations = "<<source->GetMetaData()->GetOnAccumulations()->size()<<std::endl;
                // std::cout<<"thread: "<< std::this_thread::get_id()<<"  ptr: "<<source<<" XX!! size of off accumulations = "<<source->GetMetaData()->GetOffAccumulations()->size()<<std::endl;

                // 
                // std::cout<<sink<<" !! size of on accumulations = "<<sink->GetMetaData()->GetOnAccumulations()->size()<<std::endl;
                // std::cout<<sink<<" !! size of off accumulations = "<<sink->GetMetaData()->GetOffAccumulations()->size()<<std::endl;

                // std::cout<<"------------"<<std::endl;

                //call Juha's process_vector routine
                process_vector_no_output(source->GetData(), sdata);
                // std::cout<<"processed on gpu"<<std::endl;

                //release the buffers
                this->fSourceBufferHandler.ReleaseBufferToProducer(this->fSourceBufferPool, source);
                this->fSinkBufferHandler.ReleaseBufferToConsumer(this->fSinkBufferPool, sink);

                // else
                // {
                //     std::cout<<"lock code = "<<lock_code<<std::endl;
                //     this->fSourceBufferHandler.ReleaseBufferToConsumer(this->fSourceBufferPool, source);
                //     this->fSinkBufferHandler.ReleaseBufferToProducer(this->fSinkBufferPool, sink);
                // }

                //std::cout<<"thread: "<< std::this_thread::get_id()<<" destroying lock on source: "<<source<< "\n";
                //std::cout<<"thread: "<< std::this_thread::get_id()<<" destroying lock on sink: "<<sink << "\n";
            }
            else
            {
                if(sink !=nullptr)
                {
                   this->fSinkBufferHandler.ReleaseBufferToProducer(this->fSinkBufferPool, sink);
                }
            }
        }
    }

}



}
