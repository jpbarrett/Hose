#!/bin/sh

#skelton file to set up environment variables needed to access local
#influxdb server, dummy_values should be filled in for the local configuration
#these are the environment variables to access influxdb running

#TODO, make this more secure!

export INFLUXDB_HOSTNAME="curie"
export INFLUXDB_PORT="8086"
export INFLUXDB_USER="GPUSpecOper"
export INFLUXDB_PWD="kLx14f2p"
export INFLUXDB_DATABASE="gpu_spec"
