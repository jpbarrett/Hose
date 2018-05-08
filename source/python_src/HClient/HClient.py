import sys
import zmq
 
class HClient(object):

    def __init__(self):
        self.context = zmq.Context()
        self.socket = context.socket(zmq.REQ)
        self.socket.connect("tcp://localhost:12345")
    
    def SendRecieveMessage( msg_string ):
        self.socket.send( msg_string )
        response = socket.recv()
        print "Response from server: ", response
