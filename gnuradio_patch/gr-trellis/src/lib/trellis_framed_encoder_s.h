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

// WARNING: this file is machine generated.  Edits will be over written

#ifndef INCLUDED_TRELLIS_FRAMED_ENCODER_S_H
#define INCLUDED_TRELLIS_FRAMED_ENCODER_S_H

#include "fsm.h"
#include <gr_sync_block.h>

class trellis_framed_encoder_s;
typedef boost::shared_ptr<trellis_framed_encoder_s> trellis_framed_encoder_s_sptr;

trellis_framed_encoder_s_sptr trellis_make_framed_encoder_s (const fsm &FSM, int ST);

/*!
 * \brief Convolutional encoder.
 * \ingroup block
 *
 * 
 */
class trellis_framed_encoder_s : public gr_sync_block
{
private:
  friend trellis_framed_encoder_s_sptr trellis_make_framed_encoder_s (const fsm &FSM, int ST);
  fsm d_FSM;
  int d_ST;
  int d_INIT_ST;
  trellis_framed_encoder_s (const fsm &FSM, int ST); 

public:
  fsm FSM () const { return d_FSM; }
  int ST () const { return d_ST; }

  int work (int noutput_items,
	    gr_vector_const_void_star &input_items,
	    gr_vector_void_star &output_items);
};

#endif
