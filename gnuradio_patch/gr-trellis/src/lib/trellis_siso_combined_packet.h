/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_TRELLIS_siso_combined_packet_H
#define INCLUDED_TRELLIS_siso_combined_packet_H

#include "fsm.h"
#include "trellis_siso_type.h"
#include "trellis_calc_metric.h"
#include <gr_block.h>
#include <gr_message.h>
#include <gr_msg_queue.h>

class trellis_siso_combined_packet;
typedef boost::shared_ptr<trellis_siso_combined_packet> trellis_siso_combined_packet_sptr;

trellis_siso_combined_packet_sptr trellis_make_siso_combined_packet (
    unsigned char tag, // tag identifying which instantiation this is 
    const fsm &FSM, 	// underlying FSM
    int S0,		// initial state (put -1 if not specified)
    int SK,		// final state (put -1 if not specified)
    bool POSTI,		// true if you want a-posteriori info about the input symbols to be mux-ed in the output
    bool POSTO,		// true if you want a-posteriori info about the output symbols to be mux-ed in the output
    trellis_siso_type_t d_SISO_TYPE, // perform "min-sum" or "sum-product" combining 
    int D,
    const std::vector<float> &TABLE,
    trellis_metric_type_t TYPE
);



class trellis_siso_combined_packet : public gr_block
{
  fsm d_FSM;
  int d_K;
  int d_S0;
  int d_SK;
  bool d_POSTI;
  bool d_POSTO;
  trellis_siso_type_t d_SISO_TYPE;
  int d_D;
  std::vector<float> d_TABLE;
  trellis_metric_type_t d_TYPE;
  //std::vector<float> d_alpha;
  //std::vector<float> d_beta;

  friend trellis_siso_combined_packet_sptr trellis_make_siso_combined_packet (
    unsigned char tag,
    const fsm &FSM,
    int S0,
    int SK,
    bool POSTI,
    bool POSTO,
    trellis_siso_type_t d_SISO_TYPE,
    int D,
    const std::vector<float> &TABLE,
    trellis_metric_type_t TYPE);


  trellis_siso_combined_packet (
    unsigned char tag,
    const fsm &FSM,
    int S0,
    int SK,
    bool POSTI,
    bool POSTO,
    trellis_siso_type_t d_SISO_TYPE,
    int D,
    const std::vector<float> &TABLE,
    trellis_metric_type_t TYPE);


public:
  fsm FSM () const { return d_FSM; }
  int K () const { return d_K; }
  int S0 () const { return d_S0; }
  int SK () const { return d_SK; }
  bool POSTI () const { return d_POSTI; }
  bool POSTO () const { return d_POSTO; }
  trellis_siso_type_t SISO_TYPE () const { return d_SISO_TYPE; }
  int D () const { return d_D; }
  std::vector<float> TABLE () const { return d_TABLE; }
  trellis_metric_type_t TYPE () const { return d_TYPE; }
  gr_msg_queue_sptr sisoctl_msgq() const { return d_sisoctl_msgq; }
  void forecast (int noutput_items,
                 gr_vector_int &ninput_items_required);
  int general_work (int noutput_items,
                    gr_vector_int &ninput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items);
	
  /* Added by Souvik */
  gr_msg_queue_sptr d_body_siso_msgq;
  void consume_old_samples(int num_samples, unsigned char which_input);
  void store_samples(float * new_samples, int num_samples, unsigned char which_input);
	
  gr_msg_queue_sptr	body_siso_msgq() const { 
		return d_body_siso_msgq; 
  }

protected:
  gr_msg_queue_sptr d_sisoctl_msgq;
  bool d_input_ready;
  int d_multiple;
  unsigned char d_tag;
  bool d_flush;
  int d_flush_input0;
  int d_flush_input1;

  int d_K_total;

  /* Added by Souvik */
  float old_input[2][176000];
  int num_oldinput[2];
};

#endif