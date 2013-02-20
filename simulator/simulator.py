#!/usr/bin/env python
#
# Simulates WhiteRate and SampleRate. 
# The script iterates over a given trace. The trace should ideally cover all width-mcs combinations.
# For any rate adaptation protocol the simulator goes over a predefined number of transmissions. It
# selects the transmission mcs/width and then reads from the appropriate trace to determine the outcome
# of the transmission. Statistics such as packet error rate, energy per bit and voice quality (calculated
# through e-model in case of VoIP transmission) are updated in the process.
# For WhiteRate the desired PER limit has to be defined.

import getopt
import sys
import packet_table
import samplerate
import whiterate

_INIT = 1	# initial state
_NORMAL = 2	# normal operation
_PROBING = 3	# probing
_P = 10		# probing happens every _P packets

# NOTE: MCS are coded with 1/2 rate unless followed by ++ in which case 3/4 code is used
_valid_MCS  = ['bpsk', 'bpsk++', 'qpsk', 'qpsk++', 'qam16', 'qam16++', 'qam64++']
_coding_rate = {'bpsk':2, 'bpsk++':1.25, 'qpsk':2, 'qpsk++':1.25, 'qam16':2, 'qam16++':1.25, 'qam64++':1.25}

# mapping MCSs to internal codes and back
_MCS_mapping  = {'bpsk':13, 'bpsk++':15, 'qpsk':5, 'qpsk++':7, 'qam16':9, 'qam16++':11, 'qam64++':10}
_MCS_mapping_inverse  = {'13':'bpsk', '15':'bpsk++', '5':'qpsk', '7':'qpsk++', '9':'qam16', '11':'qam16++', '10':'qam64++'}

_bits_per_tone = {'bpsk':1, 'bpsk++':1, 'qpsk':2, 'qpsk++':2, 'qam16':4, 'qam16++':4, 'qam64++':8}

# seconds per sample for a single subcarrier
_time_per_subcarrier = 0.001 

# from the power measurements number of occupied subcarriers vs current draw
_power_per_width = {174:2.187, 222:2.192, 270:2.194, 318:2.198, 366:2.201, 414:2.203, 462:2.204} #total current drawn


class simulator(object):
	def __init__(self, max_loss, back, ampl):
		self.state = _INIT
		self.t = packet_table.packet_table()
		
		if (ampl == 0.01):
			self.t.read_profile("repo/462_001_new.txt" ,462, back)
			self.t.read_profile("repo/414_001_new.txt" ,414, back)
			self.t.read_profile("repo/366_001_new.txt" ,366, back)
			self.t.read_profile("repo/318_001_new.txt" ,318, back)
			self.t.read_profile("repo/270_001_new.txt" ,270, back)
			self.t.read_profile("repo/222_001_new.txt" ,222, back)
			self.t.read_profile("repo/174_001_new.txt" ,174, back)

		elif ampl==0.03:
			self.t.read_profile("repo/462_003_new.txt" ,462, back)
			self.t.read_profile("repo/414_003_new.txt" ,414, back)
			self.t.read_profile("repo/366_003_new.txt" ,366, back)
			self.t.read_profile("repo/318_003_new.txt" ,318, back)
			self.t.read_profile("repo/270_003_new.txt" ,270, back)
			self.t.read_profile("repo/222_003_new.txt" ,222, back)
			self.t.read_profile("repo/174_003_new.txt" ,174, back)

		elif ampl==0.05:
			self.t.read_profile("repo/462_005_new.txt" ,462, back)
			self.t.read_profile("repo/414_005_new.txt" ,414, back)
			self.t.read_profile("repo/366_005_new.txt" ,366, back)
			self.t.read_profile("repo/318_005_new.txt" ,318, back)
			self.t.read_profile("repo/270_005_new.txt" ,270, back)
			self.t.read_profile("repo/222_005_new.txt" ,222, back)
			self.t.read_profile("repo/174_005_new.txt" ,174, back)

		elif ampl==0.10:
			self.t.read_profile("repo/462_010_new.txt" ,462, back)
			self.t.read_profile("repo/414_010_new.txt" ,414, back)
			self.t.read_profile("repo/366_010_new.txt" ,366, back)
			self.t.read_profile("repo/318_010_new.txt" ,318, back)
			self.t.read_profile("repo/270_010_new.txt" ,270, back)
			self.t.read_profile("repo/222_010_new.txt" ,222, back)
			self.t.read_profile("repo/174_010_new.txt" ,174, back)

		elif ampl==0.20:
			self.t.read_profile("repo/414_020_new.txt" ,414, back)
			self.t.read_profile("repo/462_020_new.txt" ,462, back)			
			self.t.read_profile("repo/366_020_new.txt" ,366, back)
			self.t.read_profile("repo/318_020_new.txt" ,318, back)
			self.t.read_profile("repo/270_020_new.txt" ,270, back)
			self.t.read_profile("repo/222_020_new.txt" ,222, back)
			self.t.read_profile("repo/174_020_new.txt" ,174, back)	

		elif ampl==0.30:
			self.t.read_profile("repo/414_030_new.txt" ,414, back)
			self.t.read_profile("repo/462_030_new.txt" ,462, back)			
			self.t.read_profile("repo/366_030_new.txt" ,366, back)
			self.t.read_profile("repo/318_030_new.txt" ,318, back)
			self.t.read_profile("repo/270_030_new.txt" ,270, back)
			self.t.read_profile("repo/222_030_new.txt" ,222, back)
			self.t.read_profile("repo/174_030_new.txt" ,174, back)				

		elif ampl==0.50:
			self.t.read_profile("repo/414_050_new.txt" ,414, back)
			self.t.read_profile("repo/462_050_new.txt" ,462, back)			
			self.t.read_profile("repo/366_050_new.txt" ,366, back)
			self.t.read_profile("repo/318_050_new.txt" ,318, back)
			self.t.read_profile("repo/270_050_new.txt" ,270, back)
			self.t.read_profile("repo/222_050_new.txt" ,222, back)
			self.t.read_profile("repo/174_050_new.txt" ,174, back)

		elif ampl==0.80:
			self.t.read_profile("repo/414_080_new.txt" ,414, back)
			self.t.read_profile("repo/462_080_new.txt" ,462, back)			
			self.t.read_profile("repo/366_080_new.txt" ,366, back)
			self.t.read_profile("repo/318_080_new.txt" ,318, back)
			self.t.read_profile("repo/270_080_new.txt" ,270, back)
			self.t.read_profile("repo/222_080_new.txt" ,222, back)
			self.t.read_profile("repo/174_080_new.txt" ,174, back)

		elif ampl==0.90:
			self.t.read_profile("repo/414_090_new.txt" ,414, back)
			self.t.read_profile("repo/462_090_new.txt" ,462, back)			
			self.t.read_profile("repo/366_090_new.txt" ,366, back)
			self.t.read_profile("repo/318_090_new.txt" ,318, back)
			self.t.read_profile("repo/270_090_new.txt" ,270, back)
			self.t.read_profile("repo/222_090_new.txt" ,222, back)
			self.t.read_profile("repo/174_090_new.txt" ,174, back)

		elif ampl==1:
			self.t.read_profile("repo/414_100_new.txt" ,414, back)
			self.t.read_profile("repo/462_100_new.txt" ,462, back)			
			self.t.read_profile("repo/366_100_new.txt" ,366, back)
			self.t.read_profile("repo/318_100_new.txt" ,318, back)
			self.t.read_profile("repo/270_100_new.txt" ,270, back)
			self.t.read_profile("repo/222_100_new.txt" ,222, back)
			self.t.read_profile("repo/174_100_new.txt" ,174, back)
		
		loss_table = self.t.calc_averages(max_loss)		

		#NOTE: uncomment only one of the lines below: 
		self.s = whiterate.state_table(max_loss, loss_table)
		#self.s = samplerate.state_table(max_loss)
		
		# start with the highest bitrate
		self.current_mcs = "qam64++"
		self.current_width = 462

		self.total_ok = 0
		self.time = 0
		self.total_energy = 0
		

	def run(self, steps):
		stats_steps = 0
		for i in range(steps):
			if (self.state == _NORMAL):
				if (i%_P == 0):
					self.state=_PROBING
					self.old_mcs = self.current_mcs
					self.old_width = self.current_width
			_new_mcs, _new_width = self.s.find_rate(self.current_mcs, self.current_width, self.state)	
			self.current_mcs = _new_mcs
			self.current_width = _new_width
			value = self.t.read_next(self.current_mcs, self.current_width)		
			
			self.total_ok += int(value)
	
			print "Tx: "+_new_mcs+"\t"+str(_new_width)+" "+str(value)
			self.s.update_table(self.current_mcs, self.current_width, value)
			#self.s.print_table()	
			if (self.state == _PROBING):		
				self.current_mcs = self.old_mcs
				self.current_width = self.old_width
			else:
				current_time = float(_coding_rate[self.current_mcs]*_time_per_subcarrier)/(_bits_per_tone[self.current_mcs]*self.current_width)
				self.time +=  current_time
				stats_steps += 1
				energy = self.calc_energy(current_time, self.current_width)
				self.total_energy += energy
			self.state = _NORMAL
		avg_bitrate = 1/(self.time/stats_steps)
		avg_energy = self.total_energy/float(stats_steps)
		avg_tput = avg_bitrate*(self.total_ok/float(steps))
		print "Avg loss: "+str(1 - self.total_ok/float(steps))+" total time "+str(self.time)+" avg bitrate "+str(avg_bitrate)+" avg tput "+str(avg_tput)
		MOS = self.calc_mos(1 - self.total_ok/float(steps))
		print "MOS: "+str(MOS)+" energy: "+str(avg_energy) #(1 - self.total_ok/float(steps)))
		#self.s.print_loss_table()
		
	"""
	Calc MOS using e-model
	"""
	def calc_mos(self, loss):
		ppl = loss*100 
		bpl = 34 
		ie = 0
		ie_eff = ie + (95.0 - ie) * ppl / (ppl + bpl)
		rlq = 93.2 - ie_eff
		MOS = 1 + rlq * 0.035 + rlq * (100 - rlq) * (rlq - 60) * 0.000007
		return MOS
	
	"""
	Calc energy from the measurement data and the time spent transmitting
	"""		
	def calc_energy(self, time, width):
		# time per bit
		# power
		P_total = _power_per_width[width]*6 - 11.76
		return P_total*time 
			
if __name__ == "__main__":
	loss = 0.01
	steps =10
	back = False #invert the trace, basically seeding
	ampl = 0.1
	
	argv = sys.argv[1:]
	try:
		opts, args = getopt.getopt(argv, "hbs:l:a:", ["steps", "loss", "back","ampl"])
	except getopt.GetoptError:          
		usage()                         
		sys.exit(2)                     
	for opt, arg in opts:             
		if opt in ("-h", "--help"):   
			usage()                     
			sys.exit()                  
		elif opt in ("-s", "--steps"):        
		 	steps = int(arg)                  
		elif opt in ("-l", "--loss"): 
			loss = float(arg)      
		elif opt in ("-a", "--ampl"): 
			ampl = float(arg)	
		elif opt in ("-b", "--back"): 
			back = True	         
	simulator = simulator(loss, back, ampl)
	#simulator.t.print_table()
	simulator.run(steps)
	
