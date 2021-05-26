#ifndef HSpectrumAveragerSigned_HH__
#define HSpectrumAveragerSigned_HH__

#include "HConsumerProducer.hh"

#include "spectrometer.h"

extern "C"
{
    #include "HDataAccumulationStruct.h"
}

#include "HDataAccumulationContainer.hh"


namespace hose
{

/*
*File: HSpectrumAveragerSigned.hh
*Class: HSpectrumAveragerSigned
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: This operator must be single threaded!!!
*/

class HSpectrumAveragerSigned: public HConsumerProducer< spectrometer_data_s, float, HConsumerBufferHandler_WaitWithTimeout< spectrometer_data_s >, HProducerBufferHandler_Immediate< float > >
{

    public:
        HSpectrumAveragerSigned(size_t spectrum_length, size_t n_buffers);  //spec size and averages are fixed at constuction time
        virtual ~HSpectrumAveragerSigned();

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


};


}

#endif /* end of include guard: HSpectrumAveragerSigned */
