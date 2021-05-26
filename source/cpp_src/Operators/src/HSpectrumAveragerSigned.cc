#include "HSpectrumAveragerSigned.hh"

namespace hose
{

HSpectrumAveragerSigned::HSpectrumAveragerSigned(size_t spectrum_length, size_t n_buffers):
    fPowerSpectrumLength(spectrum_length),
    fNBuffersToAccumulate(n_buffers)
{
        fAccumulatedSpectrum.resize(spectrum_length);
        fAccumulationBuffer = new HLinearBuffer<float>( &(fAccumulatedSpectrum[0]), spectrum_length);
        fNBuffersAccumulated = 0;
        //std::cout<<"spectrum averager = "<<this<<std::endl;
};


HSpectrumAveragerSigned::~HSpectrumAveragerSigned(){};


bool
HSpectrumAveragerSigned::WorkPresent()
{
    if( fSourceBufferPool->GetConsumerPoolSize( this->GetConsumerID() ) == 0)
    {
        return false;
    }
    return true;
}


void
HSpectrumAveragerSigned::ExecuteThreadTask()
{
    HLinearBuffer< spectrometer_data_s >* source = nullptr;
    spectrometer_data_s* sdata = nullptr;

    if( fSourceBufferPool->GetConsumerPoolSize( this->GetConsumerID() ) != 0 ) //only do work if there is stuff to process
    {
        //grab a source buffer and add its data to the accumulation buffer
        HConsumerBufferPolicyCode source_code = this->fSourceBufferHandler.ReserveBuffer(this->fSourceBufferPool, source, this->GetConsumerID());
        if( (source_code & HConsumerBufferPolicyCode::success) && source !=nullptr)
        {
            std::lock_guard<std::mutex> source_lock(source->fMutex);
            sdata = &( (source->GetData())[0] ); //should have buffer size of 1

            //std::cout<<"got a buff"<<std::endl;

            //first collect the meta-data information from this buffer
            char sideband_flag = source->GetMetaData()->GetSidebandFlag() ;
            char pol_flag = source->GetMetaData()->GetPolarizationFlag();
            uint64_t start_second = source->GetMetaData()->GetAcquisitionStartSecond();
            uint64_t sample_rate = source->GetMetaData()->GetSampleRate();
            uint64_t leading_sample_index = source->GetMetaData()->GetLeadingSampleIndex();
            uint64_t n_spectra = sdata->n_spectra;
            uint64_t n_spectrum_samples_length = sdata->spectrum_length; //number of samples used in FFT to create an individual spectrum
            uint64_t power_spectrum_length = ((sdata->spectrum_length)/2+1); //length of the power spectrum
            uint64_t n_total_samples = (sdata->n_spectra)*(sdata->spectrum_length); //total number of samples used to compute the averaged spectrum we get from this buffer

            //collect the noise power data
            float sum = sdata->sum;
            float sum2 = sdata->sum2;

            //pass checks?
            if(fNBuffersAccumulated == 0 && power_spectrum_length == fPowerSpectrumLength)
            {
                //first buffer recieved since reset, so re-initialize the values
                fNBuffersAccumulated++;
                fSidebandFlag = sideband_flag;
                fPolarizationFlag = pol_flag;
                fAcquisitionStartSecond = start_second;
                fSampleRate = sample_rate;
                fLeadingSampleIndex = leading_sample_index;
                fNTotalSpectrum = n_spectra;
                fNTotalSamplesAccumulated = n_total_samples;

                //set appropriate meta data quantities for the accumulation container
                fNoisePowerAccumulator.SetSampleRate( fSampleRate );
                fNoisePowerAccumulator.SetAcquisitionStartSecond( fAcquisitionStartSecond );
                fNoisePowerAccumulator.SetLeadingSampleIndex( fLeadingSampleIndex );
                fNoisePowerAccumulator.SetSampleLength( n_total_samples );
                fNoisePowerAccumulator.SetNoiseDiodeSwitchingFrequency(0);
                fNoisePowerAccumulator.SetNoiseDiodeBlankingPeriod(0);
                fNoisePowerAccumulator.SetSidebandFlag( fSidebandFlag );
                fNoisePowerAccumulator.SetPolarizationFlag( fPolarizationFlag );

                //noise power data
                struct HDataAccumulationStruct stat;
                stat.start_index = leading_sample_index;
                stat.stop_index = leading_sample_index + n_total_samples;
                stat.sum_x = sum;
                stat.sum_x2 = sum2;
                stat.count = n_total_samples;
                stat.state_flag = H_NOISE_UNKNOWN;
                fNoisePowerAccumulator.AppendAccumulation(stat);

            }
            else if( CheckMetaData(sideband_flag, pol_flag, start_second, sample_rate, leading_sample_index) ) //meta data matches, so accumulate another spectrum
            {
                //accumulate statistics
                //first buffer recieved since reset, so re-initialize the values
                fNBuffersAccumulated++;
                fNTotalSpectrum += n_spectra;
                fNTotalSamplesAccumulated += n_total_samples;
                Accumulate(sdata->spectrum);
                this->fSourceBufferHandler.ReleaseBufferToConsumer(this->fSourceBufferPool, source, this->GetNextConsumerID() );
                source = nullptr;

                //noise power data
                struct HDataAccumulationStruct stat;
                stat.start_index = leading_sample_index;
                stat.stop_index = leading_sample_index + n_total_samples;
                stat.sum_x = sum;
                stat.sum_x2 = sum2;
                stat.count = n_total_samples;
                stat.state_flag = H_NOISE_UNKNOWN;
                fNoisePowerAccumulator.AppendAccumulation(stat);

                //check if we have reached the desired number of buffers,
                if(fNBuffersAccumulated == fNBuffersToAccumulate)
                {
                    WriteAccumulatedSpectrumAverage();
                    WriteNoisePower();
                    Reset();
                }
            }
            else
            {
                //bail out, something changed before we accumulated the required number of buffers,
                //so write out what we have now and re-init
                //WriteAccumulatedSpectrumAverage();
                Reset();

                //now start on new buffer
                fNBuffersAccumulated++;
                fSidebandFlag = sideband_flag;
                fPolarizationFlag = pol_flag;
                fAcquisitionStartSecond = start_second;
                fSampleRate = sample_rate;
                fLeadingSampleIndex = leading_sample_index;
                fNTotalSpectrum = n_spectra;
                fNTotalSamplesAccumulated = n_total_samples;
                Accumulate(sdata->spectrum);

                //noise power data
                struct HDataAccumulationStruct stat;
                stat.start_index = leading_sample_index;
                stat.stop_index = leading_sample_index + n_total_samples;
                stat.sum_x = sum;
                stat.sum_x2 = sum2;
                stat.count = n_total_samples;
                stat.state_flag = H_NOISE_UNKNOWN;
                fNoisePowerAccumulator.AppendAccumulation(stat);

                this->fSourceBufferHandler.ReleaseBufferToConsumer(this->fSourceBufferPool, source, this->GetNextConsumerID() );
                source = nullptr;

                //check if we have reached the desired number of buffers, (unlikely here, can only happen if fNBuffersAccumulated is 1)
                if(fNBuffersAccumulated == fNBuffersToAccumulate)
                {
                    WriteAccumulatedSpectrumAverage();
                    WriteNoisePower();
                    Reset();
                }
            }
        }

        if(source != nullptr)
        {
            //std::cout<<"averager releasing spec source buffer to consumer #"<<this->GetNextConsumerID()<<" in pool: "<<this->fSourceBufferPool<<std::endl;
            //release source buffer
            this->fSourceBufferHandler.ReleaseBufferToConsumer(this->fSourceBufferPool, source, this->GetNextConsumerID() );
        }

    }
}


bool
HSpectrumAveragerSigned::CheckMetaData(char sideband_flag, char pol_flag, uint64_t start_second, uint64_t sample_rate, uint64_t leading_sample_index) const
{
    if(fSidebandFlag != sideband_flag){return false;}
    if(fPolarizationFlag != pol_flag){return false;}
    if(fAcquisitionStartSecond != start_second){return false;}
    if(fSampleRate != sample_rate){return false;}
    if(leading_sample_index < fLeadingSampleIndex){return false;}
    return true;
}

void
HSpectrumAveragerSigned::Accumulate(float* array)
{
    float* accum = fAccumulationBuffer->GetData();
    for(size_t i=0; i<fPowerSpectrumLength; i++)
    {
        accum[i] += array[i];
    }
}

bool
HSpectrumAveragerSigned::WriteAccumulatedSpectrumAverage()
{
    HLinearBuffer< float >* sink = nullptr;
    HProducerBufferPolicyCode sink_code = this->fSinkBufferHandler.ReserveBuffer(this->fSinkBufferPool, sink);
    if( (sink_code & HProducerBufferPolicyCode::success) && sink != nullptr)
    {
        std::lock_guard<std::mutex> sink_lock(sink->fMutex);
        //set the meta data values
        sink->GetMetaData()->SetSidebandFlag(fSidebandFlag);
        sink->GetMetaData()->SetPolarizationFlag(fPolarizationFlag);
        sink->GetMetaData()->SetAcquisitionStartSecond(fAcquisitionStartSecond);
        sink->GetMetaData()->SetSampleRate(fSampleRate);
        sink->GetMetaData()->SetLeadingSampleIndex(fLeadingSampleIndex);
        //bits of meta data specific to averaging spectrum together
        sink->GetMetaData()->SetNTotalSpectrum(fNTotalSpectrum);
        sink->GetMetaData()->SetNTotalSamplesCollected(fNTotalSamplesAccumulated);
        sink->GetMetaData()->SetPowerSpectrumLength(fPowerSpectrumLength);

        //compute average and finish writing meta data
        float* accum = fAccumulationBuffer->GetData();
        float* ave = sink->GetData();
        for(size_t i=0; i<fPowerSpectrumLength; i++)
        {
            ave[i] = accum[i]/(float)fNBuffersAccumulated;;
        }

        //release to consumer
        this->fSinkBufferHandler.ReleaseBufferToConsumer(this->fSinkBufferPool, sink);
        return true;

    }
    if(sink != nullptr)
    {
        this->fSinkBufferHandler.ReleaseBufferToConsumer(this->fSinkBufferPool, sink);
    }
    return false; //failed
}


void HSpectrumAveragerSigned::Reset()
{
    //std::cout<<"reset"<<std::endl;
    //reset the internal accumulation buffer for re-use
    fSidebandFlag = '?';
    fPolarizationFlag = '?';
    fNBuffersAccumulated = 0;
    fSampleRate = 0;
    fAcquisitionStartSecond = 0;
    fLeadingSampleIndex = 0;
    fNTotalSpectrum = 0;
    fNTotalSamplesAccumulated = 0;
    float* accum = fAccumulationBuffer->GetData();
    for(size_t i=0; i<fPowerSpectrumLength; i++)
    {
        accum[i] = 0.0;
    }

    //for the noise power, clear out all the accumulation structs
    fNoisePowerAccumulator.ClearAccumulation();

}

void HSpectrumAveragerSigned::WriteNoisePower()
{
    auto accum_container = &fNoisePowerAccumulator;

    //we rely on acquisitions start time, sample index, and sideband/pol flags to uniquely name/stamp a file
    std::stringstream ss;
    ss << fCurrentOutputDirectory;
    ss << "/";
    ss <<  accum_container->GetAcquisitionStartSecond();
    ss << "_";
    ss <<  accum_container->GetLeadingSampleIndex();
    ss << "_";
    ss <<  accum_container->GetSidebandFlag();
    ss <<  accum_container->GetPolarizationFlag();

    std::string noise_power_filename = ss.str() + ".npow";

    //write out the noise diode data
    struct HNoisePowerFileStruct* power_data = CreateNoisePowerFileStruct();
    if(power_data != NULL)
    {
        memcpy( power_data->fHeader.fVersionFlag, NOISE_POWER_HEADER_VERSION, HVERSION_WIDTH);
        power_data->fHeader.fSidebandFlag[0] = accum_container->GetSidebandFlag() ;
        power_data->fHeader.fPolarizationFlag[0] = accum_container->GetPolarizationFlag();
        power_data->fHeader.fStartTime = accum_container->GetAcquisitionStartSecond();
        power_data->fHeader.fSampleRate = accum_container->GetSampleRate();
        power_data->fHeader.fLeadingSampleIndex = accum_container->GetLeadingSampleIndex();
        power_data->fHeader.fSampleLength = accum_container->GetSampleLength();
        power_data->fHeader.fAccumulationLength = accum_container->GetAccumulations()->size();
        power_data->fHeader.fSwitchingFrequency =  accum_container->GetNoiseDiodeSwitchingFrequency();
        power_data->fHeader.fBlankingPeriod = accum_container->GetNoiseDiodeBlankingPeriod();
        // strcpy(power_data->fHeader.fExperimentName, fExperimentName.c_str() );
        // strcpy(power_data->fHeader.fSourceName, fSourceName.c_str() );
        // strcpy(power_data->fHeader.fScanName, fScanName.c_str() );

        //now point the accumulation data to the right memory block
        power_data->fAccumulations = static_cast< struct HDataAccumulationStruct* >( &((*(fNoisePowerAccumulatorGetAccumulations()))[0] ) );

        int ret_val = WriteNoisePowerFile(noise_power_filename.c_str(), power_data);
        if(ret_val != HSUCCESS){std::cout<<"file error!"<<std::endl;}

        InitializeNoisePowerFileStruct(power_data);
        DestroyNoisePowerFileStruct(power_data);
    }
}


}
