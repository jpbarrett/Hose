#include "HSimpleMultiThreadedSpectrumDataWriter.hh"

//needed for mkdir on *NIX
#include <sys/types.h>
#include <sys/stat.h>

namespace hose
{


HSimpleMultiThreadedSpectrumDataWriter::HSimpleMultiThreadedSpectrumDataWriter():
    fExperimentName("unknown"),
    fSourceName("unknown"),
    fScanName("unknown")
{
    //if unassigned use default data dir
    fBaseOutputDirectory = std::string(DATA_INSTALL_DIR);
    // fBufferHandler.SetNAttempts(100);
    // fBufferHandler.SetSleepDurationNanoSeconds(0);
};

HSimpleMultiThreadedSpectrumDataWriter::~HSimpleMultiThreadedSpectrumDataWriter(){};

void 
HSimpleMultiThreadedSpectrumDataWriter::SetBaseOutputDirectory(std::string output_dir)
{
    fBaseOutputDirectory = output_dir;
}

void HSimpleMultiThreadedSpectrumDataWriter::InitializeOutputDirectory()
{
    //assume fBaseOutputDirectory and fScanName have been set
    
    //check for the existence of fBaseOutputDirectory
    struct stat sb;
    if( !(stat(fBaseOutputDirectory.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode) ) )
    {
        std::cout<<"Error base output directory: "<<fBaseOutputDirectory<<" does not exist! Using: "<< DATA_INSTALL_DIR <<std::endl;
        std::stringstream dss;
        dss << DATA_INSTALL_DIR;
        fBaseOutputDirectory = dss.str();
    }

    //create data output directory
    std::stringstream ss;
    ss << fBaseOutputDirectory;
    ss << "/";
    ss << fScanName;

    //now check if the directory we want to make already exists, if not, create it
    struct stat sb2;
    if(!( stat(ss.str().c_str(), &sb2) == 0 && S_ISDIR(sb2.st_mode) ))
    {
        int dirstatus = mkdir(ss.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

        if(dirstatus)
        {
            std::cout<<"Problem when attempting to create directory: "<< ss.str() << std::endl;
        }
    }

}



void 
HSimpleMultiThreadedSpectrumDataWriter::ExecuteThreadTask()
{
    //get a buffer from the buffer handler
    HLinearBuffer< spectrometer_data >* tail = nullptr;
    
    if( this->fBufferPool->GetConsumerPoolSize() != 0 )
    {
        //grab a buffer to process
        HConsumerBufferPolicyCode buffer_code = this->fBufferHandler.ReserveBuffer(this->fBufferPool, tail);

        if(buffer_code & HConsumerBufferPolicyCode::success && tail != nullptr)
        {
            std::lock_guard<std::mutex> lock(tail->fMutex);
            //initialize the thread workspace
            spectrometer_data* sdata = nullptr;

            //get sdata pointer
            sdata = &( (tail->GetData())[0] ); //should have buffer size of 1

            if(sdata != nullptr)
            {
                //we rely on acquisitions start time and sample index to uniquely name/stamp a file
                std::stringstream ss;
                ss << fBaseOutputDirectory;
                ss << "/";
                ss << fScanName;
                ss << "/";
                ss <<  sdata->acquistion_start_second;
                ss << "_";
                ss <<  sdata->leading_sample_index;
                ss << ".bin";

                if(sdata->leading_sample_index == 0)
                {
                    std::cout<<"got a new acquisition at sec: "<<sdata->acquistion_start_second<<std::endl;
                    std::cout<<"writing to "<<ss.str()<<std::endl;
                }

                HSpectrumObject< float > spec_data;
                spec_data.SetStartTime( sdata->acquistion_start_second );
                spec_data.SetSampleRate( sdata->sample_rate );
                spec_data.SetLeadingSampleIndex(  sdata->leading_sample_index );
                spec_data.SetExperimentName(fExperimentName);
                spec_data.SetSourceName(fSourceName);
                spec_data.SetScanName(fScanName);
                spec_data.SetSampleLength( (sdata->n_spectra)*(sdata->spectrum_length)  );
                spec_data.SetNAverages( sdata->n_spectra );
                spec_data.SetSpectrumLength((sdata->spectrum_length)/2+1); //Fix naming of this
                spec_data.SetSpectrumData(sdata->spectrum);
                spec_data.ExtendOnAccumulation( tail->GetMetaData()->GetOnAccumulations() );
                spec_data.ExtendOffAccumulation( tail->GetMetaData()->GetOffAccumulations() );

                spec_data.WriteToFile(ss.str());
                spec_data.ReleaseSpectrumData();
            }
        }
        
        if(tail != nullptr)
        {
            this->fBufferHandler.ReleaseBufferToProducer(this->fBufferPool, tail);
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
