#
# Copyright 2005,2006 Free Software Foundation, Inc.
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
from gnuradio import gr
from gnuradio import eng_notation
import time, sys
import settings

#_valid_MCS  = ['8psk']
#_valid_MCS  = ['bpsk', 'qpsk', 'qam8', 'qam16', 'qam64', 'qam256']
#_bits_per_tone  = {'bpsk': 1, 'qpsk': 2, 'qam8': 3, 'qam16': 4, 'qam64': 6, 'qam256': 8, '8psk': 3}
_valid_MCS  = ['bpsk', 'qpsk', 'qam8']
_bits_per_tone  = {'bpsk': 1, 'qpsk': 2, 'qam8': 3}
#_bits_per_tone  = {'qam8': 3}
_pkt_size_bins = [250, 1000, 3000]
_max_width = 436
_min_width = 164
_width_delta = 8
_stale_timeout = 500
N0	= 0x0000  # warm up
N1	= 0x0001  # evaluate each
N2	= 0x0002  # best rate

"""
Optimal width/rate combination

best_rate:
    The best rate is the one with the lowest average Tx time. 
    Do not use rates that have more than MAX_CONSECUTIVE_FAILURES.

pick_sample:
    Go iteratively over all rates/widths

find_rate:
    Do best_rate first, then pick_sample
    Pick the best rate.
    Set time when pick_sample was called last time, refresh table after some time.	

remove_stale: 
    Clear all info.	
"""

def _size_to_bin(pkt_size):
    for i in range(len(_pkt_size_bins)):
	if _pkt_size_bins[i] > pkt_size:
	    return i

class optimal_rate(object):	
    global _iteration_cnt
    _iteration_cnt = 0

    def __init__(self, options):
	self.time_per_subcarrier = float(options.interp*options.fft_length)
	self.table = {}

	for size_bin in range(len(_pkt_size_bins)):
	    self.table[size_bin] = {}
	    self.table[size_bin]["last_update"] = 0	
	    self.table[size_bin]["all_tested"] = False
	    for candidate_MCS in _valid_MCS:
	        self.table[size_bin][candidate_MCS] = {}	    
	        for candidate_width in range(_min_width, _max_width+_width_delta, _width_delta):
	    	    self.table[size_bin][candidate_MCS][candidate_width] = {}	    	   
	    	    self.table[size_bin][candidate_MCS][candidate_width]["tries"] = 0
	            self.table[size_bin][candidate_MCS][candidate_width]["packets_acked"] = 0
	    	    self.table[size_bin][candidate_MCS][candidate_width]["succ_fails"] = 0
	    	    self.table[size_bin][candidate_MCS][candidate_width]["loss"] = -1
	    	    self.table[size_bin][candidate_MCS][candidate_width]["total_tx_time"] = -1
	    	    self.table[size_bin][candidate_MCS][candidate_width]["average_tx_time"] = -1
	    	    self.table[size_bin][candidate_MCS][candidate_width]["lossless_tx_time"] = self.time_per_subcarrier/(_bits_per_tone[candidate_MCS]*candidate_width)

	self.max_loss = options.max_loss
	self.margin_loss = options.margin_loss

    def update_table(self, pkt_size, current_mcs, current_width, num_sent, num_acked):
	_size_bin = _size_to_bin(pkt_size)
	_current_time = time.time()
	self.table[_size_bin]["last_update"] = time.time()
	self.table[_size_bin][current_mcs][current_width]["tries"] = num_sent
	self.table[_size_bin][current_mcs][current_width]["packets_acked"] = num_acked
	if num_acked == 0:
	     self.table[_size_bin][current_mcs][current_width]["succ_fails"] = num_sent
	self.table[_size_bin][current_mcs][current_width]["loss"] = 1 - self.table[_size_bin][current_mcs][current_width]["packets_acked"] / float(self.table[_size_bin][current_mcs][current_width]["tries"])
	self.table[_size_bin][current_mcs][current_width]["total_tx_time"] = self.table[_size_bin][current_mcs][current_width]["tries"] * self.table[_size_bin][current_mcs][current_width]["lossless_tx_time"]

	if self.table[_size_bin][current_mcs][current_width]["packets_acked"] > 0:
	    self.table[_size_bin][current_mcs][current_width]["average_tx_time"] = self.table[_size_bin][current_mcs][current_width]["total_tx_time"]/float(self.table[_size_bin][current_mcs][current_width]["packets_acked"])
	else:
	    self.table[_size_bin][current_mcs][current_width]["average_tx_time"] = sys.maxint

	settings.ts_print("RATE | table update for " + current_mcs + " " + str(current_width) +" to "+str(self.table[_size_bin][current_mcs][current_width]["loss"]), time.time())

    """
    Test all combinations (N1 state) and then pick the right one (N2 state).
    """	
    def find_rate(self, pkt_size, current_mcs, current_width, current_state):
	global _iteration_cnt

	_size_bin = _size_to_bin(pkt_size)
	_current_time = time.time()
	
	if _current_time - self.table[_size_bin]["last_update"] > _stale_timeout:
	    self._remove_stale(pkt_size)
	    current_mcs = _valid_MCS[len(_valid_MCS)-1]
	    current_width = _max_width
	    return current_mcs, current_width	
	
	if not self.table[_size_bin]["all_tested"]:  		    
	    _index = _valid_MCS.index(current_mcs)
	    if _index > 0:	
		return _valid_MCS[_index - 1], current_width
	    else:
		if current_width - _width_delta > _min_width:
		    return _valid_MCS[len(_valid_MCS) - 1], current_width - _width_delta
		elif current_width - _width_delta == _min_width:
		    self.table[_size_bin]["all_tested"] = True
		    return current_mcs, current_width - _width_delta	
		else:
		    print "RATE | Shouldn't go here!"
		    sys.exit(1)	
	#everything tested, go one more round:	
	print "RATE | ITERATION FINISHED"
	_iteration_cnt = _iteration_cnt + 1
	if _iteration_cnt > 9:
	    sys.exit(1)
	self.print_table()
	self._remove_stale(pkt_size)
	return _valid_MCS[len(_valid_MCS) - 1], _max_width

	#return self._best_rate(pkt_size)	
    
    def _best_rate(self, pkt_size):
	_size_bin = _size_to_bin(pkt_size)
	_best_mcs = "bpsk"
	_best_width = _min_width
	for candidate_MCS in _valid_MCS:
	    for candidate_width in range(_min_width, _max_width+_width_delta, _width_delta):
	        if self.table[_size_bin][candidate_MCS][candidate_width]["loss"] < self.max_loss:
		    if self.table[_size_bin][candidate_MCS][candidate_width]["average_tx_time"] < self.table[_size_bin][_best_mcs][_best_width]["average_tx_time"]:
 			_best_width = candidate_width
			_best_mcs = candidate_MCS
	return _best_mcs, _best_width	

    def _remove_stale(self, pkt_size):
	_current_time = time.time()
	_size_bin = _size_to_bin(pkt_size)
	self.table[_size_bin]["last_update"] = _current_time
	self.table[_size_bin]["all_tested"] = False
	for candidate_MCS in _valid_MCS:
	    for candidate_width in range(_min_width, _max_width+_width_delta, _width_delta):    	   
	    	self.table[_size_bin][candidate_MCS][candidate_width]["tries"] = 0
	        self.table[_size_bin][candidate_MCS][candidate_width]["packets_acked"] = 0
	    	self.table[_size_bin][candidate_MCS][candidate_width]["succ_fails"] = 0
	    	self.table[_size_bin][candidate_MCS][candidate_width]["loss"] = 0
	    	self.table[_size_bin][candidate_MCS][candidate_width]["total_tx_time"] = 0
	    	self.table[_size_bin][candidate_MCS][candidate_width]["average_tx_time"] = 0

    def print_table(self):
	for size_bin in range(len(_pkt_size_bins)):
	    print 'TABLE for bin %d ' % size_bin
	    for candidate_width in range(_min_width, _max_width+_width_delta, _width_delta):
		print 'WIDTH %d ' % candidate_width
	        print 'RATE\tTRIES\tACKED\tSUCC_FAILS\tLOSS\tTOTAL_TX_TIME\tAVG_TX_TIME\tLOSSLESS_TX_TIME\tLAST_UPDATE'
 	        for candidate_MCS in _valid_MCS:
		    print '| %06s \t%d\t%d\t%d\t%.2f\t%.3f\t%.3f\t%.3f\t%.3f |' % (candidate_MCS, self.table[size_bin][candidate_MCS][candidate_width]["tries"], \
		    self.table[size_bin][candidate_MCS][candidate_width]["packets_acked"],self.table[size_bin][candidate_MCS][candidate_width]["succ_fails"], \
		    self.table[size_bin][candidate_MCS][candidate_width]["loss"], self.table[size_bin][candidate_MCS][candidate_width]["total_tx_time"], \
		    self.table[size_bin][candidate_MCS][candidate_width]["average_tx_time"], self.table[size_bin][candidate_MCS][candidate_width]["lossless_tx_time"], \
		    self.table[size_bin]["last_update"])		

def add_options(normal, expert):
    """
    Adds WhiteRate-specific options to the Options Parser
    """
    normal.add_option("", "--max-loss", type="eng_float", default=0.3, metavar="LOSS",
                          help="set maximum packet error rate that the app. tolerates: 0 <= LOSS < 1 [default=%default]")        
    normal.add_option("", "--margin-loss", type="eng_float", default=0.05, metavar="MLOSS",
                          help="set margin of the loss, the algorithm tries to keep the rate in a bracket from MLOSS to LOSS: 0 <= MLOSS < 1 [default=%default]")   
