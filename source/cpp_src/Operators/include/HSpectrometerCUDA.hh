#ifndef HSpectrometerCUDA_HH__
#define HSpectrometerCUDA_HH__

#include "HConsumerProducer.hh"
#include "spectrometer.h"

namespace hose
{

/*
*File: HSpectrometerCUDA.hh
*Class: HSpectrometerCUDA
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: signed short int version
*/

class HSpectrometerCUDA: public HConsumerProducer< SAMPLE_TYPE, spectrometer_data, HConsumerBufferHandler_Immediate< SAMPLE_TYPE >, HProducerBufferHandler_Steal< spectrometer_data > >
{

    public:
        HSpectrometerCUDA(size_t spectrum_length, size_t n_averages);  //spec size and averages are fixed at constuction time
        virtual ~HSpectrometerCUDA();

    protected:

        virtual void ExecuteThreadTask() override;
        virtual bool WorkPresent() override;

        size_t fSpectrumLength;
        size_t fNAverages;

};


}

#endif /* end of include guard: HSpectrometerCUDA */
