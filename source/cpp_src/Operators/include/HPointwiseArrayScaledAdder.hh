#ifndef HPointwiseArrayScaledAdder_H__
#define HPointwiseArrayScaledAdder_H__

#include "HBinaryArrayOperator.hh"

namespace hose{

/**
*
*@file HPointwiseArrayScaledAdder.hh
*@class HPointwiseArrayScaledAdder
*@brief
*@details
*
*<b>Revision History:<b>
*Date Name Brief Description
*Fri Sep 28 15:39:37 EDT 2012 J. Barrett (barrettj@mit.edu) First Version
*
*/

template<typename ArrayType, unsigned int NDIM>
class HPointwiseArrayScaledAdder: public HBinaryArrayOperator< ArrayType, NDIM>
{
    public:

        HPointwiseArrayScaledAdder(){;};
        virtual ~HPointwiseArrayScaledAdder(){;}

        virtual void Initialize(){;};

        //scale factor is always applied to the second input!
        void SetScaleFactor(const ArrayType& fac){fScalarFactor = fac;}

        virtual void ExecuteOperation()
        {
            if(IsInputOutputValid())
            {
                ArrayType* in1ptr = this->fFirstInput->GetData();
                ArrayType* in2ptr = this->fSecondInput->GetData();
                ArrayType* outptr = this->fOutput->GetData();

                unsigned int n_elem = this->fFirstInput->GetArraySize();
                for(unsigned int i=0; i < n_elem; i++)
                {
                    outptr[i] = (in1ptr[i]) + fScalarFactor*(in2ptr[i]); //whatever the array type is, it must define the +/* operators
                }
            }
        }

    protected:

        virtual bool IsInputOutputValid() const
        {
            if(this->fFirstInput != NULL && this->fSecondInput != NULL && this->fOutput != NULL )
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

       ArrayType fScalarFactor;


};


}

#endif /* __HPointwiseArrayScaledAdder_H__ */
