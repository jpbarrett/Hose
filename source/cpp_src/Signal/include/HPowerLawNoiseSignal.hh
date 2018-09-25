#ifndef HPowerLawNoiseSignal_HH__
#define HPowerLawNoiseSignal_HH__

#include <vector>
#include <complex>
#include <random>

#include "HArrayWrapper.hh"
#include "HMultidimensionalFastFourierTransform.hh"

#include "HSimulatedAnalogSignalSampleGenerator.hh"

#ifdef HOSE_USE_FFTW
    #include "HMultidimensionalFastFourierTransformFFTW.hh"
    #define FFT_TYPE HMultidimensionalFastFourierTransformFFTW<1>
#else
    #define FFT_TYPE HMultidimensionalFastFourierTransform<1>
#endif


namespace hose
{

/*
*File: HPowerLawNoiseSignal.hh
*Class: HPowerLawNoiseSignal
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: Generates a time-series of colored noise samples (1/f^{\alpha} noise), with 0 <= alpha <= 2
*This method is somewhat memory intensive, with memory use which grows linearly with
*the time period and the sampling frequency (bandwidth). May need to make this more efficient, signal
*is not guaranteed to be bandlimited.
*Algorithm according to the paper:

@article{timmer1995generating,
  title={On generating power law noise.},
  author={Timmer, J and K{\"o}nig, M},
  journal={Astronomy and Astrophysics},
  volume={300},
  pages={707},
  year={1995}
}

*/

class HPowerLawNoiseSignal: public HSimulatedAnalogSignalSampleGenerator
{
    public:
        HPowerLawNoiseSignal();
        virtual ~HPowerLawNoiseSignal();

        void SetRandomSeed(unsigned int seed);
        void SetPowerLawExponent(double alpha);

        virtual void Initialize();

    protected:

        virtual double GenerateSample(const double& sample_time) const;

        //data
        unsigned int fNSamples;
        size_t fDim[1];
        std::mt19937::result_type fSeed;
        double fAlpha;
        double fSamplePeriod;
        std::vector< std::complex< double > > fSamplesIn;
        std::vector< std::complex< double > > fSamplesOut;
        HArrayWrapper< std::complex< double >, 1 > fWrapperIn;
        HArrayWrapper< std::complex< double >, 1 > fWrapperOut;

        FFT_TYPE* fFFTCalculator;

};

}

#endif /* end of include guard: HPowerLawNoiseSignal */
