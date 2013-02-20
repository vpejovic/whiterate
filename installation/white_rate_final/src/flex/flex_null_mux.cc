/* -*- c++ -*- */
/*
 * Copyright 2006,2009 Free Software Foundation, Inc.
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

#include <flex_null_mux.h>
#include <gr_io_signature.h>
#include <string.h>
#include <cstdio>
#include <iostream>

#define VERBOSE 0

flex_null_mux_sptr
flex_make_null_mux(size_t itemsize, float ampl)
{
  return gnuradio::get_initial_sptr(new flex_null_mux(itemsize, ampl));
}

flex_null_mux::flex_null_mux(size_t itemsize, float ampl)
  //: gr_sync_block ("null_mux",
  : gr_block ("null_mux",
	      gr_make_io_signature (1, 1, itemsize),
	      gr_make_io_signature (1, 1, itemsize)),
    d_itemsize(itemsize),
    d_rng (200),
    d_ampl (ampl)
{
    //set_fixed_rate(true);
}

void
flex_null_mux::forecast (int noutput_items, gr_vector_int &ninput_items_required)
{
  //unsigned ninputs = ninput_items_required.size();
  //for (unsigned i = 0; i < ninputs; i++)
  //  ninput_items_required[i] = fixed_rate_noutput_to_ninput (noutput_items);
}

int
flex_null_mux::fixed_rate_noutput_to_ninput(int noutput_items)
{
  return noutput_items + history() - 1;
}

int
flex_null_mux::fixed_rate_ninput_to_noutput(int ninput_items)
{
  return std::max(0, ninput_items - (int)history() + 1);
}


bool
flex_null_mux::check_topology(int ninputs, int noutputs)
{
  return ninputs == noutputs;
}

int
flex_null_mux::general_work (int noutput_items,
			     gr_vector_int &ninput_items,
			     gr_vector_const_void_star &input_items,
			     gr_vector_void_star &output_items)
{

  int	r = work (noutput_items, ninput_items, input_items, output_items);
  //if (r > 0)
  //  consume_each (r);
  return r;  
}


int
flex_null_mux::work (int noutput_items,
              gr_vector_int &ninput_items,  
		      gr_vector_const_void_star &input_items,
		      gr_vector_void_star &output_items)
{
  const uint8_t *in = (const uint8_t *) input_items[0];
  uint8_t *out = (uint8_t *) output_items[0];
  gr_complex *out_complex = (gr_complex *) output_items[0];
  int nstreams = input_items.size();

  int from_in = std::min<int>(ninput_items[0], noutput_items);
  int from_zero = std::max<int>(noutput_items - from_in, 0);

  if (VERBOSE)
       fprintf(stderr, "flex_null_mux: ninput %d, noutput %d\n", ninput_items[0], noutput_items);

  if (from_in > 0) {
    memcpy(out, in, from_in*d_itemsize);
    consume_each(from_in);
  } 
  if (from_zero > 0) {
   for (int i = 0; i < from_zero; i++)
      out_complex[from_in + i] = gr_complex(d_ampl,0) * d_rng.rayleigh_complex ();
    //memset (out+from_in*d_itemsize, 0, from_zero * d_itemsize);
  }

  if (VERBOSE)
		fprintf(stderr,"flex_null_mux: from in %d items, from zero %d items\n", from_in, from_zero);
        
  //consume_each(n);
  return from_in + from_zero;
  //return j;
  
}
