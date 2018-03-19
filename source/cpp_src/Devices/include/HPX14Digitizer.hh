#ifndef HPX14Digitizer_HH__
#define HPX14Digitizer_HH__

#include <cstddef>
#include <stdint.h>

extern "C"
{
    #include "px14.h"
}

#include <mutex>

#include "HDigitizer.hh"
#include "HPX14BufferAllocator.hh"

namespace hose {

/*
*File: HPX14Digitizer.hh
*Class: HPX14Digitizer
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: Partially ported from original gpu-spec code by Juha Vierinen/Russ McWirther
*/


class HPX14Digitizer: public HDigitizer< px14_sample_t, HPX14Digitizer >
{
    public:
        HPX14Digitizer();
        virtual ~HPX14Digitizer();


        //methods to configure board
        void SetBoardNumber(unsigned int n){fBoardNumber = n;};
        unsigned int GetBoardNumber() const {return fBoardNumber;};

        HPX14 GetBoard() {return fBoard;};
        bool IsInitialized() const {return fInitialized;};
        bool IsConnected() const {return fConnected;};
        bool IsArmed() const {return fArmed;};

    protected:

        friend class HDigitizer<px14_sample_t, HPX14Digitizer >;

        HPX14 fBoard;
        unsigned int fBoardNumber; //board id
        unsigned int fAcquisitionRateMHz; //sampling frequency in MHz
        bool fConnected;
        bool fInitialized;
        bool fArmed;
        bool fBufferLocked;

        //required by digitizer interface
        bool InitializeImpl();
        void AcquireImpl();
        void TransferImpl();
        int FinalizeImpl();
        void StopImpl();
        void TearDownImpl();

    	//global sample counter
    	uint64_t fCounter;

};

}

#endif /* end of include guard: HPX14Digitizer */
