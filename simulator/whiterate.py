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
_valid_mcs  = ['bpsk', 'bpsk++', 'qpsk', 'qpsk++', 'qam16', 'qam16++', 'qam64++']
_coding_rate = {'bpsk':2, 'bpsk++':1.25, 'qpsk':2, 'qpsk++':1.25, 'qam16':2, 'qam16++':1.25, 'qam64++':1.25}
_bits_per_tone = {'bpsk':1, 'bpsk++':1, 'qpsk':2, 'qpsk++':2, 'qam16':4, 'qam16++':4, 'qam64++':8}
_INIT = 1
_NORMAL = 2
_PROBING = 3
_max_width = 462
_min_width = 174
_delta_width = 48
_num_widths = 7

class state_table(object):
	def __init__(self, max_loss, loss_table):
		self.table = {}
	
		# contains mcs/width PERs. Ordered by decreasing mcss, widths
		self.loss = loss_table
		self.probe_cnt = 0
		self.state = _INIT
		self.max_loss = max_loss
		self.margin_loss = max_loss*0.3	# TODO: how to select the margin?
		for candidate_mcs in _valid_mcs:
			self.table[candidate_mcs]= defaultdict(int)
			self.table[candidate_mcs]["succ_fails"] = 0
			self.table[candidate_mcs]["loss"] = -1
			self.table[candidate_mcs]["tries"] = 0
			self.table[candidate_mcs]["packets_acked"] = 0
			self.table[candidate_mcs]["width"] = _max_width
			self.table[candidate_mcs]["total_tx_time"] = 0
			self.table[candidate_mcs]["average_tx_time"] = 0	
			self.table[candidate_mcs]["lossless_tx_time"] = _coding_rate[candidate_mcs]*float(_time_per_subcarrier)/(_bits_per_tone[candidate_mcs]*_max_width)
			# NOTE: lossless tx time should be time for that width
			self.table[candidate_mcs]["locked"] = True
		
	def update_table(self, current_mcs, current_width, ok):
		self.table[current_mcs]["width"] = current_width
		self.table[current_mcs]["tries"] += 1
		if int(ok) == 0:
			self.table[current_mcs]["succ_fails"] += 1
		else:
			self.table[current_mcs]["succ_fails"] = 0
			self.table[current_mcs]["packets_acked"] += 1	
			
		if self.table[current_mcs]["succ_fails"] == _C:
			self.table[current_mcs]["average_tx_time"] = sys.maxint	  
			
		self.table[current_mcs]["total_tx_time"] = self.table[current_mcs]["tries"]*_coding_rate[current_mcs]*float(_time_per_subcarrier)/(_bits_per_tone[current_mcs]*current_width)
		
		#if self.table[current_mcs]["tries"] == _L:
		if self.table[current_mcs]["packets_acked"] > 0:
			self.table[current_mcs]["average_tx_time"] = self.table[current_mcs]["total_tx_time"]/float(self.table[current_mcs]["packets_acked"])
		else:
			self.table[current_mcs]["average_tx_time"] = sys.maxint
		self.table[current_mcs]["loss"] = 1 - float(self.table[current_mcs]["packets_acked"])/self.table[current_mcs]["tries"]
		#self.table[current_mcs]["tries"] = 0
		#self.table[current_mcs]["packets_acked"] = 0
		#self.table[current_mcs]["locked"] = False
		#print "RESET TRIES FOR "+current_mcs
			
	def best_rate(self, current_mcs, current_width):
		_candidates = []
		_candidates_loss = []
		for candidate_mcs in _valid_mcs:
			if self.table[candidate_mcs]["succ_fails"]<_C and self.table[candidate_mcs]["average_tx_time"]>0 and self.table[candidate_mcs]["average_tx_time"]!=sys.maxint:
				index = len(_candidates)
				for i in range(len(_candidates)):
					if self.table[_candidates[i]]["loss"] > self.table[candidate_mcs]["loss"]:
						index = i - 1
						break
				_candidates.insert(index, candidate_mcs)
				if self.table[candidate_mcs]["loss"] < self.max_loss:
					index = len(_candidates_loss)
					for i in range(len(_candidates_loss)):
						if self.table[_candidates[i]]["average_tx_time"] > self.table[candidate_mcs]["average_tx_time"]:
							index = i-1
							break
					_candidates_loss.insert(index, candidate_mcs)		

		if _candidates_loss:
			_width = self.get_width(_candidates_loss[0], self.table[_candidates_loss[0]]["width"], self.table[_candidates_loss[0]]["loss"])	
			return _candidates_loss[0], _width
		else:
			for i in range(len(_candidates)):
				return _candidates[i], self.get_width(_candidates[i], self.table[_candidates[i]]["width"], self.table[_candidates[i]]["loss"])	
			return current_mcs, current_width
		
	def get_width(self, candidate_mcs, candidate_width, candidate_loss):
		# the algorithm should know the ordering of the channel widths, based on the periodic PN seq probing (not implemented)		
		if candidate_loss == -1:
			return _max_width			
		if candidate_loss >= (self.max_loss - self.margin_loss) and (candidate_loss <= self.max_loss): #need the same width
			return candidate_width
			
		#if self.table[candidate_mcs]["locked"]:
		#	return candidate_width
		#self.table[candidate_mcs]["locked"] = True	
		if candidate_loss > self.max_loss: # return the highest one with the loss less than the limit
			#print "Return lower width"
			_min_loss = candidate_loss
			_min_loss_width = candidate_width
			for width in range(_max_width, _min_width-1, -_delta_width):
				if self.loss[candidate_mcs][width] <= self.max_loss:
					return width
				else:
					if 	self.loss[candidate_mcs][width]< _min_loss:
						_min_loss = self.loss[candidate_mcs][width]
						_min_loss_width = width
			
			#if _min_loss_width = candidate_width: # we can't find better, has to go a step down	
			#	return -1
			#else:	
			return _min_loss_width
			
		else:
			#print "Return the same width because "+str(self.max_loss)+" higher than "+str(candidate_loss)
			return candidate_width
			#return _max_width
				
		#	_index = mcs_order[candidate_mcs][candidate_width]
		#	_new_index = math.ceil(_num_widths - _index) + _index
		#	if _new_index > _num_widths:
		#		_new_index = _num_widths
		#	_new_width = ordered_widths[candidate_mcs][_new_index]
		#else:
		#	_index = mcs_order[candidate_mcs][candidate_width]
		#	_new_index = math.ceil(_num_widths - _index) + _index
		#	if _new_index > _num_widths:
		#		_new_index = _num_widths
		#	_new_width = ordered_widths[candidate_mcs][_new_index]
				
			
		#	_width_change = math.ceil((candidate_width - _min_width)/(2*_delta_width))*_delta_width
			
		#	if (candidate_width - _width_change >= _min_width):
				#print "change width lower from "+str(candidate_width)+" to "+str(candidate_width - _width_change)
		#		return candidate_width - _width_change
		#	else:
				#print "change width to min"
		#		return _min_width
		#else: 
			# we need higher width
		#	_width_change = math.ceil((_max_width - candidate_width)/(2*_delta_width))*_delta_width
		#	if (candidate_width + _width_change <= _max_width):
				#print "change width higher"
		#		return candidate_width + _width_change
		#	else:
				#print "change width to max"
		#		return _max_width
	
	def pick_probe(self, current_mcs, current_width):
		#print "Picking probe"
		# that hasn't experienced c consecutive losses and can result in better performance	
		# the algorithm should know the ordering of the channel widths, based on the periodic PN seq probing (not implemented)
		# sampling has to pick a completely random mcs every once a while, e.g. every 100th time
		#self.probe_cnt += 1
		#if (self.probe_cnt % 11 == 0):
		#	return "qam16", 414
		_candidates = []	
		for candidate_mcs in _valid_mcs:
			if (self.table[candidate_mcs]["lossless_tx_time"] < self.table[current_mcs]["average_tx_time"]) and (self.table[candidate_mcs]["succ_fails"] < _C):
				_candidates.append(candidate_mcs)
		if (_candidates):		
			probing_mcs = choice(_candidates)
		else:
			probing_mcs = "qam64++"	
		#if self.table[probing_mcs]["locked"]:
		#	print "Locked probe for "+probing_mcs
		#	probing_width = self.table[probing_mcs]["width"]
		if self.table[probing_mcs]["loss"] > self.max_loss: # return the highest one with the loss less than the limit
			_min_loss = self.table[probing_mcs]["loss"]
			_min_loss_width = self.table[probing_mcs]["width"]
			for width in range(_max_width, _min_width-1, -_delta_width):
				if self.loss[probing_mcs][width] <= self.max_loss:
					return probing_mcs, width 
				else:
					if 	self.loss[probing_mcs][width]< _min_loss:
						_min_loss = self.loss[probing_mcs][width]
						_min_loss_width = width
				
			return probing_mcs, _min_loss_width
			
		else:
			#print "Shouldn't go here!"
			return probing_mcs, self.table[probing_mcs]["width"]
		
		
		#if (self.table[probing_mcs]["loss"]>self.max_loss):
			#print "UnLocked probe for "+probing_mcs
		#	_width_change = math.ceil((self.table[probing_mcs]["width"] - _min_width)/(2*_delta_width))*_delta_width
		#	if (self.table[probing_mcs]["width"] - _width_change >= _min_width):
		#		probing_width = self.table[probing_mcs]["width"] - _width_change
		#	else:
		#		probing_width = _min_width
		#else: 	
			#print "UnLocked probe for "+probing_mcs+" higher"
		#	_width_change = math.ceil((_max_width - self.table[probing_mcs]["width"])/(2*_delta_width))*_delta_width
		#	if (self.table[probing_mcs]["width"] + _width_change <= _max_width):
		#		probing_width = self.table[probing_mcs]["width"] + _width_change
		#	else:
		#		probing_width = _max_width	
		#self.table[probing_mcs]["width"]  = probing_width
		#self.table[probing_mcs]["locked"] = True
		#return probing_mcs, probing_width
		
	def find_rate(self, current_mcs, current_width, current_state):

		if current_state == _INIT:
			return "qam64++", _max_width

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
			#if self.table[current_mcs]["tries"] == _L: # time to evaluate loss
			return self.pick_probe(current_mcs, current_width)		
					
				
	def print_table(self):
		print 'RATE\tTRIES\tACKED\tSUCC_FAILS\tLOSS\tWIDTH\tTOTAL_TX_TIME\tAVG_TX_TIME\tLOSSLESS_TX_TIME'
		for candidate_mcs in _valid_mcs:
			print '| %06s \t%d\t%d\t%d\t%.2f\t%d\t%.3f\t%.3f\t%.3f|\n' % (candidate_mcs, self.table[candidate_mcs]["tries"], \
			self.table[candidate_mcs]["packets_acked"],self.table[candidate_mcs]["succ_fails"], \
			self.table[candidate_mcs]["loss"], self.table[candidate_mcs]["width"], self.table[candidate_mcs]["total_tx_time"], \
			self.table[candidate_mcs]["average_tx_time"]*1000000, self.table[candidate_mcs]["lossless_tx_time"]*1000000)

	def print_loss_table(self):
		for candidate_mcs in _valid_mcs:
			for width in range (_min_width, _max_width+1, _delta_width):
				print "MCS "+candidate_mcs+" width "+str(width)+" loss "+str(self.loss[candidate_mcs][width])

	def read_next(self, mcs, width):
		if self.iterator[mcs][width] == len(self.table[mcs][width]):
			self.iterator[mcs][width] = 0
		index = self.iterator[mcs][width]
		self.iterator[mcs][width] = self.iterator[mcs][width]+1
		return self.table[mcs][width][index]

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
