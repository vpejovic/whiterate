#!/usr/bin/env python
#
# Copyright 2005, 2006 Free Software Foundation, Inc.
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

from gnuradio import gr, blks2
from math import sqrt
#from Numeric import add
from numpy.oldnumeric import *
from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import flex_ofdm_code as flex_ofdm

import time, struct, sys, threading

# from current dir
import transmit_path
import usrp_transmit_path
import receive_path


class my_top_block(gr.top_block):
	def __init__(self, callback, options):
		gr.top_block.__init__(self)
		packet_len = 12160 
		options.sender = True
		self.txpath = transmit_path.transmit_path(options)
		options.sender = False
		self.rxpath = receive_path.receive_path(callback, options)		
		self.tovector_1 = gr.stream_to_vector(gr.sizeof_gr_complex, packet_len)
		self.tovector_2 = gr.stream_to_vector(gr.sizeof_gr_complex, packet_len)
		self.tostream = gr.vector_to_stream(gr.sizeof_gr_complex, packet_len)
		self.interleaver = gr.interleave(gr.sizeof_gr_complex*packet_len)
		#self.noise = gr.noise_source_c(gr.GR_GAUSSIAN, options.noise, 1002)
		self.file_sink = gr.file_sink(gr.sizeof_gr_complex, "transmitted.dat")
		#self.file_source = gr.file_source(gr.sizeof_gr_complex, "transmitted.dat", True)
		self.null_source = gr.null_source(gr.sizeof_gr_complex)
		#self.throttle = gr.throttle(gr.sizeof_gr_complex, 500000)
		#self.add = gr.add_cc()

		self.connect(self.txpath, self.tovector_1, (self.interleaver, 0))
		self.connect(self.null_source, self.tovector_2, (self.interleaver, 1))
		self.connect(self.interleaver, self.tostream)
		self.connect(self.tostream, self.file_sink)
		self.connect(self.tostream, self.rxpath)
		#self.connect(self.txpath, self.rxpath)
		#self.connect(self.file_source, self.rxpath)
		#self.connect(self.txpath, self.file_sink)
	
	def reset_ofdm_params(self, new_tone_map, new_modulation):
		self.txpath.reset_ofdm_params(new_tone_map, new_modulation)
		self.rxpath.reset_ofdm_params(new_tone_map)
		
	def add_options(normal, expert):
		normal.add_option("", "--noise", type="eng_float", default=5, help="noise amplitude [default=%default]")
	
	add_options = staticmethod(add_options)
 
# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////

def main():

	global n_rcvd, n_right, acked
	n_rcvd = 0
	n_right = 0
	acked = threading.Event()

	def send_pkt(payload='', eof=False, seqno=0):
		return tb.txpath.send_pkt(payload, eof, seqno)

	def rx_callback(ok, payload, int_fo, frac_fo, time_sinr, freq_sinr, ch_gain, avg_gain):
		global n_rcvd, n_right
		n_rcvd += 1
		try:
			(pktno,) = struct.unpack('!H', payload[0:2])
		except:
			pktno = 1
		if ok:
			n_right += 1
		print "ok: %r \t pktno: %d \t n_rcvd: %d \t n_right: %d" % (ok, pktno, n_rcvd, n_right)
		freq_offset = int_fo+frac_fo/math.pi
		print "freq offset: %+.2f(subcarriers) \t SINR: %.2f(time domain), %.2f(freq domain)" % (freq_offset, time_sinr, freq_sinr)
		#print "Channel gain: "+str(ch_gain)
		#print "Average gain: "+str(avg_gain)

		if 1:
			printlst = list()
			for x in payload[2:]:
				t = hex(ord(x)).replace('0x', '')
				if(len(t) == 1):
					t = '0' + t
				printlst.append(t)
			printable = ''.join(printlst)

			print printable
			print "\n"
		acked.set()
		
	parser = OptionParser(option_class=eng_option, conflict_handler="resolve")
	expert_grp = parser.add_option_group("Expert")
	parser.add_option ("-t", "--runtime", type=int, default=4)
	parser.add_option("-s", "--size", type="intx", default=95, help="set packet size [default=%default]")
	parser.add_option("-p", "--spacing", type="eng_float", default=1, help="set packet spacing in time [default=%default]")
	parser.add_option("-r", "--sample_rate", type="eng_float", default=500000, help="limit sample rate to RATE in throttle (%default)") 
	parser.add_option("","--discontinuous", action="store_true", default=False, help="enable discontinuous mode")
	parser.add_option("", "--snr", type="eng_float", default=30, help="set the SNR of the channel in dB [default=%default]")
	my_top_block.add_options(parser, expert_grp)
	transmit_path.transmit_path.add_options(parser, expert_grp)
	receive_path.receive_path.add_options(parser, expert_grp)
	usrp_transmit_path.add_options(parser,expert_grp)
	flex_ofdm.ofdm_mod.add_options(parser, expert_grp)
	flex_ofdm.ofdm_demod.add_options(parser, expert_grp)

	(options, args) = parser.parse_args ()
	
	# build the graph
	tb = my_top_block(rx_callback, options)

	r = gr.enable_realtime_scheduling()
	if r != gr.RT_OK:
		print "Warning: failed to enable realtime scheduling"

	tb.start()                       # start flow graph

	start_time = time.time()

	# generate and send infinite stream of packets, will be
	# interrupted by runtime
	n = 0
	pktno = 0
	pkt_size = int(options.size)

	#while time.time() < start_time + options.runtime:
	while pktno < options.runtime:
		send_pkt(pkt_size * chr(0x05), eof=False, seqno=pktno)
		n += pkt_size
		#sys.stderr.write('.')
		pktno += 1
		print "sent %d packets" % (pktno,)
		#time.sleep(0.1)
		acked.wait(3)
		acked.clear()
		time.sleep(options.spacing)
		#acked.wait()
		if (pktno == 110):
			options.tone_map = "1fffffffffffe7fffffffffff8"
			tb.reset_ofdm_params(options.tone_map, "bpsk")

	send_pkt(eof=True)
	print "\nfinishing: sent %d packets" % (pktno,)
	#raise SystemExit
	tb.wait()                       # wait for it to finish
if __name__ == '__main__':
	try:
		main()
	except KeyboardInterrupt:
		print "Bye"
		pass
