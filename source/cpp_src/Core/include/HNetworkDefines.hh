#ifndef HNetworkDefines_HH__
#define HNetworkDefines_HH__

//IP address of various devices
#define COMMAND_SERVER_IP_ADDRESS "127.0.0.1"
#define CURIE_HAY_IP_ADDRESS "192.52.61.185"
#define ODYSSEY_HAY_IP_ADDRESS "192.52.61.103"
#define ODYSSEY_LOCAL_IP_ADDRESS "192.52.63.48" //local connection as seen by gpu1080
#define GPUSPEC_HAY_IP_ADDRESS "192.52.61.149" 
#define GPUSPEC_LOCAL_IP_ADDRESS "192.52.63.49" //local connection as seen by odyssey
#define SMAX_HAY_IP_ADDRESS "192.52.61.46"

//port where commands to spectrometer daemon are issued 
#define COMMAND_SERVER_PORT "12345"

//ports where UDP messages are sent/received 
#define NOISE_POWER_UDP_PORT "8181"
#define SPECTRUM_UDP_PORT "8282"

//udp connection groups for noise and spectrum 
#define NOISE_GROUP "noise_power"
#define SPECTRUM_GROUP "spectrum"

//number of spectral points in a spectrum udp message
#define SPEC_UDP_NBINS 256


#endif /* end of include guard: HNetworkDefines_HH__ */
