#ifndef HSpectrometerCUDA_HH__
#define HSpectrometerCUDA_HH__

#include <iostream>
#include <ostream>
#include <ios>
#include <fstream>
#include <sstream>
#include <thread>
#include <map>
#include <utility>
#include <stdint.h>

#include "HLinearBuffer.hh"
#include "HBufferPool.hh"
#include "HConsumerProducer.hh"

#include "spectrometer.h"

#include "HPeriodicPowerCalculator.hh"

namespace hose
{

/*
*File: HSpectrometerCUDA.hh
*Class: HSpectrometerCUDA
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: unsigned short int version
*/

//template< typename XSourceBufferItemType, typename XSinkBufferItemType, typename XConsumerSourceBufferHandlerPolicyType, typename XProducerSinkBufferHandlerPolicyType > 

class HSpectrometerCUDA: public HConsumerProducer< uint16_t, spectrometer_data, HConsumerBufferHandler_WaitWithTimeout< uint16_t >, HProducerBufferHandler_Steal< spectrometer_data > >
{

    public:
        HSpectrometerCUDA(size_t spectrum_length, size_t n_averages);  //spec size and averages are fixed at constuction time
        virtual ~HSpectrometerCUDA();

        void SetSamplingFrequency(double samp_freq){fSamplingFrequency = samp_freq;};
        void SetSwitchingFrequency(double switch_freq){fSwitchingFrequency = switch_freq;};
        void SetBlankingPeriod(double blank_period){fBlankingPeriod = blank_period;}

    private:

        virtual void ExecuteThreadTask() override;
        virtual bool WorkPresent() override;
        virtual void Idle() override {;};

        virtual void ConfigureSinkBufferHandler(HConsumerBufferHandler_WaitWithTimeout< uint16_t >& handler)
        {
            handler->SetSleepDurationNanoSeconds(0);
        };

        size_t fSpectrumLength;
        size_t fNAverages;

        //data
        double fSamplingFrequency;
        double fSwitchingFrequency; //frequency at which the noise diode is switched
        double fBlankingPeriod; // ignore samples within +/- half the blanking period about switching time 



};


}

#endif /* end of include guard: HSpectrometerCUDA */
