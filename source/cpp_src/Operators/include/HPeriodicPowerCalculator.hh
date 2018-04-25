#ifndef HPeriodicPowerCalculator_HH__
#define HPeriodicPowerCalculator_HH__

#include <iostream>
#include <ostream>
#include <ios>
#include <fstream>
#include <sstream>
#include <thread>
#include <utility>
#include <algorithm>
#include <stdint.h>
#include <cmath>

#include "HLinearBuffer.hh"
#include "HBufferPool.hh"
#include "HConsumer.hh"

#include "spectrometer.h"


namespace hose
{

/*
*File: HPeriodicPowerCalculator.hh
*Class: HPeriodicPowerCalculator
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: Computes the sample RMS, for switched noise source,
*/

template< typename XBufferItemType > 
class HPeriodicPowerCalculator: public HConsumer< XBufferItemType, HConsumerBufferHandler_Wait< XBufferItemType> >
{
    public:
        HPeriodicPowerCalculator();
        virtual ~HPeriodicPowerCalculator();

        //only 1 thread allowed now
        virtual void SetNThreads(unsigned int /*n*/) override {this->fNThreads = 1;}

        void Reset()
        {
            //clear the rms data
            fRMSOn.clear();
            fRMSOff.clear();
        }


    private:

        virtual void ExecuteThreadTask() override
        {
            //for now we assume fNThreads is 1
            //get a buffer from the buffer handler
            HLinearBuffer< XBufferItemType >* tail = nullptr;
            if( this->fBufferPool->GetConsumerPoolSize() != 0 )
            {
                //grab a buffer to process
                HConsumerBufferPolicyCode buffer_code = this->fBufferHandler.ReserveBuffer(this->fBufferPool, tail);

                if(buffer_code == HConsumerBufferPolicyCode::success && tail != nullptr)
                {
                    std::lock_guard<std::mutex> lock( tail->fMutex );

                    uint64_t leading_sample_index = tail->GetMetaData()->GetLeadingSampleIndex();
                    uint64_t buffer_size = tail->GetMetaData()->GetArrayDimension(0);

                    //determine if there is an averaging period roll-over some where in this buffer

                    //determine which buffer samples are in the on/of/blanking period
                    //(this assumes the noise diode switching and acquisition triggering was synced with the 1pps signal)
                    
                    


                    //free the tail for re-use
                    this->fBufferHandler.ReleaseBufferToProducer(this->fBufferPool, tail);
                }
            }

        }

        virtual bool WorkPresent() override
        {
            return ( this->fBufferPool->GetConsumerPoolSize() != 0 );
        }

        uint64_t GetNextAveragingPeriodRollOverIndex(uint64_t start)
        {
            uint64_t sampfreq = fSamplingFrequency;
            if(start%sampfreq == 0)
            {
                return start;
            }
            else
            {

            }
        }

        std::vector< std::pair<uint64_t, uint64_t> GetOffIntervals(uint64_t start, uint64_t length)
        {
            uint64_t sampfreq = fSamplingFrequency;
            if(start%sampfreq == 0)
            {
                return start;
            }
            else
            {

            }
        }


        //returns intervals [x,y) of samples to be included in the 'ON' set, takes the blanking period into account
        //assumes signal synced such that ON is the first state (at index=0)
        void GetOnOffIntervals(uint64_t start, uint64_t length, 
                               std::vector< std::pair<uint64_t, uint64_t> >& on_intervals, 
                               std::vector< std::pair<uint64_t, uint64_t> >& off_intervals)
        {
            //determine the offset to the first ON period in the buffer
            //we assume that if there is any fractional sample offset of the switching signal, it remains within
            //the blanking period for the entire length of the buffer (TODO check for this)
            double sampling_period = 1.0/fSamplingFrequency;
            double switching_period = 1.0/fNoiseDiodeSwitchingFrequency;
            double time_since_acquisition_start = start*(1.0/fSamplingFrequency);
            uint64_t n_switching_periods = std::floor( time_since_acquisition_start/switching_period );

            bool on_at_start = false;
            double first_on_time_offset = time_since_acquisition_start - n_switching_periods*switching_period;
            double first_off_time_offset = first_on_time_offset + switching_period/2.0;
            if(first_on_time_offset > switching_period/2.0)
            {
                on_at_start = false;
                first_off_time_offset = first_on_time_offset - switching_period/2.0;
            }

            if(!on_at_start)
            {
                on_intervals.push_back( std::pair<uint64_t, uint64_t>(0) );
            }
            else
            {

            }
    



            uint64_t sample_offset = std::floor(time_offset/sampling_period);

            uint64_t n_switch_samples = std::floor( (1.0/fNoiseDiodeSwitchingFrequency)/fSamplingFrequency );
            uint64_t n_on_samples = n_switch_samples/2;
            uint64_t n_blank_samples = std::floor(fBlankingPeriod/fSamplingFrequency);  

            
            //list all of the on -> off transitions 
            std::vector<uint64_t> ;
            if(sample_offset > n_on_samples)

            while()





            on_intervals.clear();
            off_intervals.clear();
            //take care of the samples before the offset, to make sure we don't loose some useful on samples
            if(sample_offset > n_on_samples + n_blank_samples)
            {
                on_intervals.push_back( std::pair< uint64_t, uint64_t >(0, sample_offset - (n_on_samples + n_blank_samples);) );
            }

            //index the rest of the on intervals
            uint64_t lower = sample_offset;
            uint64_t upper = sample_offset + n_switch_samples;
        
            



            while(lower < length)
            {
                on_intervals.push_back( std::pair< uint64_t, uint64_t>( lower, std::min(upper - n_blank_samples, length) ) );
                off_intervals.push_back( std::pair< uint64_t, uint64_t>( std::min(lower + n_on_samples + n_blank_samples, ) , std::min(lower + n_on_samples - n_blank_samples, length) ) );
                lower += n_switch_samples;
            }
        }



















        bool ContainsRollOver(uint64_t start, uint64_t length)
        {
            uint64_t sampfreq = fSamplingFrequency;
            if(start%sampfreq == 0){return true;}
            unsigned int interval1 = std::floor( ((double)start)/fSamplingFrequency );
            unsigned int interval2 = std::floor( ((double)(start + length))/fSamplingFrequency );
            if(interval1 == interval2)
            {
                return false;
            }
            else
            {
                return true;
            }
        }

        uint64_t GetNextRollOverIndex(uint64_t start, uint64_t length)
        {
            uint64_t sampfreq = fSamplingFrequency;
            if(start%sampfreq == 0){return 0;}
            
            if(length < sampfreq)
            {

            }
            else
            {

            }

        }

        //data
        double fAveragingPeriod; //default is 1 second 
        double fSamplingFrequency;
        double fNoiseDiodeSwitchingFrequency; //frequency at which the noise diode is switched
        double fBlankingPeriod; // ignore samples within +/- the blanking period about switching time 

        //extends these vectors as recording goes on, only cleared on reset
        std::vector< std::pair< double, uint64_t > fRMSOn; //(value of RMS_on over the averaging period, sampling index of start)
        std::vector< std::pair< double, uint64_t > fRMSOff; //(value of RMS_on over the averaging period, sampling index of start)

};


}

#endif /* end of include guard: HPeriodicPowerCalculator */
