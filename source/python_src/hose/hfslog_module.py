import sys
from datetime import datetime, timedelta

class time_stamp(object):

    def __init__(self):
        self.year = 0
        self.day = 0
        self.hour = 0
        self.minute = 0
        self.seconds = 0
        self.microseconds = 0

    def get_formatted_utc(self):
        abstime = datetime(self.year,1,1) + timedelta(days=self.day-1, hours=self.hour, minutes=self.minute, seconds=self.seconds, microseconds = self.microseconds)
        return abstime.strftime('%Y-%m-%dT%H:%M:%S.%fZ')

class udc_status(object):

    def __init__(self):
        self.log_time = time_stamp() 
        self.parse_valid = False
        self.name = 'c'
        self.frequency_MHz = 0.0
        self.atten_h = 0.0
        self.atten_v = 0.0
        self.status_code = 0

    def as_dict(self):
        point_value = {
                "time": self.log_time.get_formatted_utc(),
                "measurement": "udc_lo_frequency",
                "fields": {
                    "frequency_MHz": self.frequency_MHz,
                    "atten_h": self.atten_h,
                    "atten_v": self.atten_v
                },
                "tags": {
                    "udc": self.name
                },
            }
        return point_value

class data_status(object):

    def __init__(self):
        self.log_time = time_stamp() 
        self.parse_valid = False
        self.status_code = 0

    def as_dict(self):
        point_value = {
                "time": self.log_time.get_formatted_utc(),
                "measurement": "data_valid",
                "fields": {
                    "status": self.status_code
                },
            }
        return point_value

class source_status(object):

    def __init__(self):
        self.log_time = time_stamp() 
        self.parse_valid = False
        self.source_name = ""
        self.ra = 0
        self.dec = 0
        self.epoch = 0
        self.cable_wrap = ""

    def as_dict(self):
        point_value = {
                "time": self.log_time.get_formatted_utc(),
                "measurement": "source",
                "fields": {
                    "ra": self.ra,
                    "dec": self.dec,
                    "epoch": self.epoch,
                    "cable_wrap": self.cable_wrap
                },
                "tags": {
                    "source_name": self.source_name
                },
            }
        return point_value


class hfslog_stripper(object):

    def __init__(self):
        self.date_string_length = 20
        self.data_flag_set = list()
        #set up dictionary of key words we look for in the log:
        #currently we only care about UDC-C, the data validity on/off and source information
        self.data_flag_dict = {"#popen#udccc/updown" : 1,  ":data_valid": 2, ":source=": 3}
        self.data_flag_list = []
        for key, val in self.data_flag_dict.items():
            self.data_flag_list.append(key)
        
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
                #process UDC-C line
                self.process_udc_status( self.extract_udcc_line(line) )
                return True
            elif line_code == 2:
                self.process_data_status( self.extract_data_valid_line(line) )
                return True
            elif line_code == 3:
                self.process_source_status( self.extract_source_line(line) )
                return True
            else:
                return False

    def get_data_points(self):
        return self.data_points

    #strip the date code from a log line, return time stamp object
    def strip_date(self, line):
        date_string = line[0:self.date_string_length]
        #date string should be formatted like 2018.060.15:05:26.36
        syear = date_string[0:4]
        sday = date_string[5:8]
        shour = date_string[9:11]
        sminute = date_string[12:14]
        ssec = date_string[15:17]
        shundredths = date_string[18:20]
        ts = time_stamp()
        ts.year = int(syear)
        ts.day = int(sday)
        ts.hour = int(shour)
        ts.minute = int(sminute)
        ts.seconds = int(ssec)
        ts.microseconds = int(shundredths)*10000
        return ts

    #returns 0 if this line is unrecognized and should be culled, 
    #otherwise numeric code indicates the line type 
    def quick_cull(self, line):
        ret_val = 0
        for key,val in self.data_flag_dict.items():
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

    def process_udc_status(self, udc):
        if udc.parse_valid is True:
            self.data_points = [ udc.as_dict() ]
            print("process:", udc.as_dict() )
        else:
            print("UDC parse error")
            self.data_points = []

    def process_data_status(self, data):
        if data.parse_valid is True:
            self.data_points = [ data.as_dict() ]
            print("process:", data.as_dict() )
        else:
            print("Data validity parse error")
            self.data_points = []

    def process_source_status(self, source):
        if source.parse_valid is True:
            self.data_points = [ source.as_dict() ]
            print("process:", source.as_dict() )
        else:
            print("Source parse error")
            self.data_points = []
