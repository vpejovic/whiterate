#!/usr/bin/env python
#
# WhiteRate algorithm
# TODO: add weighted window approach
 
from collections import defaultdict
import sys, math
from random import choice

# seconds per sample for a single subcarrier
_time_per_subcarrier = 0.001 
_C = 4	# successive fails tolerated
_L = 7	# NOTE: not used in this version of the algorithm
_P = 10 # probing happens every _P packets

# NOTE: MCS are coded with 1/2 rate unless followed by ++ in which case 3/4 code is used
_valid_mcs  = ['bpsk',  'qpsk', 'qam8', 'qam16', 'qam64']
_coding_rate = {'bpsk':2,  'qpsk':2, 'qam8':2, 'qam16':2, 'qam64':2}
_bits_per_tone = {'bpsk':1, 'qpsk':2, 'qam8':3, 'qam16':4, 'qam64':6}
_INIT = 1
_NORMAL = 2
_PROBING = 3
_refresh_period = 10000

class state_table(object):
	def __init__(self, max_loss, num_groups, group_width):#, loss_table):
		self.table = {}
	
		# contains mcs/width PERs. Ordered by decreasing mcss, widths
		#self.loss = loss_table
		self.probe_cnt = 0
		self.state = _INIT
		self.max_loss = max_loss
		self.margin_loss = max_loss*0.3	# TODO: how to select the margin?
		self.weight = 0.1	
		self._max_width = num_groups*group_width
		self._min_width = group_width
		self._delta_width = group_width
		self.total_updates = 0
		self._tested = False
		
		for candidate_mcs in _valid_mcs:
			self.table[candidate_mcs]= defaultdict(int)
			for candidate_width in range (self._min_width, self._max_width + self._delta_width, self._delta_width):
				self.table[candidate_mcs][candidate_width] = defaultdict(int)
				self.table[candidate_mcs][candidate_width]["succ_fails"] = 0
				self.table[candidate_mcs][candidate_width]["loss"] = -1
				self.table[candidate_mcs][candidate_width]["tries"] = 0
				self.table[candidate_mcs][candidate_width]["packets_acked"] = 0
				self.table[candidate_mcs][candidate_width]["total_tx_time"] = 0
				self.table[candidate_mcs][candidate_width]["average_tx_time"] = 0	
				self.table[candidate_mcs][candidate_width]["lossless_tx_time"] = _coding_rate[candidate_mcs]*float(_time_per_subcarrier)/(_bits_per_tone[candidate_mcs]*candidate_width)
				self.table[candidate_mcs]["last_succ_fail"] = 0
		
	def update_table(self, current_mcs, current_width, ok):
		self.total_updates += 1
		self.table[current_mcs][current_width]["tries"] += 1
		if int(ok) == 0:
			self.table[current_mcs][current_width]["succ_fails"] += 1
			self.table[current_mcs][current_width]["last_succ_fail"] = self.total_updates
		else:
			self.table[current_mcs][current_width]["succ_fails"] = 0
			self.table[current_mcs][current_width]["packets_acked"] += 1	

		self.table[current_mcs][current_width]["loss"] =  1 - self.table[current_mcs][current_width]["packets_acked"]/float(self.table[current_mcs][current_width]["tries"])
		self.table[current_mcs][current_width]["average_tx_time"] = (1+self.table[current_mcs][current_width]["loss"])*_coding_rate[current_mcs]*float(_time_per_subcarrier)/(_bits_per_tone[current_mcs]*current_width)
		
		
	def find_rate(self, current_mcs, current_width, current_state):
		if not self._tested:	
			_index = _valid_mcs.index(current_mcs)

			if self.table[current_mcs][current_width]["tries"] < 100:
				return current_mcs, current_width
			if current_width - self._delta_width >= self._min_width:
				return current_mcs, current_width - self._delta_width
			else:
				if _index < 5:
					return _valid_mcs[_index + 1], 462
				else:
					return "bpsk", 462
		else:
			print "RATE | Shouldn't go here!"
			sys.exit(1)	

				
	def print_table(self):
		print 'RATE\tTRIES\tACKED\tSUCC_FAILS\tLOSS\tWIDTH\tTOTAL_TX_TIME\tAVG_TX_TIME\tLOSSLESS_TX_TIME'
		for candidate_mcs in _valid_mcs:
			ss = candidate_mcs
			for candidate_width in range (self._min_width, self._max_width + self._delta_width, self._delta_width):
				ss += ("\t"+str(self.table[candidate_mcs][candidate_width]["loss"]))
			print ss+"\n"

if __name__ == "__main__":
	t = packet_table()
	t.read_profile("stats_174.txt" ,174, False)
	t.read_profile("stats_222.txt", 222, False)
	t.read_profile("stats_270.txt", 270, False)
	t.read_profile("stats_318.txt" ,318, False)
	t.read_profile("stats_366.txt", 366, False)
	t.read_profile("stats_414.txt", 414, False)
	t.read_profile("stats_462.txt", 462, False)
	t.print_table()
