#include "HSpectrometerCUDA.hh"
#include "HSpectrumObject.hh"

#include <fstream>
#include <pthread.h>

namespace hose
{

HSpectrometerCUDA::HSpectrometerCUDA(){};

HSpectrometerCUDA::~HSpectrometerCUDA(){};


bool 
HSpectrometerCUDA::WorkPresent()
{
    if(this->fSignalTerminate)
    {
        std::cout<<"work size = "<<fSourceBufferPool->GetConsumerPoolSize()<<std::endl;
    }
    return (fSourceBufferPool->GetConsumerPoolSize() != 0);
}


void 
HSpectrometerCUDA::ExecuteThreadTask()
{
    //have to assume that the data buffers are constructed with the correct sizes right now

    //initialize the thread workspace
    spectrometer_data* sdata = nullptr;

    //buffers
    HLinearBuffer< spectrometer_data >* sink = nullptr;
    HLinearBuffer< uint16_t>* source = nullptr;

    //std::cout<<"executing thread task"<<std::endl;

    if( fSourceBufferPool->GetConsumerPoolSize() != 0 ) //only do work if there is stuff to process
    {
        //first get a sink buffer from the buffer handler
        HProducerBufferPolicyCode sink_code = this->fSinkBufferHandler.ReserveBuffer(this->fSinkBufferPool, sink);
        if(sink_code == HProducerBufferPolicyCode::success)
        {
            //now grab a source buffer
            HConsumerBufferPolicyCode source_code = this->fSourceBufferHandler.ReserveBuffer(this->fSourceBufferPool, source);
            if(source_code == HConsumerBufferPolicyCode::success)
            {
                if(source != nullptr && sink != nullptr)
                {
                    std::cout<<"got a source and sink buffer"<<std::endl;
                    //lock dem buffers
                    std::lock_guard<std::mutex> sink_lock(sink->fMutex);
                    std::lock_guard<std::mutex> source_lock(source->fMutex);

                    //point the sdata to the buffer object (this is a horrible hack)
                    sdata = &( (sink->GetData())[0] ); //should have buffer size of 1

                    //call Juha's process_vector routine
                    process_vector_no_output(source->GetData(), sdata);
                    std::cout<<"processed on gpu"<<std::endl;

                    //release the buffers
                    this->fSourceBufferHandler.ReleaseBufferToProducer(this->fSourceBufferPool, source);
                    this->fSinkBufferHandler.ReleaseBufferToConsumer(this->fSinkBufferPool, sink);
                }
            }
        }

    }

}



}
