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

#include <trellis_puncturing.h>
#include <gr_io_signature.h>
#include <iostream>
#include <cstdio>

#define VERBOSE 0

trellis_puncturing_sptr 
trellis_make_puncturing (int P, int N, int K, int skip_bits,
			 const std::vector<int> &TABLE)
{
  return trellis_puncturing_sptr (new trellis_puncturing (P, N, K, skip_bits, TABLE));
}

trellis_puncturing::trellis_puncturing (int P, int N, int K, int skip_bits, 
					const std::vector<int> &TABLE)
  : gr_block ("puncturing",
	      gr_make_io_signature2 (2, 2, sizeof(unsigned char), sizeof(char)),
	      gr_make_io_signature2 (2, 2, sizeof(unsigned char), sizeof(char))),
    d_P(P), d_N (N), d_K(K), d_skip_bits(skip_bits), 
    d_TABLE (TABLE)
{
    set_output_multiple (d_K);
    assert(d_TABLE.size() == d_P * d_N);
    d_TABLESIZE = d_TABLE.size();

    d_puncture = false; //data stream starts with header always, so dont puncture first
    d_offset = 0;

#if VERBOSE
    printf("trellis_puncturing: P=%d N=%d K=%d skip_bits=%d d_TABLESIZE=%d\n", 
	   d_P, d_N, d_K, d_skip_bits, d_TABLESIZE);
#endif
}

void 
trellis_puncturing::forecast (int noutput_items, 
			      gr_vector_int &ninput_items_required) {

  if(d_puncture) {
    //you produce d_K items from d_TABLESIZE = d_P * d_N items.
    ninput_items_required[0] = (noutput_items/d_K) * d_P * d_N;

    //input stream 1 is simply uncoded bit flags
    ninput_items_required[1] = (noutput_items/d_K) * d_P;

#if VERBOSE > 4
    printf("trellis_puncturing: forecast: puncture: out=%d in0=%d in1=%d\n",
	   noutput_items,  ninput_items_required[0],  ninput_items_required[1]);
#endif
  }
  else { 
    //we're not doing puncturing
    ninput_items_required[0] = noutput_items;
    ninput_items_required[1] = noutput_items/d_N;
    
#if VERBOSE > 4
    printf("trellis_puncturing: forecast: no puncture: out=%d in0=%d in1=%d\n",
	   noutput_items,  ninput_items_required[0],  ninput_items_required[1]);
#endif
  }
  
  //to produce 4 bits of output:
  //when not puncturing: 4 bits from in0, 2 (uncoded flag) bits from in1
  //when puncturing: 6 bits from in0, 3 bits from in1
  
}

int 
trellis_puncturing::general_work (int noutput_items,
      gr_vector_int &ninput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items)
{

#if VERBOSE > 1
  printf("trellis_puncturing: start general_work: noutput_items=%d\n", 
	 noutput_items);
#endif
 
  assert(noutput_items % d_K == 0);
  
  const unsigned char *in = (unsigned char *) input_items[0];
  const char *in_flag = (char *) input_items[1];
  unsigned char *out = (unsigned char *) output_items[0];
  char *out_flag = (char *) output_items[1];

  int outblocks = (int) noutput_items/d_K;

  int inp0 = 0, inp1 = 0, outp = 0; //pointers into the input and output streams

  for(int outb = 0; outb < outblocks; outb++)  {
    //produce d_K outputs at a time
   
    //UPDATING d_puncture after header
    //header length must occur at boundary of d_K/d_N bits
    //else the equality below will never be true.
    if(d_offset == d_skip_bits) {
#if VERBOSE
      printf("trellis_puncturing: finished %d bits. start puncturing\n", d_offset);
#endif
      
      assert(!d_puncture);
      d_puncture = true;
    }
 
    int num_inputs_0, num_inputs_1; 
    //number of input items you must consume to produce d_K outputs
    
    if(d_puncture) {
      num_inputs_0 = d_P * d_N;
      num_inputs_1 = d_P;
      
#if VERBOSE > 2
      printf("trellis_puncturing: puncture: outblock=%d outp=%d in0=%d in1=%d total_in0=%d total_in1=%d\n",
	     outb, outp, num_inputs_0, num_inputs_1, inp0, inp1);	   
#endif
      
    }
    else {
      num_inputs_0 = d_K;
      num_inputs_1 = d_K/d_N;
      
#if VERBOSE > 2
      printf("trellis_puncturing: no puncture: outblock=%d outp=%d in0=%d in1=%d total_in0=%d total_in1=%d\n",
	     outb, outp, num_inputs_0, num_inputs_1, inp0, inp1);	   
#endif
      
    }
    
    //WRITE TO OUTPUT
    for(int i = 0; i < num_inputs_0; i++) {
      if(!d_puncture || d_TABLE[ i % d_TABLESIZE ]) {
#if VERBOSE > 3
	printf("output bit %d from input bit %d\n", outp, inp0);
#endif
	out[outp] = in[inp0];
	out_flag[outp] = 0;
	outp++;
      }
      inp0++;
    }
    
 
    d_offset += num_inputs_1;
   
    //RESETTING d_puncture and d_offset at end of packet
    //packet size in bits must be a multiple of d_P for this to work correctly(?)
    for(int i=0; i < num_inputs_1; i++) {
      if(in_flag[inp1++]) {
#if VERBOSE
	printf("trellis_puncturing: end of packet in0=%d in1=%d outp=%d d_offset=%d\n", 
	       inp0, inp1, outp, d_offset);
#endif

	d_puncture = false;
	d_offset = 0;
	out_flag[outp-1] = 1;
      }
    }

 

  }



#if VERBOSE > 1
  printf("trellis_puncturing: produced %d items, consumed %d, %d items\n", outp, inp0, inp1);
#endif

  assert(outp == noutput_items);
  
  consume(0, inp0);
  consume(1, inp1);

  return noutput_items;
}
