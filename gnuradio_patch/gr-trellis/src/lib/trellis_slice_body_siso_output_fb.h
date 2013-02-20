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

#ifndef INCLUDED_TRELLIS_SLICE_BODY_SISO_OUTPUT_FB_H
#define INCLUDED_TRELLIS_SLICE_BODY_SISO_OUTPUT_FB_H

#include <gr_sync_decimator.h>
#include <gr_message.h>
#include <gr_msg_queue.h>
#include <gr_ofdm_struct_dot11a.h>
class trellis_slice_body_siso_output_fb;

typedef boost::shared_ptr<trellis_slice_body_siso_output_fb> trellis_slice_body_siso_output_fb_sptr;

trellis_slice_body_siso_output_fb_sptr
trellis_make_slice_body_siso_output_fb(unsigned int D, const std::vector<unsigned char>& tbl, gr_msg_queue_sptr d_body_siso_msgq, gr_msg_queue_sptr hdrctl_msgq, gr_msg_queue_sptr recv_msgq, char * dump_file);

class trellis_slice_body_siso_output_fb : public gr_sync_decimator {
  friend trellis_slice_body_siso_output_fb_sptr
  trellis_make_slice_body_siso_output_fb(unsigned int D, const std::vector<unsigned char>& tbl, gr_msg_queue_sptr d_body_siso_msgq, gr_msg_queue_sptr hdrctl_msgq, gr_msg_queue_sptr recv_msgq, char * dump_file);

protected:
  unsigned int d_D;
  const std::vector<unsigned char> d_tbl;
  char * d_file;
  FILE * d_fp;
  trellis_slice_body_siso_output_fb(unsigned int D, const std::vector<unsigned char>& tbl, gr_msg_queue_sptr d_body_siso_msgq, gr_msg_queue_sptr hdrctl_msgq, gr_msg_queue_sptr recv_msgq, char * dump_file);

public:
  int work (int noutput_items, gr_vector_const_void_star &input_items,
	          gr_vector_void_star &output_items);
  ~trellis_slice_body_siso_output_fb();
  /* Added by Souvik */
  // inbound control messages:
  gr_msg_queue_sptr	d_hdrctl_msgq;
  gr_msg_queue_sptr	d_body_siso_msgq;
  
  //Veljko
  gr_msg_queue_sptr	d_recv_msgq;
};

#endif
