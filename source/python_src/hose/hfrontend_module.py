import copy
import numpy as np

from .hfslog_module import time_stamp

class signal(object):
    """ tone signal object """
    def __init__(self):
        #amplitude is really only a place keeper (0 or 1) to indicate if this frequency is filtered or not
        self.amplitude_frequency_pair_list = list()

    def get_n_tones(self):
        return len(self.amplitude_frequency_pair_list)

    #add a single tone with a specific frequency with a fixe amplitude
    def add_amp_freq(self, amplitude, frequency_MHz):
        self.amplitude_frequency_pair_list.append( (amplitude, frequency_MHz) )

    def print_amp_freqs(self):
        for amp, freq in self.amplitude_frequency_pair_list:
            if amp != 0.0:
                print "amp = ", amp, " and freq = ", freq

class ideal_rf_mixer(object):
    """ ideal ideal_rf_mixer object """
    def __init__(self, name="ideal_rf_mixer", lo_freq_MHz=0.0):
        self.name = name
        self.local_oscillator_frequency_MHz = lo_freq_MHz

    def apply_to_signal(self, signal_obj):
        tmp_af_pair = list()
        lo_freq = self.local_oscillator_frequency_MHz
        if lo_freq != 0.0:
            for amp,freq in signal_obj.amplitude_frequency_pair_list:
                if abs(freq) > 1e-15:
                    tmp_af_pair.append( (amp, lo_freq + freq ) )
                    tmp_af_pair.append( (amp, lo_freq - freq ) )
                else:
                    tmp_af_pair.append( (amp, lo_freq ) )
            signal_obj.amplitude_frequency_pair_list = copy.copy(tmp_af_pair)

class ideal_rf_filter(object):
    """ ideal ideal_rf_filter object """
    def __init__(self, name="ideal_rf_filter", low_freq_MHz=0.0, high_freq_MHz=0.0):
        self.name = name
        self.low_frequency_MHz = low_freq_MHz
        self.high_frequency_MHz = high_freq_MHz

    def apply_to_signal(self, signal_obj):
        tmp_af_pair = list()
        for amp,freq in signal_obj.amplitude_frequency_pair_list:
            if freq <= self.high_frequency_MHz and self.low_frequency_MHz <= freq:
                tmp_af_pair.append( (amp, freq) ) #this frequency passes the filter
                #print("signal with frequency ", freq, " passes filter: ", self.low_frequency_MHz, " - ", self.high_frequency_MHz)
            #else:
                #print("signal with frequency ", freq, " does NOT pass filter: ", self.low_frequency_MHz, " - ", self.high_frequency_MHz)
        signal_obj.amplitude_frequency_pair_list = copy.copy(tmp_af_pair)

class signal_chain(object):
    """ applies a list of signal chain elements to a signal object """
    def __init__(self, name="signal_chain"):
        self.name = name
        self.element_list = list() #list of signal chain elements, in order starting from sky going to backend

    def add_element(self,signal_element):
        self.element_list.append(signal_element)

    def convert_signal_forward(self, signal_obj): #sky to backend
        for element in self.element_list:
            element.apply_to_signal(signal_obj)

    def convert_signal_backward(self, signal_obj): #backend to sky, traverse in reverse
        for element in self.element_list[::-1]:
            element.apply_to_signal(signal_obj)

    def map_frequency_forward(self, frequency_MHz):
        tmp_signal = signal()
        tmp_signal.add_amp_freq(1, frequency_MHz)
        self.convert_signal_forward(tmp_signal)
        if tmp_signal.get_n_tones() == 1:
            return tmp_signal.amplitude_frequency_pair_list[0][1]
        else:
            return np.nan #either no tone, or more than one tone

    def map_frequency_backward(self, frequency_MHz):
        tmp_signal = signal()
        tmp_signal.add_amp_freq(1, frequency_MHz)
        self.convert_signal_backward(tmp_signal)
        #print(" n tones = ", tmp_signal.get_n_tones()  )
        if tmp_signal.get_n_tones() == 1:
            return tmp_signal.amplitude_frequency_pair_list[0][1]
        else:
            return np.nan #either no tone, or more than one tone

    def map_frequency_pair_forward(self, low_freq_MHz, high_freq_MHz):
        if low_freq_MHz > high_freq_MHz:
            tmp = high_freq_MHz
            high_freq_MHz = low_freq_MHz
            low_freq_MHz = tmp
        return [ self.map_frequency_forward(low_freq_MHz), self.map_frequency_forward(high_freq_MHz)]

    def map_frequency_pair_backward(self, low_freq_MHz, high_freq_MHz):
        if low_freq_MHz > high_freq_MHz:
            tmp = high_freq_MHz
            high_freq_MHz = low_freq_MHz
            low_freq_MHz = tmp
        return [ self.map_frequency_backward(low_freq_MHz), self.map_frequency_backward(high_freq_MHz)]


class westford_signal_chain(signal_chain):
    """ class representing the fixed (except for UDC-C luff freq) signal chain at Westford """

    def __init__(self, udc_luff_freq_MHz=7054.6, apply_last_filter=True):
        signal_chain.__init__(self, "westford_signal_chain")
        self.udc_luff_freq_MHz = udc_luff_freq_MHz
        self.luff_multiplier = 4.0
        self.apply_last_filter = apply_last_filter
        signal_chain.add_element(self, ideal_rf_filter("filter1", 4000.0, 14000.0) )
        signal_chain.add_element(self, ideal_rf_mixer("mixer1", self.udc_luff_freq_MHz*self.luff_multiplier) )
        signal_chain.add_element(self, ideal_rf_filter("filter2", 20000.0, 22000.0) )
        signal_chain.add_element(self, ideal_rf_mixer("mixer2", 22500.0) )
        if apply_last_filter is True:
            signal_chain.add_element(self, ideal_rf_filter("filter3", 700.0, 1200.0) )

class frequency_map(object):
    """ define the linear frequency map to be specified in meta data output """
    def __init__(self):
        self.name = "frequency_map"
        self.data_fields = dict()
        self.data_fields["reference_bin_index"] = 0
        self.data_fields["reference_bin_center_sky_frequency_MHz"] = 0.0
        self.data_fields["bin_delta"] = 1
        self.data_fields["frequency_delta_MHz"] = 0.0
        self.time_stamp_string = ""

    def set_time(self, time_stamp_string):
        self.time_stamp_string = time_stamp_string

    def set_reference_bin_index(self, index):
        self.data_fields["reference_bin_index"] = index

    def set_reference_bin_center_sky_frequency(self, frequency_MHz):
        self.data_fields["reference_bin_center_sky_frequency_MHz"] = frequency_MHz

    def set_bin_delta(self, delta):
        self.data_fields["bin_delta"] = delta

    def set_frequency_delta(self, frequency_delta_MHz):
        self.data_fields["frequency_delta_MHz"] = frequency_delta_MHz

    def as_dict(self):
        point_value = {
                "time": self.time_stamp_string,
                "measurement": self.name,
                "fields": self.data_fields
            }
        return point_value
