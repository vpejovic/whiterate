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

GR_SWIG_BLOCK_MAGIC(trellis,puncturing);

trellis_puncturing_sptr trellis_make_puncturing (int P, int N, int K, int skip_bits, 
						 const std::vector<int> &TABLE);

class trellis_puncturing : public gr_block
{
private:

  int d_P;
  int d_N;
  int d_K;
  int d_skip_bits;

  std::vector<int> d_TABLE;
  int d_TABLESIZE;

  trellis_puncturing (int P, int N, int K, int skip_bits, const std::vector<int> &TABLE); 

public:

  int P () const { return d_P;}
  int N () const { return d_N; }
  int K () const { return d_K;}
  int skip_bits () const {return d_skip_bits;}

  const std::vector<int> & TABLE () const { return d_TABLE; }
  
};
