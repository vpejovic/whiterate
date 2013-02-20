/* -*- c++ -*- */
/*
 * Copyright 2006,2007 Free Software Foundation, Inc.
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

#ifndef INCLUDED_FLEX_OFDM_MAPPER_flow_H
#define INCLUDED_FLEX_OFDM_MAPPER_flow_H

#include <gr_sync_block.h>
#include <gr_message.h>
#include <gr_msg_queue.h>
#include <string.h>

class flex_ofdm_mapper_flow;
typedef boost::shared_ptr<flex_ofdm_mapper_flow> flex_ofdm_mapper_flow_sptr;

flex_ofdm_mapper_flow_sptr 
flex_make_ofdm_mapper_flow (const std::vector<gr_complex> &constellation, 
       const std::vector<gr_complex> &header_constellation, unsigned msgq_limit, 
			 unsigned occupied_carriers, unsigned int fft_length, std::string tone_map, const std::vector<float> &scale);

/*!
 * \brief take a stream of bytes in and map to a vector of complex
 * constellation points suitable for IFFT input to be used in an ofdm
 * modulator.  Abstract class must be subclassed with specific mapping.
 * \ingroup modulation_blk
 * \ingroup ofdm_blk
 */

class flex_ofdm_mapper_flow : public gr_sync_block
{
	
  friend flex_ofdm_mapper_flow_sptr
  flex_make_ofdm_mapper_flow (const std::vector<gr_complex> &constellation, 
       const std::vector<gr_complex> &header_constellation, unsigned msgq_limit, 
			 unsigned occupied_carriers, unsigned int fft_length, std::string tone_map, const std::vector<float> &scale);
 protected:
  flex_ofdm_mapper_flow (const std::vector<gr_complex> &constellation, 
       const std::vector<gr_complex> &header_constellation, unsigned msgq_limit, 
			 unsigned occupied_carriers, unsigned int fft_length, std::string tone_map, const std::vector<float> &scale);

 private:
  std::vector<gr_complex> d_header_constellation;
  std::vector<gr_complex> d_body_constellation;
  std::vector<gr_complex> d_constellation;
  gr_msg_queue_sptr	d_msgq;
  gr_message_sptr	d_msg;
  unsigned		d_msg_offset;
  bool			d_eof;
  
  std::string 		d_tone_map;
  unsigned int 		d_occupied_carriers;
  std::vector<float> d_scale;
  unsigned int 		d_fft_length;
  unsigned int 		d_bit_offset;

  int			d_pending_flag;

  unsigned long  d_nbodybits;  // bits per symbol in packet body constellation
  unsigned long  d_nheaderbits;  // bits per symbol in packet header constellation
  unsigned long  d_nbits;        // CURRENT bits per symbol in constellation
  unsigned char  d_msgbytes;
  
  unsigned char d_resid;
  unsigned int d_nresid;
  
  bool d_mapping_header;
  
  std::vector<int> d_subcarrier_map;
  
  int randsym(void);

 public:
  ~flex_ofdm_mapper_flow(void);

  gr_msg_queue_sptr	msgq() const { return d_msgq; }

  int work(int noutput_items,
	   gr_vector_const_void_star &input_items,
	   gr_vector_void_star &output_items);

  // linklab, initialize subcarrier allocation 
  void initialize();
  
  // linklab, reset subcarrier allocation 
  void reset_ofdm_params(const std::vector<gr_complex> &new_constellation, std::string new_tone_map);

};

#endif
