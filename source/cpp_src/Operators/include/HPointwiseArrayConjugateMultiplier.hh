#ifndef HPointwiseArrayConjugateMultiplier_H__
#define HPointwiseArrayConjugateMultiplier_H__

#include "HBinaryArrayOperator.hh"
#include "HArrayMath.hh"

#include <complex>

namespace hose{

/**
*
*@file HPointwiseArrayConjugateMultiplier.hh
*@class HPointwiseArrayConjugateMultiplier
*@brief
*@details
*
*<b>Revision History:<b>
*Date Name Brief Description
*Fri Sep 28 15:39:37 EDT 2012 J. Barrett (barrettj@mit.edu) First Version
*
*/

template<unsigned int NDIM>
class HPointwiseArrayConjugateMultiplier: public HBinaryArrayOperator< std::complex<double>, NDIM>
{
    public:

        HPointwiseArrayConjugateMultiplier()
        {
            fInitialized = false;
            for(unsigned int i=0; i<NDIM; i++){fDim[i] = 0;};
        };

        virtual ~HPointwiseArrayConjugateMultiplier()
        {
        };

        virtual void Initialize()
        {
            if(this->fFirstInput != NULL)
            {
                for(unsigned int i=0; i<NDIM; i++)
                {
                    if( fDim[i] != this->fFirstInput->GetArrayDimension(i)){fInitialized = false;};
                };
            }

            fInitialized = true;
        };

        virtual void ExecuteOperation()
        {
            if(IsInputOutputValid())
            {
                std::complex<double>* in1ptr = this->fFirstInput->GetData();
                std::complex<double>* in2ptr = this->fSecondInput->GetData();
                std::complex<double>* outptr = this->fOutput->GetData();

                unsigned int n_elem = this->fFirstInput->GetArraySize();

                this->fSecondInput->GetArrayDimensions(fDim);

                for(unsigned int i=0; i < n_elem; i++)
                {
                    outptr[i] = std::conj(in1ptr[i])*( in2ptr[i] );
                }
            }
        }

    private:

        unsigned int fIndex[NDIM];
        unsigned int fDim[NDIM];

        bool fInitialized;


        virtual bool IsInputOutputValid() const
        {
            if(this->fFirstInput && this->fSecondInput && this->fOutput )
            {
                //check they have the same size/num elements
                if( this->HaveSameNumberOfElements(this->fFirstInput, this->fOutput) &&
                    this->HaveSameNumberOfElements(this->fSecondInput, this->fOutput)  )
                {
                    //check they have the same dimensions/shape
                    if(this->HaveSameDimensions(this->fFirstInput, this->fOutput) &&
                       this->HaveSameDimensions(this->fSecondInput, this->fOutput)  )
                    {
                        return true;
                    }
                }
            }
            return false;
        }


};


}

#endif /* __HPointwiseArrayConjugateMultiplier_H__ */
