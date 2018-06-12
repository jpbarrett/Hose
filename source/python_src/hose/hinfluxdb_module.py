from influxdb import InfluxDBClient
from datetime import datetime, timedelta

class wf_influxdb(object):
    def __init__(self):
        self.dbhostname = "wfmark5-19"
        self.dbport = 8086
        self.dbuser = "GPUSpecOper"
        self.dbpwd = "kLx14f2p"
        self.dbname = "gpu_spec"
        self.client = InfluxDBClient(self.dbhostname, self.dbport, self.dbuser, self.dbpwd, self.dbname)

        #time arguments must be datetime objects
        #valid measurement names are:
        #digitizer_config
        #spectrometer_config
        #noise_diode_config
        #recording_status 
        #udc_status
        #source 
        #data_validity

    def get_measurement_from_time_range(self, measurement_name, start_time, end_time, time_buffer_sec=0):
        # query must be in the form:
        # SELECT * FROM data_validity WHERE time < '2018-03-01 18:26:08.400' AND time > '2018-03-01 18:16:00.500' 
        start_time_string = ( start_time - datetime.timedelta(seconds=time_buffer_sec) ).strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        end_time_string = ( end_time + datetime.timedelta(seconds=time_buffer_sec) ).strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        query = "SELECT * FROM " + measurement_name + " WHERE time < '" + end_time_string + "' AND time > '" + start_time_string + "' "
        result = self.client.query(query)
        print(measurement_name, " result: {0}".format( result.get_points() ) )
        return result


    def get_most_recent_measurement(self, measurement_name, current_time):
        # query must be in the form:
        #  SELECT * FROM <measurement_name> WHERE time < '2018-03-01 18:26:08.400' GROUP BY * ORDER BY DESC LIMIT 1
        time_string = current_time.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        query = "SELECT * FROM " + measurement_name + " WHERE time < '" + time_string + "' GROUP BY * ORDER BY DESC LIMIT 1"
        result = self.client.query(query)
        print(measurement_name, " result: {0}".format( result.get_points() ) )
        return result
