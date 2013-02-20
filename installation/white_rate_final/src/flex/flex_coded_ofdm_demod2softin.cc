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

#include <flex_coded_ofdm_demod2softin.h>
#include <gr_io_signature.h>
#include <math.h>
#include <cstdio>
#include <stdexcept>
#include <iostream>

#define VERBOSE 0

flex_coded_ofdm_demod2softin_sptr
flex_make_coded_ofdm_demod2softin(int tag) {
  return flex_coded_ofdm_demod2softin_sptr(new flex_coded_ofdm_demod2softin(tag));
}

flex_coded_ofdm_demod2softin::flex_coded_ofdm_demod2softin(int tag)
  : gr_block ("coded_ofdm_demod2softin",
	      gr_make_io_signature3 (3, 3, sizeof(float), sizeof(char), sizeof(char)),
	      gr_make_io_signature2 (2,2,sizeof(float), sizeof(char))),
    d_tag(tag) {
  
  if (VERBOSE) fprintf(stderr,"flex_coded_ofdm_demod2softin: d_tag %d\n", d_tag); 
  
  set_output_multiple(2);
}

flex_coded_ofdm_demod2softin::~flex_coded_ofdm_demod2softin ()
{

}



void 
flex_coded_ofdm_demod2softin::forecast (int noutput_items, 
                            gr_vector_int &ninput_items_required) {

  ninput_items_required[0] = ninput_items_required[1] = ninput_items_required[2] = noutput_items;
}

int
flex_coded_ofdm_demod2softin::general_work (int noutput_items,
        gr_vector_int &ninput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
{
  const float *in_conf = (const float *) input_items[0];
  const char *in_bits = (const char *) input_items[1];
  const char *in_sel = (const char *) input_items[2];
  
  float *out = (float *)output_items[0];
  char *out_sel = (char *)output_items[1];
    
  int numin = ninput_items[0];
  for(int i=0; i< input_items.size(); i++)
    if(ninput_items[i] < numin)
      numin = ninput_items[i];

  int optr = 0;
  
  if (VERBOSE) fprintf(stderr,"flex_coded_ofdm_demod2softin: noutput_items=%d, ninput_items=%d\n", noutput_items, numin);

  for(int i=0; i < numin; i++) {
    char rate = in_sel[i];
    char bits = in_bits[i];
    float conf = in_conf[i];
    int numbits = 0;

	if (VERBOSE) fprintf(stderr,"flex_coded_ofdm_demod2softin: rate %x\n", rate);

    switch(rate) {
    case CODED_OFDM_RATE_BPSK_1_2:
    case CODED_OFDM_RATE_BPSK_3_4:
      numbits = 1;
      break;

    case CODED_OFDM_RATE_QPSK_1_2:
    case CODED_OFDM_RATE_QPSK_3_4:
      numbits = 2;
      break;
      
    case CODED_OFDM_RATE_8QAM_1_2:
    case CODED_OFDM_RATE_8QAM_3_4:
      numbits = 3;
      break;
      
    case CODED_OFDM_RATE_16QAM_1_2:
    case CODED_OFDM_RATE_16QAM_3_4:
      numbits = 4;
      break;

    case CODED_OFDM_RATE_64QAM_1_2:
    case CODED_OFDM_RATE_64QAM_2_3:
    case CODED_OFDM_RATE_64QAM_3_4:
      numbits = 6;
      break;

    case CODED_OFDM_RATE_256QAM_1_2:
    case CODED_OFDM_RATE_256QAM_3_4:
      numbits = 8;
      break;
      
    case CODED_OFDM_DEMOD_HDR_SEL:
      numbits = 1;
      break;

    default:
      printf("flex_coded_ofdm_demod2softin: unknown rate %x\n", rate);
      assert(0);
    }

	if (VERBOSE) fprintf(stderr,"flex_coded_ofdm_demod2softin: input %d, conf %f bits %x (numbits %d) rate %x\n", i, conf, bits, numbits, rate);

    for(int j=0; j <numbits; j++) {
      char bit = bits & 0x01;

      if(bit == 0) {
	out[optr] = 0.0;
	out[optr+1] = conf;
      }
      else {
	out[optr] = conf;
	out[optr+1] = 0.0;
      }

      out_sel[optr] = rate;
      out_sel[optr+1] = rate;

	if (VERBOSE) fprintf(stderr,"flex_coded_ofdm_demod2softin: bits %x bit %x, out %f %f, outsel %x %x\n",
	     bits, bit, out[optr], out[optr+1], out_sel[optr], out_sel[optr+1]);

      bits >>= 1;
      optr += 2;
    }

  }
  
 if (VERBOSE) fprintf(stderr,"flex_coded_ofdm_demod2softin: produced %d output items\n", optr);

  consume_each(numin);
  return optr;

}
