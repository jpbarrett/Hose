#include "HSimpleMultiThreadedSpectrumDataWriter.hh"

namespace hose
{


HSimpleMultiThreadedSpectrumDataWriter::HSimpleMultiThreadedSpectrumDataWriter(){};

HSimpleMultiThreadedSpectrumDataWriter::~HSimpleMultiThreadedSpectrumDataWriter(){};

void 
HSimpleMultiThreadedSpectrumDataWriter::SetOutputDirectory(std::string output_dir)
{
    fOutputDirectory = output_dir;
}



void 
HSimpleMultiThreadedSpectrumDataWriter::ExecuteThreadTask() override
{
    //get a buffer from the buffer handler
    HLinearBuffer< spectrometer_data >* tail = nullptr;
    if( this->fBufferPool->GetConsumerPoolSize() != 0 )
    {
        //grab a buffer to process
        HConsumerBufferPolicyCode buffer_code = this->fBufferHandler.ReserveBuffer(this->fBufferPool, tail);

        if(buffer_code == HConsumerBufferPolicyCode::success && tail != nullptr)
        {
            std::lock_guard<std::mutex> lock( tail->fMutex );

            std::stringstream ss;
            ss << "./";
            ss << tail->GetMetaData()->GetLeadingSampleIndex();
            ss << ".bin";

            std::ofstream out_file;
            out_file.open (ss.str().c_str(),  std::ios::out | std::ios::binary);
            out_file.write( (char*)(tail->GetData()), (std::streamsize) ( tail->GetArrayDimension(0) )*sizeof(spectrometer_data) );
            out_file.close();

            //free the tail for re-use
            this->fBufferHandler.ReleaseBufferToProducer(this->fBufferPool, tail);
        }
    }
}
    
bool 
HSimpleMultiThreadedSpectrumDataWriter::WorkPresent() 
{
    return ( this->fBufferPool->GetConsumerPoolSize() != 0 );
}

void 
HSimpleMultiThreadedSpectrumDataWriter::Idle() 
{
    usleep(10);
}



}//end of namespace
