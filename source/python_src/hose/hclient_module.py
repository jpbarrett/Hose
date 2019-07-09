import sys
import zmq
import os
from cmd import Cmd
from datetime import datetime
import time
import threading
import subprocess

from .hinfluxdb_module import *
from .hspeclog_module import *
from .hfrontend_module import *

class hclient(object):

    def __init__(self):
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REQ)
        self.socket.setsockopt(zmq.LINGER, 0)
        self.socket.connect("tcp://127.0.0.1:12345")

    def SendRecieveMessage(self, msg_string):
        print("Sending message: ", msg_string)
        self.socket.send( msg_string )
        poller = zmq.Poller()
        poller.register(self.socket, zmq.POLLIN)
        if(poller.poll(5000)): #5 second timeout
            response = self.socket.recv()
        else:
            raise IOError("Timeout processing auth request")
        print "Server status: ", response

    def SendMessage(self, msg_string):
        print("Sending message: ", msg_string)
        self.socket.send( msg_string )
        return 0

    def RecieveMessage(self):
        poller = zmq.Poller()
        poller.register(self.socket, zmq.POLLIN)
        if(poller.poll(5000)): #5 second timeout
            response = self.socket.recv()
        else:
            raise IOError("Timeout processing auth request")
        print "Server status: ", response
        return '\0'

    def Shutdown(self):
        self.socket.close()
        self.context.term()

class hprompt(Cmd):

    def __init__(self):
        Cmd.__init__(self)
        self.interface = hclient()
        self.default_experiment_name="ExpX"
        self.default_source_name="SrcX"
        self.default_scan_name="ScnX"
        self.is_recording = False
        self.current_experiment_name = ""
        self.current_source_name = ""
        self.current_scan_name = ""
        self.log_install_dir = ""
        self.data_install_dir = ""
        self.bin_install_dir = ""
        self.dbclient = wf_influxdb()
        self.start_time_stamp = datetime.utcnow()
        self.end_time_stamp = datetime.utcnow()
        self.process_list = []

    def do_startlog2db(self, args):
        """Parse the spectrometer log and send results to database."""
        #launch log to db as subprocess
        if(len(self.process_list) == 0):
            log2db_exe = "capture-spectrometer-log.py"
            command =  os.path.join(self.bin_install_dir, log2db_exe)
            log2db = subprocess.Popen([command], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            self.process_list.append(log2db)

    def do_record(self, args):
        """Set up recording state of the spectrometer."""
        if len(args) != 0:
            ret_code = self.parse_record_command(args) #by default this strips the string 'record'
            if(ret_code):
                print( "Error: command could not be parsed." )

    def do_quit(self, args):
        """Quits the client."""
        print "Quitting."
        time.sleep(1)
        for x in self.process_list:
            x.kill()
        self.interface.Shutdown()
        raise SystemExit

    def do_shutdown(self, args):
        """Shutdown the recording daemon and quit the client."""
        print "Shutting down."
        cmd_string = "shutdown"
        self.interface.SendRecieveMessage(cmd_string)
        time.sleep(1)
        for x in self.process_list:
            x.kill()
        self.interface.Shutdown()
        raise SystemExit

    def parse_record_command(self, args):
        if( len(args) == 1 and args[0] == "?" ):
            cmd_string = "record?"
            self.interface.SendRecieveMessage(cmd_string)
            return 0
        elif( len(args) >=3 and "=on" in args ):
            #this is a record=on command, determine the remaining arguments, if we
            #are not given any, we use the default experiment/source names, and construct
            #a default scan name based on the start time (now)
            tnow = datetime.utcnow()
            self.start_time_stamp = tnow
            year = str(tnow.year)
            day_of_year = str(tnow.timetuple().tm_yday).zfill(3)
            hour = str(tnow.hour).zfill(2)
            minute = str(tnow.minute).zfill(2)
            sec = str(tnow.second).zfill(2)
            scan_name = day_of_year + "-" + hour + minute + sec
            exp_name = self.default_experiment_name
            src_name = self.default_source_name
            self.current_experiment_name = exp_name
            self.current_scan_name = scan_name
            self.current_source_name = src_name
            cmd_string = "record=on"
            #split on ':' tokens, and construct well formatted command
            arg_list = args.split(':')
            if( len(arg_list) == 1 and "=on" in arg_list[0]):
                cmd_string += ":" + exp_name + ":" + src_name + ":" + scan_name
                self.interface.SendRecieveMessage(cmd_string)
                self.is_recording = True
                return 0
            elif ( len(arg_list) == 2 and "=on" in arg_list[0]):
                cmd_string += ":" + exp_name + ":" + src_name + ":" + scan_name
                #start time is tnow, duration is second argument (in seconds)
                duration = arg_list[1]
                start_time = year + day_of_year + hour + minute + sec
                cmd_string += ":" + start_time + ":" + duration
                self.interface.SendRecieveMessage(cmd_string)
                self.is_recording = True
                return 0
            else:
                if( len(arg_list) >= 4 ):
                    if( len(arg_list[1]) >= 1 ):
                        exp_name = arg_list[1]
                    if( len(arg_list[2]) >= 1 ):
                        src_name = arg_list[2]
                    if( len(arg_list[3]) >= 1 ):
                        scan_name = arg_list[3]
                    cmd_string += ":" + exp_name + ":" + src_name + ":" + scan_name
                    self.current_experiment_name = exp_name
                    self.current_scan_name = scan_name
                    self.current_source_name = src_name
                    if( len(arg_list) == 6):
                        start_time = arg_list[4]
                        duration = arg_list[5]
                        if len(arg_list[4]) == 13 and arg_list[4].isdigit() and arg_list[5].isdigit():
                            st_year = int(start_time[0:4])
                            st_day = int(start_time[4:7])
                            st_hour = int(start_time[7:9])
                            st_min = int(start_time[9:11])
                            st_sec = int(start_time[11:13])
                            if self.check_time_range(st_year, st_day, st_hour, st_min, st_sec ):
                                cmd_string += ":" + start_time + ":" + duration
                                self.interface.SendRecieveMessage(cmd_string)
                                self.is_recording = True
                    return 0
                else:
                    return 1 #error parsing record command
        elif( len(args) ==4 and "=off" in args ):
            #this is a record=off command
            self.interface.SendRecieveMessage("record=off")
            time.sleep(1)
            self.end_time_stamp = datetime.utcnow()
            self.create_meta_data_file()
            self.is_recording = False
            return 0

        return 1 #error command not understood


    def check_time_range(self, year, day, hour, minute, second):
        if year >= 2018:
            if day >=1 and day <= 366:
                if hour >= 0 and hour <= 23:
                    if minute >= 0 and minute <= 59:
                        if second >= 0 and second <= 59:
                            return True
        return False

    def create_meta_data_file(self):
        meta_data_filename = "meta_data.json"
        meta_data_filepath = os.path.join(self.data_install_dir, self.current_experiment_name, self.current_scan_name, meta_data_filename)
        obj_list = []

        digi_config = self.dbclient.get_most_recent_measurement("digitizer_config", self.end_time_stamp)
        sampling_frequency_Hz = 1 #defaults to 1
        if len(digi_config) >= 1:
            obj_list.append( json.dumps(digi_config[-1], indent=4, sort_keys=True) )
            sampling_frequency_Hz = float(digi_config[-1]["fields"]["sampling_frequency_Hz"])

        spec_config = self.dbclient.get_most_recent_measurement("spectrometer_config", self.end_time_stamp)
        spectrometer_fftsize = 1 #defaults to 1
        if len(spec_config) >= 1:
            obj_list.append( json.dumps(spec_config[-1], indent=4, sort_keys=True) )
            spectrometer_fftsize = int(spec_config[-1]["fields"]["fft_size"])

        noise_config = self.dbclient.get_most_recent_measurement("noise_diode_config", self.end_time_stamp)
        if len(noise_config) >= 1:
            obj_list.append( json.dumps(noise_config[-1], indent=4, sort_keys=True) )

        udc_info = self.dbclient.get_most_recent_measurement("udc_status", self.end_time_stamp)
        udc_luff_freq = 1.0 #defaults to 1
        udc_time_stamp = ""
        if len(udc_info) >= 1:
            obj_list.append( json.dumps(udc_info[-1], indent=4, sort_keys=True) )
            udc_luff_freq = float(udc_info[-1]["fields"]["frequency_MHz"])
            udc_time_stamp = udc_info[-1]["time"]

        #now add the sky frequency information as well (this is hard-coded, not in the database)
        #for this calculation we disable the pre-digitizer filter (filter4), since the center band frequency is actually just above the cut-off
        wf_signal_chain = westford_signal_chain(udc_luff_freq, apply_last_filter=False)
        spectral_resolution_MHz = float(sampling_frequency_Hz/float(spectrometer_fftsize))/1e6
        n_spec_bins = spectrometer_fftsize/2 + 1
        center_bin = int(n_spec_bins/2)
        center_bin_if_freq = (float(center_bin) + 0.5)*spectral_resolution_MHz
        center_bin_sky_freq = wf_signal_chain.map_frequency_backward(center_bin_if_freq)
        bin_delta = 1 #a bin increment of (bin_delta) corresponds to an incrment in frequecy space of (frequency_delta)
        [if_low, if_high] = [0.0, bin_delta*spectral_resolution_MHz] #[lower edge of lowest bin, upper edge of lowest bin]
        [sky_low, sky_high] = wf_signal_chain.map_frequency_pair_backward(if_low, if_high)
        slope = ( (sky_high - sky_low)/(if_high - if_low) ) #slope had better be either +1 or -1
        frequency_delta = slope*spectral_resolution_MHz
        freq_map = frequency_map()
        freq_map.set_time(udc_time_stamp) #frequency_map gets the same time stamp as the UDC LO-set time
        freq_map.set_reference_bin_index(center_bin)
        freq_map.set_reference_bin_center_sky_frequency(center_bin_sky_freq)
        freq_map.set_bin_delta(bin_delta)
        freq_map.set_frequency_delta(frequency_delta)
        obj_list.append( json.dumps(freq_map.as_dict(), indent=4, sort_keys=True) )

        source_info = self.dbclient.get_most_recent_measurement("source_status", self.end_time_stamp)
        for x in source_info:
            obj_list.append( json.dumps(x, indent=4, sort_keys=True) )

        recording_info = self.dbclient.get_measurement_from_time_range("recording_status", self.start_time_stamp, self.end_time_stamp, 1)
        for x in recording_info:
            obj_list.append( json.dumps(x, indent=4, sort_keys=True) )

        temp_list = []
        most_recent_data_validity_info = self.dbclient.get_most_recent_measurement("data_validity", self.start_time_stamp)
        for x in most_recent_data_validity_info:
            temp_list.append( json.dumps(x, indent=4, sort_keys=True) )
        data_validity_info = self.dbclient.get_measurement_from_time_range("data_validity", self.start_time_stamp, self.end_time_stamp, 1)
        for x in data_validity_info:
            temp_list.append( json.dumps(x, indent=4, sort_keys=True) )
        fset = frozenset(temp_list)
        for z in fset:
            obj_list.append( z )

        temp_list = []
        most_recent_antenna_target_info = self.dbclient.get_most_recent_measurement("antenna_target_status", self.start_time_stamp)
        for x in most_recent_antenna_target_info:
            temp_list.append( json.dumps(x, indent=4, sort_keys=True) )
        antenna_target_info = self.dbclient.get_measurement_from_time_range("antenna_target_status", self.start_time_stamp, self.end_time_stamp, 1)
        for x in antenna_target_info:
            temp_list.append( json.dumps(x, indent=4, sort_keys=True) )
        fset = frozenset(temp_list)
        for z in fset:
            obj_list.append( z )

        antenna_position_info = self.dbclient.get_measurement_from_time_range("antenna_position", self.start_time_stamp, self.end_time_stamp, 1)
        for x in antenna_position_info:
            obj_list.append( json.dumps(x, indent=4, sort_keys=True) )
        dump_dict_list_to_json_file(obj_list, meta_data_filepath)
