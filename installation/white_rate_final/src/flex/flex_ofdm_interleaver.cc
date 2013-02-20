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

#include <flex_ofdm_interleaver.h>
#include <gr_io_signature.h>
#include <math.h>
#include <cstdio>
#include <stdexcept>
#include <iostream>

#define VERBOSE 0
#define INTERLEAVE 0

flex_ofdm_interleaver_sptr
flex_make_ofdm_interleaver(int occ_tones, int bits_per_sym, int skip_bits) {
  return flex_ofdm_interleaver_sptr(new flex_ofdm_interleaver(occ_tones, bits_per_sym, skip_bits));
}

flex_ofdm_interleaver::flex_ofdm_interleaver(int occ_tones, int bits_per_sym, int skip_bits)
  : gr_block ("ofdm_interleaver",
	      gr_make_io_signature2 (2, 2, sizeof(char), sizeof(char)),
	      gr_make_io_signature2 (2,2, sizeof(char), sizeof(char))),
    d_occ_tones(occ_tones), d_bpsc(bits_per_sym), d_skip_bits(skip_bits), d_interleave(false),
    d_sym_offset(0), d_stream_offset(0) {
  
  d_cbps = d_occ_tones * d_bpsc;

  d_num_chunks = (int)(d_cbps/24); //16 bits per chunk

#if VERBOSE
  printf("flex_ofdm_interleaver: occ_tones=%d cbps=%d bpsc=%d cbpb=%d skip_bits=%d\n", 
	 d_occ_tones, d_cbps, d_bpsc, d_num_chunks, d_skip_bits); 
#endif  

  assert(d_cbps % 24 == 0); //so that d_num_chunks makes sense
  assert(d_cbps % 8 == 0); //cbps must fit on byte boundaries for various reasons
  //such as, flush does not leave depuncturere or deinterleaver in inconsistent state

  assert(d_skip_bits <= d_cbps); //header must fit in a symbol fully
  //because we do not have a flush mechanism for hdr_siso yet
  //Veljko: <= instead of <

  for(int i=0; i < MAX_BITS_PER_SYM; i++)
    d_sym_bitmap[i] = 0;
}

flex_ofdm_interleaver::~flex_ofdm_interleaver ()
{

}



int
flex_ofdm_interleaver::interleave_index(int k) {

  if(!INTERLEAVE)
    return k;

  int i = (d_cbps/d_num_chunks)*(k % d_num_chunks) + (int) floor(k/d_num_chunks);
  int s = std::max(d_bpsc/2,1);
  int j = s*(int)floor(i/s) + (i + d_cbps - (int) floor(d_num_chunks*i/d_cbps))% s;

#if VERBOSE>2
  printf("flex_ofdm_interleaver: k=%d i=%d j=%d s=%d\n", k,i,j,s);
#endif

  assert(k<d_cbps);
  assert(i<d_cbps);
  assert(j<d_cbps);
  return j;
}

void
flex_ofdm_interleaver::copy_interleaved_symbol(char *out_bits) {

#if VERBOSE > 1
  printf("flex_ofdm_interleaver: copying interleaved symbol, sym offset %d\n", 
	 d_sym_offset);
#endif

  assert(d_sym_offset == d_cbps);

  for(int k=0; k < d_cbps; k++) {
    int ind = interleave_index(k);
    out_bits[ind] = d_sym[k];
    d_sym_bitmap[ind] += 1;
  }

  //sanity check
  for(int i=0; i < d_cbps; i++)
    if(d_sym_bitmap[i] != 1) {
      printf("Error in interleaving: bit position %d used %d times\n",
	     i, d_sym_bitmap[i]);
      assert(0);
    }
  //reset
  for(int i=0; i < d_cbps; i++)
    d_sym_bitmap[i] = 0;
  d_sym_offset = 0;
}


int
flex_ofdm_interleaver::general_work (int noutput_items,
        gr_vector_int &ninput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
{
  const char *in_bits = (const char *) input_items[0];
  const char *in_sel = (const char *) input_items[1];
  
  char *out_bits = (char *)output_items[0];
  char *out_sel = (char *)output_items[1];
  
#if VERBOSE>1
  printf("flex_ofdm_interleaver: out=%d in0=%d in1=%d\n", 
	 noutput_items, ninput_items[0], ninput_items[1]);
#endif

  int numin = std::min(ninput_items[0], ninput_items[1]);

  int iptr = 0, optr = 0;

  if(!d_interleave) {

    while(iptr < numin && !in_sel[iptr] && d_stream_offset < d_skip_bits) {
      out_bits[optr] = in_bits[iptr];
      out_sel[optr] = in_sel[iptr];
      iptr++;
      optr++;
      d_stream_offset++;
    }

    //end of stream before interleaving started?
    if(in_sel[iptr]) {
      printf("flex_ofdm_interleaver: end of packet before end of header, stream off %d\n", 
	     d_stream_offset);
      assert(0);
    }

    //end of header
    if(d_stream_offset == d_skip_bits) {
      d_interleave = true;
#if VERBOSE
      printf("flex_ofdm_interleaver: stream offset %d, header done, start interleave\n",
	     d_stream_offset);
#endif

    }

  }
  else { //interleaving

    //copy as much as possible till end of symbol
    while(iptr < numin && !in_sel[iptr] && d_sym_offset < d_cbps) {
      d_sym[d_sym_offset++] = in_bits[iptr++];
      d_stream_offset++;
    }

    //collected symbol worth of data
    if(d_sym_offset == d_cbps && !in_sel[iptr]) {
#if VERBOSE > 1
      printf("flex_ofdm_interleaver: end of symbol: sym_offset %d stream_offset %d\n",
	     d_sym_offset, d_stream_offset); 
#endif

      copy_interleaved_symbol(out_bits); //copy symbol
      for(int k=0; k < d_cbps; k++)
	out_sel[k] = 0;
      optr += d_cbps;

    }
    
    else if(in_sel[iptr]) { //end of data stream

      //copy last bit
      assert(d_sym_offset < d_cbps);
      d_sym[d_sym_offset++] = in_bits[iptr++];
      d_stream_offset++;

#if VERBOSE
      printf("flex_ofdm_interleaver: data stream ended: sym_offset %d stream_offset %d\n",
	     d_sym_offset, d_stream_offset);
#endif
      while(d_sym_offset < d_cbps) 
	d_sym[d_sym_offset++] = rand() % 2;

      copy_interleaved_symbol(out_bits); //copy symbol
      optr += d_cbps;
      for(int k=0; k < d_cbps -1; k++)
	out_sel[k] = 0;
      out_sel[d_cbps -1] = 1; //mark last bit

      d_stream_offset = 0; //reset stream
      d_interleave = false; //header will follow next

    }


  }
  
#if VERBOSE > 1
  printf("flex_ofdm_interleaver: produced %d, consumed %d\n", optr, iptr);
#endif

  consume_each(iptr);
  return optr;

}
