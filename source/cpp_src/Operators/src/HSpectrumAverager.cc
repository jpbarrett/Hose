#include "HSpectrumAverager.hh"

#include <cstdlib>


namespace hose
{

HSpectrumAverager::HSpectrumAverager(size_t spectrum_length, size_t n_buffers):
    fPowerSpectrumLength(spectrum_length),
    fNBuffersToAccumulate(n_buffers),
    fSpecLowerBound(0),
    fSpecUpperBound(0),
    fEnableNoiseUDP(false),
    fEnableSpectrumUDP(false),
    fSkipInterval(8)
{
    fAccumulatedSpectrum.resize(spectrum_length);
    fAccumulationBuffer = new HLinearBuffer<float>( &(fAccumulatedSpectrum[0]), spectrum_length);
    fNBuffersAccumulated = 0;
};


HSpectrumAverager::HSpectrumAverager(size_t spectrum_length, size_t n_buffers,
                                     std::string noise_port, std::string noise_ip,
                                     std::string spec_port, std::string spec_ip):
    fPowerSpectrumLength(spectrum_length),
    fNBuffersToAccumulate(n_buffers),
    fNoisePort(noise_port),
    fNoiseIPAddress(noise_ip),
    fSpectrumPort(spec_port),
    fSpectrumIPAddress(spec_ip),
    fSpecLowerBound(0),
    fSpecUpperBound(0),
    fEnableNoiseUDP(false),
    fEnableSpectrumUDP(false),
    fSkipInterval(8)
{
        fAccumulatedSpectrum.resize(spectrum_length);
        fAccumulationBuffer = new HLinearBuffer<float>( &(fAccumulatedSpectrum[0]), spectrum_length);
        fNBuffersAccumulated = 0;

        #ifdef HOSE_USE_ZEROMQ
            //fEnableNoiseUDP = true;
            fNoiseContext = new zmq::context_t(1);
            fNoisePublisher = new zmq::socket_t(*fNoiseContext, ZMQ_RADIO);
            std::string noise_udp_connection = "udp://" + fNoiseIPAddress + ":" + fNoisePort;
            //std::cout<<"udp connection = "<<udp_connection<<std::endl;
            fNoisePublisher->connect(noise_udp_connection.c_str());
        #endif

        #ifdef HOSE_USE_ZEROMQ
            //fEnableNoiseUDP = true;
            fSpectrumContext = new zmq::context_t(1);
            fSpectrumPublisher = new zmq::socket_t(*fSpectrumContext, ZMQ_RADIO);
            std::string spec_udp_connection = "udp://" + fSpectrumIPAddress + ":" + fSpectrumPort;
            //std::cout<<"udp connection = "<<udp_connection<<std::endl;
            fSpectrumPublisher->connect(spec_udp_connection.c_str());
        #endif

        #ifdef ENABLE_SPECTRUM_UDP
            fBinFactor = fPowerSpectrumLength/NBINS;
        #endif

};


HSpectrumAverager::~HSpectrumAverager()
{
    #ifdef HOSE_USE_ZEROMQ
        delete fNoisePublisher;
        delete fNoiseContext;
        delete fSpectrumPublisher;
        delete fSpectrumContext;
    #endif
};


bool
HSpectrumAverager::WorkPresent()
{
    if( fSourceBufferPool->GetConsumerPoolSize( this->GetConsumerID() ) == 0)
    {
        return false;
    }
    return true;
}


void
HSpectrumAverager::ExecuteThreadTask()
{
    HLinearBuffer< spectrometer_data >* source = nullptr;
    spectrometer_data* sdata = nullptr;

    if( fSourceBufferPool->GetConsumerPoolSize( this->GetConsumerID() ) != 0 ) //only do work if there is stuff to process
    {
        //grab a source buffer and add its data to the accumulation buffer
        HConsumerBufferPolicyCode source_code = this->fSourceBufferHandler.ReserveBuffer(this->fSourceBufferPool, source, this->GetConsumerID());
        if( (source_code & HConsumerBufferPolicyCode::success) && source !=nullptr)
        {
            std::lock_guard<std::mutex> source_lock(source->fMutex);
            sdata = &( (source->GetData())[0] ); //should have buffer size of 1

                //first collect the meta-data information from this buffer
            char sideband_flag = source->GetMetaData()->GetSidebandFlag() ;
            char pol_flag = source->GetMetaData()->GetPolarizationFlag();
            uint64_t start_second = source->GetMetaData()->GetAcquisitionStartSecond();
            uint64_t sample_rate = source->GetMetaData()->GetSampleRate();
            uint64_t leading_sample_index = source->GetMetaData()->GetLeadingSampleIndex();
            uint64_t n_spectra = sdata->n_spectra;
            uint64_t n_spectrum_samples_length = sdata->spectrum_length; //number of samples used in FFT to create an individual spectrum
            uint64_t power_spectrum_length = ((sdata->spectrum_length)/2+1); //length of the power spectrum
            uint64_t n_total_samples = n_spectra*n_spectrum_samples_length; //total number of samples used to compute the averaged spectrum we get from this buffer

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

                #ifdef HOSE_USE_ZEROMQ
                    if(fEnableNoiseUDP && fNBuffersAccumulated%fSkipInterval == 0){SendNoisePowerUDPPacket(fAcquisitionStartSecond, fLeadingSampleIndex, fSampleRate, stat);}
                #endif 


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

                #ifdef HOSE_USE_ZEROMQ
                    if(fEnableNoiseUDP && fNBuffersAccumulated%fSkipInterval == 0){SendNoisePowerUDPPacket(fAcquisitionStartSecond, fLeadingSampleIndex, fSampleRate, stat);}
                #endif 

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
                    Reset();
                }
            }
        }

        // if(source != nullptr)
        // {
        //     //std::cout<<"averager releasing spec source buffer to consumer #"<<this->GetNextConsumerID()<<" in pool: "<<this->fSourceBufferPool<<std::endl;
        //     //release source buffer
        //     this->fSourceBufferHandler.ReleaseBufferToConsumer(this->fSourceBufferPool, source, this->GetNextConsumerID() );
        // }

    }
}


bool
HSpectrumAverager::CheckMetaData(char sideband_flag, char pol_flag, uint64_t start_second, uint64_t sample_rate, uint64_t leading_sample_index) const
{
    if(fSidebandFlag != sideband_flag){return false;}
    if(fPolarizationFlag != pol_flag){return false;}
    if(fAcquisitionStartSecond != start_second){return false;}
    if(fSampleRate != sample_rate){return false;}
    if(leading_sample_index < fLeadingSampleIndex){return false;}
    return true;
}

void
HSpectrumAverager::Accumulate(float* array)
{
    float* accum = fAccumulationBuffer->GetData();
    for(size_t i=0; i<fPowerSpectrumLength; i++)
    {
        accum[i] += array[i];
    }
    
    //hopefully this is not too inefficient --- if it is, we may have to move this to the GPU
    //accculate total 'power' in specified spectral bins
    fSpectralPowerSum = 0.0;
    for(size_t j=fSpecLowerBound; j<fSpecUpperBound; j++)
    {
        fSpectralPowerSum += array[j];
    }
}

bool
HSpectrumAverager::WriteAccumulatedSpectrumAverage()
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
        //noise diode is not used at 37m
        sink->GetMetaData()->SetNoiseDiodeSwitchingFrequency(0);
        sink->GetMetaData()->SetNoiseDiodeBlankingPeriod(0);

        //compute average and finish writing meta data
        float* accum = fAccumulationBuffer->GetData();
        float* ave = sink->GetData();
        for(size_t i=0; i<fPowerSpectrumLength; i++)
        {
            ave[i] = accum[i]/(float)fNBuffersAccumulated;
            #ifdef ENABLE_SPECTRUM_UDP
            fRebinnedSpectrum[i/fBinFactor] += ave[i];
            #endif
        }

        //stuff the noise power data into the meta data container
        sink->GetMetaData()->ClearAccumulation();
        sink->GetMetaData()->ExtendAccumulation(fNoisePowerAccumulator.GetAccumulations());

        //release to consumer
        this->fSinkBufferHandler.ReleaseBufferToConsumer(this->fSinkBufferPool, sink);

        #ifdef ENABLE_SPECTRUM_UDP
        if(fEnableSpectrumUDP)
        {
            if(fSpectrumPublisher->connected())
            {
                zmq::message_t update{ &(fRebinnedSpectrum[0]), sizeof(float)*NBINS};
                update.set_group("spectrum");
                fSpectrumPublisher->send(update);
            }
        }
        #endif 

        return true;

    }
    if(sink != nullptr)
    {
        this->fSinkBufferHandler.ReleaseBufferToConsumer(this->fSinkBufferPool, sink);
    }
    return false; //failed
}


void HSpectrumAverager::Reset()
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

    #ifdef ENABLE_SPECTRUM_UDP
    for(size_t i=0; i<NBINS; i++)
    {
        fRebinnedSpectrum[i] = 0.0;
    }
    #endif

    //for the noise power, clear out all the accumulation structs
    fNoisePowerAccumulator.ClearAccumulation();

}

#ifdef HOSE_USE_ZEROMQ
void HSpectrumAverager::SendNoisePowerUDPPacket(const uint64_t& start_sec, const uint64_t& leading_sample_index, const uint64_t& sample_rate, const struct HDataAccumulationStruct& stat)
{
    //noise power data
    std::stringstream ss;
    ss << start_sec << "; ";
    // ss << leading_sample_index << "; ";
    ss << sample_rate << "; ";
    ss << stat.start_index << "; ";
    ss << stat.stop_index << "; ";
    ss << stat.sum_x << "; ";
    ss << stat.sum_x2 << "; ";
    ss << stat.count << "; ";
    ss << stat.state_flag << "; ";

    //also dump the spectral bin power
    ss << fSpecLowerBound << "; ";
    ss << fSpecUpperBound << "; ";
    ss << fSpectralPowerSum << "; ";

    std::string msg = ss.str();
    if(fNoisePublisher->connected())
    {
        zmq::message_t update{msg.data(), msg.size()};
        update.set_group("noise_power");
        fNoisePublisher->send(update);
    }

}
#endif HOSE_USE_ZEROMQ

}
