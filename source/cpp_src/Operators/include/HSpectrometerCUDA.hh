#ifndef HSpectrometerCUDA_HH__
#define HSpectrometerCUDA_HH__

#include <iostream>
#include <ostream>
#include <ios>
#include <fstream>
#include <sstream>
#include <thread>
#include <map>
#include <utility>
#include <stdint.h>

#include "HLinearBuffer.hh"
#include "HBufferPool.hh"
#include "HConsumerProducer.hh"

#include "spectrometer.h"


namespace hose
{

/*
*File: HSpectrometerCUDA.hh
*Class: HSpectrometerCUDA
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

//template< typename XSourceBufferItemType, typename XSinkBufferItemType, typename XConsumerSourceBufferHandlerPolicyType, typename XProducerSinkBufferHandlerPolicyType > 

class HSpectrometerCUDA: public HConsumerProducer< uint16_t, spectrometer_data, HConsumerBufferHandler_Wait< uint16_t >, HProducerBufferHandler_Steal< spectrometer_data > >
{

    public:
        HSpectrometerCUDA();
        virtual ~HSpectrometerCUDA();

    private:

        virtual void ExecuteThreadTask() override;
        virtual bool WorkPresent() override;

        //length of the data, needed for workspace allocation
        size_t fDataLength;
        //default output directory
        std::string fOutputDirectory;

};


}

#endif /* end of include guard: HSpectrometerCUDA */
