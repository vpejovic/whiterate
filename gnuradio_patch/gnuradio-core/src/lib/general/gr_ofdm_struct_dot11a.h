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

#define MAX_NUM_SYMBOLS 4224*10
#ifndef INCLUDED_GR_OFDM_DOT11A_H
#define INCLUDED_GR_OFDM_DOT11A_H

#include <gr_ofdm_constants.h>

typedef struct gr_ofdm_plcp_header {
  gr_coded_ofdm_rate_t rate : 4;
  unsigned char reserved    : 1;
  unsigned short length     : 12;
  unsigned char parity      : 1;
  unsigned short seqno;
  unsigned short tail        : 14;
} __attribute__ ((packed)) gr_ofdm_plcp_header_t;

// Message sent from the header decoder to the coded OFDM demodulator:



typedef struct gr_coded_ofdm_hdrctl {
  bool                    good;
  gr_coded_ofdm_rate_t    rate;
  unsigned short          length;
  unsigned short seqno;
  unsigned short srcid;
  gr_coded_ofdm_rate_t    orig_rate;
  int num_known_symbols;
  float snr;
} gr_coded_ofdm_hdrctl_t;

// Message sent from the coded OFDM demodulator to the trellis
// decoder module:

typedef struct gr_coded_ofdm_sisoctl {
  int arg;
  unsigned short length_valid;
  unsigned short length_total;
} gr_coded_ofdm_sisoctl_t;

/* Added by Souvik */


typedef struct cbar_pkt_hdr
{
    //gr_coded_ofdm_hdrctl_t hdrctl;
	int rate;    
	int length;
    int length_valid;
	int seqno;
	int num_known_symbols;	
	int orig_rate;
    int interfered;
    int num_sym_valid;
    float	conf[MAX_NUM_SYMBOLS]; 
	unsigned char	bits[MAX_NUM_SYMBOLS];

} cbar_model_pkt_hdr;
#endif
