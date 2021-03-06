#
# Copyright 2009 Free Software Foundation, Inc.
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

from gnuradio import gr, flex
import usrp_options
import transmit_path
from gnuradio import eng_notation
import sys

def add_freq_option(parser):
    def freq_callback(option, opt_str, value, parser):
        parser.values.rx_freq = value
        parser.values.tx_freq = value

    if not parser.has_option('--freq'):
        parser.add_option('-f', '--freq', type="eng_float",
                                                action="callback", callback=freq_callback,
                                                help="set Tx and/or Rx frequency to FREQ [default=%default]",
                                                metavar="FREQ")

def add_options(parser, expert):
    add_freq_option(parser)
    usrp_options.add_tx_options(parser)
    transmit_path.transmit_path.add_options(parser, expert)
    expert.add_option("", "--tx-freq", type="eng_float", default=None, help="set transmit frequency to FREQ [default=%default]", metavar="FREQ")
    parser.add_option("-v", "--verbose", action="store_true", default=False)

class usrp_transmit_path(gr.hier_block2):
    def __init__(self, options):
        gr.hier_block2.__init__(self, "usrp_transmit_path",
                        gr.io_signature(0, 0, 0), # Input signature
                        gr.io_signature(0, 0, 0)) # Output signature
        if options.tx_freq is None:
            sys.stderr.write("-f FREQ or --freq FREQ or --tx-freq FREQ must be specified\n")
            raise SystemExit
        self.tx_path = transmit_path.transmit_path(options)
        for attr in dir(self.tx_path): #forward the methods
            if not attr.startswith('_') and not hasattr(self, attr):
                setattr(self, attr, getattr(self.tx_path, attr))

        self._setup_sink(options)
        self.mux = flex.null_mux(gr.sizeof_gr_complex, 0.0001)		
        self.connect(self.tx_path, self.mux, self.sink)
        #self.connect(self.tx_path, gr.file_sink(gr.sizeof_gr_complex, "tx_before_mux.dat"))
        #self.connect(self.mux, gr.file_sink(gr.sizeof_gr_complex, "tx_after_mux.dat"))
    def _setup_sink(self, options):
        self.sink = usrp_options.create_usrp_sink(options)
        dac_rate = self.sink.dac_rate()
        if options.verbose:
            print 'USRP Sink:', self.sink
        self._interp = options.interp
        self.sink.set_interp(self._interp)
        self._sample_rate = options.sample_rate
        self.sink.set_samp_rate(self._sample_rate)
        self.sink.set_auto_tr(True)
        self._bandwidth = options.bandwidth
        self.sink.set_bandwidth(self._bandwidth)
        if not self.sink.set_center_freq(options.tx_freq):
            print "Failed to set Tx frequency to %s" % (eng_notation.num_to_str(options.tx_freq))
            raise ValueError, eng_notation.num_to_str(options.tx_freq)
