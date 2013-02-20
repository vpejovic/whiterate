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


#ifndef INCLUDED_TRELLIS_puncturing_H
#define INCLUDED_TRELLIS_puncturing_H

#include <vector>
#include <gr_block.h>

class trellis_puncturing;
typedef boost::shared_ptr<trellis_puncturing> trellis_puncturing_sptr;

trellis_puncturing_sptr trellis_make_puncturing (int P, int N, int K, int skip_bits, 
						 const std::vector<int> &TABLE);

/*!

  \brief Punctures an incoming data stream. 
  Puncturing period P. Mother code rate 1/N.
  TABLE consists of P columns, each of size N, in column major order.

  Takes nP input items and produces K output items. 
  
  See Proakis p. 497 for a description of the puncturing
  matrix.

  example (pg 499): to produce 3/4 code using a puncting matrix of [1 1 1 0 0 1]
  d_P = 3, d_N = 2, d_K = 4
 
  Skips the first uncoded d_skip_bits and starts puncturing after that.
  Set it to uncoded header length in bits.

  d_puncture is set to true initially. Becomes false after header is done.
  reset to true at end of packet.

  First input is bits coded with 1/N rate.
  Second input is uncoded bit flags.

 \ingroup block
 */



class trellis_puncturing : public gr_block
{
private:
  friend trellis_puncturing_sptr 
  trellis_make_puncturing (int P, int N, int K, int skip_bits,
			   const std::vector<int> &TABLE);

  int d_P;
  int d_N;
  int d_K;
  int d_skip_bits;

  std::vector<int> d_TABLE;
  int d_TABLESIZE;

  bool d_puncture;
  int d_offset;

  trellis_puncturing (int P, int N, int K, int skip_bits,
		      const std::vector<int> &TABLE); 

public:
  int P () const { return d_P;}
  int N () const { return d_N; }
  int K () const { return d_K;}
  int skip_bits () const {return d_skip_bits;}

  const std::vector<int> & TABLE () const { return d_TABLE; }
  void forecast (int noutput_items, gr_vector_int &ninput_items_required);

  int general_work (int noutput_items,
      gr_vector_int &ninput_items,
	    gr_vector_const_void_star &input_items,
	    gr_vector_void_star &output_items);
};

#endif
