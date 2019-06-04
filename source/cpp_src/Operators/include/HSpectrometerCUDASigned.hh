#ifndef HSpectrometerCUDASigned_HH__
#define HSpectrometerCUDASigned_HH__

#include "HConsumerProducer.hh"
#include "spectrometer.h"

namespace hose
{

/*
*File: HSpectrometerCUDASigned.hh
*Class: HSpectrometerCUDASigned
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: signed short int version
*/

class HSpectrometerCUDASigned: public HConsumerProducer< int16_t, spectrometer_data_s, HConsumerBufferHandler_Immediate< int16_t >, HProducerBufferHandler_Steal< spectrometer_data_s > >
{

    public:
        HSpectrometerCUDASigned(size_t spectrum_length, size_t n_averages);  //spec size and averages are fixed at constuction time
        virtual ~HSpectrometerCUDASigned();

    protected:

        virtual void ExecuteThreadTask() override;
        virtual bool WorkPresent() override;

        size_t fSpectrumLength;
        size_t fNAverages;

};


}

#endif /* end of include guard: HSpectrometerCUDASigned */
