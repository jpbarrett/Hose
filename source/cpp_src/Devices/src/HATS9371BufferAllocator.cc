#include "HATS9371BufferAllocator.hh"

#include <iostream>
#include <sstream>

namespace hose
{

HATS9371BufferAllocator::~HATS9371BufferAllocator()
{
    //better free all the memory we allocated if its still around
    for(auto iter = fAllocatedMemoryBlocks.begin(); iter != fAllocatedMemoryBlocks.end(); iter++)
    {
        ats_sample_t* ptr = iter->first;
        AlazarFreeBufferU16(fBoard, ptr);
    }
    fAllocatedMemoryBlocks.clear();
};

ats_sample_t*
HATS9371BufferAllocator::AllocateImpl(size_t size)
{
    // Allocate memory for DMA buffer
    double mem_mb = ( (double)(sizeof(ats_sample_t)*size) )/(1024.0*1024.0);
    //try to allocate page aligned memory
    ats_sample_t* ptr = nullptr;
    ptr = (ats_sample_t*) AlazarAllocBufferU16(fBoard, size*sizeof(ats_sample_t) );
    if(ptr == NULL)
    {
        std::stringstream ss;
        ss << "Failed to allocate DMA buffer of size: ";
        ss << mem_mb;
        ss <<" MB.";
        return nullptr;
    }
    else
    {
        fAllocatedMemoryBlocks.push_back( std::pair< ats_sample_t*, size_t >(ptr, size) );
        return ptr;
    }

}

void
HATS9371BufferAllocator::DeallocateImpl(ats_sample_t* ptr, size_t size)
{
    //free
    AlazarFreeBufferU16(fBoard, ptr);
    //remove this entry from the list of allocated buffers
    for(auto iter = fAllocatedMemoryBlocks.begin(); iter != fAllocatedMemoryBlocks.end(); iter++)
    {
        if(iter->first == ptr && iter->second == size)
        {
            fAllocatedMemoryBlocks.erase(iter);
            break;
        }
    }
}

}
