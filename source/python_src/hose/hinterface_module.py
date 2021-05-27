import ctypes
import struct
import math
from ctypes.util import find_library
import os
from os import getenv

# base class which implements comparison operations eq and ne and summary
class hose_structure_base(ctypes.Structure):

    def __eq__(self, other):
        for field in self._fields_:
            a, b = getattr(self, field[0]), getattr(other, field[0])
            if isinstance(a, ctypes.Array):
                if a[:] != b[:]:
                    return False
            else:
                if a != b:
                    return False
        return True

    def __ne__(self, other):
        for field in self._fields_:
            a, b = getattr(self, field[0]), getattr(other, field[0])
            if isinstance(a, ctypes.Array):
                if a[:] != b[:]:
                    return True
            else:
                if a != b:
                    return True
        return False

    def printsummary(self):
        print(self.__class__.__name__, ":")
        for field in self._fields_:
            a = getattr(self, field[0])
            if isinstance(a, ctypes.Array):
                print(field[0], ":", "array of length: ", len(a), ":")
                for x in a:
                    if isinstance(x, hose_structure_base):
                        x.printsummary()
                    else:
                        print(x)
            elif isinstance(a, hose_structure_base):
                print( field[0], ":")
                a.printsummary()
            else:
                print(field[0], ":" , a)



class spectrum_file_header(hose_structure_base):
    _fields_ = [
    ('header_size', ctypes.c_uint64),
    ('version_flag', ctypes.c_char * 8),
    ('sideband_flag', ctypes.c_char * 8),
    ('polarization_flag', ctypes.c_char * 8),
    ('start_time', ctypes.c_uint64),
    ('sample_rate', ctypes.c_uint64),
    ('leading_sample_index', ctypes.c_uint64),
    ('sample_length', ctypes.c_uint64),
    ('n_averages', ctypes.c_uint64),
    ('spectrum_length', ctypes.c_uint64),
    ('spectrum_data_type_size', ctypes.c_uint64),
    ('experiment_name', ctypes.c_char * 256),
    ('source_name', ctypes.c_char * 256),
    ('scan_name', ctypes.c_char * 256)
]

class spectrum_file_data(hose_structure_base):
    _fields_ = [
    ('header', spectrum_file_header),
    ('raw_spectrum_data', ctypes.POINTER( ctypes.c_char) )
    ]

    def __del__(self):
        hinter = hinterface_load()
        hinter.ClearSpectrumFileStruct(ctypes.byref(self))

    #access to the spectrum data should be done through this function
    #not from the raw_spectrum_data which is a raw char array
    #TODO, make this more robust in the presence of other formats (check the version_flag!)
    def get_spectrum_data(self):
        #have to convert the raw char array to float
        fmt = '@'
        data_size = self.header.spectrum_data_type_size
        if data_size == 2:
            fmt = '@e'  #2 byte float
        if data_size == 4:
            fmt = '@f'  #4 byte float
        if data_size == 8:
            fmt = '@d'  #8 byte float
        spec_data = []
        for i in range(0, self.header.spectrum_length):
            offset = i*self.header.spectrum_data_type_size
            start = i*data_size
            end = (i+1)*data_size
            spec_data.append( struct.unpack(fmt, self.raw_spectrum_data[start:end])[0] )
        return spec_data


class noise_power_file_header(hose_structure_base):
    _fields_ = [
    ('header_size', ctypes.c_uint64),
    ('version_flag', ctypes.c_char * 8),
    ('sideband_flag', ctypes.c_char * 8),
    ('polarization_flag', ctypes.c_char * 8),
    ('start_time', ctypes.c_uint64),
    ('sample_rate', ctypes.c_uint64),
    ('leading_sample_index', ctypes.c_uint64),
    ('sample_length', ctypes.c_uint64),
    ('accumulation_length', ctypes.c_uint64),
    ('switching_frequency', ctypes.c_double),
    ('blanking_period', ctypes.c_double),
    ('experiment_name', ctypes.c_char * 256),
    ('source_name', ctypes.c_char * 256),
    ('scan_name', ctypes.c_char * 256)
]

class accumulation_struct(hose_structure_base):
    _fields_ = [
    ('sum', ctypes.c_double),
    ('sum_squared', ctypes.c_double),
    ('count', ctypes.c_double),
    ('state_flag', ctypes.c_uint64),
    ('start_index', ctypes.c_uint64),
    ('stop_index', ctypes.c_uint64)
    ]

    def is_noise_diode_on(self):
        #define H_NOISE_DIODE_OFF 0
        #define H_NOISE_DIODE_ON 1
        if self.state_flag == 1:
            return True
        else:
            return False

    #the 'DC' component
    def get_mean(self):
        return self.sum/self.count

    def get_rms(self):
        return math.sqrt( self.sum_squared/self.count )

    def get_rms_squared(self):
        return self.sum_squared/self.count

    def get_stddev(self):
        return math.sqrt( self.get_variance() )

    #proportional to AC power
    def get_variance(self):
        return self.get_rms_squared() - self.get_mean()*self.get_mean()



class noise_power_file_data(hose_structure_base):
    _fields_ = [
    ('header', noise_power_file_header),
    ('accumulations', ctypes.POINTER( accumulation_struct ) )
    ]

    def __del__(self):
        hinter = hinterface_load()
        hinter.ClearNoisePowerFileStruct(ctypes.byref(self))

    #access to the spectrum data should be done through this function
    #not from the raw_spectrum_data with is a raw char array
    def get_accumulation(self, index):
        val = accumulation_struct()
        val.sum = 0
        val.sum_squared = 0
        val.count = 0
        val.state_flag = -1
        val.start_index = 0
        val.stop_index = 0
        if index < self.header.accumulation_length :
            val = self.accumulations[index]
        return val

def hinterface_load():
    #first try to find the library using LD_LIBRARY_PATH
    ld_lib_path = getenv('LD_LIBRARY_PATH')
    possible_path_list = ld_lib_path.split(':')
    for a_path in possible_path_list:
        libpath = os.path.join(a_path, 'libHInterface.so')
        if os.path.isfile(libpath):
            #found the library, go ahead and load it up
            hinter = ctypes.cdll.LoadLibrary(libpath)
            return hinter

    #next try to find the library using the environmental variable HOSE_INSTALL
    prefix = getenv('$HOSE_INSTALL')
    if prefix != None:
        path = os.path.join(prefix, 'lib', 'libHInterface.so')
        if os.path.isfile(path):
            hinter = ctypes.cdll.LoadLibrary(libpath)
            return hinter

    #failed to find the library
    return None


def open_spectrum_file(filename):
    spec_file = spectrum_file_data()
    hinter = hinterface_load()
    ret_val = hinter.ReadSpectrumFile(ctypes.c_char_p(filename), ctypes.byref(spec_file))
    if ret_val == 0:
        return spec_file
    else:
        None

def open_noise_power_file(filename):
    np_file = noise_power_file_data()
    hinter = hinterface_load()
    ret_val = hinter.ReadNoisePowerFile(ctypes.c_char_p(filename), ctypes.byref(np_file))
    if ret_val == 0:
        return np_file
    else:
        return None
