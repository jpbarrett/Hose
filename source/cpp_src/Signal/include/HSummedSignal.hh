#ifndef HSummedSignal_HH__
#define HSummedSignal_HH__

#include "HSimulatedAnalogSignalSampleGenerator.hh"

#include <vector>
#include <utility>

namespace hose
{

class HSummedSignal: public HSimulatedAnalogSignalSampleGenerator
{
    public:
        HSummedSignal(){};
        ~HSummedSignal(){};

        //underlying signal type that is to be masked by the switch, must be initialized beforehand
        //samples returned will be the sum of all signal generators added, scaled by the scale factor
        void AddSignalGenerator(HSimulatedAnalogSignalSampleGenerator* sig_gen, double scale_factor);

    protected:

        virtual bool GenerateSample(const double& sample_time, double& sample) const override;

        //list of signal generator/ scale factor pairs
        std::vector< std::pair< HSimulatedAnalogSignalSampleGenerator*, double> > fSignals;

};


}

#endif /* end of include guard: HSummedSignal_HH__ */
