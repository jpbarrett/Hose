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
        HSpectrumAverager(size_t spectrum_length, size_t n_buffers, std::string port, std::string ip);  

        virtual ~HSpectrumAverager();

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
        
        bool fEnableUDP;
        std::string fPort;
        std::string fIPAddress;

        #ifdef HOSE_USE_ZEROMQ
            zmq::context_t* fContext;
            zmq::socket_t* fPublisher;
            void SendNoisePowerUDPPacket(const uint64_t& start_sec, const uint64_t& leading_sample_index, const uint64_t& sample_rate, const struct HDataAccumulationStruct& stat);
        #endif
};


}

#endif /* end of include guard: HSpectrumAverager */
