import sys
from datetime import datetime, timedelta

class time_stamp(object):

    def __init__(self):
        self.date_string_length = 20
        self.valid = False
        self.year = 0
        self.day = 0
        self.hour = 0
        self.minute = 0
        self.seconds = 0
        self.microseconds = 0

    #strip the date code from a log line, return time stamp object
    def initialize_from_line(self, line):
        self.valid = False
        if(len(line) >= self.date_string_length ):
            self.valid = True
            date_string = line[0:self.date_string_length]
            #date string should be formatted like 2018.060.15:05:26.36
            syear = date_string[0:4]
            sday = date_string[5:8]
            shour = date_string[9:11]
            sminute = date_string[12:14]
            ssec = date_string[15:17]
            shundredths = date_string[18:20]
            if syear.isdigit() and sday.isdigit() and shour.isdigit() and sminute.isdigit() and ssec.isdigit() and shundredths.isdigit():
                self.year = int(syear)
                self.day = int(sday)
                self.hour = int(shour)
                self.minute = int(sminute)
                self.seconds = int(ssec)
                self.microseconds = int(shundredths)*10000
                if self.year > 3000:
                    self.valid = False
                if self.day < 1 or self.day > 366:
                    self.valid = False
                if self.hour > 24:
                    self.valid = False
                if self.minute > 59:
                    self.valid = False
                if self.seconds > 59:
                    self.valid = False
                if self.microseconds > 1000000:
                    self.valid = False
            else:
                self.valid = False
        return self.valid

    def initialize_from_values(self, year, day, hour, minute, seconds):
        self.valid = True
        self.year = year
        self.day = day
        self.hour = hour
        self.minute = minute
        self.seconds = int(seconds)
        self.microseconds = round(seconds - self.seconds,6)*1000000
        if self.year > 3000:
            self.valid = False
        if self.day < 1 or self.day > 366:
            self.valid = False
        if self.hour > 24:
            self.valid = False
        if self.minute > 59:
            self.valid = False
        if self.seconds > 59:
            self.valid = False
        if self.microseconds > 1000000:
            self.valid = False
        return self.valid


    def get_formatted_utc(self):
        abstime = datetime(self.year,1,1) + timedelta(days=self.day-1, hours=self.hour, minutes=self.minute, seconds=self.seconds, microseconds = self.microseconds)
        return abstime.strftime('%Y-%m-%dT%H:%M:%S.%fZ')


class fs_log_line(object):

    def __init__(self):
        self.log_time = time_stamp() #every log line must have a time stamp
        self.parse_valid = False
        self.line_key = "none" #line must contain this to trigger parsing
        self.name = "none" #name given to this data object (measurement) in the database
        self.data_fields = dict() #dictionary of data fields
        self.token_map = dict() #map token indices on to data fields
        self.primary_delim = ""
        self.secondary_delim = ""

    def initialize_from_line(self, line):
        self.parse_valid = True
        self.log_time = time_stamp()
        if( self.line_key in line):
            success = self.log_time.initialize_from_line(line)
            if success is True:
                primary_split = line.split(self.primary_delim)
                if len(primary_split) == 2:
                    payload = primary_split[1].strip()
                    tokens = payload.split(self.secondary_delim)
                    for key,val in self.token_map.items():
                        if key < len(tokens):
                            self.data_fields[val] = tokens[key]
                else:
                    self.parse_valid = False
                    return
            else:
                self.parse_valid = False
                return
            self.init_hook()
        else:
            self.parse_valid = False
        return self.parse_valid

    def init_hook(self):
        #provide interface for sub-class to modify data_fields, but do nothing by default
        return

    def as_dict(self):
        point_value = {
                "time": self.log_time.get_formatted_utc(),
                "measurement": self.name,
                "fields": self.data_fields
            }
        return point_value


#currently we only care about UDC-C
#we expect the line to have the following format: 2018.061.13:12:04.39#popen#udccc/updown 7083.1 20 20 0 0 status 0
class udc_status(fs_log_line):
    def __init__(self):
        super(udc_status, self).__init__()
        self.line_key = "192.52.63.25/updown"
        self.name = "udc_status"
        self.data_fields = {"frequency_MHz": 0, "attenuation_h": 0, "attenuation_v": 0, "udc": "e"}
        self.token_map = {0: "frequency_MHz", 1: "attenuation_h", 2: "attenuation_v"}
        self.primary_delim = "192.52.63.25/updown"
        self.secondary_delim = " "

    def init_hook(self):
        self.data_fields["frequency_MHz"] = float(self.data_fields["frequency_MHz"])
        self.data_fields["attenuation_h"] = float(self.data_fields["attenuation_h"])
        self.data_fields["attenuation_v"] = float(self.data_fields["attenuation_v"])

#2018.166.18:44:41.29#flagr#flagr/antenna,acquired
#2018.166.18:24:45.00#flagr#flagr/antenna,new-source
#2018.166.18:45:29.56#flagr#flagr/antenna,off-source
#2018.166.18:45:46.57#flagr#flagr/antenna,re-acquired
class antenna_target_status(fs_log_line):
    def __init__(self):
        super(antenna_target_status, self).__init__()
        self.line_key = "flagr/antenna"
        self.name = "antenna_target_status"
        self.data_fields = {"status": "", "acquired": ""}
        self.token_map = {1: "status"}
        self.primary_delim = "flagr/antenna"
        self.secondary_delim = ","

    def init_hook(self):
        if "acquired" in self.data_fields["status"]:
            self.data_fields["acquired"] = "yes"
        else:
            self.data_fields["acquired"] = "no"

#2018.166.18:24:44.00&casa/source=casa,232324.8,+584859.,2000.
class source_status(fs_log_line):
    def __init__(self):
        super(source_status, self).__init__()
        self.line_key = "source="
        self.name = "source_status"
        self.data_fields = {"source": "", "ra": "", "dec": "", "epoch" : "" }
        self.token_map = {0 : "source", 1: "ra", 2: "dec", 3: "epoch"}
        self.primary_delim = "source="
        self.secondary_delim = ","

#we expect the line to have the following format: 2018.061.13:11:54.00:data_valid=on
#or alternatively: 2018.061.13:12:24.00:data_valid=off
class data_status(fs_log_line):
    def __init__(self):
        super(data_status, self).__init__()
        self.line_key = "data_valid="
        self.name = "data_validity"
        self.data_fields = {"status": "off"}
        self.token_map = {1 : "status"}
        self.primary_delim = "data_valid"
        self.secondary_delim = "="

class hfslog_stripper(object):

    def __init__(self):
        self.date_string_length = 20
        self.line_type_tuple = ( udc_status(), antenna_target_status(), source_status(), data_status() )
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
                    success = line_type.initialize_from_line(line)
                    if success is True:
                        self.data_points = [ line_type.as_dict() ]
                        break
                    else:
                        print("Could not parse line: ", line)
                        break
        return success

    def get_data_points(self):
        return self.data_points


#line format looks like:
#Year  DayofYear  Hour  Min  Sec  Elapsed_Sec  Az  El
#2018  170  09  50  22.806036  18.004702  180.000000  44.998627'
class antenna_position(object):
    def __init__(self):
        self.log_time = time_stamp() #every log line must have a time stamp
        self.name = "antenna_position"
        self.data_fields = {"az": 0, "el": 0}

    def initialize_from_line(self, line):
        self.parse_valid = True
        self.log_time = time_stamp()
        if '#' in line:
            self.parse_valid = False
        else:
            args = (line.strip()).split(' ')
            if len(args) != 8:
                self.parse_valid = False
            else:
                year = int(args[0])
                day = int(args[1])
                hour = int(args[2])
                minute = int(args[3])
                seconds = float(args[4])
                self.data_fields["az"] = float(args[6])
                self.data_fields["el"] = float(args[7])
                self.parse_valid = self.log_time.initialize_from_values(year, day, hour, minute, seconds)
        return self.parse_valid

    def as_dict(self):
        point_value = {
                "time": self.log_time.get_formatted_utc(),
                "measurement": self.name,
                "fields": self.data_fields
            }
        return point_value


class encrec_log_stripper(object):

    def __init__(self):
        self.line_type_tuple = ( udc_status(), antenna_target_status(), source_status() )
        #temporary storage of a parse data item (list of dicts), only valid if process_line returns true
        self.data_points = []

    def process_line(self,line):
        #quick check that the line is longer than the required date string format length
        success = False
        self.data_points = []
        antpos = antenna_position()
        success = antpos.initialize_from_line(line)
        if success is True:
            self.data_points = [ antpos.as_dict() ]
        else:
            print("Could not parse line: ", line)
        return success

    def get_data_points(self):
        return self.data_points
