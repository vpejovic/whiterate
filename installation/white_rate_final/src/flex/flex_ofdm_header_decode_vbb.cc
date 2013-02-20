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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define VERBOSE 0 

#include <flex_ofdm_header_decode_vbb.h>
#include <flex_ofdm_constants.h>
#include <flex_ofdm_struct_dot11a.h>
#include <gr_io_signature.h>
#include <gr_expj.h>
#include <gr_math.h>
#include <math.h>
#include <cstdio>
#include <stdexcept>
#include <iostream>

//TODO: the outputs are not there

flex_ofdm_header_decode_vbb_sptr
flex_make_ofdm_header_decode_vbb(gr_msg_queue_sptr hdrctl_msgq, 
			       int _footerlen) {
  return flex_ofdm_header_decode_vbb_sptr(
					new flex_ofdm_header_decode_vbb(hdrctl_msgq, _footerlen));
}

flex_ofdm_header_decode_vbb::flex_ofdm_header_decode_vbb(gr_msg_queue_sptr hdrctl_msgq,
						     int _footerlen)
  : gr_sync_block ("ofdm_header_decode_vbb",
		   gr_make_io_signature (1, 1, CODED_OFDM_HEADERLEN_BYTES),
 		   gr_make_io_signature3 (3, 3, sizeof(char), sizeof(unsigned short), 
                              sizeof(char))),
    d_hdrctl_msgq(hdrctl_msgq), d_footerlen(_footerlen) { }

flex_ofdm_header_decode_vbb::~flex_ofdm_header_decode_vbb() { }

int flex_ofdm_header_decode_vbb::work(int noutput_items,
	      		                        gr_vector_const_void_star &input_items,
			                              gr_vector_void_star &output_items) {
  for (int i = 0; i < noutput_items; i++) {
    const char *c = (const char *)input_items[i];

    // Decode the header
    unsigned char byte0 = c[0];
    unsigned char byte1 = c[1];
    unsigned char byte2 = c[2];
    unsigned char byte3 = c[3];
    unsigned char byte4 = c[4];
    unsigned char byte5 = c[5];

    unsigned char rate     = byte0 >> 4;
    
    unsigned char reserved = (byte0 & 0x8) >> 3;
    
    unsigned short length  = (((unsigned short)byte0 & 0x3) << 9) |
                             ((unsigned short)byte1 << 1) |
                             (((unsigned short)byte2 & 0x80) >> 7);
    
    unsigned char parity   = (byte2 & 0x40) >> 6;

    unsigned short seqno_srcid   = (((unsigned short)byte2 & 0x3f) << 10) |
                             ((unsigned short)byte3 << 2) |
                             (((unsigned short)byte4 & 0xc0) >> 6);

    unsigned char tail     = (((unsigned short)byte4 & 0x3f) << 8) |
                             (unsigned short)byte5;
    
    // compute the parity check bit
    unsigned char parity_sum = (byte0 + byte1 + byte2 + byte3 + byte4 + byte5) & 0x1;

    bool good_rate = (rate == CODED_OFDM_RATE_BPSK_1_2) |
                     (rate == CODED_OFDM_RATE_BPSK_3_4) |
                     (rate == CODED_OFDM_RATE_QPSK_1_2) |
                     (rate == CODED_OFDM_RATE_QPSK_3_4) | 
                     (rate == CODED_OFDM_RATE_8QAM_1_2) |
                     (rate == CODED_OFDM_RATE_8QAM_3_4)  |                   
                     (rate == CODED_OFDM_RATE_16QAM_1_2) |
                     (rate == CODED_OFDM_RATE_16QAM_3_4) |
                     (rate == CODED_OFDM_RATE_64QAM_1_2) |
                     (rate == CODED_OFDM_RATE_64QAM_2_3) |
                     (rate == CODED_OFDM_RATE_64QAM_3_4) |
                     (rate == CODED_OFDM_RATE_256QAM_1_2) |
                     (rate == CODED_OFDM_RATE_256QAM_3_4) ;

    unsigned short seqno = seqno_srcid & 0x0fff;
    unsigned short srcid = (seqno_srcid & 0xf000) >> 12;

	if (VERBOSE){
		fprintf(stderr, "flex_ofdm_header_decode_vbb: item=%d, noutput_items=%d\n", 
			   i, noutput_items);
		fprintf(stderr,"                         : rate=0x%1hx\n", rate);
		fprintf(stderr,"                         : seqno=%d (0x%04hx)\n", seqno, seqno);
		fprintf(stderr,"                         : srcid=%d (0x%04hx)\n", srcid, srcid);
		fprintf(stderr,"                         : reserved=0x%1hx\n", reserved);
		fprintf(stderr,"                         : length=0x%03hx (%d)\n", length, length);
		fprintf(stderr,"                         : parity=0x%1hx\n", parity);
		fprintf(stderr,"                         : tail=0x%1hx\n", tail);
		fprintf(stderr,"                         : parity sum=%01hx\n", parity_sum);
		fprintf(stderr,"                         : good rate=%01hx\n", good_rate);
	}

    bool good = (parity_sum == parity) && good_rate && !reserved && !tail && 
      (length < 800); 
    // TODO: remove ugly hack: checking length

    // Construct a header control message and send it to the coded OFDM
    // demodulator via a message queue:
    flex_coded_ofdm_hdrctl_t hdrctl = {good, (flex_coded_ofdm_rate_t)rate, length, seqno,srcid, (flex_coded_ofdm_rate_t)rate, -1, 1.0};
    gr_message_sptr msg = gr_make_message(0, 0, 0, sizeof(flex_coded_ofdm_hdrctl_t)); 
    memcpy(msg->msg(), &hdrctl, sizeof(flex_coded_ofdm_hdrctl_t));

    if (d_hdrctl_msgq->full_p()) {
      fprintf(stderr, "flex_ofdm_header_decode_vbb: warning -- hdrctl_msgq full!\n");
      assert(0);
    }

    d_hdrctl_msgq->insert_tail(msg);
    if (VERBOSE) fprintf(stderr, "flex_ofdm_header_decode_vbb: put into hdrctl_msgq\n");	
    msg.reset();
  }

  return noutput_items;
}
