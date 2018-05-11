import sys
import zmq
 
class hose_client(object):

    def __init__(self):
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REQ)
        self.socket.setsockopt(zmq.LINGER, 0)
        self.socket.connect("tcp://127.0.0.1:12345")
    
    def SendRecieveMessage(self, msg_string):
        self.socket.send( msg_string )
        poller = zmq.Poller()
        poller.register(self.socket, zmq.POLLIN)
        if(poller.poll(5000)): #5 second timeout
            response = self.socket.recv()
        else:
            raise IOError("Timeout processing auth request")
        print "Response from server: ", response

    def Shutdown(self):
        self.socket.close()
        self.context.term()
