#include "HPowerLawNoiseSignal.hh"

#include <functional>

namespace hose
{



HPowerLawNoiseSignal::HPowerLawNoiseSignal():
    HSimulatedAnalogSignalSampleGenerator(),
    fWrapperIn(),
    fWrapperOut(),
    fSeed(0)
{
    fFFTCalculator = new FFT_TYPE();
};

HPowerLawNoiseSignal::~HPowerLawNoiseSignal()
{
    delete fFFTCalculator;
};

void 
HPowerLawNoiseSignal::SetRandomSeed(unsigned int seed){fSeed = seed;}

void 
HPowerLawNoiseSignal::SetPowerLawExponent(double alpha)
{
    fAlpha = alpha;
}

void 
HPowerLawNoiseSignal::Initialize()
{
    //determine number of random samples and generate them
    fSamplePeriod = 1.0/(this->fSamplingFrequency);
    fNSamples = fPeriod/fSamplePeriod;

    fDim[0] = fNSamples;

    //construct a Gaussian random number generator
    auto real_rand_gauss = std::bind(std::normal_distribution< double >(0.0,1.0), std::mt19937(fSeed));
    auto real_rand_uniform = std::bind(std::uniform_real_distribution< double >(0.0, 2.0*M_PI), std::mt19937(fSeed));

    //assign frequency samples and weight according to power spectrum,
    //we assume we want real time-series output, so X[-k] = X[N-k] = X*[k]
    //and we only need unique complex samples up to N/2
    fSamplesIn.clear();
    fSamplesIn.resize(fNSamples, std::complex<double>(0,0) );

    //space for x-formed output
    fSamplesOut.clear();
    fSamplesOut.resize(fNSamples, std::complex<double>(0,0) );

    double rs, is, omega, weight;
    double mag, phi;

    //always zero out DC component
    fSamplesIn[0] = std::complex< double >(0,0);

    for(unsigned int i=1; i<fNSamples/2+1; i++)
    {
        //frequency weight (we are bandwidth limited to fSamplingFrequency/2.0)
        omega = i*(fSamplingFrequency/fNSamples);
        weight = std::pow( 1.0/omega, fAlpha/2.0);

        mag = weight*std::fabs( real_rand_gauss() );
        phi = real_rand_uniform();

        rs = mag*std::cos(phi);
        is = mag*std::sin(phi);

        fSamplesIn[i] = std::complex< double >(rs,is);
        fSamplesIn[ HArrayMath::Modulus(-i, fNSamples) ] = std::complex< double >(rs,-is);
    }

    //number of samples is even, we need to ensure X[N/2] is real to enforce hermiticity
    if(fNSamples%2 == 0)
    {
        unsigned int i = fNSamples/2;
        omega = i*(fSamplingFrequency/fNSamples);
        weight = std::pow( 1.0/omega, fAlpha/2.0);
        rs = weight*std::fabs(real_rand_gauss());
        fSamplesIn[i] = std::complex< double > (rs,0);
    }


    //wrap the array (needed for FFT interface
    fWrapperIn.SetData(&(fSamplesIn[0]));
    fWrapperIn.SetArrayDimensions(fDim);

    fWrapperOut.SetData(&(fSamplesOut[0]));
    fWrapperOut.SetArrayDimensions(fDim);

    //obtain time series by backward FFT
    fFFTCalculator->SetBackward();
    fFFTCalculator->SetInput(&fWrapperIn);
    fFFTCalculator->SetOutput(&fWrapperOut);
    fFFTCalculator->Initialize();
    fFFTCalculator->ExecuteOperation();

    //make sure we normalize the FFT
    double norm = 1.0/std::sqrt( (double)fNSamples );

    for(size_t i=0; i<fNSamples; i++)
    {
        fWrapperOut[i] *= norm;
    }

}


double 
HPowerLawNoiseSignal::GenerateSample(const double& sample_time) const
{
    
    //TODO FIXME upgrade this so we can use linear/cubic interpolation (directly in time domain) for times which lie in between samples.
    //Interpolating in the time domain may introduce higher-frequency components which cause aliasing 
    //but since we don't expect a real world noise signal to be bandlimited this is not a problem at the moment 
    //If the system to be simulated has a band-limiting filter, then this needs to be dealt with separately.

    //just need to figure out where in the array we want to pull from
    unsigned int index = std::floor(sample_time/fSamplePeriod);

    //dead simple, sample and hold
    return std::real<double>( (fWrapperOut.GetData())[index] );

}



} //end of namespace
