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
import time
import struct
import sys
import threading
import socket

from gnuradio import gr, blks2
from math import sqrt
from numpy.oldnumeric import *
from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import flex_ofdm_code as flex_ofdm

# from current dir
import transmit_path
import receive_path
import usrp_receive_path
import usrp_transmit_path
#import whiterate_ewma as whiterate
import whiterate_ewma_width as whiterate
#import samplerate_ewma as whiterate
import settings


_INIT = 1       # initial state
_NORMAL = 2     # normal operation
_PROBING = 3    # probing
_P = 20         # probing happens every _P packets

# NOTE: MCS are coded with 1/2 rate unless followed by ++ in which case 3/4 code is used
_valid_MCS  = ['bpsk',  'qpsk', 'qam8', 'qam16', 'qam64']
_coding_rate = {'bpsk':2,  'qpsk':2, 'qam8':2, 'qam16':2, 'qam64':2}

# mapping MCSs to internal codes and back
_MCS_mapping  = {'bpsk':13, 'qpsk':5, 'qam8':4, 'qam16':9, 'qam64':2}
_MCS_mapping_inverse  = {'13':'bpsk', '5':'qpsk', '4':'qam8', '9':'qam16', '2':'qam64++'}

_bits_per_tone = {'bpsk':1, 'qpsk':2, 'qam8':3, 'qam16':4, 'qam64':6}

# from the power measurements number of occupied subcarriers vs current draw
_power_per_width = {174:2.187, 222:2.192, 270:2.194, 318:2.198, 366:2.201, 414:2.203, 462:2.204} #total current drawn

class ACKSendingThread(threading.Thread):

    def __init__(self, channel, details, verbose):
        threading.Thread.__init__(self)
        self._channel = channel
        self._details = details
        self._verbose = verbose
        self.ok2ack = threading.Event() # to notify that there's an ACK to be sent out
        self.pktno = 0
        self.good = False
        self.width_ranking = [2,4,5,1,6,0,3]

    def run(self):
        if self._verbose:
            settings.ts_print (' '.join(["Established connection:", str(self._details [ 0 ])]), time.time())
        while True:
            self.ok2ack.wait()
            self.ok2ack.clear()
            _gain = ""
            for i in range(len(self.width_ranking)):
                _gain = ''.join([_gain, "_", str(self.width_ranking[i])])
            self._channel.send(''.join([str(self.pktno), "_", str(self.good), _gain]))
            if self._verbose:
                settings.ts_print(' '.join(["MAC | ACKSendingThread: sent ACK #", str(self.pktno)]), time.time())
        self._channel.close()

class WidthReceiveThread(threading.Thread):

    def __init__ (self, client, details, top_block, options):
        threading.Thread.__init__(self)
        self._tb = top_block
        self._client = client
        self._details = details
        self._options = options

    def change_width(self, payload):
        _values = payload.split('_')
        if self._options.verbose:
            settings.ts_print(' '.join(['Change map to:', str(_values[0]), 'and num carriers to', str(_values[1])]), time.time())
        _new_width_map = _values[0]
        _new_occupied_tones = int(_values[1])
        self._options.width_map = _new_width_map
        self._tb.stop()
        self._tb.wait()
        self._tb.rxpath.reset_ofdm_params(_new_width_map)
        self._tb.start()
        if self._options.verbose:
            settings.ts_print('Change width completed',time.time())

    def run(self):
        if self._options.verbose:
            settings.ts_print (' '.join(["Established connection:", self._details [ 0 ]]), time.time())
        while True:
            _payload = self._client.recv(1024)
            self.change_width(_payload)
        self._client.close()

class ACKReceivingThread(threading.Thread):

    def __init__ (self, client, top, verbose):
        threading.Thread.__init__(self)
        self._client = client
        self._top = top
        self._verbose = verbose
        self.getack = threading.Event()  # to notify that there's an ACK to be sent out
        self.acked = threading.Event()
        self.good_ack = True
        self.ackwanted = 0

    def receive_ack(self):
        _payload = ""
        try:
            _payload = self._client.recv(1024)
            if not _payload:
                if self._verbose:
                    settings.ts_print("MAC | ACKReceivingThread: ACK not received!", time.time())
                self.good_ack = False
            else:
                if self._verbose:
                    settings.ts_print(' '.join(["MAC | ACKReceivingThread: ACK received:", _payload, "wanted", str(self.ackwanted)]), time.time())
                _values = _payload.split('_')
                if (int(_values[0])==self.ackwanted) and (str(_values[1]) == "True"):
                    self.good_ack = True
                else:
                    self.good_ack = False
                if self._top.refresh_gains:
                    j = 0
                    for i in range (self._top.num_groups):
                        gain_rank = int(_values[i+2])
                        if int(gain_rank) != self._top.num_groups//2:
                             self._top.group_sort[j] = gain_rank
                             j += 1 
                    self._top.refresh_gains = False
            self.acked.set()
        except:
            if self._verbose:
                settings.ts_print("MAC | ACKReceivingThread: ACK not received (except)", time.time())
            self.good_ack = False
            self.acked.set()
        finally:
            pass

    def run(self):
        while True:
            self.getack.wait()
            settings.ts_print("MAC | ACKReceivingThread: wait for ACK", time.time())
            self.getack.clear()
            self.receive_ack()
        self._client.close()


class WidthSendingThread(threading.Thread):


    def __init__ (self, client, verbose):
        self._client = client
        self._verbose = verbose
        self.sendit = threading.Event()  # to notify that there's an ACK to be sent out
        self.tone_map = "ffff"
        self.width_map = "1111111"
        self.occupied_tones = 468
        threading.Thread.__init__(self)

    def send_width(self):
        payload = '_'.join([str(self.width_map), str(self.occupied_tones)])
        try:
            self._client.send(payload)
            if self._verbose:
                settings.ts_print("MAC | sent width change params", time.time())
        except socket.error:
            print 'Could not send width, socket error'

    def run(self):
        while True:
            self.sendit.wait()
            self.sendit.clear()
            self.send_width()
        self._client.close()

class top_class(gr.top_block):
    def __init__(self, max_loss, options):
        gr.top_block.__init__(self)
        self._state = _INIT
        self._n_rcvd = 0
        self._n_right = 0
        self._sender = options.sender
        self._occupied_tones = options.occupied_tones_sender
        self.num_groups = options.num_groups
        self._group_size = int(math.floor((self._occupied_tones - 4)/self.num_groups))
        self.group_sort = [0]*self.num_groups
        self.group_sort[self.num_groups - 1] = self.num_groups//2
        j = 0
        for i in range (self.num_groups):
            if i != self.num_groups//2:
                self.group_sort[j] = i
                j += 1
        self.group_sort = [2,4,5,1,6,0,3]
        self._current_mcs = "qam64"
        self._current_width = self.num_groups * self._group_size
        #options.tone_map_sender = _map_per_width[self._current_width]
        options.width_map = "1111111"
        self._time_per_subcarrier = options.fft_length/float(options.sample_rate)
        self.verbose = options.verbose
        if self._sender:
            self._pkt_size = options.size
            self._spacing = options.spacing
            self.txpath = usrp_transmit_path.usrp_transmit_path(options)
            self.connect(self.txpath)
            #self.file_sink = gr.file_sink(gr.sizeof_gr_complex, "transmitted.dat")
            #self.connect(self.txpath.tx_path, self.file_sink)
            
            client_a = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client_a.connect((options.rx_ip, options.rx_port))
            #client_a = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
            #client_a.connect((options.rx_ip, options.rx_port, 0, 0))
            
            client_a.settimeout(options.ack_timeout)
            self.ack_recvr = ACKReceivingThread(client_a, self, self.verbose)
            self.ack_recvr.start()
            time.sleep(2)
            
            client_w = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client_w.connect((options.rx_ip, options.rx_port+1))
            #client_w = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
            #client_w.connect((options.rx_ip, options.rx_port+1, 0, 0))
            
            self.width_sender = WidthSendingThread(client_w, self.verbose)
            self.width_sender.start()
            self.s = whiterate.state_table(max_loss, self.num_groups, self._group_size, self._time_per_subcarrier)
            self.refresh_gains = True
            #self.s = samplerate.state_table(max_loss)
            self._total_ok = 0
            self._time = 0
            self._total_energy = 0
            self._small_time = 0
            self._small_total_energy = 0
        else:
            self.rxpath = usrp_receive_path.usrp_receive_path(self.rx_callback, options)
            self.connect(self.rxpath)

            server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            #server = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
            
            server.bind(( '', options.rx_port ))
            server.listen(5)
            channel, details = server.accept()
            self.ack_sender = ACKSendingThread(channel, details, self.verbose)
            self.ack_sender.start()
            
            server_w = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            #server_w = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
            
            server_w.bind(( '', options.rx_port+1))
            server_w.listen(5)
            channel_w, details_w = server_w.accept()
            self.width_recv = WidthReceiveThread(channel_w, details_w, self, options)
            self.width_recv.start()

        #loss_table = self.t.calc_averages(max_loss)

        settings.program_start()

    def send_pkt(self, payload='', eof=False, seqno=0):
        return self.txpath.send_pkt(payload, eof, seqno)

    def rx_callback(self, ok, payload, int_fo, frac_fo, time_sinr, freq_sinr, ch_gain, ch_gain_full, avg_gain, width_ranking):
        self._n_rcvd += 1
        try:
            (pktno,) = struct.unpack('!H', payload[0:2])
        except:
            pktno = 1
        if ok:
            self._n_right += 1
            self.ack_sender.good=True
        else:
            self.ack_sender.good=False
        self.ack_sender.pktno = pktno
        self.ack_sender.width_ranking = width_ranking
        print "ok: %r \t pktno: %d \t n_rcvd: %d \t n_right: %d" % (ok, pktno, self._n_rcvd, self._n_right)
        freq_offset = int_fo+frac_fo/math.pi
        if self.verbose:
            print "freq offset: %+.2f(subcarriers) \t SINR: %.2f(time domain), %.2f(freq domain)" % (freq_offset, time_sinr, freq_sinr)
        #print "Channel gain full: "+str(ch_gain_full)
        #print "Channel gain: "+str(ch_gain)
        #print "Average gain: "+str(avg_gain)

        if self.verbose:
            printlst = list()
            for x in payload[2:]:
                t = hex(ord(x)).replace('0x', '')
                if(len(t) == 1):
                    t = '0' + t
                printlst.append(t)
            printable = ''.join(printlst)

            print printable
            print "\n"
        self.ack_sender.ok2ack.set()

    def calc_mos(self, loss):
        ppl = loss*100
        bpl = 34
        ie = 0
        ie_eff = ie + (95.0 - ie) * ppl / (ppl + bpl)
        rlq = 93.2 - ie_eff
        MOS = 1 + rlq * 0.035 + rlq * (100 - rlq) * (rlq - 60) * 0.000007
        return MOS

    def calc_energy(self, time, width):
        #  Approximate power per width linearly:
        # TODO: check the power calculation
        P_total = (((2.204 - 2.187)/(462-174)) * width + 2.177)*6 - 11.76
        # Lookup table:
        #P_total = _power_per_width[width]*6 - 11.76
        return P_total*time

    def width_to_groups(self, width):
        active_groups = width/self._group_size
        new_width_map = ['0']*self.num_groups
        for i in range(active_groups):
            new_width_map[self.group_sort[i]] = '1'
        return new_width_map

    def run(self, steps):
        stats_steps = 0 # NOTE: we do not take stats of probe packets
        correct = 0
        total = 0
        f = open('losses.txt', 'w')
        for i in range(steps):
            change = False 
            if (i == 1):
                change = True
                self.s.change_width(462)
                self._current_width_x = 462
            if (i == 301):
                change = True  
                self.s.change_width(132)
                self._current_width_x = 132
            if (i == 601):
                change = True
                self.s.change_width(462)
                self._current_width_x = 462
            if (self._state == _NORMAL):
                if (i%_P == 0):
                    self._state=_PROBING
                    self.old_mcs = self._current_mcs
                    self.old_width = self._current_width

            if not change:
                _new_mcs, _new_width = self.s.find_rate(self._current_mcs, self._current_width, self._state)
            else:
                _new_mcs, _new_width = self.s.find_rate(self._current_mcs, self._current_width_x, self._state)
            _new_width_map = "".join(self.width_to_groups(_new_width))
            if self.verbose:
                settings.ts_print( ' '.join(["Calculated", str(_new_mcs), str(_new_width), "width map", str(_new_width_map)]), time.time())
            if _new_width != self._current_width:
                while not self.txpath.ofdm_tx._pkt_input.msgq().empty_p():
                    pass
                self.txpath.reset_ofdm_params_groups(_new_width_map, _new_mcs)
                self.width_sender.width_map = _new_width_map
                self.width_sender.sendit.set()
                time.sleep(0.1) # NOTE: pause for the other end to reconfigure itself
            elif _new_mcs != self._current_mcs:
                while not self.txpath.ofdm_tx._pkt_input.msgq().empty_p():
                    pass
                self.txpath.reset_ofdm_params_groups(_new_width_map, _new_mcs)
            self._current_mcs = _new_mcs
            self._current_width = _new_width

            payload = (self._pkt_size-2) *  chr(i & 0xff)
            payload = struct.pack('!H', i & 0xffff) + payload
            time.sleep(self._spacing)
            self.send_pkt(payload, False, i)
            self.ack_recvr.ackwanted = i
            self.ack_recvr.getack.set()
            self.ack_recvr.acked.wait()
            self.ack_recvr.acked.clear()
            value = self.ack_recvr.good_ack

            settings.ts_print(' '.join(["Tx num:", str(i), _new_mcs, str(_new_width), str(value)]), time.time())

            self.s.update_table(self._current_mcs, self._current_width, value)
   
            if i%20 != 0:
                if self._state == _NORMAL:
                    small_current_time = float(_coding_rate[self._current_mcs]*self._time_per_subcarrier)/(_bits_per_tone[self._current_mcs]*self._current_width)
                    self._small_time +=  small_current_time
                    small_energy = self.calc_energy(small_current_time, self._current_width)
                    self._small_total_energy += small_energy
                    if value:
                        correct += 1
                    total += 1
            else:
                if (total > 0):
                    loss = 1 - float(correct)/total
                    bitrate = 1/(self._small_time/total)
                    energy = self._small_total_energy/float(total)
                    if correct > 0:
                        tput = bitrate*(correct/float(total))
                    else:
                        tput = 0
                else:
                    loss = 1
                    bitrate = 0
                    energy = 0
                    tput = 0
                correct = 0
                total = 0
                self._small_time = 0
                self._small_total_energy = 0  
                print str(i) + " " + str(loss) + "\t" + str(tput) + "\t" + str(bitrate) + "\t" + str(energy) + "\n" 
                f.write(str(i) + " " + str(loss) + "\t" + str(tput) + "\t" + str(bitrate) + "\t" + str(energy) + "\n")
            self.s.print_table()
            if self.verbose:
                self.s.print_table()
            if (self._state == _PROBING):
                if self._current_width != self.old_width:
                    _new_width_map = string.join(self.width_to_groups(self.old_width), "")
                    _new_mcs = self.old_mcs
                    while not self.txpath.ofdm_tx._pkt_input.msgq().empty_p():
                        pass
                    if self.verbose:
                        settings.ts_print("Reset to old params after probing - width and MCS change", time.time())
                    self.txpath.reset_ofdm_params_groups(_new_width_map, _new_mcs)
                    self.width_sender.width_map = _new_width_map
                    self.width_sender.sendit.set()
                    time.sleep(0.1)
                elif  self._current_mcs != self.old_mcs:
                    _new_width_map = string.join(self.width_to_groups(self.old_width), "")
                    _new_mcs = self.old_mcs
                    while not self.txpath.ofdm_tx._pkt_input.msgq().empty_p():
                        pass
                    if self.verbose:
                        settings.ts_print("Reset to old params after probing - MCS change", time.time())
                    self.txpath.reset_ofdm_params_groups(_new_width_map, _new_mcs)
                self._current_mcs = self.old_mcs
                self._current_width = self.old_width
            elif i > 10:   # we skip the first 100 packets when collecting the stats
                current_time = float(_coding_rate[self._current_mcs]*self._time_per_subcarrier)/(_bits_per_tone[self._current_mcs]*self._current_width)
                self._time +=  current_time
                stats_steps += 1
                energy = self.calc_energy(current_time, self._current_width)
                self._total_energy += energy
                self._total_ok += int(value)
            self._state = _NORMAL

        avg_bitrate = 1/(self._time/stats_steps)
        avg_energy = self._total_energy/float(stats_steps)
        avg_tput = avg_bitrate*(self._total_ok/float(stats_steps))
        print ' '.join(["Avg loss:", str(1 - self._total_ok/float(stats_steps)), "total time", str(self._time), " avg bitrate", str(avg_bitrate), "avg tput ", str(avg_tput)])
        MOS = self.calc_mos(1 - self._total_ok/float(stats_steps))
        print ' '.join(["MOS:", str(MOS), "energy:", str(avg_energy)]) #(1 - self.total_ok/float(steps))
        f.close()

def main():

    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")
    expert_grp = parser.add_option_group("Expert")
    parser.add_option("-t", "--runtime", type=int, default=900)
    parser.add_option("-s", "--size", type="intx", default=97, help="set packet size [default=%default]")
    parser.add_option("-p", "--spacing", type="eng_float", default=1, help="set packet spacing in time [default=%default]")
    parser.add_option("-r", "--sample_rate", type="eng_float", default=500000, help="limit sample rate to RATE in throttle (%default)")
    parser.add_option("-l", "--loss", type="eng_float", default=0.2, help="loss rate that white rate tolerates (%default)")
    parser.add_option("", "--snr", type="eng_float", default=30, help="set the SNR of the channel in dB [default=%default]")
    parser.add_option("","--sender", action="store_true", default=False)
    parser.add_option("", "--tx-ip", type="string", default="128.111.52.67", help="Trasmitter's IP, needed for over-Ethernet-ACKs [default=%default]")
    parser.add_option("", "--rx-ip", type="string", default="128.111.52.70", help="Receiver's IP, needed for over-Ethernet-ACKs [default=%default]")
    #parser.add_option("", "--rx-ip", type="string", default="2001:4200:7000:112:227:eff:fe1c:2428", help="Receiver's IP, needed for over-Ethernet-ACKs [default=%default]")
    parser.add_option("", "--rx-port", type="intx", default=2727, help="Receiver's listening port, needed for over-Ethernet-ACKs [default=%default]")
    parser.add_option("", "--ack-timeout", type="eng_float", default=1, help="ACK timeout in seconds [default=%default]")
    parser.add_option("-v", "--verbose", action="store_true", default=False)

    transmit_path.transmit_path.add_options(parser, expert_grp)
    receive_path.receive_path.add_options(parser, expert_grp)
    usrp_transmit_path.add_options(parser, expert_grp)
    usrp_receive_path.add_options(parser, expert_grp)
    flex_ofdm.ofdm_mod.add_options(parser, expert_grp)
    flex_ofdm.ofdm_demod.add_options(parser, expert_grp)

    (options, args) = parser.parse_args ()

    top = top_class(options.loss, options)

    r = gr.enable_realtime_scheduling()
    if r != gr.RT_OK:
        print "Warning: failed to enable realtime scheduling"

    top.start()                       # start flow graph

    if options.sender:
        top.run(options.runtime)
    top.wait()                       # wait for it to finish


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print "Bye"
        pass
