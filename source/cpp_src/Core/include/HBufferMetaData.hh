#ifndef HBufferMetaData_HH__
#define HBufferMetaData_HH__

#include <string>
#include <vector>
#include <stdint.h>

#include "HDataAccumulation.hh"

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
            fAcquisitionStartSecond(0),
            fSampleIndex(0),
            fSampleRate(0)
        {};

        virtual ~HBufferMetaData(){};

        virtual std::string GetName() const {return std::string("BasicMetaData");}

        //the integer second (since the epoch) at which the aqcuisition was started
        uint64_t GetAcquisitionStartSecond() const {return fAcquisitionStartSecond;};
        void SetAcquisitionStartSecond(const uint64_t& sec) {fAcquisitionStartSecond = sec;};

        //sample index relative to start of the aqcuisition
        uint64_t GetLeadingSampleIndex() const {return fSampleIndex;};
        void SetLeadingSampleIndex(const uint64_t& index){fSampleIndex = index;};

        uint64_t GetSampleRate() const {return fSampleRate;};
        void SetSampleRate(const uint64_t& rate) {fSampleRate = rate;};

        //for now we are going to stuff the noise-diode tsys data in here
        //doing otherwise will require some re-architecting of the whole consumer/producer/buffer pool system
        void AppendOnAccumulation( HDataAccumulation accum){ fOnAccumulations.push_back(accum); };
        std::vector< HDataAccumulation > GetOnAccumulations() {return fOnAccumulations;};

        void AppendOffAccumulation( HDataAccumulation accum){ fOffAccumulations.push_back(accum); };
        std::vector< HDataAccumulation > GetOffAccumulations() {return fOffAccumulations;};


        HBufferMetaData& operator= (const HBufferMetaData& rhs)
        {
            if( this != &rhs)
            {
                fAcquisitionStartSecond = rhs.fAcquisitionStartSecond;
                fSampleIndex = rhs.fSampleIndex;
                fSampleRate = rhs.fSampleRate;
                fOnAccumulations = rhs.fOnAccumulations;
                fOffAccumulations = rhs.fOffAccumulations;
            }
            return *this;
        }


    private:

        //seconds since epoch, of the start of the aquisition
        uint64_t fAcquisitionStartSecond;

        //index of the leading sample relative to the start of the aquisition
        uint64_t fSampleIndex;

        //sample rate in Hz (integer only, may need to relax this limitation)
        uint64_t fSampleRate;

        std::vector< HDataAccumulation > fOnAccumulations;
        std::vector< HDataAccumulation > fOffAccumulations;
};

#endif /* end of include guard: HBufferMetaData */
