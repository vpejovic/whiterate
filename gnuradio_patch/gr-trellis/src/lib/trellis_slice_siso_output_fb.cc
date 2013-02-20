/* -*- c++ -*- */
/*
 * Copyright 2004,2006 Free Software Foundation, Inc.
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

#include <trellis_slice_siso_output_fb.h>
#include <gr_io_signature.h>
#include <cstdio>

#define VERBOSE 0

trellis_slice_siso_output_fb_sptr
trellis_make_slice_siso_output_fb (unsigned int D,
                                   const std::vector<unsigned char> &tbl) {
  return trellis_slice_siso_output_fb_sptr (new trellis_slice_siso_output_fb (D, tbl));
}

trellis_slice_siso_output_fb
  ::trellis_slice_siso_output_fb (unsigned int D, 
                                  const std::vector<unsigned char> &tbl)
    : gr_sync_decimator ("slice_siso_output_fb",
        gr_make_io_signature (1, 1, sizeof(float)),
        gr_make_io_signature (1, 1, sizeof(char)), 
        D), // decimation factor is D
      d_D(D), d_tbl(tbl) { 
  assert(tbl.size() == D);
#if VERBOSE
  printf("trellis_slice_siso_output_fb: ");
  for (unsigned int i = 0; i < d_tbl.size(); i++)
    printf("d_tbl[%d]=%hx ", i, d_tbl[i]);
  printf("\n");
#endif
}

int
trellis_slice_siso_output_fb::work (int noutput_items,
			     gr_vector_const_void_star &input_items,
			     gr_vector_void_star &output_items)
{
  const float *in = (const float *) input_items[0];
  unsigned char *out = (unsigned char *) output_items[0];
  
#if VERBOSE
  printf("trellis_slice_siso_output_fb::work: noutput_items=%d\n", noutput_items);
#endif

  for (int i = 0; i < noutput_items; i++) {
    float max = -(float)INFINITY;
    int maxd = 0;
 
    for (unsigned int d = 0; d < d_D; d++) {
      if (in[i*d_D + d] > max) {
        maxd = d;
        max = in[i*d_D + d];
      }
      //MV
      //printf("iter=%d bit=%u prob=%.3f\n", i, d, in[i*d_D + d]);

    }
#if VERBOSE > 1 
    if(noutput_items > 48) {
      printf("trellis_slice_siso_output_fb::work: maxd=%d for i=%d -> %hx maxprob=%.3f\n", 
	     maxd, i, d_tbl[maxd], max);
    }
#endif
    out[i] = d_tbl[maxd];
  }

  return noutput_items;
}
