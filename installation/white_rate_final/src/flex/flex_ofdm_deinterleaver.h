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

#ifndef INCLUDED_FLEX_ofdm_deinterleaver_H
#define INCLUDED_FLEX_ofdm_deinterleaver_H

#include <gr_block.h>
#include <flex_ofdm_constants.h>

#define MAX_ITEMS_PER_SYM 512*6*2 

class flex_ofdm_deinterleaver;
typedef boost::shared_ptr<flex_ofdm_deinterleaver> flex_ofdm_deinterleaver_sptr;

flex_ofdm_deinterleaver_sptr 
flex_make_ofdm_deinterleaver (int occ_tones, int itemsize);

/*!
 * \brief OFDM deinterleaver. Refer to the IEEE standards document for more details.
 *
 */
class flex_ofdm_deinterleaver : public gr_block
{
  friend flex_ofdm_deinterleaver_sptr 
  flex_make_ofdm_deinterleaver (int occ_tones, int itemsize);
  
private:
  int d_occ_tones;
  int d_itemsize; //eg, 2 floats together represents one bit

  int d_rate;
  int d_cbps;//coded bits per symbol
  int d_bpsc;//bits per subcarrier
  int d_num_chunks; //see interleaver for details
  int d_items_per_sym;

  float d_sym_conf[MAX_ITEMS_PER_SYM];
  char d_sym_sel[MAX_ITEMS_PER_SYM];

  int d_sym_bitmap[MAX_ITEMS_PER_SYM];

  int d_total_sym_produced;

  void switch_rate();
  int deinterleave_index(int);
 
protected:
  flex_ofdm_deinterleaver(int bits_per_sym, int skip_bits);
  
public:
  ~flex_ofdm_deinterleaver();
  
  void forecast (int noutput_items, gr_vector_int &ninput_items_required);
  int general_work(int noutput_items,
		   gr_vector_int &ninput_items,
		   gr_vector_const_void_star &input_items,
		   gr_vector_void_star &output_items);
};

#endif /* INCLUDED_FLEX_ofdm_deinterleaver_H */
