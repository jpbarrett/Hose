#ifndef HBufferMetaData_HH__
#define HBufferMetaData_HH__

#include <string>
#include <stdint.h>

/*
*File: HBufferMetaData.hh
*Class: HBufferMetaData
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: place holder class for buffer meta data, should implement an 'extensible' struct 
*/

class HBufferMetaData
{
    public:

        HBufferMetaData():
            fAquisitionStartSecond(0),
            fSampleIndex(0),
            fSampleRate(0)
        {};

        virtual ~HBufferMetaData(){};

        virtual std::string GetName() const {return std::string("BasicMetaData");}

        //the integer second (since the epoch) at which the aqcuisition was started
        uint64_t GetAquisitionStartSecond() const {return fAquisitionStartSecond;};
        void SetAquisitionStartSecond(const uint64_t& sec) {fAquisitionStartSecond = sec;};

        //sample index relative to start of the aqcuisition
        uint64_t GetLeadingSampleIndex() const {return fSampleIndex;};
        void SetLeadingSampleIndex(const uint64_t& index){fSampleIndex = index;};

        uint64_t GetSampleRate() const {return fSampleRate;};
        void SetSampleRate(const uint64_t& rate) {fSampleRate = rate;};

    private:

        //seconds since epoch, of the start of the aquisition
        uint64_t fAquisitionStartSecond;

        //index of the leading sample relative to the start of the aquisition
        uint64_t fSampleIndex;

        //sample rate in Hz (integer only, may need to relax this limitation)
        uint64_t fSampleRate;
};

#endif /* end of include guard: HBufferMetaData */
