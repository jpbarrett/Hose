#ifndef HSpectrumAverager_HH__
#define HSpectrumAverager_HH__

#include "HConsumerProducer.hh"

#include "spectrometer.h"

extern "C"
{
    #include "HDataAccumulationStruct.h"
}

#include "HDataAccumulationContainer.hh"


#ifdef HOSE_USE_ZEROMQ
#include <zmq.hpp>
#include <string>
#include <sstream>
#endif


#define ENABLE_SPECTRUM_UDP
#define NBINS 256

namespace hose
{

/*
*File: HSpectrumAverager.hh
*Class: HSpectrumAverager
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: This operator must be single threaded!!!
*/

class HSpectrumAverager: public HConsumerProducer< spectrometer_data, float, HConsumerBufferHandler_WaitWithTimeout< spectrometer_data >, HProducerBufferHandler_Immediate< float > >
{

    public:

        //spec size and averages are fixed at constuction time
        HSpectrumAverager(size_t spectrum_length, size_t n_buffers);

        //for better or worse, the spectrum averager is the best place to insert the 
        //code to handle the noise-power UDP unicast messaging. So we add some options to
        //configure the the IP/port here (fixed at construction time)
        HSpectrumAverager(size_t spectrum_length, size_t n_buffers,
                          std::string noise_port, std::string noise_ip,
                          std::string spec_port, std::string spec_ip); 

        virtual ~HSpectrumAverager();

        void EnableNoisePowerUDPMessages(){fEnableNoiseUDP = true;};
        void DisableNoisePowerUDPMessages(){fEnableNoiseUDP = false;};

        void EnableSpectrumUDPMessages(){fEnableSpectrumUDP = true;};
        void DisableSpectrumUDPMessages(){fEnableSpectrumUDP = false;}

        //Sum a set of spectral bins to export this 'noise power'
        void SetBufferSkip(int skip_interval){fSkipInterval = skip_interval;};
        void SetSpectralPowerLowerBound(size_t lower_bound){fSpecLowerBound = lower_bound;};
        void SetSpectralPowerUpperBound(size_t upper_bound){fSpecUpperBound = upper_bound;};

    protected:

        virtual void ExecuteThreadTask() override;
        virtual bool WorkPresent() override;

        //break work into manageable chunks
        bool CheckMetaData(char sideband_flag, char pol_flag, uint64_t start_second, uint64_t sample_rate, uint64_t leading_sample_index) const;
        void Accumulate(float* array);
        bool WriteAccumulatedSpectrumAverage();
        void Reset();

        size_t fPowerSpectrumLength;
        size_t fNBuffersToAccumulate; //number of buffers to be averaged (independent of any pre-averaging done)

        //workspace for the accumulated data:
        size_t fNBuffersAccumulated;
        char fSidebandFlag;
        char fPolarizationFlag;
        uint64_t fAcquisitionStartSecond;
        uint64_t fLeadingSampleIndex;
        uint64_t fNTotalSpectrum;
        uint64_t fNTotalSamplesAccumulated;
        uint64_t fSampleRate;
        std::vector<float> fAccumulatedSpectrum;
        HLinearBuffer< float >* fAccumulationBuffer;

        //to collect the noise power info
        HDataAccumulationContainer fNoisePowerAccumulator;

    private:

        bool fEnableNoiseUDP;
        bool fEnableSpectrumUDP;

        int fSkipInterval;
        std::string fNoisePort;
        std::string fNoiseIPAddress;
        std::string fSpectrumPort;
        std::string fSpectrumIPAddress;

        //accumulation of spectral bins 
        double fSpectralPowerSum;
        size_t fSpecLowerBound;
        size_t fSpecUpperBound;


        #ifdef HOSE_USE_ZEROMQ
            zmq::context_t* fNoiseContext;
            zmq::socket_t* fNoisePublisher;
            void SendNoisePowerUDPPacket(const uint64_t& start_sec, const uint64_t& leading_sample_index, const uint64_t& sample_rate, const struct HDataAccumulationStruct& stat);
        #endif

        #ifdef ENABLE_SPECTRUM_UDP
            size_t fBinFactor;
            zmq::context_t* fSpectrumContext;
            zmq::socket_t* fSpectrumPublisher;
            float fRebinnedSpectrum[NBINS];
        #endif 


};


}

#endif /* end of include guard: HSpectrumAverager */
