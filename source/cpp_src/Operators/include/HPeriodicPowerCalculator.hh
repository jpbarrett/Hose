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

extern "C"
{
    #include "HDataAccumulationStruct.h"
}

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
            fBuffer = nullptr;
        }
        virtual ~HPeriodicPowerCalculator(){};

        void SetSamplingFrequency(double samp_freq){fSamplingFrequency = samp_freq;};
        void SetSwitchingFrequency(double switch_freq){fSwitchingFrequency = switch_freq;};
        void SetBlankingPeriod(double blank_period){fBlankingPeriod = blank_period;}

        //buffer getter/setters
        void SetBuffer(HLinearBuffer< XBufferItemType >* buffer)
        {
            //assume this buffer already locked externally by the caller
            fBuffer = buffer; 
        }

        void Calculate()
        {
            if( fBuffer != nullptr )
            {
                uint64_t leading_sample_index = fBuffer->GetMetaData()->GetLeadingSampleIndex();
                uint64_t buffer_size = fBuffer->GetArrayDimension(0);
                XBufferItemType* raw_data = fBuffer->GetData();

                //determine which buffer samples are in the on/of periods
                //(this assumes the noise diode switching and acquisition triggering was synced with the 1pps signal)
                std::vector< std::pair<uint64_t, uint64_t> > on_intervals;
                std::vector< std::pair<uint64_t, uint64_t> > off_intervals;
                // GetOnOffIntervals(leading_sample_index, buffer_size, on_intervals, off_intervals);

                GetOnOffIntervals(fSamplingFrequency, fSwitchingFrequency, fBlankingPeriod, leading_sample_index, buffer_size, on_intervals, off_intervals);

                //std::cout<<"leading index: "<<leading_sample_index<<" n on = "<<on_intervals.size()<<" n off = "<<off_intervals.size()<<std::endl;

                //debug print out the value of the first few samples:
                std::cout<<"raw_data = "<<raw_data[0]<<', '<<raw_data[1]<<", "<<raw_data[2]<<std::endl;

                fBuffer->GetMetaData()->ClearAccumulation();

                //loop over the on-intervals accumulating statistics
                for(unsigned int i=0; i<on_intervals.size(); i++)
                {
                    uint64_t begin = on_intervals[i].first;
                    uint64_t end = on_intervals[i].second;

                    struct HDataAccumulationStruct stat;
                    stat.start_index = leading_sample_index + begin;
                    stat.stop_index = leading_sample_index + end;
                    stat.sum_x = 0;
                    stat.sum_x2 = 0;
                    stat.count = 0;
                    stat.state_flag = H_NOISE_DIODE_ON;
                    for(uint64_t sample_index = begin; sample_index < end; sample_index++)
                    {
                        double val = raw_data[sample_index];
                        stat.sum_x += val;
                        stat.sum_x2 += val*val;
                        stat.count += 1.0;
                    }
                    fBuffer->GetMetaData()->AppendAccumulation(stat);
                }

                //loop over the off-intervals accumulating statistics
                for(unsigned int i=0; i<off_intervals.size(); i++)
                {
                    uint64_t begin = off_intervals[i].first;
                    uint64_t end = off_intervals[i].second;

                    struct HDataAccumulationStruct stat;
                    stat.start_index = leading_sample_index + begin;
                    stat.stop_index = leading_sample_index + end;
                    stat.sum_x = 0;
                    stat.sum_x2 = 0;
                    stat.count = 0;
                    stat.state_flag = H_NOISE_DIODE_OFF;
                    for(uint64_t sample_index = begin; sample_index < end; sample_index++)
                    {
                        double val = raw_data[sample_index];
                        stat.sum_x += val;
                        stat.sum_x2 += val*val;
                        stat.count += 1.0;
                    }
                    fBuffer->GetMetaData()->AppendAccumulation(stat);
                }
            }
        }   

    private:


        //returns intervals [x,y) of samples to be included in the ON/OFF sets, does not take the blanking period into account
        //assumes signal synced such that ON is the first state (at acquisition start, index=0)
        void GetOnOffIntervals( double sampling_frequency, 
                                double switching_frequency, 
                                double blanking_period,
                                uint64_t start, 
                                uint64_t length, 
                                std::vector< std::pair<uint64_t, uint64_t> >& on_intervals, 
                                std::vector< std::pair<uint64_t, uint64_t> >& off_intervals)
        {
            on_intervals.clear();
            off_intervals.clear();
        
            double sampling_period = 1.0/sampling_frequency;
            double switching_period = 1.0/switching_frequency;
            double half_switching_period = switching_period/2.0;
            double buffer_length = sampling_period*length;
            double buffer_start = start*sampling_period;
            double buffer_end = buffer_start + buffer_length;

            //figure out the last/next on/off-times which occured on or before the start/end of this buffer
            unsigned int a = std::floor(buffer_start/half_switching_period);
            unsigned int b = std::ceil((buffer_start+sampling_period)/half_switching_period);
            bool on_at_start = false;
            if(a % 2 == 0){on_at_start = true;};

            unsigned int c = std::floor((buffer_end)/half_switching_period);
            unsigned int d = std::ceil( (buffer_end+sampling_period)/half_switching_period);

            //std::cout<<"leading: "<<start<<", a,b,c,d = "<<a<<", "<<b<<", "<<c<<", "<<d<<std::endl;

            std::vector< std::pair<double, double> > on_times;
            std::vector< std::pair<double, double> > off_times;

            //take care of simple cases first
            if(a == c && b == d)
            {
                if(on_at_start)
                {
                    //entire interval is contained within an on period
                    on_times.push_back( std::pair<double, double>(0.0, buffer_length) );
                }
                else
                {
                    //entire interval is contained within an off period
                    off_times.push_back( std::pair<double, double>(0.0, buffer_length) );
                }
            }
            else if( b == c)
            {
                //std::cout<<"hey"<<std::endl;
                //there is a single on/off transition during the interval
                if(on_at_start)
                {
                    on_times.push_back( std::pair<double, double>(0.0, b*half_switching_period - buffer_start) );
                    off_times.push_back( std::pair<double, double>(b*half_switching_period - buffer_start, buffer_length) );
                }
                else
                {
                    off_times.push_back( std::pair<double, double>(0.0, b*half_switching_period - buffer_start) );
                    on_times.push_back( std::pair<double, double>(b*half_switching_period - buffer_start, buffer_length) );
                }
            }
            else
            {
                //there are multiple on-off transitions during the interval

                //take care of the portion between the start and b
                if(on_at_start)
                {
                    on_times.push_back( std::pair<double, double>(0.0, b*half_switching_period - buffer_start) );
                }
                else
                {
                    off_times.push_back( std::pair<double, double>(0.0, b*half_switching_period - buffer_start) );
                }

                //take care the multiple switching periods between b and c
                for(unsigned int i=b; i<c; i++)
                {
                    if(i % 2 == 0)
                    {
                        on_times.push_back( std::pair<double, double>(i*half_switching_period - buffer_start, (i+1)*half_switching_period - buffer_start) );
                    }
                    else
                    {
                        off_times.push_back( std::pair<double, double>(i*half_switching_period - buffer_start, (i+1)*half_switching_period - buffer_start) );
                    }
                }

                //take care of the portion between c and then end
                if(c % 2 == 0)
                {
                    on_times.push_back( std::pair<double, double>(c*half_switching_period - buffer_start, buffer_length) );
                }
                else
                {
                    off_times.push_back( std::pair<double, double>(c*half_switching_period- buffer_start, buffer_length) );
                }

            }
            
            uint64_t blank = std::ceil( (blanking_period/2.0)*sampling_frequency ); 
                
            //now convert to sample indices, while removing any time ranges which have a size of zero
            for(unsigned int i=0; i<on_times.size(); i++)
            {
                if(on_times[i].second - on_times[i].first > 0.0)
                {
                    uint64_t begin = std::floor(on_times[i].first/sampling_period);
                    uint64_t end = std::floor(on_times[i].second/sampling_period);
                    //eliminate any intervals which are too short, and correct for blanking period
                    //yes...we are blanking at the buffer start/stop too, this is trivial amount of data loss that would be a pain to fix
                    if(end > begin + 2*blank+1)
                    {
                        on_intervals.push_back( std::pair<uint64_t, uint64_t>(begin+blank, end-blank) );
                    }
                }
            }

            for(unsigned int i=0; i<off_times.size(); i++)
            {
                if(off_times[i].second - off_times[i].first > 0.0)
                {
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




        // 
        // //returns intervals [x,y) of samples to be included in the ON/OFF sets, does not take the blanking period into account
        // //assumes signal synced such that ON is the first state (at acquisition start, index=0)
        // void GetOnOffIntervals(uint64_t start, uint64_t length, 
        //                        std::vector< std::pair<uint64_t, uint64_t> >& on_intervals, 
        //                        std::vector< std::pair<uint64_t, uint64_t> >& off_intervals)
        // {
        //     on_intervals.clear();
        //     off_intervals.clear();
        // 
        //     double sampling_period = 1.0/fSamplingFrequency;
        //     double switching_period = 1.0/fSwitchingFrequency;
        //     double buffer_time_length = sampling_period*length;
        //     unsigned int n_switching_periods_per_buffer = std::ceil(buffer_time_length/switching_period); //possibly one more than needed, will trim later
        // 
        //     double time_since_acquisition_start = start*sampling_period;
        //     uint64_t n_switching_periods = std::floor( time_since_acquisition_start/switching_period );
        //     double time_remainder = time_since_acquisition_start - n_switching_periods*switching_period;
        // 
        // 
        //     //create the on/off intervals assuming that the buffer start is aligned with the switching frequency (not necessarily true)
        //     double lower = 0.0;
        //     double upper = switching_period/2.0;
        //     std::vector< std::pair<double, double> > on_times;
        //     std::vector< std::pair<double, double> > off_times;
        //     // for(unsigned int i=0; i<n_switching_periods_per_buffer; i++)
        //     do
        //     {
        //         on_times.push_back( std::pair<double, double>(lower, upper) );
        //         lower += switching_period;
        //         off_times.push_back( std::pair<double, double>(upper, lower) );
        //         upper += switching_period;
        //     }
        //     while(lower <= buffer_time_length + time_remainder);
        // 
        // 
        //     //determine number of samples to blank
        //     uint64_t blank = std::ceil( (fBlankingPeriod/2.0)*fSamplingFrequency );
        // 
        //     //subtract off the time_remainder from the switching times and ensure it is in the range [0,buffer_time_length]
        //     for(unsigned int i=0; i<n_switching_periods_per_buffer; i++)
        //     {
        //         on_times[i].first -= time_remainder; 
        //         if(on_times[i].first <= 0.0){on_times[i].first = 0.0;}
        //         else{on_times[i].first = std::min(on_times[i].first, buffer_time_length);}
        // 
        //         on_times[i].second -= time_remainder; 
        //         if(on_times[i].second <= 0.0){on_times[i].second = 0.0;}
        //         else{on_times[i].second = std::min(on_times[i].second, buffer_time_length);}
        // 
        //         off_times[i].first -= time_remainder; 
        //         if(off_times[i].first <= 0.0){off_times[i].first = 0.0;}
        //         else{off_times[i].first = std::min(off_times[i].first, buffer_time_length);}
        // 
        //         off_times[i].second -= time_remainder; 
        //         if(off_times[i].second <= 0.0){off_times[i].second = 0.0;}
        //         else{off_times[i].second = std::min(off_times[i].second, buffer_time_length);}
        //     }
        // 
        //     }
        // }


        //data
        double fSamplingFrequency;
        double fSwitchingFrequency; //frequency at which the noise diode is switched
        double fBlankingPeriod; // ignore samples within +/- half the blanking period about switching time 

        //buffer of data
        HLinearBuffer< XBufferItemType >* fBuffer;

};


}

#endif /* end of include guard: HPeriodicPowerCalculator */
