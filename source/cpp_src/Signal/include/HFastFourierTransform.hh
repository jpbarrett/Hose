#ifndef HFastFourierTransform_HH__
#define HFastFourierTransform_HH__

#include <complex>

#include "HArrayWrapper.hh"
#include "HUnaryArrayOperator.hh"

#include "HBitReversalPermutation.hh"
#include "HFastFourierTransformUtilities.hh"

namespace hose
{

/*
*
*@file HFastFourierTransform.hh
*@class HFastFourierTransform
*@brief This is a class for a one dimensional FFT
*@details
*
*/


class HFastFourierTransform: public HUnaryArrayOperator< std::complex<double>, 1 >
{
    public:

        HFastFourierTransform();
        virtual ~HFastFourierTransform();

        virtual void SetSize(unsigned int N);

        virtual void SetForward();
        virtual void SetBackward();

        virtual void Initialize();

        virtual void ExecuteOperation();

    private:

        virtual void AllocateWorkspace();
        virtual void DealocateWorkspace();

        bool fIsValid;
        bool fForward;
        bool fInitialized;
        bool fSizeIsPowerOfTwo;
        bool fSizeIsPowerOfThree;

        //auxilliary workspace needed for basic 1D transform
        unsigned int fN;
        unsigned int fM;
        unsigned int* fPermutation;
        std::complex<double>* fTwiddle;
        std::complex<double>* fConjugateTwiddle;
        std::complex<double>* fScale;
        std::complex<double>* fCirculant;
        std::complex<double>* fWorkspace;

};

}


#endif /* HFastFourierTransform_H__ */
