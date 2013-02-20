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

#ifndef INCLUDED_FLEX_ofdm_interleaver_H
#define INCLUDED_FLEX_ofdm_interleaver_H

#include <gr_block.h>
#include <flex_ofdm_constants.h>

#define MAX_BITS_PER_SYM 512*8 

class flex_ofdm_interleaver;
typedef boost::shared_ptr<flex_ofdm_interleaver> flex_ofdm_interleaver_sptr;

flex_ofdm_interleaver_sptr 
flex_make_ofdm_interleaver (int occ_tones, int bits_per_sym, int skip_bits);

/*!
 * \brief OFDM interleaver. Refer to the IEEE standards document for details.
 *
 */
class flex_ofdm_interleaver : public gr_block
{
  friend flex_ofdm_interleaver_sptr 
  flex_make_ofdm_interleaver (int occ_tones, int bits_per_sym, int skip_bits);
  
private:
  int d_occ_tones;
  int d_cbps; //coded bits oer symbol
  int d_bpsc; //bits per sub-carrier
  int d_num_chunks; //number of chunks in the symbol; bits striped serially to chunks; 16 in standard
  int d_skip_bits;

  bool d_interleave;

  char d_sym[MAX_BITS_PER_SYM];
  int d_sym_bitmap[MAX_BITS_PER_SYM];
  int d_sym_offset;
  int d_stream_offset;

  int interleave_index(int);
  void copy_interleaved_symbol(char *);

protected:
  flex_ofdm_interleaver(int occ_tones, int bits_per_sym, int skip_bits);
  
public:
  ~flex_ofdm_interleaver();
  
  int general_work(int noutput_items,
		   gr_vector_int &ninput_items,
		   gr_vector_const_void_star &input_items,
		   gr_vector_void_star &output_items);
};

#endif /* INCLUDED_FLEX_ofdm_interleaver_H */
