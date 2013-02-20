#!/usr/bin/env python
#
# WhiteRate algorithm
# TODO: lossless tx time should be time for that width

from collections import defaultdict
import sys, math
from random import choice

_C = 10 # successive fails tolerated
_L = 5  # minimum packets on a width, mcs combination

# NOTE: MCS are coded with 1/2 rate unless followed by ++ in which case 3/4 code is used
_valid_mcs  = ['bpsk',  'qpsk', 'qam8', 'qam16', 'qam64']
_coding_rate = {'bpsk':2,  'qpsk':2, 'qam8':2, 'qam16':2, 'qam64':2}
_bits_per_tone = {'bpsk':1, 'qpsk':2, 'qam8':3, 'qam16':4, 'qam64':6}
_INIT = 1
_NORMAL = 2
_PROBING = 3
_refresh_period = 100

class state_table(object):
    def __init__(self, max_loss, num_groups, group_width, time_per_subcarrier):#, loss_table):
        self.table = {}

        self.probe_cnt = 0
        self.state = _INIT
        self.max_loss = max_loss
        self.margin_loss = max_loss*0.3 # TODO: how to select the margin?
        self.weight = 0.1
        self._max_width = (num_groups-1)*group_width
        self._min_width = group_width
        self._delta_width = group_width
        self.total_updates = 0
        self._time_per_subcarrier = time_per_subcarrier
        for candidate_mcs in _valid_mcs:
            self.table[candidate_mcs]= defaultdict(int)
            self.table[candidate_mcs]["succ_fails"] = 0
            self.table[candidate_mcs]["loss"] = -1
            self.table[candidate_mcs]["tries"] = 0
            self.table[candidate_mcs]["packets_acked"] = 0
            self.table[candidate_mcs]["width"] = self._max_width
            self.table[candidate_mcs]["total_tx_time"] = 0
            self.table[candidate_mcs]["average_tx_time"] = 0
            self.table[candidate_mcs]["lossless_tx_time"] = _coding_rate[candidate_mcs]*float(self._time_per_subcarrier)/(_bits_per_tone[candidate_mcs]*self._max_width)
            self.table[candidate_mcs]["last_succ_fail"] = 0


    def update_table(self, current_mcs, current_width, ok):
        self.total_updates += 1
        self.table[current_mcs]["width"] = current_width
        self.table[current_mcs]["tries"] += 1
        if int(ok) == 0:
            self.table[current_mcs]["succ_fails"] += 1
            self.table[current_mcs]["last_succ_fail"] = self.total_updates
        else:
            self.table[current_mcs]["succ_fails"] = 0
            self.table[current_mcs]["packets_acked"] += 1

        if self.table[current_mcs]["loss"] == -1:
            self.table[current_mcs]["loss"] = 1-int(ok)
        else:
            self.table[current_mcs]["loss"] =  self.weight*(1-int(ok)) + (1 - self.weight)*self.table[current_mcs]["loss"]
        self.table[current_mcs]["average_tx_time"] = (1+self.table[current_mcs]["loss"])*_coding_rate[current_mcs]*float(self._time_per_subcarrier)/(_bits_per_tone[current_mcs]*current_width)
        if self.table[current_mcs]["succ_fails"] == _C:
            self.table[current_mcs]["average_tx_time"] = sys.maxint

        for candidate_mcs in _valid_mcs:
            if self.total_updates - self.table[candidate_mcs]["last_succ_fail"] > _refresh_period:
                self.table[candidate_mcs]["succ_fails"] = 0
                self.table[candidate_mcs]["last_succ_fail"] = self.total_updates
                self.table[candidate_mcs]["tries"] = 0
                self.table[candidate_mcs]["loss"] = -1
                self.table[candidate_mcs]["average_tx_time"] = 0
                self.table[candidate_mcs]["width"] = self._max_width

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
			# TODO: THIS NEEDS REVISION!
            for i in range(len(_candidates)):
                return _candidates[i], self.get_width(_candidates[i], self.table[_candidates[i]]["width"], self.table[_candidates[i]]["loss"])
            return current_mcs, current_width


    def get_width(self, candidate_mcs, candidate_width, candidate_loss):
        # The algorithm should know the ordering of the channel widths, based on the periodic PN seq probing
        # We assume that wider channels result in higher loss
        if candidate_loss == -1:
            return self._max_width
        if candidate_loss >= (self.max_loss - self.margin_loss) and (candidate_loss <= self.max_loss): #need the same width
            return candidate_width

        if candidate_loss > self.max_loss: # return the highest one with the loss less than the limit
            #print "Return lower width"
            if candidate_width - self._delta_width >= self._min_width:
                return (candidate_width - self._delta_width)
            else:
                return self._min_width
        else:
            # we need higher width
            if candidate_width + self._delta_width <= self._max_width:
                return candidate_width + self._delta_width
            else:
                return self._max_width

    def pick_probe(self, current_mcs, current_width):
        #print "Picking probe"
        # that hasn't experienced c consecutive losses and can result in better performance
        # the algorithm should know the ordering of the channel widths, based on the periodic PN seq probing
        # The probing provides ordered widths so that wider channels result in higher loss
        # TODO: sampling has to pick a completely random mcs every once a while, e.g. every 100th time
        _candidates = []
        for candidate_mcs in reversed(_valid_mcs):
            if (self.table[candidate_mcs]["lossless_tx_time"]*float(min(self.table[candidate_mcs]["width"]+self._delta_width, self._max_width))/self._max_width < self.table[current_mcs]["average_tx_time"]) and (self.table[candidate_mcs]["succ_fails"] < _C) and (candidate_mcs != current_mcs):
                _candidates.append(candidate_mcs)
        if (_candidates):
            probing_mcs = choice(_candidates)
            #probing_mcs = _candidates[0]
        else:
            probing_mcs = "qam64"

        if self.table[probing_mcs]["loss"] > self.max_loss + self.margin_loss: # return the highest one with the loss less than the limit
            #_min_loss = self.table[probing_mcs]["loss"]
            if self.table[probing_mcs]["width"] - self._delta_width >= self._min_width:
                return probing_mcs, self.table[probing_mcs]["width"] - self._delta_width
            else:
                return  probing_mcs, self._min_width
        elif self.table[probing_mcs]["loss"] < self.max_loss - self.margin_loss:  
            if self.table[probing_mcs]["width"] + self._delta_width <= self._max_width:
                return probing_mcs, self.table[probing_mcs]["width"] + self._delta_width
            else:
                return  probing_mcs, self._max_width
        else:
            return probing_mcs, self.table[probing_mcs]["width"]

    def find_rate(self, current_mcs, current_width, current_state):
        if current_state != _PROBING and self.table[current_mcs]["tries"]%_L !=0:
            return current_mcs, current_width

        if current_state == _INIT:
            return "qam64", self._max_width

        if not (current_state == _PROBING):
            if self.table[current_mcs]["tries"] < _C:
                return current_mcs, current_width

            if self.table[current_mcs]["succ_fails"] >= _C:
                for candidate_mcs in reversed(_valid_mcs):
                    if self.table[candidate_mcs]["succ_fails"] < _C:
                        return candidate_mcs, current_width

            _new_mcs, _new_width = self.best_rate(current_mcs, current_width)

            return _new_mcs, _new_width

        elif current_state == _PROBING:
            return self.pick_probe(current_mcs, current_width)


    def print_table(self):
        print 'RATE\tTRIES\tACKED\tSUCC_FAILS\tLOSS\tWIDTH\tTOTAL_TX_TIME\tAVG_TX_TIME\tLOSSLESS_TX_TIME'
        for candidate_mcs in _valid_mcs:
            print '| %06s \t%d\t%d\t%d\t%.2f\t%d\t%.3f\t%.3f\t%.3f|\n' % (candidate_mcs, self.table[candidate_mcs]["tries"], \
            self.table[candidate_mcs]["packets_acked"],self.table[candidate_mcs]["succ_fails"], \
            self.table[candidate_mcs]["loss"], self.table[candidate_mcs]["width"], self.table[candidate_mcs]["total_tx_time"], \
            self.table[candidate_mcs]["average_tx_time"]*1000000, self.table[candidate_mcs]["lossless_tx_time"]*1000000)
