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


#ifndef INCLUDED_TRELLIS_depuncturing_H
#define INCLUDED_TRELLIS_depuncturing_H

#include <vector>
#include <gr_block.h>

class trellis_depuncturing;
typedef boost::shared_ptr<trellis_depuncturing> trellis_depuncturing_sptr;

trellis_depuncturing_sptr trellis_make_depuncturing (int itemsize, 
						 const std::vector<int> &TABLE);

/*!

  \brief Depunctures an incoming data stream.

 \ingroup block
 */



class trellis_depuncturing : public gr_block
{
private:
  friend trellis_depuncturing_sptr 
  trellis_make_depuncturing (int itemsize,
			   const std::vector<int> &TABLE);

  int d_itemsize;
  std::vector<int> d_TABLE;
  int d_TABLESIZE;

  bool d_depunct_on;
  int d_rate;

  int d_total_produced;
  int d_total_consumed;

  trellis_depuncturing (int itemsize,
		      const std::vector<int> &TABLE); 

public:
  const std::vector<int> & TABLE () const { return d_TABLE; }
  void forecast (int noutput_items, gr_vector_int &ninput_items_required);

  int general_work (int noutput_items,
      gr_vector_int &ninput_items,
	    gr_vector_const_void_star &input_items,
	    gr_vector_void_star &output_items);
};

#endif
