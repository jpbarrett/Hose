#ifndef HATS9371BufferAllocator_HH__
#define HATS9371BufferAllocator_HH__

#include <utility>
#include <cstddef>
#include <vector>
#include <iostream>

#include <stdio.h>
#include <string.h>

#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"

#include "HBufferAllocatorBase.hh"

#define ats_sample_t U16

namespace hose {

/*
*File: HATS9371BufferAllocator.hh
*Class: HATS9371BufferAllocator
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

class HATS9371BufferAllocator: public HBufferAllocatorBase< ats_sample_t >
{
    public:
        HATS9371BufferAllocator(HANDLE ats_board):fBoard(ats_board)
        {
        };
        virtual ~HATS9371BufferAllocator();

    protected:

        ats_sample_t* AllocateImpl(size_t size);
        void DeallocateImpl(ats_sample_t* ptr, size_t size);

    private:

        //digitizer board
        HANDLE fBoard;

        //keep track of allocated memory (for destructor)
        std::vector< std::pair< ats_sample_t*, size_t >  > fAllocatedMemoryBlocks;
};




}

#endif /* end of include guard: HATS9371BufferAllocator */
