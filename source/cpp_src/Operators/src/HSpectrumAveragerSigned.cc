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

                //check if we have reached the desired number of buffers,
                if(fNBuffersAccumulated == fNBuffersToAccumulate)
                {
                    WriteAccumulatedSpectrumAverage();
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
                this->fSourceBufferHandler.ReleaseBufferToConsumer(this->fSourceBufferPool, source, this->GetNextConsumerID() );
                source = nullptr;

                //check if we have reached the desired number of buffers, (unlikely here, can only happen if fNBuffersAccumulated is 1)
                if(fNBuffersAccumulated == fNBuffersToAccumulate)
                {
                    WriteAccumulatedSpectrumAverage();
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
}



}
