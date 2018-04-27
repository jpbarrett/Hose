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
class HPeriodicPowerCalculator
{
    public:

        HPeriodicPowerCalculator()
        {
            fSamplingFrequency = 0.0;
            fSwitchingFrequency = 0.0;
            fBlankingPeriod = 0.0;
            fSecondCount = 0;
            fBuffer = nullptr;
            Reset();
        }
        virtual ~HPeriodicPowerCalculator(){};

        void SetSamplingFrequency(double samp_freq){fSamplingFrequency = samp_freq;};
        void SetSwitchingFrequency(double switch_freq){fSwitchingFrequency = switch_freq;};
        void SetBlankingPeriod(double blank_period){fBlankingPeriod = blank_period;}

        void Reset()
        {
            //clear the rms data
            fRMSOn.clear();
            fRMSOff.clear();
            fOnAccum = 0.0;
            fOffAccum = 0.0;
            fOnSquaredAccum = 0.0;
            fOffSquaredAccum = 0.0;
            fOnCount = 0.0;
            fOffCount = 0.0;
            fSecondCount = 0;
            fBuffer = nullptr;
        }

        //buffer getter/setters
        void SetBuffer(HLinearBuffer< XBufferItemType >* buffer)
        {
            fBuffer = buffer; //assume already locked
        }

        void Calculate()
        {
            if( fBuffer != nullptr )
            {
                uint64_t leading_sample_index = fBuffer->GetMetaData()->GetLeadingSampleIndex();
                uint64_t buffer_size = fBuffer->GetArrayDimension(0);
                XBufferItemType* raw_data = fBuffer->GetData();

                //determine if buffer is the start of a new second (averaging period is always 1 second)
                if( std::floor(leading_sample_index/fSamplingFrequency) > fSecondCount)
                {
                    if( std::floor(leading_sample_index/fSamplingFrequency) - fSecondCount > 1)
                    {
                        std::cout<<"Error: second skipped!"<<std::endl;
                    }

                    //calculate the on, off rms for the past second and push it into storage
                    double on_rms = (fOnSquaredAccum/fOnCount) - (fOnAccum/fOnCount)*(fOnAccum/fOnCount);
                    double off_rms = (fOffSquaredAccum/fOffCount) - (fOffAccum/fOffCount)*(fOffAccum/fOffCount);
                    fRMSOn.push_back( std::pair< double, unsigned int >(on_rms, fSecondCount ) );
                    fRMSOff.push_back( std::pair< double, unsigned int >(off_rms, fSecondCount) );

                    std::cout<<"on rms = "<<on_rms<<std::endl;
                    std::cout<<"off rms = "<<off_rms<<std::endl;
                    std::cout<<"second = "<<fSecondCount<<std::endl;

                    //reset the accumulators
                    fOnAccum = 0.0;
                    fOffAccum = 0.0;
                    fOnSquaredAccum = 0.0;
                    fOffSquaredAccum = 0.0;
                    fOnCount = 0.0;
                    fOffCount = 0.0;
                    fSecondCount++;
                }

                //determine which buffer samples are in the on/of periods
                //(this assumes the noise diode switching and acquisition triggering was synced with the 1pps signal)
                std::vector< std::pair<uint64_t, uint64_t> > on_intervals;
                std::vector< std::pair<uint64_t, uint64_t> > off_intervals;
                GetOnOffIntervals(leading_sample_index, buffer_size, on_intervals, off_intervals);

                double on_count = 0.0;
                double on_accum = 0.0;
                double on_squared_accum = 0.0;

                //loop over the intervals accumulating statistics
                for(unsigned int i=0; i<on_intervals.size(); i++)
                {
                    uint64_t begin = on_intervals[i].first;
                    uint64_t end = on_intervals[i].second;
                    for(uint64_t sample_index = begin; sample_index < end; sample_index++)
                    {
                        double val = raw_data[sample_index];
                        on_accum += val;
                        on_squared_accum += val*val;
                        on_count += 1.0;
                    }
                }

                double off_count = 0.0;
                double off_accum = 0.0;
                double off_squared_accum = 0.0;

                //loop over the intervals accumulating statistics
                for(unsigned int i=0; i<off_intervals.size(); i++)
                {
                    uint64_t begin = off_intervals[i].first;
                    uint64_t end = off_intervals[i].second;
                    for(uint64_t sample_index = begin; sample_index < end; sample_index++)
                    {
                        double val = raw_data[sample_index];
                        off_accum += val;
                        off_squared_accum += val*val;
                        off_count += 1.0;
                    }
                }

                fOnAccum += on_accum;
                fOffAccum += off_accum;

                fOnSquaredAccum += on_squared_accum;
                fOffSquaredAccum += off_squared_accum;

                fOnCount += on_count;
                fOffCount += off_count;

                // std::cout<<"on accum = "<<fOnAccum<<std::endl;
                // std::cout<<"on squared accum = "<<fOnSquaredAccum<<std::endl;
                // std::cout<<"on count = "<<fOnCount<<std::endl;
                // 
                // std::cout<<"off accum = "<<fOffAccum<<std::endl;
                // std::cout<<"off squared accum = "<<fOffSquaredAccum<<std::endl;
                // std::cout<<"off count = "<<fOffCount<<std::endl;

            }
        }   

    private:

        //returns intervals [x,y) of samples to be included in the ON/OFF sets, does not take the blanking period into account
        //assumes signal synced such that ON is the first state (at acquisition start, index=0)
        void GetOnOffIntervals(uint64_t start, uint64_t length, 
                               std::vector< std::pair<uint64_t, uint64_t> >& on_intervals, 
                               std::vector< std::pair<uint64_t, uint64_t> >& off_intervals)
        {
            on_intervals.clear();
            off_intervals.clear();

            double sampling_period = 1.0/fSamplingFrequency;
            double switching_period = 1.0/fSwitchingFrequency;
            double buffer_time_length = sampling_period*length;
            unsigned int n_switching_periods_per_buffer = std::ceil(buffer_time_length/switching_period); //possibly one more than needed, will trim later
    
            // std::cout<<"sampling period = "<<sampling_period<<std::endl;
            // std::cout<<"switching_period = "<<switching_period<<std::endl;
            // std::cout<<"buffer time length = "<<buffer_time_length<<std::endl;
            // std::cout<<"n switching_periods per buffer = "<<n_switching_periods_per_buffer<<std::endl;

            //create the on/off intervals assuming that the buffer start is aligned with the switching frequency (not necessarily true)
            double lower = 0.0;
            double upper = switching_period/2.0;
            std::vector< std::pair<double, double> > on_times;
            std::vector< std::pair<double, double> > off_times;
            for(unsigned int i=0; i<n_switching_periods_per_buffer; i++)
            {
                on_times.push_back( std::pair<double, double>(lower, upper) );
                lower += switching_period;
                off_times.push_back( std::pair<double, double>(upper, lower) );
                upper += switching_period;
            }

            double time_since_acquisition_start = start*sampling_period;
            uint64_t n_switching_periods = std::floor( time_since_acquisition_start/switching_period );
            double time_remainder = time_since_acquisition_start - n_switching_periods*switching_period;

            //determine number of samples to blank
            uint64_t blank = std::ceil( (fBlankingPeriod/2.0)*fSamplingFrequency );

            //subtract off the time_remainder from the switching times and ensure it is in the range [0,buffer_time_length]
            for(unsigned int i=0; i<n_switching_periods_per_buffer; i++)
            {
                on_times[i].first -= time_remainder; 
                if(on_times[i].first < 0.0){on_times[i].first = 0.0;}
                else{on_times[i].first = std::min(on_times[i].first, buffer_time_length);}

                on_times[i].second -= time_remainder; 
                if(on_times[i].second < 0.0){on_times[i].second = 0.0;}
                else{on_times[i].second = std::min(on_times[i].second, buffer_time_length);}

                off_times[i].first -= time_remainder; 
                if(off_times[i].first < 0.0){off_times[i].first = 0.0;}
                else{off_times[i].first = std::min(off_times[i].first, buffer_time_length);}

                off_times[i].second -= time_remainder; 
                if(off_times[i].second < 0.0){off_times[i].second = 0.0;}
                else{off_times[i].second = std::min(off_times[i].second, buffer_time_length);}
            }

            //now convert to sample indices, while removing any time ranges which have a size of zero
            for(unsigned int i=0; i<n_switching_periods_per_buffer; i++)
            {
                if(on_times[i].second - on_times[i].first > 0.0)
                {
                    // std::cout<<"on time first = "<<on_times[i].first<<std::endl;
                    // std::cout<<"on time second = "<<on_times[i].second<<std::endl;

                    uint64_t begin = std::floor(on_times[i].first/sampling_period);
                    uint64_t end = std::floor(on_times[i].second/sampling_period);
                    //eliminate any intervals which are too short, and correct for blanking period
                    //yes...we are blanking at the buffer start/stop too, this is trivial amount of data loss that would be a pain to fix
                    if(end > begin + 2*blank+1)
                    {
                        on_intervals.push_back( std::pair<uint64_t, uint64_t>(begin+blank, end-blank) );
                    }
                }

                if(off_times[i].second - off_times[i].first > 0.0)
                {
                    // std::cout<<"off time first = "<<off_times[i].first<<std::endl;
                    // std::cout<<"off time second = "<<off_times[i].second<<std::endl;

                    uint64_t begin =  std::floor(off_times[i].first/sampling_period);
                    uint64_t end = std::floor(off_times[i].second/sampling_period);
                    //eliminate any intervals which are too short, and correct for blanking period
                    //yes...we are blanking at the buffer start/stop too, this is trivial amount of data loss that would be a pain to fix
                    if(end > begin + 2*blank+1)
                    {
                        off_intervals.push_back( std::pair<uint64_t, uint64_t>(begin+blank,end-blank) );
                    }
                }
            }

        }


        //data
        double fSamplingFrequency;
        double fSwitchingFrequency; //frequency at which the noise diode is switched
        double fBlankingPeriod; // ignore samples within +/- half the blanking period about switching time 

        double fOnAccum;
        double fOffAccum;
        double fOnSquaredAccum;
        double fOffSquaredAccum;
        double fOnCount;
        double fOffCount;

        unsigned int fSecondCount;

        //extends these vectors as recording goes on, only cleared on reset
        std::vector< std::pair< double, unsigned int > > fRMSOn; //(value of RMS_on over the averaging period, indexed by seconds from start of acquisition)
        std::vector< std::pair< double, unsigned int > > fRMSOff; //(value of RMS_on over the averaging period, indexed by seconds from start of acquisition)

        //buffer of data
        HLinearBuffer< XBufferItemType >* fBuffer;

};


}

#endif /* end of include guard: HPeriodicPowerCalculator */
