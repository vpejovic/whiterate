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

#ifndef INCLUDED_FLEX_ofdm_header_decode_vbb_H
#define INCLUDED_FLEX_ofdm_header_decode_vbb_H

#include <gr_sync_block.h>
#include <gr_message.h>
#include <gr_msg_queue.h>

class flex_ofdm_header_decode_vbb;
typedef boost::shared_ptr<flex_ofdm_header_decode_vbb> flex_ofdm_header_decode_vbb_sptr;

flex_ofdm_header_decode_vbb_sptr 
flex_make_ofdm_header_decode_vbb (gr_msg_queue_sptr hdrctl_msgq, int _footerlen);

/*!
 * \brief Takes a vector of bytes containing a candidate coded OFDM header,
 * parses it, verifies that it is a header, extracts the rate field,
 * length field.
 */
class flex_ofdm_header_decode_vbb : public gr_sync_block
{
  friend flex_ofdm_header_decode_vbb_sptr 
  flex_make_ofdm_header_decode_vbb (gr_msg_queue_sptr hdrctl_msgq, int _footerlen);

 protected:
  flex_ofdm_header_decode_vbb(gr_msg_queue_sptr hdrctl_msgq,
			    int _footerlen);

  gr_msg_queue_sptr	d_hdrctl_msgq;
  int d_footerlen;

 public:
  ~flex_ofdm_header_decode_vbb();

  int work(int noutput_items,
	   gr_vector_const_void_star &input_items,
	   gr_vector_void_star &output_items);
};

#endif /* INCLUDED_FLEX_ofdm_header_decode_vbb_H */
