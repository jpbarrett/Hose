import sys
import zmq
 
class hose_client(object):

    def __init__(self):
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REQ)
        self.socket.connect("tcp://127.0.0.1:12345")
    
    def SendRecieveMessage(self, msg_string):
        self.socket.send( msg_string )
        response = self.socket.recv()
        print "Response from server: ", response
