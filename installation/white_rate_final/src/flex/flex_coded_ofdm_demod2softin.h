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

#ifndef INCLUDED_FLEX_coded_ofdm_demod2softin_H
#define INCLUDED_FLEX_coded_ofdm_demod2softin_H

#include <gr_block.h>
#include <flex_ofdm_constants.h>

class flex_coded_ofdm_demod2softin;
typedef boost::shared_ptr<flex_coded_ofdm_demod2softin> flex_coded_ofdm_demod2softin_sptr;

flex_coded_ofdm_demod2softin_sptr 
flex_make_coded_ofdm_demod2softin (int tag);

/*!
 * \brief 
 *
 */
class flex_coded_ofdm_demod2softin : public gr_block
{
  friend flex_coded_ofdm_demod2softin_sptr
  flex_make_coded_ofdm_demod2softin (int tag);
  
private:
  int d_tag;
protected:
  flex_coded_ofdm_demod2softin(int tag);
  
public:
  ~flex_coded_ofdm_demod2softin();
  
  void forecast (int noutput_items, gr_vector_int &ninput_items_required);
  
  int general_work(int noutput_items,
		   gr_vector_int &ninput_items,
		   gr_vector_const_void_star &input_items,
		   gr_vector_void_star &output_items);
};

#endif /* INCLUDED_FLEX_coded_ofdm_demod2softin_H */
