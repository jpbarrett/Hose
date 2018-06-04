import ctypes
import struct
from ctypes.util import find_library
import os
from os import getenv


def redefine_array_length(array, new_size):
    return (array._type_*new_size).from_address(ctypes.addressof(array))
    
def redefine_char_array_length(array, new_size):
    return (ctypes.c_char*new_size).from_address(ctypes.addressof(array))


# base class which implements comparison operations eq and ne and summary
class HoseStructureBase(ctypes.Structure):

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
        print self.__class__.__name__, ":"
        for field in self._fields_:
            a = getattr(self, field[0])
            if isinstance(a, ctypes.Array):
                print field[0], ":", "array of length: ", len(a), ":"
                for x in a:
                    if isinstance(x, HoseStructureBase):
                        x.printsummary()
                    else:
                        print x
            elif isinstance(a, HoseStructureBase):
                print field[0], ":"
                a.printsummary()
            else:
                print field[0], ":" , a



class spectrum_file_header(HoseStructureBase):
    _fields_ = [
    ('header_size', ctypes.c_size_t),
    ('version_flag', ctypes.c_char * 8),
    ('sideband_flag', ctypes.c_char * 8),
    ('polarization_flag', ctypes.c_char * 8),
    ('start_time', ctypes.c_ulonglong),
    ('sample_rate', ctypes.c_ulonglong),
    ('leading_sample_index', ctypes.c_ulonglong),
    ('sample_length', ctypes.c_size_t),
    ('n_averages', ctypes.c_size_t),
    ('spectrum_length', ctypes.c_size_t),
    ('spectrum_data_type_size', ctypes.c_size_t),
    ('experiment_name', ctypes.c_char * 256),
    ('source_name', ctypes.c_char * 256),
    ('scan_name', ctypes.c_char * 256)
]

class spectrum_file_data(HoseStructureBase):
    _fields_ = [
    ('header', spectrum_file_header),
    ('raw_spectrum_data', ctypes.POINTER( ctypes.c_char) )
    ]

    def __del__(self):
        hinter = hinterface_load()
        hinter.ClearSpectrumFileStruct(ctypes.byref(self))

    #access to the spectrum data should be done through this function
    #not from the raw_spectrum_data with is a raw char array
    def get_spectrum_data(self):
        #have to convert the raw char array to float
        fmt = '@'
        data_size = self.header.spectrum_data_type_size
        if data_size == 2:
            fmt = '@e'
        if data_size == 4:
            fmt = '@f'
        if data_size == 8:
            fmt = '@d'
        spec_data = []
        for i in range(0, self.header.spectrum_length):
            offset = i*self.header.spectrum_data_type_size
            start = i*data_size
            end = (i+1)*data_size
            spec_data.append( struct.unpack(fmt, self.raw_spectrum_data[start:end])[0] )
        return spec_data

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
    hinter.ReadSpectrumFile(ctypes.c_char_p(filename), ctypes.byref(spec_file))
    return spec_file
    
