#include "HSpectrometerCUDA.hh"

namespace hose
{

HSpectrometerCUDA::HSpectrometerCUDA(size_t spectrum_length, size_t n_averages):
    fSpectrumLength(spectrum_length),
    fNAverages(n_averages)
    {
        //std::cout<<"cuda spectrometer  = "<<this<<std::endl;
    };


HSpectrometerCUDA::~HSpectrometerCUDA(){};


bool
HSpectrometerCUDA::WorkPresent()
{
    if( fSourceBufferPool->GetConsumerPoolSize( this->GetConsumerID() ) == 0)
    {
        return false;
    }
    return true;
}


void
HSpectrometerCUDA::ExecuteThreadTask()
{
    //initialize the thread workspace
    spectrometer_data* sdata = nullptr;

    //buffers
    HLinearBuffer< spectrometer_data >* sink = nullptr;
    HLinearBuffer< SAMPLE_TYPE >* source = nullptr;

    if( fSourceBufferPool->GetConsumerPoolSize( this->GetConsumerID() ) != 0 ) //only do work if there is stuff to process
    {
        //first get a sink buffer from the buffer handler
        HProducerBufferPolicyCode sink_code = this->fSinkBufferHandler.ReserveBuffer(this->fSinkBufferPool, sink);

        if( (sink_code & HProducerBufferPolicyCode::success) && sink != nullptr)
        {
            std::lock_guard<std::mutex> sink_lock(sink->fMutex);

            HConsumerBufferPolicyCode source_code = this->fSourceBufferHandler.ReserveBuffer(this->fSourceBufferPool, source, this->GetConsumerID());

            if( (source_code & HConsumerBufferPolicyCode::success) && source !=nullptr)
            {

                std::lock_guard<std::mutex> source_lock(source->fMutex);

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

                sdata->validity_flag = 1;

                //release the buffers
                this->fSourceBufferHandler.ReleaseBufferToConsumer(this->fSourceBufferPool, source, this->GetNextConsumerID());
                this->fSinkBufferHandler.ReleaseBufferToConsumer(this->fSinkBufferPool, sink);
            }
            else
            {
                if(sink !=nullptr)
                {
                   this->fSinkBufferHandler.ReleaseBufferToConsumer(this->fSinkBufferPool, sink);
                }
            }
        }
    }

}



}
