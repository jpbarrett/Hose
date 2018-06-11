import sys
from datetime import datetime, timedelta

class spec_log_time_stamp(object):

    def __init__(self):
        self.year = 0
        self.month = 0
        self.mday = 0
        self.hour = 0
        self.minute = 0
        self.seconds = 0
        self.microseconds = 0

    def get_formatted_utc(self):
        abstime = datetime(self.year,1,1) + timedelta(days=self.day-1, hours=self.hour, minutes=self.minute, seconds=self.seconds, microseconds = self.microseconds)
        return abstime.strftime('%Y-%m-%dT%H:%M:%S.%fZ')


class spec_log_line(object):

    def __init__(self):
        self.log_time = time_stamp() 
        self.parse_valid = False

    def as_dict(self):
        point_value = {
                "time": self.log_time.get_formatted_utc(),
                },
            }
        return point_value

class config_digitizer(object):

    def __init__(self):
        super(config_digitizer, self).__init__()
        self.n_digitizer_threads = 0
        self.sideband = ''
        self.polarization = ''
        self.sampling_frequency_Hz = 0

    def as_dict(self):
        point_value = {
                "time": self.log_time.get_formatted_utc(),
                "measurement": "digitizer_config",
                "fields": {
                    "n_digitizer_threads": self.n_digitizer_threads,
                    "sideband": self.sideband,
                    "polarization": self.polarization,
                    "sampling_frequency_Hz":, self.sampling_frequency_Hz
                },
            }
        return point_value

class config_spectrometer(object):

    def __init__(self):
        super(config_spectrometer, self).__init__()
        self.n_spectrometer_threads = 0
        self.n_averages = 0
        self.fft_size = 0
        self.n_writer_threads = 0

    def as_dict(self):
        point_value = {
                "time": self.log_time.get_formatted_utc(),
                "measurement": "spectrometer_config",
                "fields": {
                    "n_spectrometer_threads": self.n_spectrometer_threads,
                    "n_averages": self.n_averages,
                    "fft_size": self.fft_size,
                    "n_writer_threads": self.n_writer_threads
                },
            }
        return point_value

class config_noise_diode(object):

    def __init__(self):
        super(config_noise_diode, self).__init__()
        self.switching_frequency_Hz = 0.0
        self.blanking_period = 0.0

    def as_dict(self):
        point_value = {
                "time": self.log_time.get_formatted_utc(),
                "measurement": "noise_diode_config",
                "fields": {
                    "switching_frequency_Hz": self.n_spectrometer_threads,
                    "blanking_period": self.n_averages,
                },
            }
        return point_value

[2018-06-11 15:40:11.431] [status] [info] $$$ New session, manager log initialized. $$$
[2018-06-11 15:40:11.431] [status] [info] Initializing.
[2018-06-11 15:40:14.244] [config] [info] n_digitizer_threads=2
[2018-06-11 15:40:14.244] [config] [info] sideband=U
[2018-06-11 15:40:14.244] [config] [info] polarization=X
[2018-06-11 15:40:14.244] [config] [info] n_averages=256
[2018-06-11 15:40:14.244] [config] [info] fft_size=131072
[2018-06-11 15:40:14.244] [config] [info] n_spectrometer_threads=3
[2018-06-11 15:40:14.244] [config] [info] sampling_frequency_Hz=4e+08
[2018-06-11 15:40:14.244] [config] [info] noise_diode_switching_frequency_Hz=80
[2018-06-11 15:40:14.244] [config] [info] noise_blanking_period=5e-08
[2018-06-11 15:40:14.244] [config] [info] n_writer_threads=1
[2018-06-11 15:40:14.246] [status] [info] Ready.
[2018-06-11 15:40:16.034] [status] [info] recording=on, experiment_name=test, source_name=dunno, scan_name=162-1540
[2018-06-11 15:41:07.002] [status] [info] recording=off
[2018-06-11 15:41:27.006] [status] [info] recording=on, experiment_name=test, source_name=dunno, scan_name=162-1541
[2018-06-11 15:42:18.006] [status] [info] recording=off
[2018-06-11 15:42:44.006] [status] [info] recording=on, experiment_name=test, source_name=dunno, scan_name=162-1542
[2018-06-11 15:43:41.002] [status] [info] recording=off


class hstatuslog_stripper(object):

    def __init__(self):
        self.date_string_length = 25
        self.status_flag_list = list()
        self.config_flag_list = list()
        #set up dictionary of key words we look for in the log:
        #currently we only care about UDC-C, the data validity on/off and source information
        self.line_type_dict = {"[config]" : 1,  "[status]": 2}
        self.status_flag_dict = {"recording=on" : 1,  "recording=off": 2}
        self.config_flag_dict = {"n_digitizer_threads" : 1, "sideband": 2,  
                                "polarization": 3,  "n_averages": 4,  "fft_size": 5,
                                "n_spectrometer_threads": 6, "sampling_frequency_Hz": 7, 
                                "noise_diode_switching_frequency_Hz":8, 
                                "noise_blanking_period":9,  "n_writer_threads":10 }

        for key, val in self.status_flag_dict.items():
            self.status_flag_list.append(key)
        
        for key, val in self.config_flag_dict.items():
            self.config_flag_list.append(key)

        #temporary storage of a parse data item (list of dicts), only valid if process_line returns true
        self.data_points = []

    def process_line(self,line):
        #quick check that the line is longer
        #than the required date string format length
        self.data_points = []
        if len(line) < self.date_string_length :
            return False
        else:
            #determine if we have a line that contains information we are interested in
            line_code = self.quick_cull(line)
            if line_code == 1: 
                #process a 'config line'
                self.process_config( line.strip() )
                return True
            elif line_code == 2:
                #process a status line
                self.process_status( line.strip() )
            else:
                return False

    def get_data_points(self):
        return self.data_points

    #strip the date code from a log line, return time stamp object
    def strip_date(self, line):
        date_string = line[0:self.date_string_length]
        #date string should be formatted like [2018-06-11 15:43:41.002]
        syear = date_string[1:5]
        smonth = date_string[6:8]
        sday = date_string[9:11]
        shour = date_string[12:14]
        sminute = date_string[15:17]
        ssec = date_string[18:20]
        sthousandths = date_string[21:24]
        ts = speclog_time_stamp()
        ts.year = int(syear)
        ts.month = int(smonth)
        ts.mday = int(sday)
        ts.hour = int(shour)
        ts.minute = int(sminute)
        ts.seconds = int(ssec)
        ts.microseconds = int(sthousandths)*1000
        return ts

    def line_type(self, line):
        ret_val = 0
        for key,val in self.line_type_dict.items():
            if key in line:
                ret_val = val
                break
        return ret_val

    def extract_udcc_line(self, line):
        #we expect the line to have the following format: 2018.061.13:12:04.39#popen#udccc/updown 7083.1 20 20 0 0 status 0
        stat = udc_status()
        tokens = line.split()
        if len(tokens) == 8:
            if "#popen#udccc/updown" in tokens[0]:
                stat.parse_valid = True
                stat.log_time = self.strip_date(line)
                stat.frequency_MHz = float(tokens[1])
                stat.atten_h = float(tokens[2])
                stat.atten_v = float(tokens[3])
                stat.status_code = int(tokens[7])
        return stat

    def extract_data_valid_line(self, line):
        #we expect the line to have the following format: 2018.061.13:11:54.00:data_valid=on 
        #or alternatively: 2018.061.13:12:24.00:data_valid=off
        stat = data_status()
        tokens = line.split('=')
        if len(tokens) == 2:
            if ":data_valid" in tokens[0]:
                stat.parse_valid = True
                stat.log_time = self.strip_date(line)
                stat.status_code = tokens[1]
        return stat

    def extract_source_line(self, line):
        #we expect the line to have the following format: 2018.061.13:11:16.00:source=1741-038,174120.64,-034848.9,1950.0,neutral
        stat = source_status()
        tokens = line.split('=')
        if len(tokens) == 2:
            if ":source" in tokens[0]:
                tokens2 = tokens[1].split(',')
                if len(tokens2) == 5:
                    stat.parse_valid = True
                    stat.log_time = self.strip_date(line)
                    stat.source_name = tokens2[0]
                    stat.ra = tokens2[1]
                    stat.dec = tokens2[2]
                    stat.epoch = tokens2[3]
                    stat.cable_wrap = tokens2[4]
        return stat

    def process_line_result(self, result):
