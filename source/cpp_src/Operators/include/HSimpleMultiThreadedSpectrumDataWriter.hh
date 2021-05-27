#ifndef HSimpleMultiThreadedSpectrumDataWriter_HH__
#define HSimpleMultiThreadedSpectrumDataWriter_HH__

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

#include "spectrometer.h"

extern "C"
{
    #include "HBasicDefines.h"
    #include "HSpectrumFile.h"
}

namespace hose
{

/*
*File: HSimpleMultiThreadedSpectrumDataWriter.hh
*Class: HSimpleMultiThreadedSpectrumDataWriter
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

class HSimpleMultiThreadedSpectrumDataWriter: public HConsumer< spectrometer_data, HConsumerBufferHandler_Wait< spectrometer_data > >, public HDirectoryWriter
{
    public:

        HSimpleMultiThreadedSpectrumDataWriter();
        virtual ~HSimpleMultiThreadedSpectrumDataWriter();

    private:

        virtual void ExecuteThreadTask() override;
        virtual bool WorkPresent() override;
        virtual void Idle() override;


};

}

#endif /* end of include guard: HSimpleMultiThreadedSpectrumDataWriter */
