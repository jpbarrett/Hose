#include "HSimpleMultiThreadedSpectrumDataWriter.hh"

#include <cstring>

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
    ss << fExperimentName;

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


    std::stringstream ss2;
    ss2 << fBaseOutputDirectory;
    ss2 << "/";
    ss2 << fExperimentName;
    ss2 << "/";
    ss2 << fScanName;

    //now check if the directory we want to make already exists, if not, create it
    struct stat sb3;
    if(!( stat(ss2.str().c_str(), &sb3) == 0 && S_ISDIR(sb3.st_mode) ))
    {
        int dirstatus = mkdir(ss2.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

        if(dirstatus)
        {
            std::cout<<"Problem when attempting to create directory: "<< ss2.str() << std::endl;
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
                //we rely on acquisitions start time, sample index, and sideband/pol flags to uniquely name/stamp a file
                std::stringstream ss;
                ss << fBaseOutputDirectory;
                ss << "/";
                ss << fExperimentName;
                ss << "/";
                ss << fScanName;
                ss << "/";
                ss <<  sdata->acquistion_start_second;
                ss << "_";
                ss <<  sdata->leading_sample_index;
                ss << "_";
                ss <<  tail->GetMetaData()->GetSidebandFlag();
                ss <<  tail->GetMetaData()->GetPolarizationFlag();

                std::string spec_filename = ss.str() + ".spec";
                std::string noise_power_filename = ss.str() + ".npow";

                if(sdata->leading_sample_index == 0)
                {
                    std::cout<<"got a new acquisition at sec: "<<sdata->acquistion_start_second<<std::endl;
                    std::cout<<"writing to "<<ss.str()<<std::endl;
                }

                // HSpectrumObject< float > spec_data;
                // spec_data.SetSidebandFlag(tail->GetMetaData()->GetSidebandFlag() );
                // spec_data.SetPolarizationFlag(tail->GetMetaData()->GetPolarizationFlag() );
                // spec_data.SetStartTime( sdata->acquistion_start_second );
                // spec_data.SetSampleRate( sdata->sample_rate );
                // spec_data.SetLeadingSampleIndex(  sdata->leading_sample_index );
                // spec_data.SetExperimentName(fExperimentName);
                // spec_data.SetSourceName(fSourceName);
                // spec_data.SetScanName(fScanName);
                // spec_data.SetSampleLength( (sdata->n_spectra)*(sdata->spectrum_length)  );
                // spec_data.SetNAverages( sdata->n_spectra );
                // spec_data.SetSpectrumLength((sdata->spectrum_length)/2+1); //Fix naming of this
                // spec_data.SetSpectrumData(sdata->spectrum);
                // spec_data.ExtendOnAccumulation( tail->GetMetaData()->GetOnAccumulations() );
                // spec_data.ExtendOffAccumulation( tail->GetMetaData()->GetOffAccumulations() );
                // 
                // spec_data.WriteToFile(ss.str());
                // spec_data.ReleaseSpectrumData();



                struct HSpectrumFileStruct* spec_data = CreateSpectrumFileStruct();
                if(spec_data != NULL)
                {
                    spec_data->fHeader.fVersionFlag[0] = SPECTRUM_HEADER_VERSION;
                    spec_data->fHeader.fVersionFlag[1] = 'F'; //F indicates the spectrum data type is a float
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

                    //wipe the struct
                    InitializeSpectrumFileStruct(spec_data);
                    DestroySpectrumFileStruct(spec_data);
                }

                //write out the noise diode data
                struct HNoisePowerFileStruct* power_data = CreateNoisePowerFileStruct();
                if(power_data != NULL)
                {
                    power_data->fHeader.fVersionFlag[0] = NOISE_POWER_HEADER_VERSION;
                    power_data->fHeader.fSidebandFlag[0] = tail->GetMetaData()->GetSidebandFlag() ;
                    power_data->fHeader.fPolarizationFlag[0] = tail->GetMetaData()->GetPolarizationFlag();
                    power_data->fHeader.fStartTime = sdata->acquistion_start_second;
                    power_data->fHeader.fSampleRate = sdata->sample_rate;
                    power_data->fHeader.fLeadingSampleIndex = sdata->leading_sample_index;
                    power_data->fHeader.fSampleLength = (sdata->n_spectra)*(sdata->spectrum_length);
                    power_data->fHeader.fAccumulationLength = tail->GetMetaData()->GetAccumulations()->size();
                    power_data->fHeader.fSwitchingFrequency = 0.0;
                    power_data->fHeader.fBlankingPeriod = 0.0;
                    strcpy(power_data->fHeader.fExperimentName, fExperimentName.c_str() );
                    strcpy(power_data->fHeader.fSourceName, fSourceName.c_str() );
                    strcpy(power_data->fHeader.fScanName, fScanName.c_str() );
        
                    //now point the accumulation data to the right memory block
                    power_data->fAccumulations = static_cast< struct HDataAccumulationStruct* >( &((*(tail->GetMetaData()->GetAccumulations()))[0] ) );


                    
                    std::cout<<"Accum: s,c,sq = "<<power_data->fAccumulations[0].sum_x<<", "<<power_data->fAccumulations[0].count<<", "<<power_data->fAccumulations[0].sum_x2<<std::endl;

                    int ret_val = WriteNoisePowerFile(noise_power_filename.c_str(), power_data);
                    if(ret_val != HSUCCESS){std::cout<<"file error!"<<std::endl;}

                    InitializeNoisePowerFileStruct(power_data);
                    DestroyNoisePowerFileStruct(power_data);
                }
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
