from influxdb import InfluxDBClient
from datetime import datetime, timedelta
import json
import os

class wf_influxdb(object):
    def __init__(self):
        self.dbhostname = os.getenv('INFLUXDB_HOSTNAME')
        self.dbport = os.getenv('INFLUXDB_PORT')
        self.dbuser = os.getenv('INFLUXDB_USER')
        self.dbpwd = os.getenv('INFLUXDB_PWD')
        self.dbname = os.getenv('INFLUXDB_DATABASE')
        if self.dbhostname != None and self.dbport != None and self.dbuser != None and self.dbpwd != None and self.dbname != None:
            self.client = InfluxDBClient(self.dbhostname, self.dbport, self.dbuser, self.dbpwd, self.dbname)
        else:
            self.client = None

    #time arguments must be datetime objects
    #valid measurement names are:
    #digitizer_config
    #spectrometer_config
    #noise_diode_config
    #recording_status
    #udc_status
    #source
    #data_validity

    def get_measurement_from_time_range(self, measurement_name, start_time, end_time, time_buffer_sec=0, as_dict=True):
        if self.client != None:
            # query must be in the form:
            # SELECT * FROM data_validity WHERE time < '2018-03-01 18:26:08.400' AND time > '2018-03-01 18:16:00.500'
            start_time_string = ( start_time - timedelta(seconds=time_buffer_sec) ).strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
            end_time_string = ( end_time + timedelta(seconds=time_buffer_sec) ).strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
            query = "SELECT * FROM " + measurement_name + " WHERE time < '" + end_time_string + "' AND time > '" + start_time_string + "' "
            result = self.client.query(query)
            points = list( result.get_points() )

            #now we want to convert the points data to dict objects with the measurement_name attached
            if as_dict is True:
                mod_points = []
                for x in points:
                    x_dict ={"time": x.pop("time", None),  "measurement": measurement_name, "fields": x }
                    mod_points.append(x_dict)
                return mod_points
            else:
                return points
        else:
            #return nothing
            return []


    def get_most_recent_measurement(self, measurement_name, current_time, as_dict=True):
        if self.client != None:
            # query must be in the form:
            #  SELECT * FROM <measurement_name> WHERE time < '2018-03-01 18:26:08.400' GROUP BY * ORDER BY DESC LIMIT 1
            time_string = current_time.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
            query = "SELECT * FROM " + measurement_name + " WHERE time < '" + time_string + "' GROUP BY * ORDER BY DESC LIMIT 1"
            result = self.client.query(query)
            points = list( result.get_points() )

            #now we want to convert the points data to dict objects with the measurement_name attached
            if as_dict is True:
                mod_points = []
                for x in points:
                    x_dict ={"time": x.pop("time", None),  "measurement": measurement_name, "fields": x }
                    mod_points.append(x_dict)
                return mod_points
            else:
                return points
        else:
            #return nothing
            return []


def dump_dict_list_to_json_file(obj_list, filename):
    with open(filename, 'w') as outfile:
        outfile.write("[ \n") #open root element
        n_obj = len(obj_list)
        for i in range(0, n_obj):
            x = obj_list[i]
            if (i == n_obj-1):
                outfile.write(x + "\n") # json.dumps(x, indent=4, sort_keys=True) + "\n")
            else:
                outfile.write(x + ", \n") #json.dumps(x, indent=4, sort_keys=True) + ", \n")
        outfile.write("] \n") #close root element
