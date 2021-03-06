#!/usr/bin/env python
#
# Copyright 2006, 2007, 2008 Free Software Foundation, Inc.
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
from numpy import fft
from gnuradio import gr, flex
from gnuradio.blks2impl.ofdm_sync_ml import ofdm_sync_ml
from gnuradio.blks2impl.ofdm_sync_pnac import ofdm_sync_pnac
from gnuradio.blks2impl.ofdm_sync_fixed import ofdm_sync_fixed

import flex_ofdm_sync_pn_custom as ofdm_sync_pn

from ncofdm_filter import ncofdm_filt

class ofdm_receiver(gr.hier_block2):
    def __init__(self, fft_length, cp_length, occupied_tones, snr, ks, carrier_map_bin, nc_filter, logging=False):
        gr.hier_block2.__init__(self, "ofdm_receiver",
                        gr.io_signature(1, 1, gr.sizeof_gr_complex), # Input signature
                        gr.io_signature4(4, 4, gr.sizeof_gr_complex*occupied_tones, gr.sizeof_char, gr.sizeof_char, gr.sizeof_float)) # Output signature

        self._fft_length = fft_length
        self._occupied_tones = occupied_tones
        self._cp_length = cp_length
        self._nc_filter = nc_filter
        self._carrier_map_bin = carrier_map_bin

        win = [1 for i in range(self._fft_length)]

        self.initialize(ks, self._carrier_map_bin)

        SYNC = "pn"
        if SYNC == "ml":
            nco_sensitivity = -1.0/fft_length                             # correct for fine frequency
            self.ofdm_sync = ofdm_sync_ml(fft_length, cp_length, snr, self._ks0time, logging)
        elif SYNC == "pn":
            nco_sensitivity = -2.0/fft_length                             # correct for fine frequency
            self.ofdm_sync = ofdm_sync_pn.ofdm_sync_pn(fft_length, cp_length, logging)
        elif SYNC == "pnac":
            nco_sensitivity = -2.0/fft_length                             # correct for fine frequency
            self.ofdm_sync = ofdm_sync_pnac(fft_length, cp_length, self._ks0time, logging)
        elif SYNC == "fixed":                                             # for testing only; do not user over the air
            self.chan_filt = gr.multiply_const_cc(1.0)                    # remove filter and filter delay for this
            nsymbols = 18                                                 # enter the number of symbols per packet
            freq_offset = 0.0                                             # if you use a frequency offset, enter it here
            nco_sensitivity = -2.0/fft_length                             # correct for fine frequency
            self.ofdm_sync = ofdm_sync_fixed(fft_length, cp_length, nsymbols, freq_offset, logging)

        self.reset_filter()


        # TODO: why? Create a delay line, linklab
        self.delay = gr.delay(gr.sizeof_gr_complex, fft_length)

        self.nco = gr.frequency_modulator_fc(nco_sensitivity)         # generate a signal proportional to frequency error of sync block
        self.sigmix = gr.multiply_cc()
        self.sampler = flex.ofdm_sampler(fft_length, fft_length+cp_length)
        self.fft_demod = gr.fft_vcc(fft_length, True, win, True)
        self.ofdm_frame_acq = flex.ofdm_frame_acquisition(self._occupied_tones, self._fft_length,
                                                                                                        self._cp_length, self._ks[0], 1)

        if self._nc_filter:
            print '\nMulti-band Filter Turned ON!'
            self.ncofdm_filt = ncofdm_filt(self._fft_length, self._occupied_tones, self._carrier_map_bin)
            self.connect(self, self.chan_filt, self.ncofdm_filt)
            self.connect(self.ncofdm_filt, self.ofdm_sync)             # into the synchronization alg.
            self.connect((self.ofdm_sync,0), self.nco, (self.sigmix,1))   # use sync freq. offset output to derotate input signal
            self.connect(self.ncofdm_filt, self.delay, (self.sigmix,0))                 # signal to be derotated
        else :
            print '\nMulti-band Filter Turned OFF!'
            self.connect(self, self.chan_filt)
            self.connect(self.chan_filt, self.ofdm_sync)             # into the synchronization alg.
            self.connect((self.ofdm_sync,0), self.nco, (self.sigmix,1))   # use sync freq. offset output to derotate input signal
            self.connect(self.chan_filt, self.delay, (self.sigmix,0))                 # signal to be derotated

        self.connect(self.sigmix, (self.sampler,0))                   # sample off timing signal detected in sync alg
        self.connect((self.ofdm_sync,1), (self.sampler,1))            # timing signal to sample at

        self.connect((self.sampler,0), self.fft_demod)                # send derotated sampled signal to FFT
        self.connect(self.fft_demod, (self.ofdm_frame_acq,0))         # find frame start and equalize signal
        # TODO: do we need a char delay for the timing signal?
        self.connect((self.sampler,1), (self.ofdm_frame_acq,1))       # send timing signal to signal frame start

        # TODO: reconnect properly
        self.connect((self.ofdm_frame_acq,0), (self,0))               # finished with fine/coarse freq correction,
        self.connect((self.ofdm_sync,1), gr.delay(gr.sizeof_char,1), (self,1))

        self.connect((self.ofdm_frame_acq,1), (self,2))               # frame and symbol timing, and equalization
        self.connect((self.ofdm_frame_acq,2), (self,3))               # snr estimates

        if logging:
            self.connect(self.chan_filt, gr.file_sink(gr.sizeof_gr_complex, "flex_ofdm_recv-chan_filt_c.dat"))
            self.connect(self.ncofdm_filt, gr.file_sink(gr.sizeof_gr_complex, "flex_ofdm_recv-ncofdm_filt_c.dat"))
            self.connect(self.nco, gr.file_sink(gr.sizeof_gr_complex, "flex_ofdm_recv-nco_c.dat"))
            self.connect(self.fft_demod, gr.file_sink(gr.sizeof_gr_complex*fft_length, "flex_ofdm_recv-fft_out_c.dat"))
            self.connect(self.ofdm_frame_acq, gr.file_sink(gr.sizeof_gr_complex*occupied_tones, "flex_ofdm_recv-frame_acq_data_c.dat"))
            self.connect((self.ofdm_frame_acq,3), gr.keep_one_in_n(gr.sizeof_float*occupied_tones, 32), gr.file_sink(gr.sizeof_float*occupied_tones, "flex_ofdm_recv-frame_acq_gain_f.dat"))
            self.connect((self.ofdm_frame_acq,1), gr.file_sink(1, "flex_ofdm_recv-frame_acq_signal_b.dat"))
            self.connect((self.ofdm_frame_acq,2), gr.file_sink(gr.sizeof_float, "flex_ofdm_recv-frame_acq_snr_f.dat"))
            self.connect(self.sampler, gr.file_sink(gr.sizeof_gr_complex*fft_length, "flex_ofdm_recv-sampler_data_c.dat"))
            self.connect(self.sampler, gr.file_sink(gr.sizeof_gr_complex*fft_length, "flex_ofdm_recv-sampler_data_c.dat"))
            self.connect((self.sampler, 1), gr.file_sink(1*fft_length, "flex_ofdm_recv-sampler_signal_b.dat"))
            self.connect((self.ofdm_sync, 1), gr.file_sink(1, "flex_ofdm_recv-sync_b.dat"))
            self.connect(self.sigmix, gr.file_sink(gr.sizeof_gr_complex, "flex_ofdm_recv-sigmix_c.dat"))
        else:
            self.connect((self.ofdm_frame_acq,3), gr.null_sink(gr.sizeof_float*self._occupied_tones))

    def __del__(self):
        print "KILLING OFDM receiver block\n"

    def initialize(self, ks, carrier_map_bin):
        self._ks = ks
        self._zeros_on_left = int(math.ceil((self._fft_length - self._occupied_tones)/2.0))
        self._ks0 = self._fft_length*[0,]
        self._ks0[self._zeros_on_left : self._zeros_on_left + self._occupied_tones] = self._ks[0]

        self._ks0 = fft.ifftshift(self._ks0)
        self._ks0time = fft.ifft(self._ks0)

        self._ks0time = self._ks0time.tolist()

    def reset_ofdm_params(self, new_ks, new_carrier_map_bin):
        self._carrier_map_bin = new_carrier_map_bin
        self.initialize(new_ks, new_carrier_map_bin)
        self.reset_filter()
        self.ofdm_frame_acq.reset_known_symbol(new_ks[0])
        if self._nc_filter:
            self.ncofdm_filt.reset_tone_map(self._carrier_map_bin, self._occupied_tones)

    def reset_filter(self):
        bw = (float(self._occupied_tones) / float(self._fft_length)) / 2.0
        tb = bw*0.06
        chan_coeffs = gr.firdes.low_pass (1.0,                     # gain
                                          1.0,                     # sampling rate
                                          bw+tb,                   # midpoint of trans. band
                                          tb,                      # width of trans. band
                                          gr.firdes.WIN_HAMMING)   # filter type
        self.chan_filt = gr.fft_filter_ccc(1, chan_coeffs)

    def filter_fo_comp(self, freq_offset):
        if self._nc_filter:
            self.ncofdm_filt.freq_offset_comp(freq_offset)

    def get_freq_offset(self):
        int_freq =  self.ofdm_frame_acq.d_coarse_freq
        frac_freq = self.ofdm_sync.get_frac_fo()
        return int_freq, frac_freq

    def get_sinr(self):
        freq_sinr =  self.ofdm_frame_acq.d_sinr
        time_sinr = self.ofdm_sync.get_sinr()
        return time_sinr, freq_sinr

    def get_chgain(self):
        ch_gain = self.ofdm_frame_acq.d_ch
        return ch_gain

    def get_chgain_full(self):
        ch_gain_full = self.ofdm_frame_acq.gain_full()
        return ch_gain_full

    def get_avggain(self):
        avg_gain = self.ofdm_frame_acq.d_gain
        return avg_gain
