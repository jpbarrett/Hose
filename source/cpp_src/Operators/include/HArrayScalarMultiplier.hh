#ifndef HArrayScalarMultiplier_H__
#define HArrayScalarMultiplier_H__

#include "HUnaryArrayOperator.hh"

namespace hose{

/**
*
*@file HArrayScalarMultiplier.hh
*@class HArrayScalarMultiplier
*@brief
*@details
*
*<b>Revision History:<b>
*Date Name Brief Description
*Wed Oct  3 10:42:16 EDT 2012 J. Barrett (barrettj@mit.edu) First Version
*
*/

template<typename ArrayType, unsigned int NDIM>
class HArrayScalarMultiplier: public HUnaryArrayOperator< ArrayType, NDIM>
{
    public:
        HArrayScalarMultiplier(){};
        virtual ~HArrayScalarMultiplier(){};

        void SetScalarMultiplicationFactor(const ArrayType& fac){fScalarFactor = fac;}

        virtual void Initialize(){;};

        virtual void ExecuteOperation()
        {

            if(this->fInput != NULL && this->fOutput != NULL)
            {
                if(HArrayOperator<ArrayType,NDIM>::HaveSameNumberOfElements(this->fInput, this->fOutput))
                {
                    ArrayType* inptr = this->fInput->GetData();
                    ArrayType* outptr = this->fOutput->GetData();

                    unsigned int n_elem = this->fInput->GetArraySize();
                    for(unsigned int i=0; i < n_elem; i++)
                    {
                        outptr[i] = (inptr[i])*(fScalarFactor);
                    }
                }
            }

        }


    protected:

       ArrayType fScalarFactor;

};


}

#endif /* __HArrayScalarMultiplier_H__ */
