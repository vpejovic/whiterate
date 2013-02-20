/* -*- c++ -*- */
/*
 * Copyright 2008 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <trellis_depuncturing.h>
#include <gr_io_signature.h>
#include <iostream>
#include <gr_ofdm_constants.h>

#define VERBOSE 0

static const float INF = 1.0e9;

trellis_depuncturing_sptr 
trellis_make_depuncturing (int itemsize,
			 const std::vector<int> &TABLE)
{
  return trellis_depuncturing_sptr (new trellis_depuncturing (itemsize, TABLE));
}

trellis_depuncturing::trellis_depuncturing (int itemsize, 
					const std::vector<int> &TABLE)
  : gr_block ("depuncturing",
	      gr_make_io_signature2 (2, 2, sizeof(float), sizeof(char)),
	      gr_make_io_signature (1, 1, sizeof(float))),
    d_itemsize(itemsize), d_TABLE (TABLE), d_depunct_on(false),
    d_total_produced(0), d_total_consumed(0)
{

  
  d_TABLESIZE = d_TABLE.size();
  //set_output_multiple((int)(d_TABLESIZE*d_itemsize));
  set_output_multiple(2);
  
#if VERBOSE
  printf("trellis_depuncturing: itemsize=%d tablesz=%d\n", 
	 d_itemsize, d_TABLESIZE);
#endif
}

void 
trellis_depuncturing::forecast (int noutput_items, 
			      gr_vector_int &ninput_items_required) {

  if(d_depunct_on) 
    ninput_items_required[0] =  ninput_items_required[1] = 2*noutput_items/3; 
  else
    ninput_items_required[0] =  ninput_items_required[1] = noutput_items; 
}

int 
trellis_depuncturing::general_work (int noutput_items,
      gr_vector_int &ninput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items)
{


  const float *in = (float *) input_items[0];
  const char *in_rate = (char *) input_items[1];
  
  d_rate = in_rate[0];

#if VERBOSE > 1
  printf("trellis_depuncturing: start general_work: noutput_items=%d, rate=%x\n", 
	 noutput_items, d_rate);
#endif

  if(!d_depunct_on && 
     (d_rate == CODED_OFDM_RATE_BPSK_3_4  ||
      d_rate == CODED_OFDM_RATE_QPSK_3_4  ||
      d_rate == CODED_OFDM_RATE_8QAM_3_4 ||      
      d_rate == CODED_OFDM_RATE_16QAM_3_4 ||
      d_rate == CODED_OFDM_RATE_64QAM_2_3 || 
      d_rate == CODED_OFDM_RATE_64QAM_3_4 ||
      d_rate == CODED_OFDM_RATE_256QAM_3_4 )) {
    
    set_output_multiple((int)(d_TABLESIZE*d_itemsize));
    d_depunct_on = true;

#if VERBOSE
    printf("trellis_depuncturing: start depuncturing, rate=%x\n", d_rate);
#endif

    return 0;
  }

  if(d_depunct_on && 
     (d_rate == CODED_OFDM_RATE_BPSK_1_2 ||
      d_rate == CODED_OFDM_RATE_QPSK_1_2 ||
      d_rate == CODED_OFDM_RATE_8QAM_1_2 ||      
      d_rate == CODED_OFDM_RATE_64QAM_1_2 || //Veljko
      d_rate == CODED_OFDM_RATE_256QAM_1_2 ||
      d_rate == CODED_OFDM_RATE_16QAM_1_2)) {
    
    set_output_multiple(2);
    d_depunct_on = false;

#if VERBOSE
    printf("trellis_depuncturing: stop depuncturing, rate=%x\n", d_rate);
#endif
    return 0;
  }
  
  if(d_depunct_on) 
    assert(noutput_items % (int) (d_itemsize*d_TABLESIZE) == 0);
  
  float *out = (float *) output_items[0];
  int inp = 0, outp = 0;
  
  
  while(outp < noutput_items && in_rate[inp] == d_rate) {

    int blocknum = (int) outp/d_itemsize;

    if(d_depunct_on && d_TABLE[blocknum % d_TABLESIZE] == 0) {
      
      for(int j=0; j < d_itemsize; j++) {

#if VERBOSE > 1
	printf("trellis_depuncturing: output item %d DUMMY\n", outp);
#endif
	out[outp++] = 0;

      }
      
    }
    else {
      for(int j=0; j < d_itemsize; j++) {

#if VERBOSE > 1
	printf("trellis_depuncturing: output item %d from input item %d\n", 
	       outp, inp);
#endif
	out[outp++] = in[inp++];
      }
    }

	//if(in_rate[inp] != d_rate)
		//printf("inrate[%d] = %d\n", inp, in_rate[inp]);
    
  }

  d_total_consumed += inp;
  d_total_produced += outp;
  
#if VERBOSE
  printf("trellis_depuncturing: end general_work: noutput_items=%d, produced %d (total=%d). consumed %d (total=%d), rate=%x, last_rate=%x\n", 
	 noutput_items, outp, d_total_produced, inp, d_total_consumed, d_rate, in_rate[inp]);
#endif

  consume_each(inp);
  return outp;
}
