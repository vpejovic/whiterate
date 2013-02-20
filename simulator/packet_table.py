#!/usr/bin/env python
#
# Reads the trace and sorts the information about the MCS/width used 
# along with the transmission outcome into a table used by the simulator
# In addition, it calculates stats about the optimal rates.

from collections import defaultdict
import re

_time_per_subcarrier = 0.001
_valid_MCS  = ['bpsk', 'bpsk++', 'qpsk', 'qpsk++', 'qam16', 'qam16++', 'qam64++']
_valid_MCS_code = [13, 15, 5, 7, 9, 11, 10]
_MCS_mapping  = {'bpsk':13, 'bpsk++':15, 'qpsk':5, 'qpsk++':7, 'qam16':9, 'qam16++':11, 'qam64++':10}
_MCS_mapping_inverse  = {'13':'bpsk', '15':'bpsk++', '5':'qpsk', '7':'qpsk++', '9':'qam16', '11':'qam16++', '10':'qam64++'}
_max_width = 462
_min_width = 174
_delta_width = 48
_coding_rate = {'bpsk':2, 'bpsk++':1.25, 'qpsk':2, 'qpsk++':1.25, 'qam16':2, 'qam16++':1.25, 'qam64++':1.25}
_bits_per_tone = {'bpsk':1, 'bpsk++':1, 'qpsk':2, 'qpsk++':2, 'qam16':4, 'qam16++':4, 'qam64++':8}
_power_per_width = {174:2.187, 222:2.192, 270:2.194, 318:2.198, 366:2.201, 414:2.203, 462:2.204} #total current drawn


class packet_table(object):
	def __init__(self):
		self.table = {}
		self.loss = {}
		self.tput = {}
		self.bitrate = {}	
		self.iterator = {}

	#	40	 7	 7	 1	 0.011794	 -1	 0.000000	 8.128763
	def read_profile(self, filename, width, back):
		f = open(filename, 'r')
		exp = re.compile('(\d+)\s+(\d+)\s+(\d+)\s+(\d+).+')
		for line in f:
			matched = exp.match(line)
			if matched:
				if (int(matched.group(2)) in _valid_MCS_code):
					rate = _MCS_mapping_inverse[matched.group(2)]
					ok = matched.group(4)
					if not (rate in self.table):
						self.table[rate] = defaultdict(list)
						self.iterator[rate] = defaultdict(int)
						#if not (width in self.table[rate]):
						#	self.table[rate][width] = 
					self.table[rate][width].append(int(ok))
	
		if (back):
			for rate in _valid_MCS:	
				for width in range(_min_width, _max_width+1, _delta_width):
					if len(self.table[rate][width]) > 0:
						self.iterator[rate][width] = len(self.table[rate][width]) - 1
					
		
	def calc_averages(self, loss_thold):
		_max_tput = 0
		_max_tput_rate = "bpsk"
		_max_tput_width = _min_width
		_max_bitrate = 0
		_max_bitrate_rate = "bpsk"
		_max_bitrate_width = _min_width		
		for rate in _valid_MCS:			
			if not (rate in self.loss):
				self.loss[rate] = defaultdict(float)
				self.tput[rate] = defaultdict(float)	
				self.bitrate[rate] = defaultdict(float)								
			for width in range(_min_width, _max_width+1, _delta_width):
				if (self.table[rate][width]):
					avg = float(sum(self.table[rate][width]))/len(self.table[rate][width])
					#if len(self.table[rate][width])<50:
					#	print "WARNING: only "+str(len(self.table[rate][width]))+" entries for "+rate+" "+str(width)
				else:
					avg = 0
					#print "WARNING: no entries for "+rate+" "+str(width)
				self.loss[rate][width] = 1 - avg
				mos = self.calc_mos(1-avg)
				energy = self.calc_energy(float(_coding_rate[rate]*_time_per_subcarrier)/(_bits_per_tone[rate]*width), width)
				print rate + " "+ str(width) + " "+str(self.loss[rate][width])+"\t"+str(energy)+" "+str(mos)
				self.bitrate[rate][width] = _bits_per_tone[rate]*width/(_coding_rate[rate]*float(_time_per_subcarrier))
				self.tput[rate][width] = avg*(_bits_per_tone[rate]*width)/(_coding_rate[rate]*float(_time_per_subcarrier))				
				#print str(self.loss[rate][width])+"\t"+str(self.tput[rate][width])+" kbps"
				if (_max_bitrate < self.bitrate[rate][width] and self.loss[rate][width]< loss_thold):
					_max_bitrate = self.bitrate[rate][width]
					_max_bitrate_rate = rate
					_max_bitrate_width = width
				if (_max_tput < self.tput[rate][width] and self.loss[rate][width]< loss_thold):
					_max_tput = self.tput[rate][width]
					_max_tput_rate = rate
					_max_tput_width = width
					
		print "MAX BITRATE: "+str(_max_bitrate)+" at "+_max_bitrate_rate+", "+str(_max_bitrate_width)+" loss: "+	str(self.loss[_max_bitrate_rate][_max_bitrate_width])			
		print "MAX TPUT: "+str(_max_tput)+" at "+_max_tput_rate+", "+str(_max_tput_width)+" loss: "+	str(self.loss[_max_tput_rate][_max_tput_width])
		return self.loss

	def calc_mos(self, loss):
		ppl = loss*100 
		bpl = 34 
		ie = 0
		ie_eff = ie + (95.0 - ie) * ppl / (ppl + bpl)
		rlq = 93.2 - ie_eff
		MOS = 1 + rlq * 0.035 + rlq * (100 - rlq) * (rlq - 60) * 0.000007
		return MOS
	
	def calc_energy(self, time, width):
		# time per bit
		# power
		P_total = _power_per_width[width]*6 - 11.76
		return P_total*time 
		#energy = s
				
	def print_table(self):
		for entry in self.table.items():
			print str(entry)+"\n"
			
	def read_next(self, mcs, width):
		if self.iterator[mcs][width] == len(self.table[mcs][width]):
			self.iterator[mcs][width] = 0
		index = self.iterator[mcs][width]
		self.iterator[mcs][width] = self.iterator[mcs][width]+1
		#print "MCS: "+mcs+" Width "+str(width)
		if (self.table[mcs][width]):
			return self.table[mcs][width][index]
		else:
			return 0	

if __name__ == "__main__":
	t = packet_table()
	back = False
	#t.read_profile("Pretoria/174_100.txt" ,174, False)
	#t.read_profile("Pretoria/222_100.txt" ,222, False)
	#t.read_profile("Pretoria/270_100.txt" ,270, False)
	#t.read_profile("Pretoria/318_100.txt" ,318, False)
	#t.read_profile("Pretoria/366_100.txt" ,366, False)
	#t.read_profile("Pretoria/414_100.txt" ,414, False)
	#t.read_profile("Pretoria/462_100.txt" ,462, False)
	ampl = 0.3



	if (ampl == 0.01):
		t.read_profile("repo/462_001_new.txt" ,462, back)
		t.read_profile("repo/414_001_new.txt" ,414, back)
		t.read_profile("repo/366_001_new.txt" ,366, back)
		t.read_profile("repo/318_001_new.txt" ,318, back)
		t.read_profile("repo/270_001_new.txt" ,270, back)
		t.read_profile("repo/222_001_new.txt" ,222, back)
		t.read_profile("repo/174_001_new.txt" ,174, back)

	elif ampl==0.03:	
		t.read_profile("repo/462_003_new.txt" ,462, back)
		t.read_profile("repo/414_003_new.txt" ,414, back)
		t.read_profile("repo/366_003_new.txt" ,366, back)
		t.read_profile("repo/318_003_new.txt" ,318, back)
		t.read_profile("repo/270_003_new.txt" ,270, back)
		t.read_profile("repo/222_003_new.txt" ,222, back)
		t.read_profile("repo/174_003_new.txt" ,174, back)

	elif ampl==0.05:	
		t.read_profile("repo/462_005_new.txt" ,462, back)
		t.read_profile("repo/414_005_new.txt" ,414, back)
		t.read_profile("repo/366_005_new.txt" ,366, back)
		t.read_profile("repo/318_005_new.txt" ,318, back)
		t.read_profile("repo/270_005_new.txt" ,270, back)
		t.read_profile("repo/222_005_new.txt" ,222, back)
		t.read_profile("repo/174_005_new.txt" ,174, back)

	elif ampl==0.10:
		t.read_profile("repo/462_010_new.txt" ,462, back)
		t.read_profile("repo/414_010_new.txt" ,414, back)
		t.read_profile("repo/366_010_new.txt" ,366, back)
		t.read_profile("repo/318_010_new.txt" ,318, back)
		t.read_profile("repo/270_010_new.txt" ,270, back)
		t.read_profile("repo/222_010_new.txt" ,222, back)
		t.read_profile("repo/174_010_new.txt" ,174, back)
	
	elif ampl==0.20	:
		t.read_profile("repo/414_020_new.txt" ,414, back)
		t.read_profile("repo/462_020_new.txt" ,462, back)		
		t.read_profile("repo/366_020_new.txt" ,366, back)
		t.read_profile("repo/318_020_new.txt" ,318, back)
		t.read_profile("repo/270_020_new.txt" ,270, back)
		t.read_profile("repo/222_020_new.txt" ,222, back)
		t.read_profile("repo/174_020_new.txt" ,174, back)		

	elif ampl==0.30	:
		t.read_profile("repo/414_030_new.txt" ,414, back)
		t.read_profile("repo/462_030_new.txt" ,462, back)		
		t.read_profile("repo/366_030_new.txt" ,366, back)
		t.read_profile("repo/318_030_new.txt" ,318, back)
		t.read_profile("repo/270_030_new.txt" ,270, back)
		t.read_profile("repo/222_030_new.txt" ,222, back)
		t.read_profile("repo/174_030_new.txt" ,174, back)
	
	elif ampl==0.50	:
		t.read_profile("repo/414_050_new.txt" ,414, back)
		t.read_profile("repo/462_050_new.txt" ,462, back)		
		t.read_profile("repo/366_050_new.txt" ,366, back)
		t.read_profile("repo/318_050_new.txt" ,318, back)
		t.read_profile("repo/270_050_new.txt" ,270, back)
		t.read_profile("repo/222_050_new.txt" ,222, back)
		t.read_profile("repo/174_050_new.txt" ,174, back)

	elif ampl==0.80	:
		t.read_profile("repo/414_080_new.txt" ,414, back)
		t.read_profile("repo/462_080_new.txt" ,462, back)		
		t.read_profile("repo/366_080_new.txt" ,366, back)
		t.read_profile("repo/318_080_new.txt" ,318, back)
		t.read_profile("repo/270_080_new.txt" ,270, back)
		t.read_profile("repo/222_080_new.txt" ,222, back)
		t.read_profile("repo/174_080_new.txt" ,174, back)
	
	elif ampl==0.90	:
		t.read_profile("repo/414_090_new.txt" ,414, back)
		t.read_profile("repo/462_090_new.txt" ,462, back)		
		t.read_profile("repo/366_090_new.txt" ,366, back)
		t.read_profile("repo/318_090_new.txt" ,318, back)
		t.read_profile("repo/270_090_new.txt" ,270, back)
		t.read_profile("repo/222_090_new.txt" ,222, back)
		t.read_profile("repo/174_090_new.txt" ,174, back)
		
	elif ampl==1:
		t.read_profile("repo/414_100_new.txt" ,414, back)
		t.read_profile("repo/462_100_new.txt" ,462, back)		
		t.read_profile("repo/366_100_new.txt" ,366, back)
		t.read_profile("repo/318_100_new.txt" ,318, back)
		t.read_profile("repo/270_100_new.txt" ,270, back)
		t.read_profile("repo/222_100_new.txt" ,222, back)
		t.read_profile("repo/174_100_new.txt" ,174, back)
			
#	t.read_profile("Pretoria/462_050_new.txt" ,462, back)
#	t.read_profile("Pretoria/414_050_new.txt" ,414, back)
#	t.read_profile("Pretoria/366_050_new.txt" ,366, back)
#	t.read_profile("Pretoria/318_050_new.txt" ,318, back)
#	t.read_profile("Pretoria/270_050_new.txt" ,270, back)
#	t.read_profile("Pretoria/222_050_new.txt" ,222, back)
#	t.read_profile("Pretoria/174_050_new.txt" ,174, back)	
	t.calc_averages(0.05)
