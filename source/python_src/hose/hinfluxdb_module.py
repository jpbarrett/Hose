from influxdb import InfluxDBClient

class wf_influxdb(object):
    def __init__(self):
        self.dbhostname = "wfmark5-19"
        self.dbport = 8086
        self.dbuser = "GPUSpecOper"
        self.dbpwd = "kLx14f2p"
        self.dbname = "gpu-spec"
        self.client = InfluxDBClient(self.dbhostname, self.dbport, self.dbuser, self.dbpwd, self.dbname)
