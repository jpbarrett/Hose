#ifndef HBufferAllocatorSpectrometerDataCUDA_HH__
#define HBufferAllocatorSpectrometerDataCUDA_HH__

#include "spectrometer.h"

#include <iostream>
#include <cstdlib>

#include "HBufferAllocatorBase.hh"

namespace hose{

/*
*File: HBufferAllocatorSpectrometerDataCUDA.hh
*Class: HBufferAllocatorSpectrometerDataCUDA
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: buffer allocator which uses new (for testing)
*/

template< typename XBufferItemType >
class HBufferAllocatorSpectrometerDataCUDA: public HBufferAllocatorBase< XBufferItemType >
{
    public:

        HBufferAllocatorSpectrometerDataCUDA():
            HBufferAllocatorBase< XBufferItemType >(),
            fSpectrumLength(2),
            fSampleArrayLength(3) //default values will fail on alloc
        {};

        virtual ~HBufferAllocatorSpectrometerDataCUDA(){};

        //must set the spectrum and array lengths
        void SetSpectrumLength(size_t spec_len){fSpectrumLength = spec_len;};
        void SetSampleArrayLength(size_t array_len){fSampleArrayLength = array_len;}

        size_t GetSpectrumLength() const {return fSpectrumLength;};
        size_t GetSampleArrayLength() const {return fSampleArrayLength;};

    protected:

        virtual XBufferItemType* AllocateImpl(size_t size) override;
        virtual void DeallocateImpl(XBufferItemType* ptr, size_t size) override;

        size_t fSpectrumLength;
        size_t fSampleArrayLength;

};

//template specialization spectrometer_data

template<>
spectrometer_data*
HBufferAllocatorSpectrometerDataCUDA< spectrometer_data >::AllocateImpl(size_t size)
{
    if(size != 1)
    {
        std::cout<<"HBufferAllocatorSpectrometerDataCUDA::AllocateImpl: Error! spectrometer_data buffers can only be of size 1."<<std::endl;
        std::exit(1); //crash and burn
    }

    if(fSampleArrayLength%fSpectrumLength != 0)
    {
        std::cout<<"HBufferAllocatorSpectrometerDataCUDA::AllocateImpl: Error! Sample data array must be multiple of spectrum length"<<std::endl;
        std::exit(1); //crash and burn
    }

    spectrometer_data* ptr = nullptr;
    ptr = new_spectrometer_data(fSampleArrayLength, fSpectrumLength);
    return ptr;
}

template<>
void
HBufferAllocatorSpectrometerDataCUDA< spectrometer_data >::DeallocateImpl(spectrometer_data* ptr, size_t /*size*/)
{
    free_spectrometer_data(ptr);
}

template< typename XBufferItemType>
XBufferItemType*
HBufferAllocatorSpectrometerDataCUDA< XBufferItemType >::AllocateImpl(size_t size)
{
    //fail away
    std::cout<<"HBufferAllocatorSpectrometerDataCUDA::AllocateImpl: No generic impl!"<<std::endl;
    std::exit(1);
}

template< typename XBufferItemType >
void
HBufferAllocatorSpectrometerDataCUDA< XBufferItemType >::DeallocateImpl(XBufferItemType* ptr, size_t /*size*/)
{
    //fail away
    std::cout<<"HBufferAllocatorSpectrometerDataCUDA::DeallocateImpl: No generic impl!"<<std::endl;
    std::exit(1);
}




}

#endif /* end of include guard: HBufferAllocatorSpectrometerDataCUDA */
