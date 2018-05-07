#include "HSpectrometerCUDA.hh"
#include "HSpectrumObject.hh"

#include <fstream>
#include <pthread.h>

namespace hose
{

HSpectrometerCUDA::HSpectrometerCUDA(size_t spectrum_length, size_t n_averages):
    fSpectrumLength(spectrum_length),
    fNAverages(n_averages)
    {

    };


HSpectrometerCUDA::~HSpectrometerCUDA(){};


bool 
HSpectrometerCUDA::WorkPresent()
{
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

    if( fSourceBufferPool->GetConsumerPoolSize() != 0 ) //only do work if there is stuff to process
    {
        //first get a sink buffer from the buffer handler
        HProducerBufferPolicyCode sink_code = this->fSinkBufferHandler.ReserveBuffer(this->fSinkBufferPool, sink);
        if(sink_code == HProducerBufferPolicyCode::stolen)
        {
            std::cout<<"spec stealing buffer"<<std::endl;
            std::cout<<"consumer (raw data) pool size = "<<fSourceBufferPool->GetConsumerPoolSize()<<std::endl;
            std::cout<<"producer (spec data) pool size = "<<fSinkBufferPool->GetProducerPoolSize()<<std::endl;
        }
        if( (sink_code & HProducerBufferPolicyCode::success) && sink != nullptr)
        {
            std::lock_guard<std::mutex> sink_lock(sink->fMutex);

            HConsumerBufferPolicyCode source_code = this->fSourceBufferHandler.ReserveBuffer(this->fSourceBufferPool, source);

            if( (source_code & HConsumerBufferPolicyCode::success) && source !=nullptr)
            {

                std::lock_guard<std::mutex> source_lock(source->fMutex);
                
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

                //call Juha's process_vector routine
                //std::cout<<"CALLING FFT"<<std::endl;
                process_vector_no_output(source->GetData(), sdata);

                //release the buffers
                this->fSourceBufferHandler.ReleaseBufferToProducer(this->fSourceBufferPool, source);
                this->fSinkBufferHandler.ReleaseBufferToConsumer(this->fSinkBufferPool, sink);
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
