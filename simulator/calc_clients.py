#!/usr/bin/env python
#
# Get the number of clients that are simultaneously supported.

if __name__ == "__main__":
	#TDMA
	subcarriers = 174
	bitrate_max = 275669
	
	
	bitrate_req = 21333
	total_width = subcarriers*(1953)
	result = 0.06/((160*8/float(bitrate_max))+0.0005)
	
	result = bitrate_max/(bitrate_req+20000)
			
	print "For width "+str(total_width)+" calls: "+str(int(result))
