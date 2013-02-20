#
# Copyright 2005, 2006, 2007 Free Software Foundation, Inc.
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

from gnuradio import gr, gru
from gnuradio import eng_notation

import copy
import sys
import flex_ofdm_code
# /////////////////////////////////////////////////////////////////////////////
#                              transmit path
# /////////////////////////////////////////////////////////////////////////////

class transmit_path(gr.hier_block2):
    def __init__(self, options):
        gr.hier_block2.__init__(self, "transmit_path",
                        gr.io_signature(0, 0, 0),                    # Input signature
                        gr.io_signature(1, 1, gr.sizeof_gr_complex)) # Output signature

        options = copy.copy(options)    # make a copy so we can destructively modify

        self._verbose            = options.verbose
        self._tx_amplitude       = options.tx_amplitude    # digital amplitude sent to USRP

        self.ofdm_tx = flex_ofdm_code.ofdm_mod(options, msgq_limit=1, pad_for_usrp=False)
        self.amp = gr.multiply_const_cc(1)
        self.set_tx_amplitude(self._tx_amplitude)

        if self._verbose:
            self._print_verbage()

        self.connect(self.ofdm_tx, self.amp, self)

    def set_tx_amplitude(self, ampl):
        self._tx_amplitude = max(0.0, min(ampl, 1))
        self.amp.set_k(self._tx_amplitude)

    def reset_ofdm_params(self, new_tone_map, new_modulation):
        return self.ofdm_tx.reset_ofdm_params(new_tone_map, new_modulation)

    def reset_ofdm_params_groups(self, new_width_map, new_modulation):
        return self.ofdm_tx.reset_ofdm_params_groups(new_width_map, new_modulation)

    def send_pkt(self, payload='', eof=False, seqno=0):
        return self.ofdm_tx.send_pkt(payload, eof, seqno)

    def add_options(normal, expert):
        normal.add_option("", "--tx-amplitude", type="eng_float", default=0.250, metavar="AMPL", help="set transmitter digital amplitude: 0 <= AMPL < 1 [default=%default]")
        expert.add_option("", "--log", action="store_true", default=False, help="Log all parts of flow graph to file (CAUTION: lots of data)")

    # Make a static method to call before instantiation
    add_options = staticmethod(add_options)

    def _print_verbage(self):
        print "Tx amplitude     %s"    % (self._tx_amplitude)
