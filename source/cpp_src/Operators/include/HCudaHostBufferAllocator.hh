#ifndef HCudaHostBufferAllocator_HH__
#define HCudaHostBufferAllocator_HH__

#include <cstdlib>

#include "HBufferAllocatorBase.hh"


namespace hose{

/*
*File: HCudaHostBufferAllocator.hh
*Class: HCudaHostBufferAllocator
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: buffer allocator which uses new (for testing)
*/

template< typename XBufferItemType >
class HCudaHostBufferAllocator: public HBufferAllocatorBase< XBufferItemType >
{
    public:

        HCudaHostBufferAllocator():HBufferAllocatorBase< XBufferItemType >() {};
        virtual ~HCudaHostBufferAllocator(){};

    protected:

        virtual XBufferItemType* AllocateImpl(size_t size) override;
        virtual void DeallocateImpl(XBufferItemType* ptr, size_t size) override;

};

template< typename XBufferItemType >
XBufferItemType*
HCudaHostBufferAllocator< XBufferItemType >::AllocateImpl(size_t size)
{

    XBufferItemType* ptr = nullptr;
    size_t s =  size*sizeof(XBufferItemType);
    cuda_alloc_pinned_memory( (void**)&ptr, s );
    return ptr;
}

template< typename XBufferItemType >
void
HCudaHostBufferAllocator< XBufferItemType >::DeallocateImpl(XBufferItemType* ptr, size_t /*size*/)
{
    cuda_free_pinned_memory( (void*) ptr);
}

}

#endif /* end of include guard: HCudaHostBufferAllocator */
