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

#ifndef INCLUDED_GR_OFDM_DOT11A_H
#define INCLUDED_GR_OFDM_DOT11A_H

typedef struct gr_coded_ofdm_plcp_header {
  gr_coded_ofdm_rate_t rate : 4;
  unsigned char reserved    : 1;
  unsigned short length     : 12;
  unsigned char parity      : 1;
  unsigned char tail        : 6;
  unsigned short service;
} __attribute__ ((packed)) gr_coded_ofdm_plcp_header_t;

#endif
