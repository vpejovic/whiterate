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

#include <flex_framed_message_source.h>
#include <gr_io_signature.h>
#include <cstdio>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdexcept>

#define VERBOSE 0 

// public constructor that returns a shared_ptr

flex_framed_message_source_sptr
flex_make_framed_message_source(size_t itemsize, int msgq_limit)
{
  return flex_framed_message_source_sptr(new flex_framed_message_source(itemsize, msgq_limit));
}

flex_framed_message_source::flex_framed_message_source (size_t itemsize, int msgq_limit)
  : gr_sync_block("framed_message_source",
		  gr_make_io_signature(0, 0, 0),
		  gr_make_io_signature2(2, 2, itemsize, sizeof(char))),
    d_itemsize(itemsize), d_msgq(gr_make_msg_queue(msgq_limit)),
    d_msg_offset(0), d_eof(false)
{
  assert(d_msg == 0); // sanity check...
}

flex_framed_message_source::~flex_framed_message_source()
{
}

int
flex_framed_message_source::work(int noutput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items)
{
  char *out = (char *) output_items[0];
  char *out_flag = (char *)output_items[1];
  int nn = 0;

  while (nn < noutput_items){
    if (d_msg){
      //
      // Consume whatever we can from the current message
      //
      int mm = std::min(noutput_items - nn, 
                        (int)((d_msg->length() - d_msg_offset) / d_itemsize));
      memcpy (out, &(d_msg->msg()[d_msg_offset]), mm * d_itemsize);

      nn += mm;
      out += mm * d_itemsize;
      d_msg_offset += mm * d_itemsize;
      assert(d_msg_offset <= d_msg->length());

      if (d_msg_offset == d_msg->length()){
	if (d_msg->type() == 1)	           // type == 1 sets EOF
	  d_eof = true;
  else {
    assert(mm>0);
    out_flag[mm-1] = (d_msg_offset == d_msg->length()) ? 1 : 0;
#if VERBOSE
    printf("gr_framed_message_source: out_flag[%d]=1\n", mm-1);
#endif
  }
	d_msg.reset();
      }
    }
    else {
      //
      // No current message
      //
      if (d_msgq->empty_p() && nn > 0){    // no more messages in the queue, return what we've got
	break;
      }

      if (d_eof)
	return -1;

      d_msg = d_msgq->delete_head();	   // block, waiting for a message
      d_msg_offset = 0;

      if ((d_msg->length() % d_itemsize) != 0)
	throw std::runtime_error("msg length is not a multiple of d_itemsize");
    }
  }

  return nn;
}
