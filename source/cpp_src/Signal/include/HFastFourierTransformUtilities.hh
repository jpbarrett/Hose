#ifndef HFastFourierTransformUtilities_HH__
#define HFastFourierTransformUtilities_HH__

#include "HBitReversalPermutation.hh"

#include <complex>

namespace hose
{

/*
*
*@file HFastFourierTransformUtilities.hh
*@class HFastFourierTransformUtilities
*@brief
*@details
*
*/


class HFastFourierTransformUtilities
{
    public:
        HFastFourierTransformUtilities(){};
        virtual ~HFastFourierTransformUtilities(){};


        ////////////////////////////////////////////////////////////////////////
        //compute all the twiddle factors e^{i*2*pi/N} for 0 to N-1 through recursion
        static void ComputeTwiddleFactors(unsigned int N, std::complex<double>* twiddle);
        static void ComputeConjugateTwiddleFactors(unsigned int N, std::complex<double>* conj_twiddle);
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        //RADIX-2
        static void FFTRadixTwo_DIT(unsigned int N, double* data, double* twiddle);
        static void ButterflyRadixTwo_CooleyTukey(double* H0, double* H1, double* W);

        static void FFTRadixTwo_DIF(unsigned int N, double* data, double* twiddle);
        static void ButterflyRadixTwo_GentlemanSande(double* H0, double* H1, double* W);

        //wrappers for complex array
        static void FFTRadixTwo_DIT(unsigned int N, std::complex<double>* data, std::complex<double>* twiddle);
        static void FFTRadixTwo_DIF(unsigned int N, std::complex<double>* data, std::complex<double>* twiddle);

        ////////////////////////////////////////////////////////////////////////


        ////////////////////////////////////////////////////////////////////////
        //RADIX-3
        static void ButterflyRadixThree(std::complex<double>& H0,
                                        std::complex<double>& H1,
                                        std::complex<double>& H2,
                                        std::complex<double>& W0,
                                        std::complex<double>& W1);
        static void FFTRadixThree(unsigned int N, std::complex<double>* data, std::complex<double>* twiddle);

        ////////////////////////////////////////////////////////////////////////
        //Bluestein/Chirp-Z Algorithm for arbitrary N
        //"Inside the FFT Black Box", E. Chu and A. George, Ch. 13, CRC Press, 2000

        static unsigned int ComputeBluesteinArraySize(unsigned int N); //returns smallest M = 2^p >= (2N - 2);

        //scale factor array must be length N
        static void ComputeBluesteinScaleFactors(unsigned int N, std::complex<double>* scale);

        //twiddle and circulant array must be length M = 2^p >= (2N - 2), where N is the data length
        static void ComputeBluesteinCirculantVector(unsigned int N,
                                                    unsigned int M,
                                                    std::complex<double>* twiddle,
                                                    std::complex<double>* scale,
                                                    std::complex<double>* circulant);

        //N is length of the data
        static void FFTBluestein(unsigned int N,
                                 unsigned int M,
                                 std::complex<double>* data,
                                 std::complex<double>* twiddle,
                                 std::complex<double>* conj_twiddle,
                                 std::complex<double>* scale,
                                 std::complex<double>* circulant,
                                 std::complex<double>* workspace);

        ////////////////////////////////////////////////////////////////////////

    private:

};


}


#endif /* HFastFourierTransformUtilities_H__ */
