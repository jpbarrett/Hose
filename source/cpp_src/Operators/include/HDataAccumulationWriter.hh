#ifndef HDataAccumulationWriter_HH__
#define HDataAccumulationWriter_HH__

#include <iostream>
#include <ostream>
#include <ios>
#include <fstream>
#include <sstream>
#include <string>

extern "C"
{
    #include "HBasicDefines.h"
    #include "HNoisePowerFile.h"
}

#include "HLinearBuffer.hh"
#include "HBufferPool.hh"
#include "HConsumer.hh"
#include "HDirectoryWriter.hh"


#include "HDataAccumulationContainer.hh"

namespace hose
{

/*
*File: HDataAccumulationWriter.hh
*Class: HDataAccumulationWriter
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: write data accumulation to file
*/

class HDataAccumulationWriter: public HConsumer< HDataAccumulationContainer, HConsumerBufferHandler_Immediate< HDataAccumulationContainer > >, public HDirectoryWriter
{

    public:
        HDataAccumulationWriter();
        virtual ~HDataAccumulationWriter(){};

    protected:

        virtual void ExecuteThreadTask() override;
        virtual bool WorkPresent() override;
        virtual void Idle() override;
};


}

#endif /* end of include guard: HDataAccumulationWriter */
