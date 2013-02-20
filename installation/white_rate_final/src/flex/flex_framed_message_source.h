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

#ifndef INCLUDED_FLEX_framed_message_source_H
#define INCLUDED_FLEX_framed_message_source_H

#include <gr_sync_block.h>
#include <gr_message.h>
#include <gr_msg_queue.h>

class flex_framed_message_source;
typedef boost::shared_ptr<flex_framed_message_source> flex_framed_message_source_sptr;

flex_framed_message_source_sptr flex_make_framed_message_source (size_t itemsize, int msgq_limit=0);

/*!
 * \brief Turn received messages into a stream
 * \ingroup source
 */
class flex_framed_message_source : public gr_sync_block
{
 private:
  size_t	 	d_itemsize;
  gr_msg_queue_sptr	d_msgq;
  gr_message_sptr	d_msg;
  unsigned		d_msg_offset;
  bool			d_eof;

  friend flex_framed_message_source_sptr
  flex_make_framed_message_source(size_t itemsize, int msgq_limit);

 protected:
  flex_framed_message_source (size_t itemsize, int msgq_limit);

 public:
  ~flex_framed_message_source ();

  gr_msg_queue_sptr	msgq() const { return d_msgq; }

  int work (int noutput_items,
	    gr_vector_const_void_star &input_items,
	    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_FLEX_framed_message_source_H */
