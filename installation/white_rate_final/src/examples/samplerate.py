#!/usr/bin/env python
#
# SampleRate algorithm

from collections import defaultdict
import sys, math
from random import choice

# seconds per sample for a single subcarrier
_time_per_subcarrier = 0.001 #second (200inter*512subcarriers)/100000000MHz  
_C = 4 # successive fails tolerated
_P = 10 # probing happens every _P packets

# NOTE: MCS are coded with 1/2 rate unless followed by ++ in which case 3/4 code is used
_valid_mcs  = ['bpsk',  'qpsk', 'qam8', 'qam16', 'qam64']
_coding_rate = {'bpsk':2,  'qpsk':2, 'qam8':2, 'qam16':2, 'qam64':2}
_bits_per_tone = {'bpsk':1, 'qpsk':2, 'qam8':3, 'qam16':4, 'qam64':6}
_INIT = 1
_NORMAL = 2
_PROBING = 3
_max_width = 174 

class state_table(object):
	def __init__(self, max_loss):
		self.table = {}
		self.state = _INIT 
		for candidate_mcs in _valid_mcs:
			self.table[candidate_mcs]= defaultdict(int)
			self.table[candidate_mcs]["succ_fails"] = 0
			self.table[candidate_mcs]["loss"] = -1
			self.table[candidate_mcs]["tries"] = 0
			self.table[candidate_mcs]["packets_acked"] = 0
			self.table[candidate_mcs]["total_tx_time"] = 0
			self.table[candidate_mcs]["average_tx_time"] = 0	
			self.table[candidate_mcs]["lossless_tx_time"] = _coding_rate[candidate_mcs]*float(_time_per_subcarrier)/(_bits_per_tone[candidate_mcs]*_max_width)		
	
	def update_table(self, current_mcs, current_width, ok):
		self.table[current_mcs]["tries"] += 1
		if int(ok) == 0:
			self.table[current_mcs]["succ_fails"] += 1
		else:
			self.table[current_mcs]["succ_fails"] = 0
			self.table[current_mcs]["packets_acked"] += 1	
			
		if self.table[current_mcs]["succ_fails"] == _C:
			self.table[current_mcs]["average_tx_time"] = sys.maxint	  
			
		self.table[current_mcs]["total_tx_time"] = self.table[current_mcs]["tries"]*_coding_rate[current_mcs]*float(_time_per_subcarrier)/(_bits_per_tone[current_mcs]*current_width)
		
		if self.table[current_mcs]["packets_acked"] > 0:
			self.table[current_mcs]["average_tx_time"] = self.table[current_mcs]["total_tx_time"]/float(self.table[current_mcs]["packets_acked"])
		#else:
			#self.table[current_mcs]["average_tx_time"] = sys.maxint
			#self.table[current_mcs]["loss"] = 1 - float(self.table[current_mcs]["packets_acked"])/self.table[current_mcs]["tries"]
			#self.table[current_mcs]["tries"] = 0
			#self.table[current_mcs]["packets_acked"] = 0
			#self.table[current_mcs]["locked"] = False
			#print "RESET TRIES FOR "+current_mcs
			
	def best_rate(self, current_mcs, current_width):
		_candidates = []
		for candidate_mcs in _valid_mcs:
			if self.table[candidate_mcs]["succ_fails"]<_C and self.table[candidate_mcs]["average_tx_time"]>0 and self.table[candidate_mcs]["average_tx_time"]!=sys.maxint:
				index = len(_candidates)
				for i in range(len(_candidates)):
					if self.table[_candidates[i]]["average_tx_time"] > self.table[candidate_mcs]["average_tx_time"]:
						index = i - 1
						break
				_candidates.insert(index, candidate_mcs)
		
		if _candidates:
			return _candidates[0], _max_width			
		else:
			return current_mcs, current_width
	
	def pick_probe(self, current_mcs, current_width):
		# that hasn't experienced c consecutive losses and can result in better performance		
		_candidates = []	
		for candidate_mcs in _valid_mcs:
			if (self.table[candidate_mcs]["lossless_tx_time"] < self.table[current_mcs]["average_tx_time"]) and (self.table[candidate_mcs]["succ_fails"] < _C):
				_candidates.append(candidate_mcs)
		if (_candidates):		
			probing_mcs = choice(_candidates)
		else:
			probing_mcs = current_mcs	
			
		return probing_mcs, _max_width
		
	def find_rate(self, current_mcs, current_width, current_state):

		if current_state == _INIT:
			return "qam64", _max_width

		if not (current_state == _PROBING):
			if self.table[current_mcs]["succ_fails"] >= _C:
				for candidate_mcs in reversed(_valid_mcs):
					if self.table[candidate_mcs]["succ_fails"] < _C:
						return candidate_mcs, current_width

			_new_mcs, _new_width = self.best_rate(current_mcs, current_width)
			#if _new_mcs == current_mcs:
			#	if self.table[current_mcs]["loss"] < self.max_loss:
			#		if _new_width + _delta_width < _max_width:
			#			_new_width = _new_width + _delta_width
			return _new_mcs, _new_width
		
		elif current_state == _PROBING:
			return self.pick_probe(current_mcs, current_width)		
					
				
	def print_table(self):
		print 'RATE\tTRIES\tACKED\tSUCC_FAILS\tLOSS\tTOTAL_TX_TIME\tAVG_TX_TIME\tLOSSLESS_TX_TIME'
		for candidate_mcs in _valid_mcs:
			print '| %06s \t%d\t%d\t%d\t%.2f\t%.3f\t%.3f\t%.3f|\n' % (candidate_mcs, self.table[candidate_mcs]["tries"], \
			self.table[candidate_mcs]["packets_acked"],self.table[candidate_mcs]["succ_fails"], \
			self.table[candidate_mcs]["loss"], self.table[candidate_mcs]["total_tx_time"], \
			self.table[candidate_mcs]["average_tx_time"], self.table[candidate_mcs]["lossless_tx_time"])


if __name__ == "__main__":
	t = state_table()
	t.read_profile("stats_174.txt" ,174)
	t.read_profile("stats_222.txt", 222)
	t.read_profile("stats_270.txt", 270)
	t.read_profile("stats_318.txt" ,318)
	t.read_profile("stats_366.txt", 366)
	t.read_profile("stats_414.txt", 414)
	t.read_profile("stats_462.txt", 462)
	t.print_table()
