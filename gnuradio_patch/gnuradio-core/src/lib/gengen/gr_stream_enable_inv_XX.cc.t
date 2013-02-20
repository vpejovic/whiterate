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

// @WARNING@

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <@NAME@.h>
#include <gr_io_signature.h>
#include <assert.h>
#include <gr_log2_const.h>

#define VERBOSE 0 

@SPTR_NAME@ 
gr_make_@BASE_NAME@ (unsigned char enable_val)
{
  return @SPTR_NAME@ 
    (new @NAME@ (enable_val));
}

@NAME@::@NAME@ (unsigned char enable_val)
  : gr_block ("@BASE_NAME@",
	      gr_make_io_signature2 (2, 2, sizeof (@I_TYPE@), sizeof (char)),
	      gr_make_io_signature2 (2, 2, sizeof (@O_TYPE@), sizeof(char))), 
    d_enable_val(enable_val)
{
}

void
@NAME@::forecast(int noutput_items, gr_vector_int &ninput_items_required)
{

  int input_required = noutput_items;
  unsigned ninputs = ninput_items_required.size();
  for (unsigned int i = 0; i < ninputs; i++) {
    ninput_items_required[i] = input_required;
  }
}

int
@NAME@::general_work (int noutput_items,
					gr_vector_int &ninput_items,
					gr_vector_const_void_star &input_items,
					gr_vector_void_star &output_items)
{
  const @I_TYPE@ *in = (@I_TYPE@ *) input_items[0];
  const char *in_enable_val = (const char *) input_items[1];
  @O_TYPE@ *out = (@O_TYPE@ *) output_items[0];
  char *out_enable_val = (char *) output_items[1];

  int numin = (ninput_items[0] > ninput_items[1]) ? ninput_items[1] : ninput_items[0];
  int opos = 0;

  for (int ipos = 0; ipos < numin; ipos++) {
    if (in_enable_val[ipos] != d_enable_val) {
      out[opos] = in[ipos];
      out_enable_val[opos] = in_enable_val[ipos];
      opos++;
    }
  }

  consume_each (numin);
#if VERBOSE
  printf("@NAME@(0x%02hx): consumed %d, noutput_items=%d\n", 
         d_enable_val, numin, opos);
#endif
  return opos;
}
