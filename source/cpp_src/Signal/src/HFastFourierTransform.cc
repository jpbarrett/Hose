#include <cstring>
#include <cmath>
#include <iostream>
#include <cstddef>

#include "HFastFourierTransform.hh"

#include "HBitReversalPermutation.hh"
#include "HFastFourierTransformUtilities.hh"

namespace hose
{



HFastFourierTransform::HFastFourierTransform()
{
    fIsValid = true;
    fForward = true;
    fInitialized = false;
    fSizeIsPowerOfTwo = false;
    fSizeIsPowerOfThree = false;

    fN = 0;
    fM = 0;
    fPermutation = NULL;
    fTwiddle = NULL;
    fConjugateTwiddle = NULL;
    fScale = NULL;
    fCirculant = NULL;
    fWorkspace = NULL;
}

HFastFourierTransform::~HFastFourierTransform()
{
    DealocateWorkspace();
}

void
HFastFourierTransform::SetSize(unsigned int N)
{
    if(N != fN)
    {
        fN = N;
        fSizeIsPowerOfTwo = HBitReversalPermutation::IsPowerOfTwo(N);
        fSizeIsPowerOfThree = HBitReversalPermutation::IsPowerOfBase(N,3);
        fM = HFastFourierTransformUtilities::ComputeBluesteinArraySize(N);
        fInitialized = false;
    }
}

void
HFastFourierTransform::SetForward(){fForward = true;}

void
HFastFourierTransform::SetBackward(){fForward = false;}

void
HFastFourierTransform::Initialize()
{
    // std::cout<<"fN = "<<fN<<std::endl;
    // std::cout<<"input size = "<<fInput->GetArraySize()<<std::endl;
    // std::cout<<"output size = "<<fOutput->GetArraySize()<<std::endl;
    if(fInput->GetArraySize() != fN || fOutput->GetArraySize() != fN)
    {
        fIsValid = false;
    }

    if( !fInitialized )
    {
        //initialize
        DealocateWorkspace();
        AllocateWorkspace();

        //compute the permutation arrays and twiddle factors
        if(fSizeIsPowerOfTwo)
        {
            //use radix-2
            HBitReversalPermutation::ComputeBitReversedIndicesBaseTwo(fN, fPermutation);
            HFastFourierTransformUtilities::ComputeTwiddleFactors(fN, fTwiddle);
            HFastFourierTransformUtilities::ComputeConjugateTwiddleFactors(fN, fConjugateTwiddle);
        }

        if(fSizeIsPowerOfThree)
        {
            //use radix-3
            HBitReversalPermutation::ComputeBitReversedIndices(fN, 3, fPermutation);
            HFastFourierTransformUtilities::ComputeTwiddleFactors(fN, fTwiddle);
            HFastFourierTransformUtilities::ComputeConjugateTwiddleFactors(fN, fConjugateTwiddle);
        }

        if(!fSizeIsPowerOfThree && !fSizeIsPowerOfTwo)
        {
            //use Bluestein algorithm
            HBitReversalPermutation::ComputeBitReversedIndicesBaseTwo(fM, fPermutation);
            HFastFourierTransformUtilities::ComputeTwiddleFactors(fM, fTwiddle);
            HFastFourierTransformUtilities::ComputeConjugateTwiddleFactors(fM, fConjugateTwiddle);
            HFastFourierTransformUtilities::ComputeBluesteinScaleFactors(fN, fScale);
            HFastFourierTransformUtilities::ComputeBluesteinCirculantVector(fN, fM, fTwiddle, fScale, fCirculant);
        }

        fIsValid = true;
        fInitialized = true;

        //std::cout<<"initialize = "<<fInitialized<<" and valid = "<<fIsValid<<std::endl;
    }
}

///Make a call to execute the FFT plan and perform the transformation
void
HFastFourierTransform::ExecuteOperation()
{
    //std::cout<<"initialize = "<<fInitialized<<" and valid = "<<fIsValid<<std::endl;

    if(fIsValid)
    {
        //if input and output point to the same array, don't bother copying data over
        if(fInput != fOutput)
        {
            //the arrays are not identical so copy the input over to the output
            std::memcpy( (void*) fOutput->GetData(), (void*) fInput->GetData(), fN*sizeof(std::complex<double>) );
        }


        if(!fForward) //for IDFT we conjugate first
        {
            std::complex<double>* data = fOutput->GetData();
            for(unsigned int i=0; i<fN; i++)
            {
                data[i] = std::conj(data[i]);
            }
        }

        if(fSizeIsPowerOfTwo)
        {
            //use radix-2
            HBitReversalPermutation::PermuteArray< std::complex<double> >(fN, fPermutation, fOutput->GetData());
            HFastFourierTransformUtilities::FFTRadixTwo_DIT(fN, fOutput->GetData(), fTwiddle);
        }

        if(fSizeIsPowerOfThree)
        {
            //use radix-3
            HBitReversalPermutation::PermuteArray< std::complex<double> >(fN, fPermutation, fOutput->GetData());
            HFastFourierTransformUtilities::FFTRadixThree(fN, fOutput->GetData(), fTwiddle);
        }

        if(!fSizeIsPowerOfThree && !fSizeIsPowerOfTwo)
        {
            //use bluestein algorithm for arbitrary N
            HFastFourierTransformUtilities::FFTBluestein(fN, fM, fOutput->GetData(), fTwiddle, fConjugateTwiddle, fScale, fCirculant, fWorkspace);
        }

        if(!fForward) //for IDFT we conjugate again
        {
            std::complex<double>* data = fOutput->GetData();
            for(unsigned int i=0; i<fN; i++)
            {
                data[i] = std::conj(data[i]);
            }
        }
    }
    else
    {
        //warning
        std::cout<<"HFastFourierTransform::ExecuteOperation: Warning, transform not valid. Aborting."<<std::endl;
    }
}

void
HFastFourierTransform::AllocateWorkspace()
{
    if(!fSizeIsPowerOfTwo && !fSizeIsPowerOfThree)
    {
        //can't perform an in-place transform, need workspace
        fPermutation = new unsigned int[fM];
        fTwiddle = new std::complex<double>[fM];
        fConjugateTwiddle = new std::complex<double>[fM];
        fScale = new std::complex<double>[fN];
        fCirculant = new std::complex<double>[fM];
        fWorkspace = new std::complex<double>[fM];
    }
    else
    {
        //can do an in-place transform,
        //only need space for the permutation array, and twiddle factors
        fPermutation = new unsigned int[fN];
        fTwiddle = new std::complex<double>[fN];
        fConjugateTwiddle = new std::complex<double>[fN];
    }
}

void
HFastFourierTransform::DealocateWorkspace()
{
    delete[] fPermutation; fPermutation = NULL;
    delete[] fTwiddle; fTwiddle = NULL;
    delete[] fConjugateTwiddle; fConjugateTwiddle = NULL;
    delete[] fScale; fScale = NULL;
    delete[] fCirculant; fCirculant = NULL;
    delete[] fWorkspace; fWorkspace = NULL;
}

}
