/* -*- c++ -*- */
/*
 * Copyright 2006,2007 Free Software Foundation, Inc.
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

#ifndef INCLUDED_FLEX_NULL_MUX_H
#define INCLUDED_FLEX_NULL_MUX_H

#include <gr_block.h>
//#include <gr_sync_block.h>
#include <gr_random.h>

class flex_null_mux;
typedef boost::shared_ptr<flex_null_mux> flex_null_mux_sptr;

flex_null_mux_sptr flex_make_null_mux(size_t itemsize, float ampl);

/*!
 * \brief output[i] = input[i]
 * \ingroup misc_blk
 *
 * This block copies its input to its output, or sends zeros.
 *
 */
//class flex_null_mux : public gr_sync_block
class flex_null_mux : public gr_block
{
  size_t		d_itemsize;
  gr_random     d_rng;
  float                 d_ampl;
  friend flex_null_mux_sptr flex_make_null_mux(size_t itemsize, float ampl);
  flex_null_mux(size_t itemsize, float ampl);

 public:

  bool check_topology(int ninputs, int noutputs);

  int work(int noutput_items,
            gr_vector_int &ninput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
            
  void forecast (int noutput_items, gr_vector_int &ninput_items_required);
 
  int fixed_rate_ninput_to_noutput(int ninput);
  int fixed_rate_noutput_to_ninput(int noutput);
  
  int  general_work (int noutput_items,
		     gr_vector_int &ninput_items,
		     gr_vector_const_void_star &input_items,
		     gr_vector_void_star &output_items);

};

#endif

