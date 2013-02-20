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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <flex_framed_message_sink.h>
#include <gr_io_signature.h>
#include <cstdio>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdexcept>

#define VERBOSE 0


// public constructor that returns a shared_ptr

flex_framed_message_sink_sptr 
flex_make_framed_message_sink (size_t itemsize, gr_msg_queue_sptr msgq, bool dont_block)
{
  return flex_framed_message_sink_sptr(new flex_framed_message_sink(itemsize, msgq, dont_block));
}

flex_framed_message_sink::flex_framed_message_sink (size_t itemsize, gr_msg_queue_sptr msgq, bool dont_block)
  : gr_sync_block("framed_message_sink",
		  gr_make_io_signature2(2, 2, itemsize, sizeof(char)),
		  gr_make_io_signature(0, 0, 0)),
    d_itemsize(itemsize), d_msgq(msgq), d_dont_block(dont_block),
    d_cur_item(0)
{
  d_buf = new unsigned char[BUFLEN_BYTES];
  d_buflen_items = BUFLEN_BYTES / itemsize;
}

flex_framed_message_sink::~flex_framed_message_sink()
{
  delete[] d_buf;
}

int
flex_framed_message_sink::work(int noutput_items,
		      gr_vector_const_void_star &input_items,
		      gr_vector_void_star &output_items)
{
  const char *in = (const char *) input_items[0];
  const char *in_flag = (const char *)input_items[1];

  // if we'd block, drop the data on the floor and say everything is OK
  if (d_dont_block && d_msgq->full_p()) {
    printf("flex_framed_message_sink: warning: dropping %d items\n", noutput_items);
    return noutput_items;
  }
  
  // locate the end of this packet, if present:
  int consume_items = 0;
  bool send_pkt = false;
  while (consume_items < noutput_items && !in_flag[consume_items]) 
    consume_items++;
  if (consume_items < noutput_items && in_flag[consume_items]) {
    consume_items++;
    send_pkt = true;
  }

#if VERBOSE > 1
  printf("flex_framed_message_sink: noutput_items=%d, consume_items=%d, d_cur_item=%d\n",
         noutput_items, consume_items, d_cur_item);
  printf("flex_framed_message_sink: in_flag[%d]=%d\n", 
         consume_items - 1, in_flag[consume_items - 1]);
#endif

  // check to make sure that we won't overflow the buffer:
  assert(d_cur_item + consume_items <= d_buflen_items);

  memcpy(d_buf + d_cur_item * d_itemsize, in, consume_items * d_itemsize);

  d_cur_item += consume_items;

  if (send_pkt) {
#if VERBOSE
    printf("flex_framed_message_sink: building packet of %d items\n",
           d_cur_item);
#endif
      
    // build a message to hold whatever we've got
    gr_message_sptr msg = gr_make_message(0, 		// msg type
            d_itemsize, 	// arg1 for other end
            d_cur_item, 	// arg2 for other end (redundant)
            d_cur_item * d_itemsize);   // len of msg
    memcpy(msg->msg(), d_buf, d_cur_item * d_itemsize);
    d_msgq->handle(msg);		// send it

    d_cur_item = 0;
  }

  return consume_items;
}
