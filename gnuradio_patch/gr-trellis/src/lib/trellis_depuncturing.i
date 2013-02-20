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

GR_SWIG_BLOCK_MAGIC(trellis,depuncturing);

trellis_depuncturing_sptr trellis_make_depuncturing (int itemsize, 
						 const std::vector<int> &TABLE);

class trellis_depuncturing : public gr_block
{
private:

  int itemsize;

  std::vector<int> d_TABLE;
  int d_TABLESIZE;

  trellis_depuncturing (int itemsize, const std::vector<int> &TABLE); 

public:

  const std::vector<int> & TABLE () const { return d_TABLE; }
  
};
