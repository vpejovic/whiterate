/* -*- c++ -*- */
/*
 * Copyright 2007 Free Software Foundation, Inc.
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

#include <flex_ofdm_deinterleaver.h>
#include <gr_io_signature.h>
#include <math.h>
#include <cstdio>
#include <stdexcept>
#include <iostream>

#define VERBOSE 0
#define INTERLEAVE 0

flex_ofdm_deinterleaver_sptr
flex_make_ofdm_deinterleaver(int occ_tones, int itemsize) {
  return flex_ofdm_deinterleaver_sptr(new flex_ofdm_deinterleaver(occ_tones, itemsize));
}

flex_ofdm_deinterleaver::flex_ofdm_deinterleaver(int occ_tones, int itemsize)
  : gr_block ("ofdm_deinterleaver",
	      gr_make_io_signature2 (2, 2, sizeof(float), sizeof(char)),
	      gr_make_io_signature2 (2,2, sizeof(float), sizeof(char))),
    d_occ_tones(occ_tones), d_itemsize(itemsize), 
    d_total_sym_produced(0)
{
  
#if VERBOSE
  printf("flex_ofdm_deinterleaver: occ_tones=%d, itemsize=%d\n", d_occ_tones, d_itemsize);
#endif  
  d_rate = 0;
}

flex_ofdm_deinterleaver::~flex_ofdm_deinterleaver ()
{

}


void 
flex_ofdm_deinterleaver::forecast (int noutput_items, 
				 gr_vector_int &ninput_items_required) {

  ninput_items_required[0] = ninput_items_required[1] = noutput_items;

}

int
flex_ofdm_deinterleaver::deinterleave_index(int j) {

  if(!INTERLEAVE)
    return j;

  int s = std::max(d_bpsc/2, 1);
  int i = s*(int) floor(j/s) + (j+(int)floor(d_num_chunks*j/d_cbps)) % s;
  int k = d_num_chunks*i - (d_cbps -1)*(int)floor(d_num_chunks*i/d_cbps);

#if VERBOSE > 1
  printf("flex_ofdm_interleaver: j=%d i=%d k=%d s=%d", j, i,k,s);
#endif

  assert(k<d_cbps);
  assert(i<d_cbps);
  assert(j<d_cbps);

  return k;
}

void
flex_ofdm_deinterleaver::switch_rate() {

  switch(d_rate) {

  case CODED_OFDM_RATE_BPSK_1_2:
  case CODED_OFDM_RATE_BPSK_3_4:
    d_bpsc = 1;
    break;

  case CODED_OFDM_RATE_QPSK_1_2:
  case CODED_OFDM_RATE_QPSK_3_4:
    d_bpsc = 2;
    break;
    
  case CODED_OFDM_RATE_8QAM_1_2:
  case CODED_OFDM_RATE_8QAM_3_4:
    d_bpsc = 3;
    break;    

  case CODED_OFDM_RATE_16QAM_1_2:
  case CODED_OFDM_RATE_16QAM_3_4:
    d_bpsc = 4;
    break;

  case CODED_OFDM_RATE_64QAM_1_2:
  case CODED_OFDM_RATE_64QAM_2_3:
  case CODED_OFDM_RATE_64QAM_3_4:
    d_bpsc = 6;
    break;

  case CODED_OFDM_RATE_256QAM_1_2:
  case CODED_OFDM_RATE_256QAM_3_4:
    d_bpsc = 8;
    break;

  default:
    printf("flex_ofdm_deinterleaver: unknown rate %d\n", d_rate);
    assert(0);
  }

  d_cbps = d_bpsc*d_occ_tones;
  //d_num_chunks = (int) d_cbps/16;
  d_num_chunks = (int) d_cbps/24;


  d_items_per_sym = d_cbps*d_itemsize;

#if VERBOSE
    printf("flex_ofdm_deinterleaver: new rate %x items_per_sym %d cbps=%d cbpb=%d bpsc=%d\n",
	   d_rate, d_items_per_sym, d_cbps, d_num_chunks, d_bpsc);
#endif
}

int
flex_ofdm_deinterleaver::general_work (int noutput_items,
        gr_vector_int &ninput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
{
  const float *in_conf = (const float *) input_items[0];
  const char *in_sel = (const char *) input_items[1];
  
  float *out_conf = (float *)output_items[0];
  char *out_sel = (char *)output_items[1];

#if VERBOSE
  printf("flex_ofdm_deinterleaver: out=%d in0=%d in1=%d total_sym=%d\n", 
	 noutput_items, ninput_items[0], ninput_items[1], d_total_sym_produced);
#endif

  if(d_rate != in_sel[0]) {
    d_rate = in_sel[0];
    switch_rate();
    set_output_multiple(d_items_per_sym);
    return 0;
  }



  assert(noutput_items % d_items_per_sym == 0);

  int nblocks = noutput_items/d_items_per_sym;

  for(int n=0; n < nblocks; n++) {

    //copy n-th block of inputs to buffer
    for(int k=0; k < d_items_per_sym; k++) {
      d_sym_conf[k] = in_conf[n*d_items_per_sym + k];
      d_sym_sel[k] = in_sel[n*d_items_per_sym + k];
	  //printf("d_sym_sel[%d]=%d\n", k, d_sym_sel[k]);
    }
    //printf("\n");
    //reset bitmap
    for(int i=0; i < d_items_per_sym; i++)
      d_sym_bitmap[i] = 0;

    //copy from buffer to n-th block of outputs
    for(int i=0; i < d_items_per_sym/d_itemsize;  i++) {
      int deinter_ind = deinterleave_index(i);
      for(int j=0; j < d_itemsize; j++) {
	out_conf[n*d_items_per_sym + deinter_ind*d_itemsize+j] = d_sym_conf[i*d_itemsize+j];
	out_sel[n*d_items_per_sym + deinter_ind*d_itemsize+j] = d_sym_sel[i*d_itemsize+j];
	//printf("out[%d]=d_sym_sel[%d]=%d\n", n*d_items_per_sym + deinter_ind*d_itemsize+j, i*d_itemsize+j, out_sel[n*d_items_per_sym + deinter_ind*d_itemsize+j]);
	d_sym_bitmap[deinter_ind*d_itemsize+j] += 1;
      }
    }

    //sanity check
    for(int i=0; i < d_items_per_sym; i++)
      if(d_sym_bitmap[i] != 1) {
	printf("Error in interleaving: bit position %d used %d times\n",
	       i, d_sym_bitmap[i]);
	assert(0);
      }

    d_total_sym_produced++;
  }

  consume_each(noutput_items);
  return noutput_items;
}
