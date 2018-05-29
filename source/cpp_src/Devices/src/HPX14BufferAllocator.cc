#include "HPX14BufferAllocator.hh"

#include <iostream>
#include <sstream>

namespace hose
{

HPX14BufferAllocator::~HPX14BufferAllocator()
{
    //better free all the memory we allocated if its still around
    for(auto iter = fAllocatedMemoryBlocks.begin(); iter != fAllocatedMemoryBlocks.end(); iter++)
    {
        px14_sample_t* ptr = iter->first;
        int code = FreeDmaBufferPX14 (fBoard, ptr);
        (void) code; //shut up compiler
    }
    fAllocatedMemoryBlocks.clear();
};

px14_sample_t* 
HPX14BufferAllocator::AllocateImpl(size_t size)
{
    double mem_mb = ( (double)(sizeof(px14_sample_t)*size) )/(1024.0*1024.0);
    px14_sample_t* ptr = nullptr;
    int code = AllocateDmaBufferPX14(fBoard, size, &ptr);
    if( code != SIG_SUCCESS)
    {
        std:stringstream ss;
        ss << "Failed to allocate DMA buffer of size: ";
        ss << mem_mb;
        ss <<" MB.";
        DumpLibErrorPX14(code, ss.str().c_str() );
        return nullptr;
    }
    else
    {
        fAllocatedMemoryBlocks.push_back( std::pair< px14_sample_t*, size_t >(ptr, size) );
        return ptr;
    }

}

void 
HPX14BufferAllocator::DeallocateImpl(px14_sample_t* ptr, size_t size)
{
    //free
    int code = FreeDmaBufferPX14 (fBoard, ptr);
    (void) code; //shut up compiler
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
