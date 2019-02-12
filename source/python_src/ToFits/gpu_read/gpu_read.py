#!/usr/bin/python2

#------------------------------------------------------------------------
__doc__="""\
gpu_read.py - Read in GPU spectrograph data and metadata files.

These classes and functions are likely to be called by higher level
routines to construct SDFITS files from GPU spectrograph raw data.

 To run:
  1) bash
  2) source <Hose>/install/bin/hoseenv.sh
  3a) python2
      import gpu_read as r
  3b) ./gpu_read.py METADATA_FILE

 To load a spectrum file and metadata files:
  import gpu_read as r
  m = r.GPUMeta(ifile=MetaDataFilename) # defaults to meta_data.json
  s = r.GPUSpec(ifile=SpectrumFilename)
  n = r.GPUNoise(ifile=NoiseFilename)
"""

__intro__="""\ Basic routines to handle reading and parsing the data and
metadata files for the Haystack GPU based spectrograph. Assumed to be
used for later conversion into SDFITS format files.  
"""

__author__="S. Levine"
__date__="2019 Feb 12"

#------------------------------------------------------------------------
# Imports

# JSON reading routines 
import json

# for use in sorting by a dictionary key within a list of dictionaries
from operator import itemgetter

# time/date routines
import datetime as dt

# Package for parsing directories/files
import glob

# Sustem interface
import sys

# GPU/Hose routines
import hose

#------------------------------------------------------------------------
# Classes

class GPUBase():
    """
    Common parts for GPU spectrum data.

    Header elements:
    Element (data type, # of bytes) - Description
    ---------------------------------------------
    HeaderSize     (uint64, 8) - total size of the header in bytes
    VersionFlag    (char *, 8) - header/file format version
    SidebandFlag   (char *, 8) - sideband indicator
    PolarizationFlag (char *, 8) - polarization indicator
    StartTime      (uint64, 8) - Start time in seconds from UNIX epoch
    SampleRate     (uint64, 8) - Data rate (Hz)
    Leading SampleIndex (uint64, 8) - Index of the first ADC sample in the file
    SampleLength   (uint64, 8) - # of ADC samples collected for this file

    ExperimentName (char *, 256) - Experiment name
    SourceName     (char *, 256) - Source name
    ScanName       (char *, 256) - Scan name

    E.g.: To access parts of the file header:
     sample_rate = self.specdata.header.sample_rate
    """
    def __init__ (self, ifile, echo=False):
        self.specdata = hose.open_spectrum_file (ifile)
        self.hdr  = self.specdata.header
        self.data = self.specdata.get_spectrum_data()

        if (echo == True):
            self.specdata.printsummary()
            
        return

    # -- raw header structure values
    def version_flag (self):
        """VersionFlag    (char *, 8) - header/file format version."""
        return self.hdr.version_flag
    
    def sideband_flag (self):
        """SidebandFlag   (char *, 8) - sideband indicator"""
        return self.hdr.sideband_flag
    
    def polarization_flag (self):
        """PolarizationFlag (char *, 8) - polarization indicator"""
        return self.hdr.polarization_flag
    
    def start_time (self):
        """StartTime (uint64, 8) - Start time in seconds from UNIX epoch"""
        return self.hdr.start_time
    
    def sample_rate (self):
        """SampleRate     (uint64, 8) - Data rate (Hz)"""
        return self.hdr.sample_rate
    
    def leading_sample_index(self):
        """Leading SampleIndex (uint64, 8) - Index of the first ADC sample in the file"""
        return self.hdr.leading_sample_index
    
    def sample_length (self):
        """SampleLength   (uint64, 8) - # of ADC samples collected for this file"""
        return self.hdr.sample_length

    def experiment_name (self):
        """ExperimentName (char *, 256) - Experiment name"""
        return self.hdr.experiment_name

    def source_name (self):
        """SourceName (char *, 256) - Source name"""
        return self.hdr.source_name

    def scan_name(self):
        """ScanName (char *, 256) - Scan name"""
        return self.hdr.scan_name

    # -- Derived header values

    def scan_number(self):
        """ScanNumber (int ), derived from
        ScanName (char *, 256) - Scan name.
        If conversion of scan name to int fails, return 1"""
        try:
            sc_num = int(self.hdr.scan_name)
        except:
            sc_num = 1
        return sc_num

    def obstime (self):
        """
        Spectrum data observing time.
        Computed from the number of samples divided by the sample rate.
        """
        # construct exposure duration (obstime)
        obstime = float(self.hdr.sample_length) / \
            float(self.hdr.sample_rate)
        return obstime

    def start_ut (self):
        """
        Spectrum data acquisition start time.
        Computed from the experiment UT zero point plus the 
        offset in number of samples divided by the sample rate.
        """
        # construct exposure start time
        baseut  = dt.datetime.utcfromtimestamp(self.hdr.start_time)
        offset  = float(self.hdr.leading_sample_index) / \
            float(self.hdr.sample_rate)
        utstart = baseut + dt.timedelta(seconds=offset)
        return utstart

    def fits_hdr (self):
        """
        convenience function
        returns the parts likely to go into a FITS table header all at once.
        """
        ut = self.start_ut()
        ob = self.obstime()
        # sl = self.spectrum_length()
        ex = self.experiment_name()
        sc = self.scan_name()
        so = self.source_name()

        return ut, ob, ex, sc, so  #, sp
        
class GPUSpec(GPUBase):
    """
    Load GPU spectrum data.
    Several convenience methods to parse out what is needed for
    conversion to SD FITS table row.

    Header elements:
    Element (data type, # of bytes) - Description
    ---------------------------------------------
    HeaderSize     (uint64, 8) - total size of the header in bytes
    VersionFlag    (char *, 8) - header/file format version
    SidebandFlag   (char *, 8) - sideband indicator
    PolarizationFlag (char *, 8) - polarization indicator
    StartTime      (uint64, 8) - Start time in seconds from UNIX epoch
    SampleRate     (uint64, 8) - Data rate (Hz)
    Leading SampleIndex (uint64, 8) - Index of the first ADC sample in the file
    SampleLength   (uint64, 8) - # of ADC samples collected for this file

    ExperimentName (char *, 256) - Experiment name
    SourceName     (char *, 256) - Source name
    ScanName       (char *, 256) - Scan name

    <Add the following to the Base>
    NAverages      (uint64, 8) - # of spectra averaged together
    SpectrumLength (uint64, 8) - # of spectral points
    SpectrumDataTypeSize (uint64, 8) - Size in bbytes of spectral point 
      data type

    E.g.: To access parts of the file header:
     sample_rate = self.specdata.header.sample_rate
    """
    
    def __init__ (self, ifile, echo=False):
        self.specdata = hose.open_spectrum_file (ifile)
        self.hdr  = self.specdata.header
        self.data = self.specdata.get_spectrum_data()

        if (echo == True):
            self.specdata.printsummary()
            print('{}'.format(self.data[0]))
            
        return

    def n_averages (self):
        """    NAverages      (uint64, 8) - # of spectra averaged together"""
        return self.hdr.n_averages

    def spectrum_length (self):
        """SpectrumLength (uint64, 8) - # of spectral points """
        return self.hdr.spectrum_length

    def spectrum_data_type_size (self):
        """SpectrumDataTypeSize (uint64, 8) - Size in bytes of spectral
           point data type """
        return self.hdr.spectrum_data_type_size

    def spectrum (self):
        """Data segment"""
        return self.data
    
        
#------------------------------------------------------------------------
class GPUNoise(GPUBase):
    """
    Load GPU noise data.
    Inherits from GPUSpec, since there is significant overlap in the
    struct definitions.

    Header elements:
    Element (data type, # of bytes) - Description
    ---------------------------------------------
    HeaderSize     (uint64, 8) - total size of the header in bytes
    VersionFlag    (char *, 8) - header/file format version
    SidebandFlag   (char *, 8) - sideband indicator
    PolarizationFlag (char *, 8) - polarization indicator
    StartTime      (uint64, 8) - Start time in seconds from UNIX epoch
    SampleRate     (uint64, 8) - Data rate (Hz)
    Leading SampleIndex (uint64, 8) - Index of the first ADC sample in the file
    SampleLength   (uint64, 8) - # of ADC samples collected for this file

    ExperimentName (char *, 256) - Experiment name
    SourceName     (char *, 256) - Source name
    ScanName       (char *, 256) - Scan name

    <Add the following to the Base>
    AccumulationLength (uint64, 8) - # of accumulation periods stored
    SwitchingFrequency (uint64, 8) - Noise diode switching frequency (Hz)
    BlankingPeriod (uint64, 8) - Switch transition blanking period (sec)

    The accumulations have the following methods:
     get_mean(self) - #the 'DC' component
     get_rms(self)
     get_rms_squared(self)
     get_stddev(self)
     get_variance(self) - proportional to AC power
     is_noise_diode_on()
 
    and the structure data descriptors are:
     count - # of samples accumulated
     start_index - index of first sample in the acc. period
     state_flag - diode state 0=off, 1=on
     stop_index - index of the last sample in the acc. period
     sum - sum of each ADC sample during the acc. period
     sum_squared - sum of each sample squared during the acc. period

    E.g.: To access parts of the file header:
     sample_rate = self.specdata.header.sample_rate
                 = self.hdr.sample_rate

    """

    def __init__ (self, ifile, echo=False):
        self.noisedata = hose.open_noise_power_file (ifile)
        self.hdr  = self.noisedata.header
        # self.pwr  = self.noisedata.get_accumulation(0)
        self.pwr = []
        for i in range(self.hdr.accumulation_length):
            self.pwr.append(self.noisedata.get_accumulation(i))
            
        if (echo == True):
            self.noisedata.printsummary()
            for i in range(self.hdr.accumulation_length):
                print ('Acc. {}:'.format(i))
                self.pwr[i].printsummary()
            
        return

    def accumulation_length (self):
        """AccumulationLength (uint64, 8) - # of accumulation periods
           stored"""
        return self.hdr.accumulation_length

    def switching_frequency (self):
        """SwitchingFrequency (uint64, 8) - Noise diode switching
           frequency (Hz)"""
        return self.hdr.switching_frequency

    def blanking_period (self):
        """BlankingPeriod (uint64, 8) - Switch transition blanking period
           (sec)"""
        return self.hdr.blanking_period

    def noise (self):
        """Power accumulations segment - array of structs"""
        return self.pwr
    
    
#------------------------------------------------------------------------
# Routines to import and parse a metadata file for a GPU spectrometer
#  experiment.
#
# Nominal use:
# Load the class as:
#      x = GPUMeta(ifile=INPUTFILE, echo=False|True)
#           ifile defaults to 'meta_data.json'
#           echo defaults to False
#    Then the various subsets of the metadata can be accessed as:
#      ap = x.antenna_pos()
#      spec = x.spectrometer()
#
# On init, this will
# 1) Load the file from disk into a list of dictionaries.
#
# 2) Then each subset can be extracted using the specific parser
#    e.g for the antenna position data:
#      ap = x.antenna_pos()
#
# 3) The specific parsers all call gpu_meta_parse(meta, mtype). If you
#    have a previously undefined metadata type, you can extract that 
#    using gpu_meta_parse().
#      alt = x.gpu_meta_parse(mtype='alternate_item')
#
# 4) To compute a particular value at a specific time, call the variants
#    _at_time()

class GPUMeta():
    """
    Load up metadata for a GPU spectrometer run.  All the subset
    methods return lists of dictionaries.
    """

    def __init__ (self, ifile='meta_data.json', echo=False):
        """
        Initialize - loads up the requested JSON metadata file.
        """
        self.sdmdata = self.gpu_meta_import (ifile=ifile, echo=echo)
        return

    def gpu_meta_import (self, ifile='meta_data.json', echo=False):
        """
        Import a JSON encoded metadata file. Sort it by the time field
        in case things came in out of order.
        
        This also truncates the fractional part of the time stamp seconds 
        to have at most 6 digits. It should be rounded. Fix that later.
        """
        with open (ifile, 'r') as f:
            mdata = json.load(f)
            f.close ()

        # If the time stamp has more than 6 digits in the fractional
        #  seconds, truncate to 6 (ie to microseconds). Add a 'Z' at
        #  the end, since we expect to work in UT.  We should round,
        #  but we can do that later. Add a conversion to separate
        #  datetime structure element as well.
        for i in mdata:
            j = i['time']
            if ( len(j) > 26 ):
                # case where there are too many digits
                k = j[0:26] + 'Z'
                i['time'] = k
                
            elif ((len(j) == 20) and (j[19] == 'Z')):
                # case where there is no fractional part
                k = j[0:19] + '.0Z'
                i['time'] = k
               
            i['datetime'] = dt.datetime.strptime(i['time'],
                                                 '%Y-%m-%dT%H:%M:%S.%fZ')
            
        # sort the list of dictionaries by the time field of each
        # dictionary
        smdata = sorted(mdata, key=itemgetter('datetime'))

        if (echo == True):
            print_struct (smdata)
            
            #print json.dumps(smdata, indent=2, separators=(',', ':'),
            #                 sort_keys=True)

        return smdata

    def gpu_meta_parse (self, mtype='antenna_position', echo=False):
        """
        Extract a particular metadata type of measurement from the overall
        metadata packet. This defaults to antenna_position for convenience.
        """
        k = []
        for i in self.sdmdata:
            if (i['measurement'] == mtype):
                k.append(i)

        # pretty print option
        if (echo == True):
            print_struct (k)
            
            #print json.dumps(k, indent=2, separators=(',', ':'),
            #                 sort_keys=True)

        return k

    def gpu_meta_at_time (self, itime, mtype='antenna_position',
                          rtype='interp', echo=False):
        """
        Extract a particular metadata type of measurement from the overall
        metadata packet at a particular time of interest. This
        defaults to antenna_position for convenience.

        rtype - return type method 
              = interp == interpolation
                prevval == previous value before requested time
                nexttval == next value after requested time
                nearest == nearest value in time
        """
        subset = self.gpu_meta_parse (mtype=mtype, echo=echo)


        retval = {'measurement':'UNK', 'fields': 'UNK',
                  'time': 'UNK', 'datetime': 'UNK'}

        # return either (i) the value of appropriate type nearest in
        # time, or (ii) an interpolated value or (iii) a state value
        # (eg on/off) based on last value.
        
        retval['datetime']    = itime
        retval['time']        = itime.isoformat()
        
        # If there are no entries, return None
        if (subset == []):
            retval['measurement'] = None

        # if there is only one, return that one
        elif (len(subset) == 1):
            retval['measurement'] = subset[0]['measurement']
            retval['fields']      = subset[0]['fields'].copy()

            # if (subset[0]['datetime'] <= itime):
            #     pass
            # else:
            #     retval['fields']['status'] = 'Unknown'

        elif (len(subset) > 1):
            
            # itime is before first metadata point
            if (itime < subset[0]['datetime']):
                tdidx = 0

            # itime is after last metadata point
            elif (subset[-1]['datetime'] < itime):
                tdidx = -1

            else:
                
                for j in range(len(subset)-1):
                    if ((itime > subset[j]['datetime']) and
                        (itime <= subset[j+1]['datetime'])):
                        q = j
                        r = j+1
                        break
                a = (itime - subset[j]['datetime']).total_seconds()
                b = (subset[j+1]['datetime'] - itime).total_seconds()
                c = (subset[j+1]['datetime'] - subset[j]['datetime']).total_seconds()
                d = a/c

                if (rtype == 'interp'):
                    # a = (t - t0)
                    # b = (t1 - t)
                    # c = (t1 - t0) 
                    # y = y0 + (t - t0)/(t1 - t0) * (y1 - y0)
                    # y = y0 + a / c * (y1 - y0)
                    #   = a/c y1 + (1-a/c) * y0
                    tdidx   = q
                    tdidxp1 = r
                    pass
                
                elif (rtype == 'prevval'):
                    tdidx = q
                            
                elif (rtype == 'nextval'):
                    tdidx = r
                            
                elif (rtype == 'nearest'):
                    if (a < b):
                        tdidx = q
                    else:
                        tdidx = r

            # make base copy - sufficient for all except interp, which
            # will need an added part, if point is not outside extrema
            retval['measurement'] = subset[tdidx]['measurement']
            retval['fields']      = subset[tdidx]['fields'].copy()
            
            if ((rtype == 'interp') and  (tdidx != 0) and (tdidx != -1)):
                for irf in retval['fields']:
                    retval['fields'][irf] += d * \
                                             (subset[tdidxp1]['fields'][irf] \
                                              - subset[tdidx]['fields'][irf])
                    #print ('irf1 == {} {} {}'.
                    #       format(subset[tdidx]['fields'][irf],
                    #       subset[tdidxp1]['fields'][irf], d))
                    #print ('irf2 == {}'.format(retval['fields'][irf]))
                    
        return retval

    def dump_all (self, echo=False):
        """
        Dump the full list of dictionaries.
        """
        if (echo == True):
            print_struct (self.sdmdata)
            #print json.dumps(self.sdmdata, indent=2,
            #                 separators=(',', ':'), sort_keys=True)

        return self.sdmdata

# All input dictionaries have: measurement, time and fields.
#  measurement is taken as the identifier for the type of metadata
#  time is an ISO style timestamp in UTC.
#  fields are a set appropriate to the measurement type. They are listed
#    in the input routine comments.
#
# I expect that we'll want to customize the returns for each of these. So
#  only antenna_pos() has such.

    def antenna_pos (self, echo=False):
        """
        Extract the antenna_position information.
        fields: az, el

        echo can take 3 values: True, False or 'Short'

        Earlier version changed this into a simpler list of lists.
        Each sub list has time, datetime, az, el.
        The expectation is that this will get used as input for an interpolator
        to construct telescope az, el at any specific time within the
        range of the values.  That is not used. Now return in same
        style as all other data.
        """
        apos = self.gpu_meta_parse (mtype='antenna_position',
                                    echo=echo)

        # k = []
        # for i in apos:
        #     j    = i['fields']
        #     ti   = i['time']
        #     dtti = i['datetime']
        #     az   = j['az']
        #     el   = j['el']
        #     k.append([ti,dtti, az,el])

        # option to display the list of lists
        # if ((echo == True) or (echo == 'Short')):
        #     for m in k:
        #         print (m)

        return apos

    def antenna_pos_at_time (self, itime, echo=False):
        """
        Extract the antenna_position information at time.
        (Gets interpolated value.) 
        """
        apos = self.gpu_meta_at_time (itime, mtype='antenna_position',
                                      rtype='interp', echo=echo)

        return apos

    def antenna_target (self, echo=False):
        """
        Extract the antenna_target_status information.
        fields: acquired, status
        """
        atstat = self.gpu_meta_parse (mtype='antenna_target_status', 
                                      echo=echo)

        return atstat

    def antenna_target_at_time (self, itime, echo=False):
        """
        Extract the antenna_target_status information at time.
        """
        atstat = self.gpu_meta_at_time (itime,
                                        mtype='antenna_target_status',
                                        rtype='prevval', echo=echo)

        return atstat

    def digitizer (self, echo=False):
        """
        Extract the digitizer_config information. Digitizer configuration.
        fields: n_digitizer_threads, polarization,
        sampling_frequency_Hz, sideband
        """
        dcon = self.gpu_meta_parse (mtype='digitizer_config', 
                                    echo=echo)
        
        return dcon

    def digitizer_at_time (self, itime, echo=False):
        """
        Extract the digitizer_config information at time.
        """
        dcon = self.gpu_meta_at_time (itime, mtype='digitizer_config',
                                      rtype='prevval', echo=echo)
        
        return dcon

    def frequency_map (self, echo=False):
        """
        Extract the frequency_map information.
        fields: bin_delta, frequency_delta_MHz, 
        reference_bin_center_sky_frequency_MHz, reference_bin_index
        """
        fmcon = self.gpu_meta_parse (mtype='frequency_map',
                                    echo=echo)
        
        return fmcon

    def frequency_map_at_time (self, itime, echo=False):
        """
        Extract the frequency_map information at time.
        """
        fmcon = self.gpu_meta_at_time (itime, mtype='frequency_map',
                                       rtype='prevval', echo=echo)
        
        return fmcon

    def noise (self, echo=False):
        """
        Extract the noise_diode_config information.
        fields: noise_blanking_period, noise_diode_switching_frequency_Hz
        """
        ndcon = self.gpu_meta_parse (mtype='noise_diode_config', 
                                     echo=echo)
        
        return ndcon

    def noise_at_time (self, itime, echo=False):
        """
        Extract the noise_diode_config information at time.
        """
        ndcon = self.gpu_meta_at_time (itime, mtype='noise_diode_config',
                                       rtype='prevval', echo=echo)
        
        return ndcon

    def recording (self, echo=False):
        """
        Extract the recording_status information.
        fields: experiment_name, recording, scan_name, source_name
        Map to FITS: 
         RECEXPER = experiment name
         RECORD = recording on/off
         RECSCAN = scan_name
         RECSRC = source_name
        """
        rstat = self.gpu_meta_parse (mtype='recording_status', 
                                     echo=echo)

        return rstat

    def recording_at_time (self, itime, echo=False):
        """
        Extract the recording_status information at time.
        """
        rstat = self.gpu_meta_at_time (itime, mtype='recording_status',
                                       rtype='prevval', echo=echo)

        return rstat

    def source (self, echo=False):
        """
        Extract the source_status information. This is the requested
        target's catalogue information. This came from the observing
        scheduler, NOT from the telescope/antenna encoders etc.
        fields: dec, epoch, ra, source
        Map to FITS:
         SRCID = source
         SRCRA = ra
         SRCDEC = dec
         SRCEPOCH = epoch
        """
        sstat = self.gpu_meta_parse (mtype='source_status', echo=echo)
        
        return sstat

    def source_at_time (self, itime, echo=False):
        """
        Extract the source_status information at time.
        """
        sstat = self.gpu_meta_at_time (itime, mtype='source_status',
                                       rtype='prevval', echo=echo)
        
        return sstat

    def spectrometer (self, echo=False):
        """
        Extract the  GPU Spectrometer configuration.
        fields: fft_size, n_averages, n_spectrometer_threads, n_writer_threads
        Map to FITS: 
         SPECFFT = fft_size
         SPECNAVG = n_averages
        """
        speccon = self.gpu_meta_parse (mtype='spectrometer_config', echo=echo)
        
        return speccon

    def spectrometer_at_time (self, itime, echo=False):
        """
        Extract the GPU Spectrometer configuration at time.
        """
        speccon = self.gpu_meta_at_time (itime, mtype='spectrometer_config',
                                         rtype='prevval', echo=echo)
        
        return speccon

    def udc (self, echo=False):
        """
        Extract the udc_status information. Up/Down converter information.
        fields: attenuation_h, attenuation_v, frequency_MHz, udc
        Map to FITS: 
         UDCATTEH = attenuation_h
         UDCATTEV = attenuation_v
         UDCFREQ = frequency_MHz
         UDCID = udc
        """
        udcstat = self.gpu_meta_parse (mtype='udc_status', echo=echo)

        return udcstat

    def udc_at_time (self, itime, echo=False):
        """
        Extract the udc_status information at time.
        """
        udcstat = self.gpu_meta_at_time (itime, mtype='udc_status',
                                         rtype='prevval', echo=echo)

        return udcstat

    def valid (self, echo=False):
        """
        Extract the data_validity information.
        fields: status
        Map to FITS: 
         DVALID = status
        """
        dvalid = self.gpu_meta_parse (mtype='data_validity', echo=echo)
        
        return dvalid

    def valid_at_time (self, itime, echo=False):
        """
        Extract the data_validity information at time.
        """
        dvalid = self.gpu_meta_at_time (itime, mtype='data_validity',
                                        rtype='prevval', echo=echo)

        return dvalid

#------------------------------------------------------------------------
# Functions

def print_struct (toprint):
    """
    Print out the json style dictionary portion requested.
    If there is a datetime item, reformat as a string for printing,
    since the json.dumps doesn't handle datetime structures nicely.
    """
    lcl = []
    for i in toprint:
        # force a local copy so as not to modify the original list
        j = i.copy()
        if (type(i['datetime']) is dt.datetime):
            j['datetime'] = '{}'.format(i['datetime'].isoformat())
        lcl.append(j)
            
    print (json.dumps(lcl, indent=2,
                      separators=(',', ':'), sort_keys=True))
            
    return

def list_dirs (dirname = './', echo=False):
    """
    Listing of names of subdirs in a directory.
    Sorts alphabetically.
    """
    dirs = sorted(glob.glob(dirname))
    if (echo == True):
        print ('{}'.format(dirs))
    return dirs

def list_files (dirname = './', ftail= '.txt', echo=False):
    """
    Listing of files in a directory with particular extension.
    Sorts alphabetically.
    Defaults to list .txt files in the current directory.
    """
    files = sorted(glob.glob(dirname + '/*' + ftail))
    if (echo == True):
        print ('{}'.format(files))
    return files
                
def construct_lists (dirname = './', echo=False):
    """
    Given a directory path, construct list of scan directory(ies) and
    associated files.
    Input: Directory name
    Output: 1 list and 4 lists of lists - list of directories, and
      Spectrum, Noise, Metadata and Fits file lists of lists.
    """

    # construct list of scan directories
    d = list_dirs (dirname=dirname)

    if (echo == True):
        print ('{}'.format(d))

    # within each scan directory, construct lists of spec, noise and
    # metadata files
    fs = []
    fn = []
    fm = []
    ff = []
    for n in d:
        fs.append(list_files (dirname=n, ftail='.spec'))
        fn.append(list_files (dirname=n, ftail='.npow'))
        fm.append(list_files (dirname=n, ftail='.json'))
        ff.append(list_files (dirname=n, ftail='.fits'))
        
    if (echo == True):
        for n in range(len(d)):
            print ('Start {}'.format(dt.datetime.now().ctime()))
            print ('Dir: {}'.format(d[n]))
            print ('  Spec files: {}'.format(len(fs[n])))
            print ('  Noise files: {}'.format(len(fn[n])))
            print ('  Metadata file(s): {} {}'.format(len(fm[n]), fm[n]))
            print ('  FITS files: {} {}'.format(len(ff[n]), ff[n]))

    return d, fs, fn, fm, ff

#------------------------------------------------------------------------
# Execute, if run standalone:

if __name__ == '__main__':

    # Expect argument to be directory with scans or scan directories
    argone = sys.argv[1]
    print ('{}'.format(argone))

    d, fs, fn, fm, ff = construct_lists (argone)
    
    # within each scan directory, construct lists of spec, noise and
    # metadata files
    for n in range(len(d)):
        print ('Start {}'.format(dt.datetime.now().ctime()))
        print ('Dir: {}'.format(d[n]))
        print ('  Spec files: {}'.format(len(fs[n])))
        print ('  Noise files: {}'.format(len(fn[n])))
        print ('  Metadata file(s): {} {}'.format(len(fm[n]), fm[n]))
        print ('  FITS files: {} {}'.format(len(ff[n]), ff[n]))

        for ifm in fm[n]:
            md = GPUMeta(ifm)
            v1 = md.antenna_pos()
            print ('  META - antpos - {}'.format(v1[0]))

        ct = 0
        for ifs in fs[n]:
            sp = GPUSpec(ifs)
            if (ct %100 == 0):
                ut, ob, ex, sc, so = sp.fits_hdr()
                print ('  M+S - {} {} {} {}'.\
                       format(ct, ut,
                              md.antenna_pos_at_time(ut)['fields']['az'],
                              md.antenna_pos_at_time(ut)['fields']['el']))
            ct += 1
            
        print ('End {}'.format(dt.datetime.now().ctime()))
        print ('')
        
    exit()
    
    a = GPUMeta(argone)
    v1 = a.valid()

    c = dt.datetime.now()
    d = dt.datetime(2018,1,2,10,11,12)

    e1 = a.valid_at_time(c, echo=True)
    e2 = a.valid_at_time(d, echo=True)

    f1 = a.gpu_meta_at_time(c, mtype='data_validity', rtype='prevval', echo=True)
    f2 = a.gpu_meta_at_time(d, mtype='data_validity', rtype='prevval', echo=True)
    
    v2 = a.valid()

    print ('{}\n'.format(v1))
    print ('{}\n'.format(v2))

    print ('{}\n'.format(e1))
    print ('{}\n'.format(f1))
    print ('{}\n'.format(e2))
    print ('{}\n'.format(f2))
    
    exit()
