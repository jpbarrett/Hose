#!/usr/bin/python2
# """#!@PYTHON_EXECUTABLE@"""

#------------------------------------------------------------------------
__doc__="""\
gpu_fits_plot - create a quick summary plot of the contents of an SDFITS file.

 Uses: 

 Assume:

 Debugging tip: 
 - NOT python3 compatible at this point. A bunch of syntax changes
   will need to be chased down.

 Structure:
  - primary HDU
  - spectrum binary table (named SINGLE DISH)

"""
__usage__="""\
 Usage: ./gpu_fits_plot.py FitsFile
  This will create a quick summary plot of the contents of an SDFITS file.

"""
__author__="S. Levine"
__date__="2019 Jun 16"

#------------------------------------------------------------------------
# import the FITS io routines for the I/O
from astropy.io import fits

# import astropy coordinates routines
#from astropy.coordinates import EarthLocation, SkyCoord, AltAz, ICRS, Galactic
#from astropy.time        import Time
#import astropy.units     as u

# JSON reading routines
#import json

# import pyplot from matplotlib for simple graphics display
import matplotlib.pyplot   as plt
import matplotlib.colors   as clrs
import matplotlib.gridspec as grd

# import numpy for its array handling
import numpy as np

# time/date routines
#from time import gmtime, strftime
import datetime as dt

# system path operations
import os

# GPU file reading and parsing classes/routines
#import gpu_read as r

# random was used for some testing. can be removed.
#import random

#------------------------------------------------------------------------
def gpu_f_read (fname, use_mmap=False):
    """
    Load a gpu SDFITS file for display.
    fname == file name (decide on search/parse options)
    use_mmap == use mmap option for large files. False by default.

    Return header, data and converted date-obs columns from first
    bintable HDU found that is valid SINGLE DISH format.

    Do some basic format checking, including:
    1) make sure it has two HDUs:
    - PRIMARY - basic HDU
    - SINGLE DISH - binary table HDU
    2) Any particular keyword checks within each? So far, XTENSION, and
    EXTNAME.
    """

    with fits.open(fname, memmap=use_mmap) as gff:
        gff.info()

        # first HDU should always exist
        gff0h = gff[0].header
        gff0d = gff[0].data
        # print (gff.fileinfo(index=0))
        print (repr(gff0h))

        # find the first HDU with an XTENSION keyword == BINTABLE
        # and if found, check that EXTNAME == SINGLE DISH
        valid = False
        sp_date = []
        for i in gff:
            j = i.header
            try:
                if (j['XTENSION'] == 'BINTABLE'):
                    if (j['EXTNAME'] == 'SINGLE DISH'):
                        valid = True
                        gff1h = i.header
                        gff1d = i.data
                        gff1c = i.columns
                        try:
                            sp_date = [dt.datetime.strptime(kt, 
                                                            '%Y-%m-%dT%H:%M:%S.%f')
                                       for kt in gff1d['date-obs'] ]
                        except:
                            pass

#                        for kt in gff1d['date-obs']:
#                            sp_date.append(dt.datetime.strptime(kt,
#                                                      '%Y-%m-%dT%H:%M:%S.%f'))
#                        print (gff1d['date-obs'][0], sp_date[0])
#                        print (repr(gff1h))

                        break
            except:
                pass

#        print (gff)

        if (valid == True):
            print ('Found valid SINGLE DISH HDU')
        else:
            print ('Error: Did not find a valid SINGLE DISH HDU, Exiting')
            return None, None, None

#        print (gff1h)
#        if (gff1h['XTENSION'] != 'BINTABLE'):
#            print ({}.format('WARNING: XTENSION is not a BINTABLE'))
#
#        if (gff1h['EXTNAME'] != 'SINGLE DISH'):
#            print ('{}'.format('WARNING: EXTNAME is not SINGLE DISH'))
#        else:
#            print ('{} {}'.format('EXTNAME is ', gff1h['EXTNAME']))
#
#        gff1c.info()
#        print (gff1c)

        print (repr(gff1h))

        print ('Table Columns:\n{:>3} {:<8} {:<8} {:<8}'.format('#', 'Name', 
                                                          'Unit', 'Format'))
        ic = 1
        for i in sorted(gff1c.names):
            try:
                print ('{:>3d} {:<8} {:<8} {:<8}'.format(ic, i, gff1c[i].unit, 
                                                         gff1c[i].format))
            except:
                pass

            ic += 1

#            print (i.name, i.dtype, i.format, i.unit)

#        print (gff1d.names, gff1d.dtype, gff1d.formats)

        return gff1h, gff1d, sp_date

def gpu_comp_freqs (crval1, crpix1, cdelt1, array):
    """
    compute freq at each pixel of array based on cr*
    """
    al = len(array)

    rtnarr = ((np.arange (1,al+1,1) - crpix1) * cdelt1 + crval1) / 10.**9

    return rtnarr

def gpu_p_array (xarray, yarray, 
                 clr='red', line='solid',
                 xmin=None, xmax=None, xlog=False,
                 ymin=None, ymax=None, ylog=False,
                 xlabel=None, ylabel=None, title=None):
    """
    plot an array
    Options: if (xarray, yarray), then plot as (x,y)
    If you leave out xarray, it will auto plot yarray against index.
    """

    if (type(xarray) == type(None)):
        plt.plot (yarray, color=clr, linestyle=line)
    else:
        plt.plot (xarray, yarray, color=clr, linestyle=line)

    # Mods to the default x axis scaling
    if ((xmin != None) and (xmax != None)):
        plt.xlim (xmin=xmin, xmax=xmax)

    # Mods to the default y axis scaling
    if ((ymin != None) and (ymax != None)):
        plt.ylim (ymin=ymin, ymax=ymax)

    if (ylog == True):
        plt.semilogy()

    # axis labels
    if (xlabel != None):
        plt.xlabel (xlabel)
    if (ylabel != None):
        plt.ylabel (ylabel)
    if (title != None):
        plt.title (title)

    plt.show()

    return

def agpu_p_array (ax, xarray, yarray, 
                  clr='red', line='solid',
                  xmin=None, xmax=None, xlog=False,
                  ymin=None, ymax=None, ylog=False,
                  xlabel=None, ylabel=None, title=None,
                  legend=None, xrot=None, firstpt=False):
    """
    plot an array in an axes (ax)
    Options: if (xarray, yarray), then plot as (x,y)
    If you leave out xarray, it will auto plot yarray against index.
    """

    if (type(xarray) == type(None)):
        ax.plot (yarray, color=clr, linestyle=line)
        if (firstpt == True):
            ax.plot (yarray[0:1], 'o', color='green')
    else:
        ax.plot (xarray, yarray, color=clr, linestyle=line)
        if (firstpt == True):
            ax.plot (xarray[0], yarray[0], 'o', color='green')

    # Mods to the default x axis scaling
    if ((xmin != None) and (xmax != None)):
        ax.set_xlim (xmin=xmin, xmax=xmax)

    # Mods to the default y axis scaling
    if ((ymin != None) and (ymax != None)):
        ax.set_ylim (ymin=ymin, ymax=ymax)

    if (ylog == True):
        ax.semilogy()

    # axis labels
    if (xlabel != None):
        ax.set_xlabel (xlabel, fontsize='x-small')
    if (ylabel != None):
        ax.set_ylabel (ylabel, fontsize='x-small')
    if (title != None):
        ax.set_title (title, fontsize='small')
    if (legend != None):
        ax.legend (legend, fontsize='x-small')

    ax.tick_params(labelsize='x-small', direction='in')

    if (xrot != None):
        ax.tick_params(axis='x', labelbottom=False)
# looks labelrotation is not in the version of matplotlib used by py2
#        ax.tick_params(axis='x', labelrotation=xrot)

    if (ylog == True):
        ax.minorticks_off()

    return

def gpu_one_page(filename='284-144959.fits', disp=False):
    """\
    Generate a one page summary set of plots of an SDFITS file
    from the Haystack GPU spectrometer project.
    """

    bt1h, bt1d, bt1_dt = gpu_f_read (filename)

    # parse out interesting info from header keywords
    maxspec = bt1h['naxis2']
    telescop = bt1h['telescop']
    instrume = bt1h['instrume']

    # parse out interesting info from first record in the data table
    src_id  = bt1d['srcid'][0]
    src_ra  = bt1d['srcra'][0]
    src_dec = bt1d['srcdec'][0]
    obstime = bt1d['obstime'][0]
    # utstart = bt1d['ut'][0]
    # utend   = bt1d['ut'][-1]
    utstart = bt1d['date-obs'][0]
    utend   = bt1d['date-obs'][-1]
    barycorr = bt1d['barycorr'][0]
    vlsrcorr = bt1d['vlsrcorr'][0]

    fig_p1 = plt.figure (tight_layout=True)
    p1_gs  = grd.GridSpec (ncols=3, nrows=3)
    fp1_a1 = fig_p1.add_subplot(p1_gs[0,0:2])
    fp1_a2 = fig_p1.add_subplot(p1_gs[1:,0:2])
    fp1_a5 = fig_p1.add_subplot(p1_gs[2,2])

    fig_p1.suptitle('File: '+filename, x=0.8, y=0.99)
    dy = 0.03
    infoy = 0.93
    infox = 0.68

    fig_p1.text(infox, infoy, '{}: {}'.format('Telescope', telescop),
                                                      fontsize='x-small')
    infoy = infoy - dy
    fig_p1.text(infox, infoy, '{}: {}'.format('Instrument', instrume),
                                                      fontsize='x-small')
    infoy = infoy - dy
    fig_p1.text(infox, infoy, '{:<9}: {:>27}'.format('UT Start', utstart),
                                                      fontsize='x-small')
    infoy = infoy - dy
    fig_p1.text(infox, infoy, '{:<9}: {:>27}'.format('UT End', utend), 
                fontsize='x-small')

    infoy = infoy - dy
    fig_p1.text(infox, infoy, '{:<}'.format('For Spectrum 1'), 
                fontsize='x-small')

    infox = infox + 0.02
    infoy = infoy - dy
    fig_p1.text(infox, infoy, '{:<10}: {:>12}'.format('Source ID',src_id), 
                fontsize='x-small')
    tmpstr = src_ra[0:2] + ":" + src_ra[2:4] + ":" + src_ra[4:]
    infoy = infoy - dy
    fig_p1.text(infox, infoy, '{:<10}: {:>12}'.format('Source RA',tmpstr), 
                fontsize='x-small')
    tmpstr = src_dec[0:3] + ":" + src_dec[3:5] + ":" + src_dec[5:]
    infoy = infoy - dy
    fig_p1.text(infox, infoy, '{:<10}: {:>12}'.format('Source DEC',tmpstr), 
                fontsize='x-small')

    infoy = infoy - dy
    fig_p1.text(infox, infoy, '{:<}: {:>.3f} km/s'.format(r'${\rm v_{bary}}$', barycorr),
                fontsize='x-small')
    infoy = infoy - dy
    fig_p1.text(infox, infoy, '{:<}: {:>.3f} km/s'.format(r'${\rm v_{LSR}}$', vlsrcorr),
                fontsize='x-small')

    #Az/El
    agpu_p_array (fp1_a5, bt1d['azimuth'], bt1d['elevatio'], 
                  xlabel='Azimuth [deg]', ylabel='Elevation [deg]',
                  firstpt=True)

#        fig_p1 = plt.figure (constrained_layout=True)
#        p1_gs  = fig_p1.add_gridspec (3, 3)
#    fp1_a3 = fig_p1.add_subplot(p1_gs[0,2])
#    fp1_a4 = fig_p1.add_subplot(p1_gs[1,2])

##    # RA,Dec
##    agpu_p_array (fp1_a4, bt1d['crval2'], bt1d['crval3'], 
##                  xlabel=r'$\alpha [deg]$', ylabel=r'$\delta [deg]$', 
##                  firstpt=True)

##    # UT, V_barycenter
##    agpu_p_array (fp1_a5, bt1_dt, bt1d['barycorr'], 
##                  xlabel='UT', ylabel=r'$v_{\rm bary}$',
##                  xrot=70.)

    # plot one spectrum from near the cube middle
    i = int(maxspec/2)
    farr = gpu_comp_freqs(bt1d['crval1'][i], bt1d['crpix1'][i], 
                          bt1d['cdelt1'][i], bt1d['spectrum'][i])
    
    xlab = 'Freq [GHz]'
    ylab = 'Power'
    tlab = 'Spectrum {}'.format(i)
    # ylab=r'$\log_{10}$[Power]', 
    agpu_p_array (fp1_a1,
                  farr, bt1d['spectrum'][i], xlabel=xlab,
                  xmin=farr[-1], xmax=farr[0],
##                  ymin=0.02, ymax=1,
##                  ymin=0.01, ymax=3162,
                  ylabel=ylab, title=tlab, ylog=True)

    # array image of all spectra
    ylab = 'Spectrum'
    farr = gpu_comp_freqs(bt1d['crval1'][0], bt1d['crpix1'][0], 
                          bt1d['cdelt1'][0], bt1d['spectrum'][0])
    nsp = np.arange(0,maxspec,1)
    lgs = np.log10(bt1d['spectrum'])
    print (lgs.min(), lgs.max())
    # originally used pcolormesh(), but pcolorfast is ~4x faster for this
    tp = fp1_a2.pcolorfast(farr, nsp, lgs,
##                           norm=clrs.Normalize(vmin=-1.7, vmax=0))
##                           norm=clrs.Normalize(vmin=-2, vmax=3.5))
                           norm=clrs.Normalize().autoscale(lgs))
    
    _yp = farr * 0.0 + maxspec/2
    fp1_a2.plot(farr, _yp, color='red')

    fp1_a2.tick_params(labelsize='x-small', direction='in', 
                       top='on', left='on', bottom='off', right='off',
                       labelleft=True, labeltop=True, labelbottom=False)
    # fp1_a2.set_xlabel (xlab, fontsize='x-small')
    fp1_a2.set_ylabel (ylab, fontsize='x-small')

    cfpa = fig_p1.colorbar(tp, ax=fp1_a2, orientation='horizontal', 
                           shrink=0.5)
    cfpa.ax.tick_params(labelsize='x-small')
    cfpa.ax.set_xlabel(r'$\log_{10}$[Power]', fontsize='x-small')

    # Save output file
    plt.savefig (filename.replace('fits','png'), orientation='portrait')

    # display (if desired)
    if (disp == True):
        plt.show()

    return bt1h, bt1d, bt1_dt

#------------------------------------------------------------------------
# Execute as script

if __name__ == '__main__':
    import sys

    # Pull in the command line
    # expect argv[0] == script, [1] == output file name
    argv = sys.argv
    argc = len(argv)

    bt1h, bt1d, bt1_dt = gpu_one_page(filename=argv[1])
    maxspec = bt1h['naxis2']

    # option to look at individual spectra
    odyn = 'y'
    for i in range (maxspec):
        break

    # the break skips everything below here. The following was
    # put in for debugging and quick look access to the individual
    # spectra in the FITS file.
    
        prompt = 'Display Spectrum (y,n,q,#: 0-{}) {}? '.format(maxspec-1, 
                                                                    i)

        dyn = str(raw_input(prompt))
        if (len(dyn) == 0):
            dyn = odyn
            
        if ((dyn.lower()[0] == 'n') or (dyn.lower()[0] == 'q')):
            break

        elif (dyn.isdigit() == True):
            k = int(dyn)
            if (k < maxspec):
                i = k
                dyn = 'y'

            else:
                break

        odyn = dyn

        farr = gpu_comp_freqs(bt1d['crval1'][i], bt1d['crpix1'][i], 
                              bt1d['cdelt1'][i], bt1d['spectrum'][i])
        
        xlab = 'Freq [GHz]'
        tlab = 'Spectrum {}'.format(i)
        gpu_p_array (farr, bt1d['spectrum'][i], xlabel=xlab,
                     ylabel=r'$\log_{10}$[Power]', title=tlab, ylog=True)

#        gpu_p_array (bt1d['azimuth'], bt1d['elevatio'], xlabel='Azimuth',
#                     ylabel='Elevation', ylog=False)
#
#        # RA,Dec
#        gpu_p_array (bt1d['crval2'], bt1d['crval3'], xlabel=r'$\alpha$',
#                     ylabel=r'$\delta$', ylog=False)
#
#        # UT, V_barycenter
#        gpu_p_array (bt1_dt, bt1d['barycorr'], xlabel='UT',
#                     ylabel=r'$v_{\rm bary}$', ylog=False)


## tp = plt.pcolormesh(farr, nsp, bt1d['spectrum'], 
##                norm=clrs.LogNorm().autoscale(bt1d['spectrum']))
        
#    ti = plt.imshow(lgs, norm=clrs.Normalize().autoscale(lgs), aspect='auto')
#    plt.colorbar(ti)
#    plt.show()

