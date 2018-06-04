import sys
import zmq
from cmd import Cmd
from datetime import datetime
 
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
        print "Response from server: ", response

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
        print "Response from server: ", response
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

    def do_record(self, args):
        """Set up recording state of the spectrometer."""
        if len(args) != 0:
            ret_code = self.parse_record_command(args) #by default this strips the string 'record'
            if(ret_code):
                print( "Error: command could not be parsed." )

    def do_quit(self, args):
        """Quits the client."""
        print "Quitting."
        self.interface.Shutdown()
        raise SystemExit

    def parse_record_command(self, args):
        if( len(args) == 1 and args[0] == "?" ):
            cmd_string = "record?" 
            self.interface.SendRecieveMessage(cmd_string)
            return 0
        elif( len(args) >=3 and "=on" in args ):
            #this is a record=on command, determien the remaining arguments, if we
            #are not given any, we use the default experiment/source names, and construct
            #a default scan name based on the start time (now)
            tnow = datetime.utcnow()
            day_of_year = str(tnow.timetuple().tm_yday).zfill(3)
            hour = str(tnow.hour).zfill(2)
            minute = str(tnow.minute).zfill(2)
            sec = str(tnow.second).zfill(2)
            scan_name = day_of_year + "-" + hour + minute + sec
            exp_name = self.default_experiment_name
            src_name = self.default_source_name
            cmd_string = "record=on" 
            #split on ':' tokens, and construct well formatted command
            arg_list = args.split(':')
            if( len(arg_list) == 1 and "=on" in arg_list[0]):
                cmd_string += ":" + exp_name + ":" + src_name + ":" + scan_name
                self.interface.SendRecieveMessage(cmd_string)
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
                    if( len(arg_list) == 6): # and len(arg_list[4]) == 13 and arg_list[4].isdigit() and arg_list[5].isdigit() ):]
                        start_time = arg_list[4]
                        duration = arg_list[5]
                        st_year = int(start_time[0:4])
                        st_day = int(start_time[4:7])
                        st_hour = int(start_time[7:9])
                        st_min = int(start_time[9:11])
                        st_sec = int(start_time[11:13])
                        if self.check_time_range(st_year, st_day, st_hour, st_min, st_sec ):
                            cmd_string += ":" + start_time + ":" + duration
                    self.interface.SendRecieveMessage(cmd_string)
                    return 0
                else:
                    return 1 #error parsing record command
        elif( len(args) ==4 and "=off" in args ):
            #this is a record=off command
            self.interface.SendRecieveMessage("record=off")
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