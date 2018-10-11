#include "HSimpleMultiThreadedSpectrumDataWriter.hh"

namespace hose
{


HSimpleMultiThreadedSpectrumDataWriter::HSimpleMultiThreadedSpectrumDataWriter():
    HDirectoryWriter()
    {
            std::cout<<"simple spec writer = "<<this<<std::endl;
    };

HSimpleMultiThreadedSpectrumDataWriter::~HSimpleMultiThreadedSpectrumDataWriter(){};


void 
HSimpleMultiThreadedSpectrumDataWriter::ExecuteThreadTask()
{
    //get a buffer from the buffer handler
    HLinearBuffer< spectrometer_data >* tail = nullptr;
    
    if( this->fBufferPool->GetConsumerPoolSize(this->GetConsumerID()) != 0 )
    {
        //grab a buffer to process
        std::cout<<"simple spec writer trying to grab a buffer"<<std::endl;
        HConsumerBufferPolicyCode buffer_code = this->fBufferHandler.ReserveBuffer(this->fBufferPool, tail, this->GetConsumerID());

        if(buffer_code & HConsumerBufferPolicyCode::success && tail != nullptr)
        {
            std::lock_guard<std::mutex> lock(tail->fMutex);
            //initialize the thread workspace
            spectrometer_data* sdata = nullptr;

            //get sdata pointer
            sdata = &( (tail->GetData())[0] ); //should have buffer size of 1

            if(sdata != nullptr)
            {
                //we rely on acquisitions start time, sample index, and sideband/pol flags to uniquely name/stamp a file
                std::stringstream ss;
                ss << fCurrentOutputDirectory;
                ss << "/";
                ss <<  sdata->acquistion_start_second;
                ss << "_";
                ss <<  sdata->leading_sample_index;
                ss << "_";
                ss <<  tail->GetMetaData()->GetSidebandFlag();
                ss <<  tail->GetMetaData()->GetPolarizationFlag();

                std::string spec_filename = ss.str() + ".spec";
                std::string noise_power_filename = ss.str() + ".npow";

                //if(sdata->leading_sample_index == 0)
                {
                    std::cout<<"simple spec writer got a new acquisition at sec: "<<sdata->acquistion_start_second<<std::endl;
                    std::cout<<"simple spec sample id: "<<sdata->leading_sample_index<<std::endl;
                    std::cout<<"simple writing to "<<ss.str()<<std::endl;
                }

                struct HSpectrumFileStruct* spec_data = CreateSpectrumFileStruct();
                if(spec_data != NULL)
                {
                    memcpy( spec_data->fHeader.fVersionFlag, SPECTRUM_HEADER_VERSION, HVERSION_WIDTH);
                    spec_data->fHeader.fVersionFlag[HVERSION_WIDTH] = 'R'; //R indicates the spectrum data type is a real quantity
                    spec_data->fHeader.fVersionFlag[HVERSION_WIDTH+1] = 'F'; //F indicates the spectrum data type is a float
                    spec_data->fHeader.fSidebandFlag[0] = tail->GetMetaData()->GetSidebandFlag() ;
                    spec_data->fHeader.fPolarizationFlag[0] = tail->GetMetaData()->GetPolarizationFlag();
                    spec_data->fHeader.fStartTime = sdata->acquistion_start_second;
                    spec_data->fHeader.fSampleRate = sdata->sample_rate;
                    spec_data->fHeader.fLeadingSampleIndex = sdata->leading_sample_index;

                    spec_data->fHeader.fSampleLength = (sdata->n_spectra)*(sdata->spectrum_length);
                    spec_data->fHeader.fNAverages = sdata->n_spectra;
                    spec_data->fHeader.fSpectrumLength = ((sdata->spectrum_length)/2+1); //Fix naming of this
                    spec_data->fHeader.fSpectrumDataTypeSize = sizeof(float);

                    strcpy(spec_data->fHeader.fExperimentName, fExperimentName.c_str() );
                    strcpy(spec_data->fHeader.fSourceName, fSourceName.c_str() );
                    strcpy(spec_data->fHeader.fScanName, fScanName.c_str() );

                    //directly set the pointer to the raw data
                    spec_data->fRawSpectrumData = reinterpret_cast< char* >(sdata->spectrum);

                    int ret_val = WriteSpectrumFile(spec_filename.c_str(), spec_data);
                    if(ret_val != HSUCCESS){std::cout<<"file error!"<<std::endl;}

                    //wipe the struct (init removes ptr to the sdata->spectrum which we do not want to delete!)
                    InitializeSpectrumFileStruct(spec_data);
                    DestroySpectrumFileStruct(spec_data);
                }
            }
        }
        
        if(tail != nullptr)
        {
            std::cout<<"spec writer releasing buffer, consumer id = "<<this->GetConsumerID()<<std::endl;
            this->fBufferHandler.ReleaseBufferToConsumer(this->fBufferPool, tail, this->GetNextConsumerID() );
        }
    }
}
    
bool 
HSimpleMultiThreadedSpectrumDataWriter::WorkPresent() 
{
    if(this->fBufferPool->GetConsumerPoolSize() == 0)
    {
        return false;
    }
    return true;
}

void 
HSimpleMultiThreadedSpectrumDataWriter::Idle() 
{
    usleep(10);
}



}//end of namespace
