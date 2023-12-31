#ifndef HSpectrometerManager_HH__
#define HSpectrometerManager_HH__

#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <ctime>
#include <cstdio>
#include <cmath>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef HOSE_USE_SPDLOG
#include "spdlog/spdlog.h"
#endif

extern "C"
{
    #include "HBasicDefines.h"
}

#include "HNetworkDefines.hh"
#include "HParameters.hh"
#include "HTokenizer.hh"
#include "HBufferAllocatorNew.hh"

#include "HBufferPool.hh"
#include "HSpectrometerCUDA.hh"
#include "HSpectrumAverager.hh"
#include "HCudaHostBufferAllocator.hh"
#include "HBufferAllocatorSpectrometerDataCUDA.hh"
#include "HSwitchedPowerCalculator.hh"
#include "HDataAccumulationWriter.hh"
#include "HSimpleMultiThreadedSpectrumDataWriter.hh"
#include "HAveragedMultiThreadedSpectrumDataWriter.hh"
#include "HRawDataDumper.hh"

#include "HApplicationBackend.hh"
#include "HServer.hh"

#define SPECTROMETER_TYPE HSpectrometerCUDA
#define SPECTRUM_TYPE spectrometer_data
#define AVERAGER_TYPE HSpectrumAverager

#define NO_WIN 0
#define BH_WIN 1
#define HANN_WIN 2

#ifdef HOSE_USE_PX14
    #include "HPX14Digitizer.hh"
    #define DIGITIZER_TYPE HPX14Digitizer
#endif

#ifdef HOSE_USE_ADQ7
    #include "HADQ7Digitizer.hh"
    #define DIGITIZER_TYPE HADQ7Digitizer

#endif

#ifndef HOSE_USE_ADQ7
    #ifndef HOSE_USE_PX14
        #include "HPX14DigitizerSimulator.hh"
        #define DIGITIZER_TYPE HPX14DigitizerSimulator
    #endif
#endif


//TODO FIXME: replace these with real enums

//command types
#define UNKNOWN 0
#define RECORD_ON 1
#define RECORD_OFF 2
#define CONFIGURE_NEXT_RECORDING 3
#define QUERY 4
#define SHUTDOWN 5
#define SET_POWER_BINS 6

//recording states
#define RECORDING_UNTIL_OFF 1
#define RECORDING_UNTIL_TIME 2
#define IDLE 3
#define PENDING 4

//time states
#define TIME_ERROR -1
#define TIME_BEFORE 0
#define TIME_PENDING 1
#define TIME_AFTER 2

namespace hose
{

/*
*File: HSpectrometerManager.hh
*Class: HSpectrometerManager
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/


//XDigitizerType must inherit from HDigitizer<> and HProducer<>

template< class XDigitizerType = DIGITIZER_TYPE >
class HSpectrometerManager: public HApplicationBackend
{
    public:

        HSpectrometerManager():
            fInitialized(false),
            fStop(false),
            fIP(COMMAND_SERVER_IP_ADDRESS),
            fPort(COMMAND_SERVER_PORT),
            fUDPNoisePowerIP(ODYSSEY_LOCAL_IP_ADDRESS),
            fUDPNoisePowerPort(NOISE_POWER_UDP_PORT),
            fUDPNoisePowerSkipInterval(1),
            fNoisePowerBinLow(0),
            fNoisePowerBinHigh(0),
            fServer(nullptr),
            fDigitizer(nullptr),
            fCUDABufferAllocator(nullptr),
            fSpectrometerBufferAllocator(nullptr),
            fSpectrometer(nullptr),
            fDumper(nullptr),
            fDigitizerSourcePool(nullptr),
            fSpectrometerSinkPool(nullptr),
            fSpectrumAveragingBufferAllocator(nullptr),
            fSpectrumAveragingBufferPool(nullptr),
            fSpectrumAverager(nullptr),
            fAveragedSpectrumWriter(nullptr)
            #ifdef HOSE_USE_SPDLOG
            ,fSink(nullptr),
            fStatusLogger(nullptr),
            fConfigLogger(nullptr)
            #endif
        {
            fCannedStopCommand = "record=off";
            fWindowFlag = NO_WIN;
            fParameters.Initialize();
            
            fEnableSpectrumWriteToFile=1;
            fEnableNoisePowerWriteToFile=0;
        }

        virtual ~HSpectrometerManager()
        {
            delete fServer;
            delete fDigitizer;
            delete fSpectrometer;
            delete fDumper;
            delete fDigitizerSourcePool;
            delete fSpectrometerSinkPool;
            delete fCUDABufferAllocator;
            delete fSpectrometerBufferAllocator;
            delete fSpectrumAveragingBufferAllocator;
            delete fSpectrumAveragingBufferPool;
            delete fSpectrumAverager;
            delete fAveragedSpectrumWriter;
        }

        void SetServerIP(std::string ip){fIP = ip;};
        void SetServerPort(std::string port){fPort = port;};

        void SetNSpectrumAverages(size_t n_ave){fNSpectrumAverages = n_ave;};
        void SetFFTSize(size_t n_fft){fFFTSize = n_fft;};
        void SetDigitizerPoolSize(size_t n_chunks){fDigitizerPoolSize = n_chunks;};
        void SetSpectrometerPoolSize(size_t n_chunks){fSpectrometerPoolSize = n_chunks;};

        void SetNDigitizerThreads(size_t n){fNDigitizerThreads = n;};
        void SetNSpectrometerThreads(size_t n){fNSpectrometerThreads = n;};

        void SetParameters(const HParameters& params)
        {
            //accept configuration parameters
            fParameters = params;
        }

        void Initialize()
        {
            //get our pid
            fPID = getpid();

            if(!fInitialized)
            {
                bool lock_success = GetLockByPID();
                if(lock_success)
                {

                    //extract parameters from configuration
                    fPort = fParameters.GetStringParameter("command_server_port");
                    fIP = fParameters.GetStringParameter("command_server_ip_address");
                    fEnableSpectrumWriteToFile = fParameters.GetIntegerParameter("enable_spectrum_write_to_file");
                    fEnableNoisePowerWriteToFile = fParameters.GetIntegerParameter("enable_noise_power_write_to_file");

                    std::string aWindowName = fParameters.GetStringParameter("window_type");
                    if(aWindowName == "none"){fWindowFlag = NO_WIN;}
                    if(aWindowName == "blackman_harris"){fWindowFlag = BH_WIN;}
                    if(aWindowName == "hann"){fWindowFlag = HANN_WIN;}

                    fUDPNoisePowerPort = fParameters.GetStringParameter("noise_power_port");
                    fUDPNoisePowerIP = fParameters.GetStringParameter("noise_power_ip_address");
                    fUDPNoisePowerSkipInterval = fParameters.GetIntegerParameter("noise_power_udp_skip_interval");
                    fNoisePowerBinLow = fParameters.GetIntegerParameter("spectral_noise_power_bin_low");
                    fNoisePowerBinHigh = fParameters.GetIntegerParameter("spectral_noise_power_bin_high");
                    fEnableNoisePowerUDPMessages = fParameters.GetIntegerParameter("enable_noise_power_udp");
        
                    fUDPSpectrumPort = fParameters.GetStringParameter("spectrum_port");
                    fUDPSpectrumIP = fParameters.GetStringParameter("spectrum_ip_address");
                    fEnableSpectrumUDPMessages = fParameters.GetIntegerParameter("enable_spectrum_udp");

                    fNSpectrumAverages = fParameters.GetIntegerParameter("n_ave_spectra_gpu");
                    fFFTSize = fParameters.GetIntegerParameter("n_fft_pts");
                    fDigitizerPoolSize = fParameters.GetIntegerParameter("n_digitizer_pool_size");
                    fSpectrometerPoolSize = fParameters.GetIntegerParameter("n_spec_pool_size");
                    fNDigitizerThreads = fParameters.GetIntegerParameter("n_digitizer_threads");
                    fNSpectrometerThreads = fParameters.GetIntegerParameter("n_spec_threads");
                    fNSpectrumAveragesCPU = fParameters.GetIntegerParameter("n_ave_spectra_cpu");
                    fNDumpSkip = fParameters.GetIntegerParameter("n_dump_skip");
                    fNSpectrumAveragerPoolSize = fParameters.GetIntegerParameter("n_spec_ave_pool_size");
                    #ifdef HOSE_USE_ADQ7
                    fNADQ7SampleSkip = fParameters.GetIntegerParameter("n_adq7_sample_skip");
                    #endif

                    //create the loggers
                    #ifdef HOSE_USE_SPDLOG
                    try
                    {
                        std::stringstream lfss;
                        lfss << STR2(LOG_INSTALL_DIR);
                        lfss << "/status.log";
                        fSinkFileName = lfss.str();

                        std::string status_logger_name("status");
                        std::string config_logger_name("config");

                        std::cout<<"creating a log file: "<<fSinkFileName<<std::endl;
                        bool trunc = true;
                        fSink = std::make_shared<spdlog::sinks::simple_file_sink_mt>( fSinkFileName.c_str(), trunc );
                        fStatusLogger = std::make_shared<spdlog::logger>(status_logger_name.c_str(), fSink);
                        fConfigLogger = std::make_shared<spdlog::logger>(config_logger_name.c_str(), fSink);

                        fStatusLogger->flush_on(spdlog::level::info); //make logger flush on every message
                        fConfigLogger->flush_on(spdlog::level::info); //make logger flush on every message

                        fStatusLogger->info("$$$ New session, manager log initialized. $$$");
                    }
                    catch (const spdlog::spdlog_ex& ex)
                    {
                        std::cout << "Manager log initialization failed: " << ex.what() << std::endl;
                    }

                    fStatusLogger->info("Initializing.");
                    #endif

                    std::cout<<"Initializing..."<<std::endl;

                    //create command server
                    fServer = new HServer(fIP, fPort);
                    fServer->SetApplicationBackend(this);
                    fServer->Initialize();

                    //create digitizer
                    fDigitizer = new XDigitizerType();
                    fDigitizer->SetNThreads(fNDigitizerThreads);
                    #ifdef HOSE_USE_ADQ7
                    fDigitizer->SetSampleSkipFactor(fNADQ7SampleSkip);
                    #endif
                    bool digitizer_init_success = fDigitizer->Initialize();

                    if(digitizer_init_success)
                    {
                        //create source buffer pool
                        fCUDABufferAllocator = new HCudaHostBufferAllocator< typename XDigitizerType::sample_type >();
                        fDigitizerSourcePool = new HBufferPool< typename XDigitizerType::sample_type  >( fCUDABufferAllocator );
                        fDigitizerSourcePool->Allocate(fDigitizerPoolSize, fNSpectrumAverages*fFFTSize);
                        fDigitizer->SetBufferPool(fDigitizerSourcePool);

                        //TODO fill these in with real values!
                        fDigitizer->SetSidebandFlag('U');
                        fDigitizer->SetPolarizationFlag('X');

                        //create spectrometer data pool
                        fSpectrometerBufferAllocator = new HBufferAllocatorSpectrometerDataCUDA<SPECTRUM_TYPE>();
                        fSpectrometerBufferAllocator->SetSampleArrayLength(fNSpectrumAverages*fFFTSize);
                        fSpectrometerBufferAllocator->SetSpectrumLength(fFFTSize);
                        fSpectrometerBufferAllocator->SetWindowFunction(fWindowFlag);
                        fSpectrometerSinkPool = new HBufferPool< SPECTRUM_TYPE >( fSpectrometerBufferAllocator );
                        fSpectrometerSinkPool->Allocate(fSpectrometerPoolSize, 1);

                        //create spectrometer
                        fSpectrometer = new SPECTROMETER_TYPE(fFFTSize, fNSpectrumAverages);
                        fSpectrometer->SetNThreads(fNSpectrometerThreads);
                        fSpectrometer->SetSourceBufferPool(fDigitizerSourcePool);
                        fSpectrometer->SetSinkBufferPool(fSpectrometerSinkPool);

                        //create post-spectrometer data pool for averaging
                        fSpectrumAveragingBufferAllocator = new HBufferAllocatorNew< float >();
                        fSpectrumAveragingBufferPool = new HBufferPool< float >(fSpectrumAveragingBufferAllocator);
                        fSpectrumAveragingBufferPool->Allocate(fNSpectrumAveragerPoolSize, fFFTSize/2+1); //create a work space of buffers

                        // fSpectrumAverager = new AVERAGER_TYPE(fFFTSize/2+1, fNSpectrumAveragesCPU); //further average down on cpu
                        fSpectrumAverager = new AVERAGER_TYPE(fFFTSize/2+1, fNSpectrumAveragesCPU, fUDPNoisePowerPort, fUDPNoisePowerIP, fUDPSpectrumPort, fUDPSpectrumIP);
                        fSpectrumAverager->SetNThreads(1); //ONE THREAD ONLY!
                        fSpectrumAverager->SetSourceBufferPool(fSpectrometerSinkPool);
                        fSpectrumAverager->SetSinkBufferPool(fSpectrumAveragingBufferPool);
                        fSpectrumAverager->SetBufferSkip(fUDPNoisePowerSkipInterval);
                        fSpectrumAverager->SetSpectralPowerUpperBound(fNoisePowerBinHigh);
                        fSpectrumAverager->SetSpectralPowerLowerBound(fNoisePowerBinLow);
                        if(fEnableNoisePowerUDPMessages){fSpectrumAverager->EnableNoisePowerUDPMessages();}
                        else{fSpectrumAverager->DisableNoisePowerUDPMessages();}
                        if(fEnableSpectrumUDPMessages){fSpectrumAverager->EnableSpectrumUDPMessages();}
                        else{fSpectrumAverager->DisableSpectrumUDPMessages();}

                        fAveragedSpectrumWriter = new HAveragedMultiThreadedSpectrumDataWriter();
                        fAveragedSpectrumWriter->SetBufferPool(fSpectrumAveragingBufferPool);
                        fAveragedSpectrumWriter->SetNThreads(1);
                        
                        if(fEnableSpectrumWriteToFile){fAveragedSpectrumWriter->EnableSpectrumWriteToDisk();}
                        else{fAveragedSpectrumWriter->DisableSpectrumWriteToDisk();}

                        if(fEnableNoisePowerWriteToFile){fAveragedSpectrumWriter->EnableNoisePowerWriteToDisk();}
                        else{fAveragedSpectrumWriter->DisableNoisePowerWriteToDisk();}

                        //create an itermittent raw data dumper
                        fDumper = new HRawDataDumper< typename XDigitizerType::sample_type >();
                        fDumper->SetBufferPool(fDigitizerSourcePool);
                        fDumper->SetBufferDumpFrequency(fNDumpSkip);
                        fDumper->SetNThreads(1);

                        fDigitizerSourcePool->Initialize();
                        fSpectrometerSinkPool->Initialize();
                        fSpectrumAveragingBufferPool->Initialize();

                        #ifdef HOSE_USE_SPDLOG

                        //digitizer configuration info
                        std::stringstream ndtss;
                        ndtss << "n_digitizer_threads=";
                        ndtss << fNDigitizerThreads;

                        std::stringstream sbfss;
                        sbfss << "sideband=";
                        sbfss << 'U';

                        std::stringstream pfss;
                        pfss << "polarization=";
                        pfss << 'X';

                        std::stringstream sfss;
                        sfss << "sampling_frequency_Hz=";
                        sfss << fDigitizer->GetSamplingFrequency();

                        std::string digitizer_config = "digitizer_config; " + ndtss.str() + "; " + sbfss.str() + "; " + pfss.str() + "; " + sfss.str();
                        fConfigLogger->info( digitizer_config.c_str() );

                        //spectrometer configuration line
                        std::stringstream navess;
                        navess << "n_averages=";
                        navess << fNSpectrumAverages;

                        std::stringstream fftss;
                        fftss << "fft_size=";
                        fftss << fFFTSize;

                        //window function information, this is essentially hard-coded for now,
                        //but it is possible we may implement different types in the future
                        //TODO FIXME move this calculation out of here
                        std::vector<double> pOut;
                        double s1 = 0;
                        double s2 = 0;
                        pOut.resize(fFFTSize);


                        if(fWindowFlag == NO_WIN)
                        {
                            s1 = fFFTSize;
                            s2 = fFFTSize;
                        }

                        if(fWindowFlag == BH_WIN)
                        {
                            double a0 = 0.35875f;
                            double a1 = 0.48829f;
                            double a2 = 0.14128f;
                            double a3 = 0.01168f;
                            unsigned int idx = 0;
                            while( idx < fFFTSize )
                            {
                                pOut[idx]   = a0 - (a1 * std::cos( (2.0f * M_PI * idx) / (fFFTSize- 1) )) + (a2 * std::cos( (4.0f * M_PI * idx) / (fFFTSize- 1) )) - (a3 * std::cos( (6.0f * M_PI * idx) / (fFFTSize - 1) ));
                                s1 += pOut[idx];
                                s2 += pOut[idx]*pOut[idx];
                                idx++;
                            }
                        }

                        if(fWindowFlag == HANN_WIN)
                        {
                            unsigned int idx    = 0;
                            while( idx < fFFTSize )
                            {
                                pOut[idx] = 0.5 + 0.5*( std::cos( (2.0f * M_PI * idx) / (fFFTSize - 1) ) );
                                s1 += pOut[idx];
                                s2 += pOut[idx]*pOut[idx];
                                idx++;
                            }
                        }

                        std::stringstream wtss;
                        wtss << "window_type=" << aWindowName; 
                        std::stringstream wts1ss;
                        wts1ss << "window_s1=";
                        wts1ss << s1;
                        std::stringstream wts2ss;
                        wts2ss << "window_s2=";
                        wts2ss << s2;
                        std::stringstream wtnenbwss;
                        wtnenbwss << "window_normalized_equivalent_noise_bandwidth=";
                        wtnenbwss << fFFTSize*s2/(s1*s1);
                        std::stringstream wtenbwss;
                        wtenbwss << "window_equivalent_noise_bandwidth_Hz=";
                        wtenbwss <<  (fFFTSize*s2/(s1*s1) )*(fDigitizer->GetSamplingFrequency()/fFFTSize);
                        std::stringstream nstss;
                        nstss << "n_spectrometer_threads=";
                        nstss << fNSpectrometerThreads;
                        std::stringstream nwtss;
                        nwtss << "n_writer_threads=";
                        nwtss << 1;


                        std::string spectrometer_config = "spectrometer_config; " + navess.str() + "; "
                            + fftss.str() + "; "
                            + nstss.str() + "; "
                            + nwtss.str() + "; "
                            + wtss.str() + "; "
                            + wts1ss.str() + "; "
                            + wts2ss.str() + "; "
                            + wtnenbwss.str() + "; "
                            + wtenbwss.str();

                        fConfigLogger->info( spectrometer_config.c_str() );

                        //noise diode configuration
                        std::stringstream ndsfss;
                        ndsfss << "noise_diode_switching_frequency_Hz=";
                        ndsfss << 0;
                        // ndsfss << noise_diode_switching_freq;

                        std::stringstream ndbpss;
                        ndbpss << "noise_blanking_period=";
                        ndbpss << 0;
                        // ndbpss << noise_diode_blanking_period;

                        std::string noise_diode_config = "noise_diode_config; " + ndsfss.str() + "; " + ndbpss.str();
                        fConfigLogger->info( noise_diode_config.c_str() );

                        #endif
                        fInitialized = true;
                    }
                }
                else 
                {
                    fStatusLogger->info("Initializtion fail. Could not lock by PID.");
                }
            }
        }

        void Run()
        {
            if(fInitialized)
            {
                //start the command server thread
                std::thread server_thread( &HServer::Run, fServer );

                fAveragedSpectrumWriter->StartConsumption();

                // NUMA node0 CPU(s):     0-7,16-23
                // NUMA node1 CPU(s):     8-15,24-31

                unsigned int core_id = 8;
                for(unsigned int i=0; i<1; i++)
                {
                    fAveragedSpectrumWriter->AssociateThreadWithSingleProcessor(i, core_id++);
                };

                fDumper->StartConsumption();

                fSpectrumAverager->StartConsumptionProduction();
                for(unsigned int i=0; i<1; i++)
                {
                    fSpectrumAverager->AssociateThreadWithSingleProcessor(i, core_id++);
                };

                fSpectrometer->StartConsumptionProduction();
                core_id = 24;
                for(size_t i=0; i<fNSpectrometerThreads; i++)
                {
                    fSpectrometer->AssociateThreadWithSingleProcessor(i, core_id++);
                };

                fDigitizer->StartProduction();
                for(size_t i=0; i<fNDigitizerThreads; i++)
                {
                    fDigitizer->AssociateThreadWithSingleProcessor(i, core_id++);
                };

                fRecordingState = IDLE;

                std::cout<<"Ready."<<std::endl;

                #ifdef HOSE_USE_SPDLOG
                fStatusLogger->info("Ready.");
                #endif

                while(!fStop)
                {
                    //loop, recieve commands from the server, and process them
                    if(fServer->GetNMessages() != 0)
                    {
                        std::string command = fServer->PopMessage();
                        ProcessCommand(command);
                    }

                    //if we are pending, check if we are within 1 second of starting the recording
                    if(fRecordingState == PENDING)
                    {
                        //check if the end time is after the current time
                        if( DetermineTimeStateWRTNow(fEndTime) == TIME_AFTER )
                        {
                            //check if the start time is before the current time
                            if( DetermineTimeStateWRTNow(fStartTime) == TIME_BEFORE ||  DetermineTimeStateWRTNow(fStartTime) == TIME_PENDING )
                            {
                                fRecordingState = RECORDING_UNTIL_TIME;
                                fDigitizer->Acquire();
                                #ifdef HOSE_USE_SPDLOG
                                std::stringstream ss;
                                ss << "recording_status; ";
                                ss << "recording=on; ";
                                ss << "experiment_name=" << fExperimentName << "; ";
                                ss << "source_name=" << fSourceName << "; ";
                                ss << "scan_name=" << fScanName;
                                fStatusLogger->info( ss.str().c_str() );
                                #endif
                            }
                        }
                    }

                    if(fRecordingState == RECORDING_UNTIL_TIME)
                    {
                        if( DetermineTimeStateWRTNow(fEndTime) == TIME_BEFORE || DetermineTimeStateWRTNow(fEndTime) == TIME_PENDING )
                        {
                            ProcessCommand(fCannedStopCommand);
                        }
                    }

                    //sleep for 5 microsecond
                    //usleep(5);
                }

                //kill the command server
                fServer->Terminate();

                //make sure the recording is stopped if we are terminating
                if(fRecordingState != IDLE)
                {
                    ProcessCommand(fCannedStopCommand);
                }

                sleep(1);
                fDigitizer->StopProduction();
                sleep(1);
                fSpectrometer->StopConsumptionProduction();
                sleep(1);
                fAveragedSpectrumWriter->StopConsumption();

                CleanUp();

                //join the server thread
                server_thread.join();
            }

        }

        void Shutdown()
        {
            fStop = true;
        }

    private:

        void CleanUp()
        {
            //remove the lock file
            remove( fLockFileName.c_str() );

            #ifdef HOSE_USE_SPDLOG
            //close and archive the log file
            if(fSink)
            {
                fSink->flush();

                time_t current_time = std::time(nullptr);
                tm current_utc_tm;

                //make sure to use thread safe version of gmtime
                tm* tmp = gmtime_r(&current_time, &current_utc_tm);

                std::stringstream lfss;
                lfss << STR2(LOG_INSTALL_DIR);
                lfss << "/status-";
                // lfss << std::put_time(&current_utc_tm, "%d-%m-%YT%H-%M-%SZ");
                char tmpbuff[50];
                strftime(tmpbuff, sizeof(tmpbuff), "%d-%m-%YT%H-%M-%SZ", &current_utc_tm);
                lfss << std::string(tmpbuff);
                lfss << ".log";

                //rename the
                std::rename(fSinkFileName.c_str(), lfss.str().c_str());
            }

            #endif
        }



        //this is quite primitive, but we only have a handful of commands to support for now
        void ProcessCommand(std::string command)
        {
            std::vector< std::string > tokens = Tokenize(command);
            if(tokens.size() != 0)
            {
                int command_type = LookUpCommand(tokens);
                std::stringstream ss;
                switch(command_type)
                {
                    case SHUTDOWN:
                        fStop = true;
                        if(fRecordingState == RECORDING_UNTIL_OFF || fRecordingState == RECORDING_UNTIL_TIME || fRecordingState == PENDING )
                        {
                            //stop recording immediately
                            if(fRecordingState == RECORDING_UNTIL_OFF || fRecordingState == RECORDING_UNTIL_TIME)
                            {
                                fDigitizer->StopAfterNextBuffer();
                            }
                            fRecordingState = IDLE;
                            #ifdef HOSE_USE_SPDLOG
                            
                            ss << "recording_status; ";
                            ss << "recording=off; ";
                            ss << "experiment_name=" << fExperimentName << "; ";
                            ss << "source_name=" << fSourceName << "; ";
                            ss << "scan_name=" << fScanName;
                            fStatusLogger->info( ss.str().c_str() );
                            #endif
                        }
                    break;
                    case QUERY:
                            //do nothing
                    return;
                    case SET_POWER_BINS:
                            fNoisePowerBinLow = std::atoi(tokens[1].c_str());
                            fNoisePowerBinHigh = std::atoi(tokens[2].c_str());
                            fSpectrumAverager->SetSpectralPowerUpperBound(fNoisePowerBinHigh);
                            fSpectrumAverager->SetSpectralPowerLowerBound(fNoisePowerBinLow);
                            #ifdef HOSE_USE_SPDLOG
                            ss << "set_power_bins;";
                            ss << "low_bin=" << fNoisePowerBinLow << ";";
                            ss << "high_bin=" << fNoisePowerBinHigh << ";";
                            fStatusLogger->info( ss.str().c_str() );
                            #endif
                    break;
                    case RECORD_ON:
                        if(fRecordingState == IDLE)
                        {
                            //configure and turn on recording
                            fExperimentName = tokens[2];
                            fSourceName = tokens[3];
                            fScanName = tokens[4];
                            ConfigureWriters();
                            //start recording immediately
                            fDigitizer->Acquire();
                            fRecordingState = RECORDING_UNTIL_OFF;
                            #ifdef HOSE_USE_SPDLOG
                            ss << "recording_status; ";
                            ss << "recording=on; ";
                            ss << "experiment_name=" << fExperimentName << "; ";
                            ss << "source_name=" << fSourceName << "; ";
                            ss << "scan_name=" << fScanName;
                            fStatusLogger->info( ss.str().c_str() );
                            #endif
                        }
                    break;
                    case RECORD_OFF:
                        if(fRecordingState == RECORDING_UNTIL_OFF || fRecordingState == RECORDING_UNTIL_TIME || fRecordingState == PENDING )
                        {
                            //stop recording immediately
                            if(fRecordingState == RECORDING_UNTIL_OFF || fRecordingState == RECORDING_UNTIL_TIME)
                            {
                                fDigitizer->StopAfterNextBuffer();
                            }
                            fRecordingState = IDLE;
                            #ifdef HOSE_USE_SPDLOG
                            ss << "recording_status; ";
                            ss << "recording=off; ";
                            ss << "experiment_name=" << fExperimentName << "; ";
                            ss << "source_name=" << fSourceName << "; ";
                            ss << "scan_name=" << fScanName;
                            fStatusLogger->info( ss.str().c_str() );
                            #endif
                        }
                    break;
                    case CONFIGURE_NEXT_RECORDING:
                        if(fRecordingState == IDLE )
                        {
                            // std::cout<<"configuring for timed recording"<<std::endl;
                            //configure writer
                            fExperimentName = tokens[2];
                            fSourceName = tokens[3];
                            fScanName = tokens[4];
                            ConfigureWriters();
                            bool success = ConvertStringToTime(tokens[5], fStartTime);
                            // std::cout<<"success flag = "<<success<<std::endl;
                            uint64_t duration = ConvertStringToDuration(tokens[6]);
                            // std::cout<<"duration ="<<duration<<std::endl;
                            // std::cout<<"start time ="<<fStartTime<<std::endl;
                            fEndTime = fStartTime + duration;
                            // std::cout<<"end time = "<<fEndTime<<std::endl;

                            if( (fStartTime < fEndTime) && success)
                            {
                                // std::cout<<"end is after start, ok"<<std::endl;
                                //check if the end time is after the current time
                                if( DetermineTimeStateWRTNow(fEndTime) == TIME_AFTER )
                                {
                                    // std::cout<<"end time is after now, ok"<<std::endl;
                                    //check if the start time is before the current time
                                    if( DetermineTimeStateWRTNow(fStartTime) == TIME_BEFORE ||  DetermineTimeStateWRTNow(fStartTime) == TIME_PENDING )
                                    {
                                        std::cout<<"start time has passed, starting recording"<<std::endl;
                                        fRecordingState = RECORDING_UNTIL_TIME;
                                        fDigitizer->Acquire();
                                        #ifdef HOSE_USE_SPDLOG
                                        ss << "recording_status; ";
                                        ss << "recording=on; ";
                                        ss << "experiment_name=" << fExperimentName << "; ";
                                        ss << "source_name=" << fSourceName << "; ";
                                        ss << "scan_name=" << fScanName;
                                        fStatusLogger->info( ss.str().c_str() );
                                        #endif
                                        return;
                                    }
                                    else
                                    {
                                        //std::cout<<"waiting for start time"<<std::endl;
                                        //start time has not passed, so we are pending until then
                                        fRecordingState = PENDING;
                                        return;
                                    }
                                }
                            }
                        }
                    break;
                    default:
                        //do nothing
                    break;
                };
            }
        }


        std::vector< std::string > Tokenize(std::string command)
        {
            std::vector< std::string > temp_tokens;
            std::vector< std::string > tokens;

            //state query
            if(command == std::string("record?"))
            {
                tokens.push_back( std::string("record?") );
                return tokens;
            }

            //shutdown command
            if(command == std::string("shutdown"))
            {
                tokens.push_back( std::string("shutdown") );
                return tokens;
            }

            //first we split the string by the '=' delimiter
            fTokenizer.SetString(&command);
            fTokenizer.SetIncludeEmptyTokensFalse();
            fTokenizer.SetDelimiter(std::string("="));
            fTokenizer.GetTokens(&temp_tokens);

            if(temp_tokens.size() != 2)
            {
                //error, bail out
                return tokens;
            }

            //now we split the second string by the ':' delimiter
            fTokenizer.SetString(&(temp_tokens[1]));
            fTokenizer.SetIncludeEmptyTokensTrue();
            fTokenizer.SetDelimiter(std::string(":"));
            fTokenizer.GetTokens(&tokens);

            //insert the start token in the front
            tokens.insert(tokens.begin(), temp_tokens[0]);

            return tokens;
        }

        int LookUpCommand(const std::vector< std::string > command_tokens)
        {
            if(command_tokens.size() == 1)
            {
                if( command_tokens[0] == std::string("record?") )
                {
                    return QUERY;
                }
                if( command_tokens[0] == std::string("shutdown") )
                {
                    return SHUTDOWN;
                }
            }

            if(command_tokens.size() >= 2)
            {
                if(command_tokens[0] == std::string("set_power_bins") && command_tokens.size() == 3)
                {
                    return SET_POWER_BINS;
                }

                if(command_tokens[0] == std::string("record") )
                {
                    if(command_tokens[1] == std::string("on") && command_tokens.size() == 5)
                    {
                        return RECORD_ON;
                    }
                    else if (command_tokens[1] == "off" && command_tokens.size() == 2)
                    {
                        return RECORD_OFF;
                    }
                    else if(command_tokens[1] == "on" && command_tokens.size() == 7)
                    {
                        return CONFIGURE_NEXT_RECORDING;
                    }
                }
            }
            return UNKNOWN;
        }


        void ConfigureWriters()
        {

            if(fExperimentName.size() == 0)
            {
                fExperimentName = "ExpX";
            }

            if(fSourceName.size() == 0 )
            {
                fSourceName = "SrcX";
            }

            if(fScanName.size() == 0)
            {
                fScanName = "ScnX";
            }

            fDumper->SetExperimentName(fExperimentName);
            fDumper->SetSourceName(fSourceName);
            fDumper->SetScanName(fScanName);
            fDumper->InitializeOutputDirectory();

            fAveragedSpectrumWriter->SetExperimentName(fExperimentName);
            fAveragedSpectrumWriter->SetSourceName(fSourceName);
            fAveragedSpectrumWriter->SetScanName(fScanName);
            fAveragedSpectrumWriter->InitializeOutputDirectory();
        }


        int DetermineTimeStateWRTNow(uint64_t epoch_sec_then)
        {

            //if time < now - 1 second, return TIME_BEFORE
            //if (now-1) < time < now, return TIME_PENDING
            //if time > now, return TIME_AFTER

            std::time_t now = std::time(nullptr);
            uint64_t epoch_sec_now = (uint64_t) now;

            //std::cout<<"now, then = "<<epoch_sec_now<<", "<<epoch_sec_then<<std::endl;

            if( epoch_sec_then < epoch_sec_now - 1 )
            {
                return TIME_BEFORE;
            }

            if( (epoch_sec_now - 1 <= epoch_sec_then) && (epoch_sec_then < epoch_sec_now) )
            {
                return TIME_PENDING;
            }

            if( epoch_sec_now <= epoch_sec_then )
            {
                return TIME_AFTER;
            }
            return TIME_ERROR;

        }


        //date must be in YYYYDDDHHMMSS format, UTC
        bool ConvertStringToTime(std::string date, uint64_t& epoch_sec)
        {
            // std::cout<<"trying to parse date string: "<<date<<" with length: "<<date.size()<<std::endl;
            //first split up the chunks of the date string
            if(date.size() == 13)
            {
                std::string syear = date.substr(0,4);
                std::string sdoy = date.substr(4,3);
                std::string shour = date.substr(7,2);
                std::string smin = date.substr(9,2);
                std::string ssec = date.substr(11,2);

                int year = 0;
                int doy = 0;
                int hour = 0;
                int min = 0;
                int sec = 0;


                // std::cout<<"year, doy, hour, min, sec = "<<syear<<", "<<sdoy<<", "<<shour<<", "<<smin<<", "<<ssec<<std::endl;

                //conver to ints w/ sanity checks
                {
                    std::stringstream ss;
                    ss.str(std::string());
                    ss << syear;
                    ss >> year; if(year < 2000 || year > 2100 ){epoch_sec = 0; return false;}
                    // std::cout<<"year ok"<<std::endl;
                }

                {
                    std::stringstream ss;
                    ss.str(std::string());
                    ss << sdoy;
                    ss >> doy;
                    // std::cout<<"doy = "<<doy<<std::endl;
                    if(doy < 1 || doy > 366 ){epoch_sec = 0; return false;}
                    // std::cout<<"day ok"<<std::endl;
                }

                {
                    std::stringstream ss;
                    ss.str(std::string());
                    ss << shour;
                    ss >> hour;  if(hour < 0 || hour > 23 ){epoch_sec = 0; return false;}
                    // std::cout<<"hour ok"<<std::endl;
                }

                {
                    std::stringstream ss;
                    ss.str(std::string());
                    ss << smin;
                    ss >> min;  if(min < 0 || min > 59 ){epoch_sec = 0; return false;}
                    // std::cout<<"min ok"<<std::endl;
                }

                {
                    std::stringstream ss;
                    ss.str(std::string());
                    ss << ssec;
                    ss >> sec;  if( sec < 0 || sec > 59 ){epoch_sec = 0; return false;}
                }

                // std::cout<<"year, doy, hour, min, sec = "<<year<<", "<<doy<<", "<<hour<<", "<<min<<", "<<sec<<std::endl;
                //

                //now convert year, doy, hour, min, sec to epoch second
                struct tm tmdate;
                tmdate.tm_sec = sec;
                tmdate.tm_min = min;
                tmdate.tm_hour = hour;
                tmdate.tm_mon = 0;
                tmdate.tm_mday = doy;
                tmdate.tm_year = year - 1900;
                //tmdate.tm_yday = doy;
                tmdate.tm_isdst	= 0;
                std::time_t epsec = timegm(&tmdate);

                // std::cout<<"epsec = "<<epsec<<std::endl;

                epoch_sec = (uint64_t) epsec;
                //
                // std::cout<<"start time is epoch sec: "<<epoch_sec<<std::endl;
                return true;
            }
            //
            // std::cout<<"failure"<<std::endl;
            epoch_sec = 0;
            return false;
        }

        uint64_t ConvertStringToDuration( std::string duration )
        {
            std::stringstream ss;
            ss << duration;
            int sec;
            ss >> sec;
            if(sec < 12*3600 && sec >= 0) //don't allow durations larger than 12 hours or less than 0
            {
                return (uint64_t) sec;
            }
            else
            {
                fStatusLogger->info("Scan durations longer than 12 hours not allowed, setting duration to 0.");
                return 0;
            }
        }


        virtual bool CheckRequest(const std::string& request_string) override
        {
            //std::cout<<"got: "<<request_string<<std::endl;
            std::vector< std::string > tokens = Tokenize(request_string);

            if(tokens.size() != 0)
            {
                int command_type = LookUpCommand(tokens);
                switch(command_type)
                {
                    case SHUTDOWN:
                        return true;
                    break;
                    case QUERY:
                        return true;
                    break;
                    case RECORD_ON:
                        return true;
                    break;
                    case SET_POWER_BINS:
                        return true;
                    break;
                    case RECORD_OFF:
                        return true;
                    break;
                    case CONFIGURE_NEXT_RECORDING:
                        return true;
                    break;
                    default:
                        return false;
                    break;
                };
            }
            return false;
        };


        virtual HStateStruct GetCurrentState() override
        {
            HStateStruct st;
            st.status_code = HStatusCode::unknown;
            st.status_message = "unknown";

            switch (fRecordingState)
            {
                case RECORDING_UNTIL_OFF:
                    st.status_code = HStatusCode::recording;
                    st.status_message = "recording";
                break;
                case RECORDING_UNTIL_TIME:
                    st.status_code = HStatusCode::recording;
                    st.status_message = "recording";
                break;
                case IDLE:
                    st.status_code = HStatusCode::idle;
                    st.status_message = "idle";
                break;
                case PENDING:
                    st.status_code = HStatusCode::pending;
                    st.status_message = "pending";
                break;
                default:
                st.status_code = HStatusCode::unknown;
                st.status_message = "unknown";
                break;
            }

            return st;
        }

        bool GetLockByPID()
        {
            bool other_daemon_running = false;
            std::string lock_dir( STR2(LOG_INSTALL_DIR) );
            //scan the list of all files in the directory
            //for ones with the ".lock" extension
            DIR* d;
            struct dirent* dir;
            d = opendir(lock_dir.c_str());
            if(d)
            {
                while( (dir = readdir(d)) != NULL)
                {
                    if(strstr(dir->d_name, ".lock") != NULL)
                    {
                        std::string lock_file_name(dir->d_name);
                        //split off the PID from the lock file name

                        std::stringstream ss;
                        int pid = -1;
                        size_t pos = lock_file_name.find('.');
                        if(pos != std::string::npos)
                        {
                            ss << lock_file_name.substr(0, pos);
                            ss >> pid;
                        }

                        //check if that process is running, and it is not us
                        if( kill(pid, 0) == 0 && pid != fPID)
                        {
                            other_daemon_running = true;
                        }
                    }
                }
            }

            if(!other_daemon_running)
            {
                //go ahead and create a lock file
                std::stringstream ss;
                ss << STR2(LOG_INSTALL_DIR);
                ss << "/";
                ss << fPID;
                ss << ".lock";

                fLockFileName = ss.str();

                std::fstream fs;
                fs.open( fLockFileName.c_str(), std::ios::out);
                fs.close();
                return true; //success
            }

            return false; //failure
        }

        //config data data
        bool fInitialized;
        volatile bool fStop;
        //this is for communication with hoseclient.py
        std::string fIP;
        std::string fPort; 
        int fEnableSpectrumWriteToFile; //control whether or not data is dumped to disk (may want to disable when only runnin as noise-power for pointing mode)
        int fEnableNoisePowerWriteToFile;

        //this is for UDP unicast of noise power data 
        std::string fUDPNoisePowerIP;
        std::string fUDPNoisePowerPort;
        int fUDPNoisePowerSkipInterval;
        int fNoisePowerBinLow;
        int fNoisePowerBinHigh;
        int fEnableNoisePowerUDPMessages;

        //configure spectrum monitoring UDP messages
        std::string fUDPSpectrumPort;
        std::string fUDPSpectrumIP;
        int fEnableSpectrumUDPMessages;

        size_t fNSpectrumAverages;
        size_t fFFTSize;
        size_t fDigitizerPoolSize;
        size_t fSpectrometerPoolSize;
        size_t fNDigitizerThreads;
        size_t fNSpectrometerThreads;
        int fWindowFlag;

        size_t fNADQ7SampleSkip;
        size_t fNSpectrumAveragesCPU;
        size_t fNDumpSkip;
        size_t fNSpectrumAveragerPoolSize;

        //state data
        int fRecordingState;
        std::string fExperimentName;
        std::string fSourceName;
        std::string fScanName;
        uint64_t fEndTime;
        uint64_t fStartTime;

        //objects
        HParameters fParameters;
        HTokenizer fTokenizer;
        HServer* fServer;
        XDigitizerType* fDigitizer;
        HCudaHostBufferAllocator< typename XDigitizerType::sample_type >* fCUDABufferAllocator;
        HBufferAllocatorSpectrometerDataCUDA< SPECTRUM_TYPE >* fSpectrometerBufferAllocator;
        SPECTROMETER_TYPE* fSpectrometer;
        HRawDataDumper< typename XDigitizerType::sample_type >* fDumper;
        HBufferPool< typename XDigitizerType::sample_type >* fDigitizerSourcePool;
        HBufferPool< SPECTRUM_TYPE >* fSpectrometerSinkPool;

        HBufferAllocatorNew< float >* fSpectrumAveragingBufferAllocator;
        HBufferPool< float >* fSpectrumAveragingBufferPool;
        AVERAGER_TYPE* fSpectrumAverager;
        HAveragedMultiThreadedSpectrumDataWriter* fAveragedSpectrumWriter;

        std::string fCannedStopCommand;

        //logger
        #ifdef HOSE_USE_SPDLOG
        std::string fSinkFileName;
        std::shared_ptr<spdlog::sinks::simple_file_sink_mt> fSink;
        std::shared_ptr<spdlog::logger> fStatusLogger;
        std::shared_ptr<spdlog::logger> fConfigLogger;
        #endif

        //PID management
        pid_t fPID;
        std::string fLockFileName;
};

}

#endif /* end of include guard: HSpectrometerManager */
