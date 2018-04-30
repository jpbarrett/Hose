#include "HSimpleMultiThreadedSpectrumDataWriter.hh"

namespace hose
{


HSimpleMultiThreadedSpectrumDataWriter::HSimpleMultiThreadedSpectrumDataWriter()
{
    //if unassigned use default data dir
    fOutputDirectory = std::string(DATA_INSTALL_DIR);
};

HSimpleMultiThreadedSpectrumDataWriter::~HSimpleMultiThreadedSpectrumDataWriter(){};

void 
HSimpleMultiThreadedSpectrumDataWriter::SetOutputDirectory(std::string output_dir)
{
    fOutputDirectory = output_dir;
}


void 
HSimpleMultiThreadedSpectrumDataWriter::ExecuteThreadTask()
{
    //get a buffer from the buffer handler
    HLinearBuffer< spectrometer_data >* tail = nullptr;

    std::cout<<"writer pro buff pool size = "<<fBufferPool->GetProducerPoolSize()<<std::endl;
    std::cout<<"writer cons buff pool size = "<<fBufferPool->GetConsumerPoolSize()<<std::endl;

    if( this->fBufferPool->GetConsumerPoolSize() != 0 )
    {
        //grab a buffer to process
        HConsumerBufferPolicyCode buffer_code = this->fBufferHandler.ReserveBuffer(this->fBufferPool, tail);

        if(buffer_code & HConsumerBufferPolicyCode::success && tail != nullptr)
        {

            std::unique_lock<std::mutex> lock(tail->fMutex, std::try_to_lock);
            if(lock.owns_lock())
            {
                //initialize the thread workspace
                spectrometer_data* sdata = nullptr;

                //get sdata pointer
                sdata = &( (tail->GetData())[0] ); //should have buffer size of 1

                if(sdata != nullptr)
                {
                    //we rely on acquisitions start time and sample index to uniquely name/stamp a file
                    std::stringstream ss;
                    ss << fOutputDirectory;
                    ss << "/";
                    ss <<  sdata->acquistion_start_second;
                    ss << "_";
                    ss <<  sdata->leading_sample_index;
                    ss << ".bin";

                    std::cout<<"file name = "<<ss.str()<<std::endl;
                    // std::cout<<"size of on accumulations = "<<tail->GetMetaData()->GetOnAccumulations()->size()<<std::endl;
                    // std::cout<<"size of off accumulations = "<<tail->GetMetaData()->GetOffAccumulations()->size()<<std::endl;


                    HSpectrumObject< float > spec_data;
                    spec_data.SetStartTime( sdata->acquistion_start_second );
                    spec_data.SetSampleRate( sdata->sample_rate );
                    spec_data.SetLeadingSampleIndex(  sdata->leading_sample_index );
                    spec_data.SetSampleLength( (sdata->n_spectra)*(sdata->spectrum_length)  );
                    spec_data.SetNAverages( sdata->n_spectra );
                    spec_data.SetSpectrumLength((sdata->spectrum_length)/2+1); //Fix naming of this
                    spec_data.SetSpectrumData(sdata->spectrum);
                    spec_data.ExtendOnAccumulation( tail->GetMetaData()->GetOnAccumulations() );
                    spec_data.ExtendOffAccumulation( tail->GetMetaData()->GetOffAccumulations() );

                    spec_data.WriteToFile(ss.str());
                    spec_data.ReleaseSpectrumData();

                    lock.unlock();
                
                    //free the tail for re-use
                    this->fBufferHandler.ReleaseBufferToProducer(this->fBufferPool, tail);
                }
            }

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
