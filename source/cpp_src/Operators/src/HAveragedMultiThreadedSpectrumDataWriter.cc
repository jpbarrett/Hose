#include "HAveragedMultiThreadedSpectrumDataWriter.hh"

namespace hose
{


HAveragedMultiThreadedSpectrumDataWriter::HAveragedMultiThreadedSpectrumDataWriter():
    HDirectoryWriter(),
    fEnable(true)
{};

HAveragedMultiThreadedSpectrumDataWriter::~HAveragedMultiThreadedSpectrumDataWriter(){};


void
HAveragedMultiThreadedSpectrumDataWriter::ExecuteThreadTask()
{
    //get a buffer from the buffer handler
    HLinearBuffer< float >* tail = nullptr;

    if( this->fBufferPool->GetConsumerPoolSize( this->GetConsumerID() ) != 0 )
    {
        //grab a buffer to process
        HConsumerBufferPolicyCode buffer_code = this->fBufferHandler.ReserveBuffer(this->fBufferPool, tail, this->GetConsumerID() );
        if(fEnable && (buffer_code & HConsumerBufferPolicyCode::success) && tail != nullptr)
        {
            std::lock_guard<std::mutex> lock(tail->fMutex);

            //we rely on acquisitions start time, sample index, and sideband/pol flags to uniquely name/stamp a file
            std::stringstream ss;
            ss << fCurrentOutputDirectory;
            ss << "/";
            ss <<  tail->GetMetaData()->GetAcquisitionStartSecond();
            ss << "_";
            ss <<  tail->GetMetaData()->GetLeadingSampleIndex();
            ss << "_";
            ss <<  tail->GetMetaData()->GetSidebandFlag();
            ss <<  tail->GetMetaData()->GetPolarizationFlag();

            std::string spec_filename = ss.str() + ".spec";

            if(tail->GetMetaData()->GetLeadingSampleIndex() == 0)
            {
                std::cout<<"ave spec writer got a new acquisition at sec: "<<tail->GetMetaData()->GetAcquisitionStartSecond()<<std::endl;
                std::cout<<"writing to "<<ss.str()<<std::endl;
            }

            struct HSpectrumFileStruct* spec_data = CreateSpectrumFileStruct();
            if(spec_data != NULL)
            {
                memcpy( spec_data->fHeader.fVersionFlag, SPECTRUM_HEADER_VERSION, HVERSION_WIDTH);
                spec_data->fHeader.fVersionFlag[HVERSION_WIDTH] = 'R'; //R indicates the spectrum data type is a real quantity
                spec_data->fHeader.fVersionFlag[HVERSION_WIDTH+1] = 'F'; //F indicates the spectrum data type is a float
                spec_data->fHeader.fSidebandFlag[0] = tail->GetMetaData()->GetSidebandFlag() ;
                spec_data->fHeader.fPolarizationFlag[0] = tail->GetMetaData()->GetPolarizationFlag();
                spec_data->fHeader.fStartTime = tail->GetMetaData()->GetAcquisitionStartSecond();
                spec_data->fHeader.fSampleRate = tail->GetMetaData()->GetSampleRate();
                spec_data->fHeader.fLeadingSampleIndex =tail->GetMetaData()->GetLeadingSampleIndex();

                spec_data->fHeader.fSampleLength = tail->GetMetaData()->GetNTotalSamplesCollected();
                spec_data->fHeader.fNAverages = tail->GetMetaData()->GetNTotalSpectrum();
                spec_data->fHeader.fSpectrumLength = tail->GetMetaData()->GetPowerSpectrumLength();
                spec_data->fHeader.fSpectrumDataTypeSize = sizeof(float);

                strcpy(spec_data->fHeader.fExperimentName, fExperimentName.c_str() );
                strcpy(spec_data->fHeader.fSourceName, fSourceName.c_str() );
                strcpy(spec_data->fHeader.fScanName, fScanName.c_str() );

                //directly set the pointer to the raw data
                spec_data->fRawSpectrumData = reinterpret_cast< char* >( tail->GetData() );

                int ret_val = WriteSpectrumFile(spec_filename.c_str(), spec_data);
                if(ret_val != HSUCCESS){std::cout<<"file error!"<<std::endl;}

                //wipe the struct (init removes ptr to the tail->GetData spectrum which we do not want to delete!)
                InitializeSpectrumFileStruct(spec_data);
                DestroySpectrumFileStruct(spec_data);
            }


            //Now we write out the noise power data (stored in the buffer meta data)
            std::stringstream ss2;
            ss2 << fCurrentOutputDirectory;
            ss2 << "/";
            ss2 << tail->GetMetaData()->GetAcquisitionStartSecond();
            ss2 << "_";
            ss2 <<  tail->GetMetaData()->GetLeadingSampleIndex();
            ss2 << "_";
            ss2 << tail->GetMetaData()->GetSidebandFlag();
            ss2 << tail->GetMetaData()->GetPolarizationFlag();

            std::string noise_power_filename = ss2.str() + ".npow";

            //write out the noise diode data
            struct HNoisePowerFileStruct* power_data = CreateNoisePowerFileStruct();
            if(power_data != NULL)
            {
                memcpy( power_data->fHeader.fVersionFlag, NOISE_POWER_HEADER_VERSION, HVERSION_WIDTH);
                power_data->fHeader.fSidebandFlag[0] = tail->GetMetaData()->GetSidebandFlag() ;
                power_data->fHeader.fPolarizationFlag[0] = tail->GetMetaData()->GetPolarizationFlag();
                power_data->fHeader.fStartTime = tail->GetMetaData()->GetAcquisitionStartSecond();
                power_data->fHeader.fSampleRate = tail->GetMetaData()->GetSampleRate();
                power_data->fHeader.fLeadingSampleIndex = tail->GetMetaData()->GetLeadingSampleIndex();
                power_data->fHeader.fSampleLength = tail->GetMetaData()->GetNTotalSamplesCollected();
                power_data->fHeader.fAccumulationLength = tail->GetMetaData()->GetAccumulations()->size();
                power_data->fHeader.fSwitchingFrequency =  tail->GetMetaData()->GetNoiseDiodeSwitchingFrequency();
                power_data->fHeader.fBlankingPeriod = tail->GetMetaData()->GetNoiseDiodeBlankingPeriod();
                // strcpy(power_data->fHeader.fExperimentName, fExperimentName.c_str() );
                // strcpy(power_data->fHeader.fSourceName, fSourceName.c_str() );
                // strcpy(power_data->fHeader.fScanName, fScanName.c_str() );

                //now point the accumulation data to the right memory block
                power_data->fAccumulations = static_cast< struct HDataAccumulationStruct* >( &((*(tail->GetMetaData()->GetAccumulations()))[0] ) );

                int ret_val = WriteNoisePowerFile(noise_power_filename.c_str(), power_data);
                if(ret_val != HSUCCESS){std::cout<<"file error!"<<std::endl;}

                InitializeNoisePowerFileStruct(power_data);
                DestroyNoisePowerFileStruct(power_data);
            }

        }

        if(tail != nullptr)
        {
            this->fBufferHandler.ReleaseBufferToConsumer(this->fBufferPool, tail, this->GetNextConsumerID() );
        }

    }
}

bool
HAveragedMultiThreadedSpectrumDataWriter::WorkPresent()
{
    if(this->fBufferPool->GetConsumerPoolSize(this->GetConsumerID()) == 0)
    {
        return false;
    }
    return true;
}

void
HAveragedMultiThreadedSpectrumDataWriter::Idle()
{
    usleep(10);
}



}//end of namespace
