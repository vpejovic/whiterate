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

#ifndef INCLUDED_FLEX_OFDM_CONSTANTS_H
#define INCLUDED_FLEX_OFDM_CONSTANTS_H

const unsigned int CODED_OFDM_HEADERLEN_BYTES = 6;
const unsigned int CODED_OFDM_FOOTERLEN_BYTES = 5;

// the following are calculated from the above two constants:
const unsigned int CODED_OFDM_HEADERLEN_CODED_BYTES = CODED_OFDM_HEADERLEN_BYTES*2;
const unsigned int CODED_OFDM_HEADERLEN_BITS = CODED_OFDM_HEADERLEN_BYTES*8;
const unsigned int CODED_OFDM_HEADERLEN_CODED_BITS = CODED_OFDM_HEADERLEN_CODED_BYTES*8;

// 802.11a-1999 RATE field (4 bits)

typedef enum {
  CODED_OFDM_RATE_BPSK_1_2  = 0xD,
  CODED_OFDM_RATE_BPSK_3_4  = 0xF,
  CODED_OFDM_RATE_QPSK_1_2  = 0x5,
  CODED_OFDM_RATE_QPSK_3_4  = 0x7,
  CODED_OFDM_RATE_8QAM_1_2 = 0x4,
  CODED_OFDM_RATE_8QAM_3_4 = 0x6,
  CODED_OFDM_RATE_16QAM_1_2 = 0x9,
  CODED_OFDM_RATE_16QAM_3_4 = 0xB,
  CODED_OFDM_RATE_64QAM_1_2 = 0x2,  
  CODED_OFDM_RATE_64QAM_2_3 = 0x3,
  CODED_OFDM_RATE_64QAM_3_4 = 0xA,
  CODED_OFDM_RATE_256QAM_1_2 = 0x8,
  CODED_OFDM_RATE_256QAM_3_4 = 0xC,
} flex_coded_ofdm_rate_t;

typedef enum {
  CODED_OFDM_DEMOD_HDR_SEL = 0x1,
  CODED_OFDM_DEMOD_INVALID = 0x0,
} flex_coded_ofdm_sel_t;

const unsigned int OFDM_HEADERLEN_BYTES = 4;
const unsigned int OFDM_HEADERLEN_BITS = OFDM_HEADERLEN_BYTES*8;

#endif
