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

#include "HTokenizer.hh"
#include "HPX14Digitizer.hh"
#include "HBufferPool.hh"
#include "HSpectrometerCUDA.hh"
#include "HCudaHostBufferAllocator.hh"
#include "HBufferAllocatorSpectrometerDataCUDA.hh"
#include "HSimpleMultiThreadedSpectrumDataWriter.hh"
#include "HApplicationBackend.hh"
#include "HServer.hh"


//TODO FIXME: replace these with real enums

//command types
#define UNKNOWN 0
#define RECORD_ON 1
#define RECORD_OFF 2
#define CONFIGURE_NEXT_RECORDING 3
#define QUERY 4
#define SHUTDOWN 5

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

template< class XDigitizerType = HPX14Digitizer >
class HSpectrometerManager: public HApplicationBackend
{
    public:

        HSpectrometerManager():
            fInitialized(false),
            fStop(false),
            fIP("127.0.0.1"),
            fPort("12345"),
            fNSpectrumAverages(256),
            fFFTSize(131072),
            fDigitizerPoolSize(32),
            fSpectrometerPoolSize(16),
            fNDigitizerThreads(2),
            fNSpectrometerThreads(3),
            fServer(nullptr),
            fDigitizer(nullptr),
            fCUDABufferAllocator(nullptr),
            fSpectrometerBufferAllocator(nullptr),
            fSpectrometer(nullptr),
            fWriter(nullptr),
            fDigitizerSourcePool(nullptr),
            fSpectrometerSinkPool(nullptr)
            #ifdef HOSE_USE_SPDLOG
            ,fSink(nullptr),
            fStatusLogger(nullptr),
            fConfigLogger(nullptr)
            #endif
        {
            fCannedStopCommand = "record=off";
        }

        virtual ~HSpectrometerManager()
        {
            delete fServer;
            delete fDigitizer;
            delete fSpectrometer;
            delete fWriter;
            delete fDigitizerSourcePool;
            delete fSpectrometerSinkPool;
            delete fCUDABufferAllocator;
            delete fSpectrometerBufferAllocator;
        }

        void SetServerIP(std::string ip){fIP = ip;};
        void SetServerPort(std::string port){fPort = port;};

        void SetNSpectrumAverages(size_t n_ave){fNSpectrumAverages = n_ave;};
        void SetFFTSize(size_t n_fft){fFFTSize = n_fft;};
        void SetDigitizerPoolSize(size_t n_chunks){fDigitizerPoolSize = n_chunks;};
        void SetSpectrometerPoolSize(size_t n_chunks){fSpectrometerPoolSize = n_chunks;};

        void SetNDigitizerThreads(size_t n){fNDigitizerThreads = n;};
        void SetNSpectrometerThreads(size_t n){fNSpectrometerThreads = n;};

        void Initialize()
        {
            //get our pid
            fPID = getpid();

            if(!fInitialized)
            {

                bool lock_success = GetLockByPID();
                if(lock_success)
                {

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
                        // spdlog::drop_all();
                        // auto daily_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logfile", 23, 59);
                        // // create synchronous  loggers
                        // auto net_logger = std::make_shared<spdlog::logger>("net", daily_sink);
                        // auto hw_logger  = std::make_shared<spdlog::logger>("hw",  daily_sink);
                        // auto db_logger  = std::make_shared<spdlog::logger>("db",  daily_sink);
                        //
                        // net_logger->set_level(spdlog::level::critical); // independent levels
                        // hw_logger->set_level(spdlog::level::debug);
                        //

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
                    bool digitizer_init_success = fDigitizer->Initialize();

                    if(digitizer_init_success)
                    {
                        //create source buffer pool
                        fCUDABufferAllocator = new HCudaHostBufferAllocator< typename XDigitizerType::sample_type >();
                        fDigitizerSourcePool = new HBufferPool< typename XDigitizerType::sample_type  >( fCUDABufferAllocator );
                        fDigitizerSourcePool->Allocate(fDigitizerPoolSize, fNSpectrumAverages*fFFTSize);
                        fDigitizer->SetBufferPool(fDigitizerSourcePool);

                        //TODO fill these in with real values!
                        // fDigitizer->SetSidebandFlag('U');
                        // fDigitizer->SetPolarizationFlag('X');

                        //create spectrometer data pool
                        fSpectrometerBufferAllocator = new HBufferAllocatorSpectrometerDataCUDA<spectrometer_data>();
                        fSpectrometerBufferAllocator->SetSampleArrayLength(fNSpectrumAverages*fFFTSize);
                        fSpectrometerBufferAllocator->SetSpectrumLength(fFFTSize);
                        fSpectrometerSinkPool = new HBufferPool< spectrometer_data >( fSpectrometerBufferAllocator );
                        fSpectrometerSinkPool->Allocate(fSpectrometerPoolSize, 1);

                        //create spectrometer
                        fSpectrometer = new HSpectrometerCUDA(fFFTSize, fNSpectrumAverages);
                        fSpectrometer->SetNThreads(fNSpectrometerThreads);
                        fSpectrometer->SetSourceBufferPool(fDigitizerSourcePool);
                        fSpectrometer->SetSinkBufferPool(fSpectrometerSinkPool);
                        fSpectrometer->SetSamplingFrequency( fDigitizer->GetSamplingFrequency() );

                        double noise_diode_switching_freq = 80.0;
                        double noise_diode_blanking_period = 20.0*(1.0/fDigitizer->GetSamplingFrequency());

                        fSpectrometer->SetSwitchingFrequency( noise_diode_switching_freq );
                        fSpectrometer->SetBlankingPeriod( noise_diode_blanking_period );

                        //file writing consumer to drain the spectrum data buffers
                        fWriter = new HSimpleMultiThreadedSpectrumDataWriter();
                        fWriter->SetBufferPool(fSpectrometerSinkPool);
                        fWriter->SetNThreads(1);

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

                        std::stringstream nstss;
                        nstss << "n_spectrometer_threads=";
                        nstss << fNSpectrometerThreads;

                        std::stringstream nwtss;
                        nwtss << "n_writer_threads=";
                        nwtss << 1;

                        std::string spectrometer_config = "spectrometer_config; " + navess.str() + "; " + fftss.str() + "; " + nstss.str() + "; " + nwtss.str();
                        fConfigLogger->info( spectrometer_config.c_str() );

                        //noise diode configuration
                        std::stringstream ndsfss;
                        ndsfss << "noise_diode_switching_frequency_Hz=";
                        ndsfss << noise_diode_switching_freq;

                        std::stringstream ndbpss;
                        ndbpss << "noise_blanking_period=";
                        ndbpss << noise_diode_blanking_period;

                        std::string noise_diode_config = "noise_diode_config; " + ndsfss.str() + "; " + ndbpss.str();
                        fConfigLogger->info( noise_diode_config.c_str() );

                        #endif
                        fInitialized = true;
                    }
                }
            }
        }

        void Run()
        {
            if(fInitialized)
            {
                //start the command server thread
                std::thread server_thread( &HServer::Run, fServer );
                fWriter->StartConsumption();

                fSpectrometer->StartConsumptionProduction();
                for(size_t i=0; i<fNSpectrometerThreads; i++)
                {
                    fSpectrometer->AssociateThreadWithSingleProcessor(i, i+1);
                };

                fDigitizer->StartProduction();
                for(size_t i=0; i<fNDigitizerThreads; i++)
                {
                    fDigitizer->AssociateThreadWithSingleProcessor(i, i+fNSpectrometerThreads+1);
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
                fWriter->StopConsumption();

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
                lfss << std::put_time(&current_utc_tm, "%d-%m-%YT%H-%M-%SZ");
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
                            std::stringstream ss;
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
                    case RECORD_ON:
                        if(fRecordingState == IDLE)
                        {
                            //configure and turn on recording
                            fExperimentName = tokens[2];
                            fSourceName = tokens[3];
                            fScanName = tokens[4];
                            ConfigureWriter();
                            //start recording immediately
                            fDigitizer->Acquire();
                            fRecordingState = RECORDING_UNTIL_OFF;
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
                            std::stringstream ss;
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
                            ConfigureWriter();
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
                                        std::stringstream ss;
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


        void ConfigureWriter()
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

            fWriter->SetExperimentName(fExperimentName);
            fWriter->SetSourceName(fSourceName);
            fWriter->SetScanName(fScanName);
            fWriter->InitializeOutputDirectory();

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
        std::string fIP;
        std::string fPort;
        size_t fNSpectrumAverages;
        size_t fFFTSize;
        size_t fDigitizerPoolSize;
        size_t fSpectrometerPoolSize;
        size_t fNDigitizerThreads;
        size_t fNSpectrometerThreads;

        //state data
        int fRecordingState;
        std::string fExperimentName;
        std::string fSourceName;
        std::string fScanName;
        uint64_t fEndTime;
        uint64_t fStartTime;

        //objects
        HTokenizer fTokenizer;
        HServer* fServer;
        XDigitizerType* fDigitizer;
        HCudaHostBufferAllocator< typename XDigitizerType::sample_type >* fCUDABufferAllocator;
        HBufferAllocatorSpectrometerDataCUDA< spectrometer_data >* fSpectrometerBufferAllocator;
        HSpectrometerCUDA* fSpectrometer;
        HSimpleMultiThreadedSpectrumDataWriter* fWriter;
        HBufferPool< typename XDigitizerType::sample_type >* fDigitizerSourcePool;
        HBufferPool< spectrometer_data >* fSpectrometerSinkPool;
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
