#ifndef HPointwiseArrayMultiplier_H__
#define HPointwiseArrayMultiplier_H__

#include "HBinaryArrayOperator.hh"

namespace hose{

/**
*
*@file HPointwiseArrayMultiplier.hh
*@class HPointwiseArrayMultiplier
*@brief
*@details
*
*<b>Revision History:<b>
*Date Name Brief Description
*Fri Sep 28 15:39:37 EDT 2012 J. Barrett (barrettj@mit.edu) First Version
*
*/

template<typename ArrayType, unsigned int NDIM>
class HPointwiseArrayMultiplier: public HBinaryArrayOperator< ArrayType, NDIM>
{
    public:
        HPointwiseArrayMultiplier(){};
        virtual ~HPointwiseArrayMultiplier(){};

        virtual void Initialize(){;};

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
                    outptr[i] = (in1ptr[i])*(in2ptr[i]);
                }
            }
        }

    private:

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

#endif /* __HPointwiseArrayMultiplier_H__ */
