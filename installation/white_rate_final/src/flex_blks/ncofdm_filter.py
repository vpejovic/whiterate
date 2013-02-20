#/usr/bin/env python
#
#copyright 2006, 2007, 2008 Free Software Foundation, Inc.
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

import math
import cmath 
from numpy import fft
from gnuradio import gr, ofdm_packet_utils

# Non-contigous filter for NC-OFDM.
class ncofdm_filt(gr.hier_block2):
    def __init__(self, fft_length, occupied_tones, carrier_map_bin):
        
        """
        Hierarchical block for receiving OFDM symbols.
        """
        gr.hier_block2.__init__(self, "ncofdm_filt",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature(1, 1, gr.sizeof_gr_complex)) # Input signature

        # fft length, e.g. 256
        self._fft_length = fft_length
        # the number of used subcarriers, e.g. 240
        self._occupied_tones = occupied_tones
        # a binary array indicates the used subcarriers
        self._carrier_map_bin = carrier_map_bin

        # setup filter banks
        self.chan_filt_low = gr.fft_filter_ccc(1,[1]) 
        self.chan_filt_high1 = gr.fft_filter_ccc(1,[1]) 
        self.chan_filt_high2 = gr.fft_filter_ccc(1,[1])
        self.chan_filt_high3 = gr.fft_filter_ccc(1,[1])
        self.chan_filt_high4 = gr.fft_filter_ccc(1,[1])
        self.chan_filt_high5 = gr.fft_filter_ccc(1,[1])
        
        # calculate the filter taps
        filt_num = self.calc_filter_taps(2, 0)
        

        # signals run into a serial of filters, one lowpass filter and 5 highpass filters
        self.connect(self, self.chan_filt_high1,
                           self.chan_filt_high2, self.chan_filt_high3,
                           self.chan_filt_high4, self.chan_filt_high5,
                           self.chan_filt_low, self) 


    def calc_filter_taps(self, bw_mrgn, freq_offset): # bandwith margin, frequency offset 
       
        """
        Calculate and set filter taps 
        """

        # calculate ncofdm filers, with the format of
        # [low_width, low_center, high_width_1, high_center_1, high_width_2, high_center_2, ...]
        filt_param = self.ncofdm_filter_param(self._occupied_tones, self._carrier_map_bin)
        # print "\nfilter_param: ", filt_param

        # filter bandwidth and transition bandwidth
        bw = (float(self._occupied_tones) / float(self._fft_length)) / 2.0
        tb = 1.0 / float(self._fft_length)

        # low-pass filter taps
        self._chan_coeffs_low_tmp = gr.firdes.low_pass (1.0,                    # gain
                                              1.0,                          # sampling rate
                                              bw*filt_param[0]+tb*bw_mrgn, # midpoint of trans. band
                                              tb,                           # width of trans. band
                                              gr.firdes.WIN_HAMMING)        # filter type
        self._chan_coeffs_low = ()
        for i in range(len(self._chan_coeffs_low_tmp)):
            self._chan_coeffs_low = self._chan_coeffs_low \
            + (self._chan_coeffs_low_tmp[i]*cmath.exp(1j*2*math.pi*(filt_param[1]+freq_offset)*i/float(self._fft_length)), )

        # high-pass filter taps
        filt_num = len(filt_param)/2 - 1
        # print "\nhigh-pass filter #:", high_pass_num

        self._chan_coeffs_high = ()
        for i in range(1, filt_num+1):
            self._chan_coeffs_high_i = gr.firdes.high_pass (1.0,                     # gain
                                                      1.0,                           # sampling rate
                                                      bw*filt_param[2*i]-tb*bw_mrgn, # midpoint of trans. band
                                                      tb,                            # width of trans. band
                                                      gr.firdes.WIN_HAMMING)         # filter type
            self._chan_coeffs_high_c = ()
            for j in range(1, len(self._chan_coeffs_high_i)):
                self._chan_coeffs_high_c = self._chan_coeffs_high_c \
                + (self._chan_coeffs_high_i[j]*cmath.exp(1j*2*math.pi*(filt_param[2*i+1]+freq_offset)*j/float(self._fft_length)), )
            self._chan_coeffs_high = self._chan_coeffs_high + (self._chan_coeffs_high_c, )

        # set filter taps 
        self.set_taps(filt_num)
 
    def ncofdm_filter_param(self, len, preamble):
        # calculate lowpass filter and highpass parameters(band and location) 
        param = []
        
        # search for the first used subcarrier
        for i in range(len):
            if abs(preamble[i]):
                low1=i
                break
        # search for the last used subcarrier
        for i in range(len):
            if abs(preamble[len-i-1]):
                low2=len-i
                break
        # save bandwidth and central frequency
        param.append((low2-low1)/float(len))
        param.append((low1+low2-len)/float(2.0)-0.5) # 0.5 is for center freq. compensation
        
        # check parameters for the highpass filters
        wfl = 1 # wait for lowside
        for i in range(low1+1,low2-5):
            # add new high pass only if more than 5 consecutive subcarriers are free
            flag = abs(preamble[i] | preamble[i+1] | preamble[i+2] | preamble[i+3] | preamble[i+4])
            if (flag == 0) & (wfl == 1) :
                wfl = 0
                high1=i
            elif (flag == 1) & (wfl == 0) :
                wfl = 1
                high2 =i+4
                param.append((high2-high1)/float(len))
                param.append((high1+high2-len)/float(2.0)-0.5)# 0.5 is for center freq. compensation
        return param

    def set_taps(self, filt_num):
        # set low-pass filter taps
        self.chan_filt_low.set_taps(self._chan_coeffs_low)
        
        # set high-pass filter blocks
        self.chan_filt_high1.set_taps([1])
        self.chan_filt_high2.set_taps([1])
        self.chan_filt_high3.set_taps([1])
        self.chan_filt_high4.set_taps([1])
        self.chan_filt_high5.set_taps([1])
        if filt_num >= 1 :
            self.chan_filt_high1.set_taps(self._chan_coeffs_high[0])
        if filt_num >= 2 :
            self.chan_filt_high2.set_taps(self._chan_coeffs_high[1])
        if filt_num >= 3 :
            self.chan_filt_high3.set_taps(self._chan_coeffs_high[2])
        if filt_num >= 4 :
            self.chan_filt_high4.set_taps(self._chan_coeffs_high[3])
        if filt_num >= 5 :
            self.chan_filt_high5.set_taps(self._chan_coeffs_high[4])

    def reset_tone_map(self, new_carrier_map_bin, new_occupied_tones):
        #print "Setting up new filter taps ..."
        # adaptively change the filter parameters when combined with channel sensing
        self._carrier_map_bin = new_carrier_map_bin
        self._occupied_tones = new_occupied_tones
        # recalc the filter taps
        filt_num = self.calc_filter_taps(2, 0) #bandwidth margin 1 sub-carrier, offset 0
       
    def freq_offset_comp(self, freq_offset):
        # recalc the filter taps
        #self.calc_filter_taps(0.2, freq_offset) #bandwidth margin 0 sub-carrier, frequency offset freq_offset
        self.calc_filter_taps(freq_offset, 0) #bandwidth margin 0 sub-carrier, frequency offset freq_offset

