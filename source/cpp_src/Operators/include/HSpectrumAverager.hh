#ifndef HSpectrumAverager_HH__
#define HSpectrumAverager_HH__

#include "HConsumerProducer.hh"
#include "spectrometer.h"

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
        HSpectrumAverager(size_t spectrum_length, size_t n_buffers);  //spec size and averages are fixed at constuction time
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

};


}

#endif /* end of include guard: HSpectrumAverager */
