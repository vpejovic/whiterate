/* -*- c++ -*- */
/*
 * Copyright 2005 Free Software Foundation, Inc.
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

#ifndef INCLUDED_FLEX_framed_message_sink_H
#define INCLUDED_FLEX_framed_message_sink_H

#include <gr_sync_block.h>
#include <gr_message.h>
#include <gr_msg_queue.h>

#define BUFLEN_BYTES 8192

class flex_framed_message_sink;
typedef boost::shared_ptr<flex_framed_message_sink> flex_framed_message_sink_sptr;

flex_framed_message_sink_sptr flex_make_framed_message_sink (size_t itemsize,
					   gr_msg_queue_sptr msgq,
					   bool dont_block);

/*!
 * \brief Gather received items into messages and insert into msgq
 * \ingroup sink
 */
class flex_framed_message_sink : public gr_sync_block
{
 private:
  size_t	 	d_itemsize;
  gr_msg_queue_sptr	d_msgq;
  bool			d_dont_block;
  unsigned char *d_buf;          // buffer containing growing packet
  int d_cur_item;       // item number offset of beginning of growing packet
  int d_buflen_items;   // length of buffer in item count

  friend flex_framed_message_sink_sptr
  flex_make_framed_message_sink(size_t itemsize, gr_msg_queue_sptr msgq, bool dont_block);

 protected:
  flex_framed_message_sink (size_t itemsize, gr_msg_queue_sptr msgq, bool dont_block);

 public:
  ~flex_framed_message_sink ();

  int work (int noutput_items,
	    gr_vector_const_void_star &input_items,
	    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_FLEX_framed_message_sink_H */