import sys
from datetime import datetime, timedelta

class spec_log_time_stamp(object):

    def __init__(self):
        self.date_string_length = 25
        self.year = 0
        self.month = 0
        self.mday = 0
        self.hour = 0
        self.minute = 0
        self.seconds = 0
        self.microseconds = 0

    #strip the date code from a log line, return time stamp object
    def initialize_from_line(self, line):
        if len(line) >= 25:
            date_string = line[0:self.date_string_length]
            #date string should be formatted like [2018-06-11 15:43:41.002]
            syear = date_string[1:5]
            smonth = date_string[6:8]
            sday = date_string[9:11]
            shour = date_string[12:14]
            sminute = date_string[15:17]
            ssec = date_string[18:20]
            sthousandths = date_string[21:24]
            self.year = int(syear)
            self.month = int(smonth)
            self.mday = int(sday)
            self.hour = int(shour)
            self.minute = int(sminute)
            self.seconds = int(ssec)
            self.microseconds = int(sthousandths)*1000
            print "initializing date from ", date_string

    def get_formatted_utc(self):
        abstime = datetime(year=self.year, month=self.month, day=self.day, hour=self.hour, minute=self.minute, second=self.seconds, microsecond = self.microseconds)
        return abstime.strftime('%Y-%m-%dT%H:%M:%S.%fZ')


class spectrometer_log_line(object):

    def __init__(self):
        self.log_time = spec_log_time_stamp() #every log line must have a time stamp
        self.parse_valid = False
        self.line_key = "none" #line must contain this to trigger parsing
        self.name = "none" #name given to this data object (measurement) in the database
        self.data_fields = {"none": 0} #dictionary of data fields

    def initialize_from_line(self, line):
        self.parse_valid = True
        if( self.line_key in line):
            args = line.split(";")
            print "args = ", str(args)
            if len(args) == len(self.data_fields) + 1:
                self.log_time.initialize_from_line(args[0])
                for i in range(1, len(args)):
                    sub_args = args[i].split("=")
                    print "sub args = ", str(sub_args)
                    if len(sub_args) == 2:
                        if sub_args[0] in self.data_fields:
                            self.data_fields[ sub_args[0] ] = sub_args[1]
                        else:
                            print "sub args 0 :", sub_args[0], "not in data fields"
                            self.parse_valid = False
                            break
                    else:
                        print "bad num sub args: ", str(len(sub_args))
                        self.parse_valid = False
            else:
                print "wrong number of args"
                self.parse_valid = False
        else:
            print "line key missing"
            self.parse_valid = False
        return self.parse_valid

    def as_dict(self):
        point_value = {
                "time": self.log_time.get_formatted_utc(),
                "measurement": self.name,
                "fields": self.data_fields
            }
        return point_value

class config_digitizer(spectrometer_log_line):

    def __init__(self):
        super(config_digitizer, self).__init__()
        self.line_key = "digitizer_config"
        self.name = "digitizer_config"
        self.data_fields = { "n_digitizer_threads": 0, "sideband": '-', "polarization": '-', "sampling_frequency_Hz": 0}

class config_spectrometer(spectrometer_log_line):

    def __init__(self):
        super(config_spectrometer, self).__init__()
        self.line_key = "spectrometer_config"
        self.name = "spectrometer_config"
        self.data_fields = { "n_spectrometer_threads": 0, "n_averages": 0, "fft_size": 0, "n_writer_threads": 0}

class config_noise_diode(spectrometer_log_line):

    def __init__(self):
        super(config_noise_diode, self).__init__()
        self.line_key = "noise_diode_config"
        self.name = "noise_diode_config"
        self.data_fields = {"switching_frequency_Hz": 0, "blanking_period": 0}

class recording_status(spectrometer_log_line):

    def __init__(self):
        super(recording_status, self).__init__()
        self.line_key = "recording_status"
        self.name = "recording_status"
        self.data_fields = {"recording": "-", "experiment_name": "-", "source_name": "-", "scan_name": "-"}

class hstatuslog_stripper(object):

    def __init__(self):
        self.date_string_length = 25
        self.line_type_tuple = ( config_digitizer(), config_spectrometer(), config_noise_diode(), recording_status() )
    
        #temporary storage of a parse data item (list of dicts), only valid if process_line returns true
        self.data_points = []

    def process_line(self,line):
        #quick check that the line is longer than the required date string format length
        success = False
        self.data_points = []
        if len(line) < self.date_string_length :
            success = False
        else:
            for line_type in self.line_type_tuple:
                if line_type.line_key in line:
                    print "found key: ", line_type.line_key 
                    success = line_type.initialize_from_line(line)
                    if success is True:
                        self.data_points = [ line_type.as_dict() ]
                        break
        return success

    def get_data_points(self):
        return self.data_points
