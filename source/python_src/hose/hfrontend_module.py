import copy
import numpy as np
import math

from .hfslog_module import time_stamp

def map_zone_frequency_to_aliased_counterpart(input_frequency_MHz, sampling_frequency_MHz):
    nint = int(round(float(input_frequency_MHz)/float(sampling_frequency_MHz)))
    measured_frequency_MHz = input_frequency_MHz - sampling_frequency_MHz*nint
    return abs(measured_frequency_MHz)

def map_aliased_counterpart_to_zone_frequency(aliased_input_frequency_MHz, sampling_frequency_MHz, nyquist_zone):
    sign_val = -1.0
    if nyquist_zone%2 == 1:
        sign_val = 1.0
    return abs( aliased_input_frequency_MHz + sign_val*sampling_frequency_MHz*math.floor(nyquist_zone/2))


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

class aliasing_sampler(object):
    """ ideal aliasing sampler """

    def __init__(self, name="aliasing_sampler", sampling_frequency_MHz=0.0, nyquist_zone=1):
        self.name = name
        self.sampling_frequency_MHz = float(sampling_frequency_MHz)
        self.nyquist_zone = nyquist_zone #this is needed to map the mapping 1-to-1

    def is_freq_in_first_nyquist_zone(self, freq):
        if 0.0 < freq and freq < self.sampling_frequency_MHz/2.0:
            return True
        else:
            return False

    def apply_to_signal(self, signal_obj):
        """ takes an input frequency f_a to it's aliased counter-part if f_a, if we have f_s > f_nyquist"""
        """ if 0 < f_input < f_nyquist and nyquist_zone !=1, then this maps the aliased frequency f_input to the proper nyquist zone"""
        tmp_af_pair = list()
        for amp,freq in signal_obj.amplitude_frequency_pair_list:
            if self.is_freq_in_first_nyquist_zone(freq) is True:
                if self.nyquist_zone != 1:
                    print("upconverting signal from: ", freq)
                    #convert the input frequency to the proper nyquist_zone
                    converted_freq = map_aliased_counterpart_to_zone_frequency(freq, self.sampling_frequency_MHz, self.nyquist_zone)
                    tmp_af_pair.append( (amp, converted_freq) )
                    print(" to: ", converted_freq)
                else:
                    # pass this frequncy untouched
                    tmp_af_pair.append( (amp, freq) )
            else:
                #input isn't in first nyquist-zone, so assume we are doing the aliasing conversion
                aliased_freq = map_zone_frequency_to_aliased_counterpart(freq, self.sampling_frequency_MHz)
                tmp_af_pair.append( (amp, aliased_freq) )

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
        print(" n tones = ", tmp_signal.get_n_tones()  )
        if tmp_signal.get_n_tones() == 1:
            return tmp_signal.amplitude_frequency_pair_list[0][1]
        else:
            return np.nan #either no tone, or more than one tone

    def map_frequency_pair_forward(self, low_freq_MHz, high_freq_MHz):
        val1 =  self.map_frequency_forward(low_freq_MHz)
        val2 = self.map_frequency_forward(high_freq_MHz)
        return [ val1, val2 ]

    def map_frequency_pair_backward(self, low_freq_MHz, high_freq_MHz):
        print("calling map pair backward")
        val1 = self.map_frequency_backward(low_freq_MHz)
        val2 = self.map_frequency_backward(high_freq_MHz)
        print("values are ", val1, val2)
        return [ val1, val2 ]


class westford_signal_chain(signal_chain):
    """ class representing the fixed (except for UDC-C luff freq) signal chain at Westford """

    def __init__(self, udc_luff_freq_MHz=7054.6, apply_last_filter=False):
        signal_chain.__init__(self, "westford_signal_chain")
        self.udc_luff_freq_MHz = udc_luff_freq_MHz
        self.luff_multiplier = 4.0
        signal_chain.add_element(self, ideal_rf_filter("filter1", 4000.0, 14000.0) )
        signal_chain.add_element(self, ideal_rf_mixer("mixer1", self.udc_luff_freq_MHz*self.luff_multiplier) )
        signal_chain.add_element(self, ideal_rf_filter("filter2", 20000.0, 22000.0) )
        signal_chain.add_element(self, ideal_rf_mixer("mixer2", 22500.0) )
        if apply_last_filter is True:
            signal_chain.add_element(self, ideal_rf_filter("filter3", 700.0, 1200.0) )
        signal_chain.add_element(self, aliasing_sampler("aliasing_sampler1", 1250.0, 2) )

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
