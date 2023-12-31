#ifndef HMultidimensionalFastFourierTransform_HH__
#define HMultidimensionalFastFourierTransform_HH__

#include <cstring>
#include <iostream>

#include "HArrayWrapper.hh"
#include "HFastFourierTransform.hh"

namespace hose
{

/*
*
*@file HMultidimensionalFastFourierTransform.hh
*@class HMultidimensionalFastFourierTransform
*@brief
*@details
*/

template<size_t NDIM>
class HMultidimensionalFastFourierTransform: public HUnaryArrayOperator< std::complex<double>, NDIM >
{
    public:
        HMultidimensionalFastFourierTransform()
        {
            for(size_t i=0; i<NDIM; i++)
            {
                fDimensionSize[i] = 0;
                fWorkspace[i] = NULL;
                fWorkspaceWrapper[i] = NULL;
                fTransformCalculator[i] = NULL;
            }

            fIsValid = false;
            fInitialized = false;
            fForward = true;
        };

        virtual ~HMultidimensionalFastFourierTransform()
        {
            DealocateWorkspace();
        };

        virtual void SetForward(){fForward = true;}
        virtual void SetBackward(){fForward = false;};

        virtual void Initialize() override
        {
            if(DoInputOutputDimensionsMatch())
            {
                fIsValid = true;
                this->fInput->GetArrayDimensions(fDimensionSize);
            }
            else
            {
                fIsValid = false;
            }

            if(!fInitialized && fIsValid)
            {
                DealocateWorkspace();
                AllocateWorkspace();

                fInitialized = true;
            }
        }

        virtual void ExecuteOperation() override
        {
            if(fIsValid && fInitialized)
            {
                size_t total_size = 1;
                for(size_t i=0; i<NDIM; i++)
                {
                    total_size *= fDimensionSize[i];
                    if(fForward)
                    {
                        fTransformCalculator[i]->SetForward();
                    }
                    else
                    {
                        fTransformCalculator[i]->SetBackward();
                    }
                }

                //if input and output point to the same array, don't bother copying data over
                if(this->fInput != this->fOutput)
                {
                    //the arrays are not identical so copy the input over to the output
                    std::memcpy( (void*) this->fOutput->GetData(), (void*) this->fInput->GetData(), total_size*sizeof(std::complex<double>) );
                }

                size_t index[NDIM];
                size_t non_active_dimension_size[NDIM-1];
                size_t non_active_dimension_value[NDIM-1];
                size_t non_active_dimension_index[NDIM-1];

                //select the dimension on which to perform the FFT
                for(size_t d = 0; d < NDIM; d++)
                {
                    //now we loop over all dimensions not specified by d
                    //first compute the number of FFTs to perform
                    size_t n_fft = 1;
                    size_t count = 0;
                    for(size_t i = 0; i < NDIM; i++)
                    {
                        if(i != d)
                        {
                            n_fft *= fDimensionSize[i];
                            non_active_dimension_index[count] = i;
                            non_active_dimension_size[count] = fDimensionSize[i];
                            count++;
                        }
                    }

                    //loop over the number of FFTs to perform
                    for(size_t n=0; n<n_fft; n++)
                    {
                        //invert place in list to obtain indices of block in array
                        HArrayMath::RowMajorIndexFromOffset<NDIM-1>(n, non_active_dimension_size, non_active_dimension_value);

                        //copy the value of the non-active dimensions in to index
                        for(size_t i=0; i<NDIM-1; i++)
                        {
                            index[ non_active_dimension_index[i] ] = non_active_dimension_value[i];
                        }

                        size_t data_location;
                        //copy the row selected by the other dimensions
                        for(size_t i=0; i<fDimensionSize[d]; i++)
                        {
                            index[d] = i;
                            data_location = HArrayMath::OffsetFromRowMajorIndex<NDIM>(fDimensionSize, index);
                            (*(fWorkspaceWrapper[d]))[i] = (*(this->fOutput))[data_location];
                        }

                        //compute the FFT of the row selected
                        fTransformCalculator[d]->ExecuteOperation();

                        //copy the row selected back
                        for(size_t i=0; i<fDimensionSize[d]; i++)
                        {
                            index[d] = i;
                            data_location = HArrayMath::OffsetFromRowMajorIndex<NDIM>(fDimensionSize, index);
                            (*(this->fOutput))[data_location] = (*(fWorkspaceWrapper[d]))[i];
                        }
                    }
                }
            }
        }


    private:

        virtual void AllocateWorkspace()
        {
            size_t dim[1];
            for(size_t i=0; i<NDIM; i++)
            {
                dim[0] = fDimensionSize[i];
                fWorkspace[i] = new std::complex<double>[ fDimensionSize[i] ];
                fWorkspaceWrapper[i] = new HArrayWrapper< std::complex<double>, 1 >(fWorkspace[i], dim);
                fTransformCalculator[i] = new HFastFourierTransform();
                fTransformCalculator[i]->SetSize(fDimensionSize[i]);
                fTransformCalculator[i]->SetInput(fWorkspaceWrapper[i]);
                fTransformCalculator[i]->SetOutput(fWorkspaceWrapper[i]);
                fTransformCalculator[i]->Initialize();
            }
        }

        virtual void DealocateWorkspace()
        {
            for(size_t i=0; i<NDIM; i++)
            {
                delete[] fWorkspace[i]; fWorkspace[i] = NULL;
                delete fWorkspaceWrapper[i]; fWorkspaceWrapper[i] = NULL;
                delete fTransformCalculator[i]; fTransformCalculator[i] = NULL;
            }
        }

        virtual bool DoInputOutputDimensionsMatch()
        {
            size_t in[NDIM];
            size_t out[NDIM];

            this->fInput->GetArrayDimensions(in);
            this->fOutput->GetArrayDimensions(out);

            for(size_t i=0; i<NDIM; i++)
            {
                if(in[i] != out[i])
                {
                    return false;
                }
            }
            return true;
        }

        bool fIsValid;
        bool fForward;
        bool fInitialized;

        size_t fDimensionSize[NDIM];

        HFastFourierTransform* fTransformCalculator[NDIM];
        std::complex<double>* fWorkspace[NDIM];
        HArrayWrapper<std::complex<double>, 1>* fWorkspaceWrapper[NDIM];


};


}

#endif /* HMultidimensionalFastFourierTransform_H__ */
