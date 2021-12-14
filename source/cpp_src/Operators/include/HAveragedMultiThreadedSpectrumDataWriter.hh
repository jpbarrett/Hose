#ifndef HAveragedMultiThreadedSpectrumDataWriter_HH__
#define HAveragedMultiThreadedSpectrumDataWriter_HH__

#include <iostream>
#include <ostream>
#include <ios>
#include <fstream>
#include <sstream>
#include <thread>

#include "HLinearBuffer.hh"
#include "HBufferPool.hh"
#include "HConsumer.hh"
#include "HDirectoryWriter.hh"

extern "C"
{
    #include "HBasicDefines.h"
    #include "HSpectrumFile.h"
    #include "HNoisePowerFile.h"
}

namespace hose
{

/*
*File: HAveragedMultiThreadedSpectrumDataWriter.hh
*Class: HAveragedMultiThreadedSpectrumDataWriter
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

class HAveragedMultiThreadedSpectrumDataWriter: public HConsumer< float, HConsumerBufferHandler_Immediate< float > >, public HDirectoryWriter
{
    public:

        HAveragedMultiThreadedSpectrumDataWriter();
        virtual ~HAveragedMultiThreadedSpectrumDataWriter();

        void EnableWriteToDisk(){fEnable = true;};
        void DisableWriteToDisk(){fEnable = false;}

    private:

        virtual void ExecuteThreadTask() override;
        virtual bool WorkPresent() override;
        virtual void Idle() override;

        bool fEnable;

};

}

#endif /* end of include guard: HAveragedMultiThreadedSpectrumDataWriter */
