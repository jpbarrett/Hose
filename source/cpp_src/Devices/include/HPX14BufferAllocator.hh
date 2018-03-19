#ifndef HPX14BufferAllocator_HH__
#define HPX14BufferAllocator_HH__

#include <utility>
#include <cstddef>
#include <vector>
#include <iostream>

extern "C"
{
    #include "px14.h"
}

#include "HBufferAllocatorBase.hh"


namespace hose {

/*
*File: HPX14BufferAllocator.hh
*Class: HPX14BufferAllocator
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

class HPX14BufferAllocator: public HBufferAllocatorBase< px14_sample_t >
{
    public:
        HPX14BufferAllocator(HPX14 board):fBoard(board)
        {
        };
        virtual ~HPX14BufferAllocator();

    protected:

        px14_sample_t* AllocateImpl(size_t size);
        void DeallocateImpl(px14_sample_t* ptr, size_t size);

    private:
        
        //digitizer board 
        HPX14 fBoard;

        //keep track of allocated memory (for destructor)
        std::vector< std::pair< px14_sample_t*, size_t >  > fAllocatedMemoryBlocks;
};




}

#endif /* end of include guard: HPX14BufferAllocator */
