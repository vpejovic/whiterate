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
#include <iostream>

#define VERBOSE 0

@SPTR_NAME@ 
trellis_make_@BASE_NAME@ (const fsm &FSM, int ST)
{
  return @SPTR_NAME@ (new @NAME@ (FSM,ST));
}

@NAME@::@NAME@ (const fsm &FSM, int ST)
  : gr_sync_block ("@BASE_NAME@",
		   gr_make_io_signature2 (2, 2, sizeof (@I_TYPE@), sizeof(char)),
		   gr_make_io_signature (1, 1, sizeof (@O_TYPE@))),
    d_FSM (FSM),
    d_ST (ST),
    d_INIT_ST (ST)
{
}



int 
@NAME@::work (int noutput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items)
{
  int ST_tmp=0;
  int nstreams = input_items.size();

  const @I_TYPE@ *in = (const @I_TYPE@ *) input_items[0];
  const char *in_flag = (const char *) input_items[1];
  @O_TYPE@ *out = (@O_TYPE@ *) output_items[0];
  ST_tmp = d_ST;

  for (int i = 0; i < noutput_items; i++){
    if (in_flag[i]) // transition to initial state if we see the flag set
      ST_tmp = d_INIT_ST;
    out[i] = (@O_TYPE@) d_FSM.OS()[ST_tmp*d_FSM.I()+in[i]]; // direction of time?
    ST_tmp = (int) d_FSM.NS()[ST_tmp*d_FSM.I()+in[i]];
  }
  d_ST = ST_tmp;

#if VERBOSE
  printf("framed_encoder: produced %d items\n", noutput_items);
#endif

  return noutput_items;
}

