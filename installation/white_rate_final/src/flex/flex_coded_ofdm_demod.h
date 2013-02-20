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

#ifndef INCLUDED_FLEX_CODED_OFDM_DEMOD_H
#define INCLUDED_FLEX_CODED_OFDM_DEMOD_H

//#include <gr_sync_block.h>
#include <gr_block.h>
#include <gr_msg_queue.h>
#include <gr_block.h>
#include <gr_message.h>
#include <gr_msg_queue.h>
#include <flex_ofdm_constants.h>

class flex_coded_ofdm_demod;
typedef boost::shared_ptr<flex_coded_ofdm_demod> flex_coded_ofdm_demod_sptr;

flex_coded_ofdm_demod_sptr 
flex_make_coded_ofdm_demod (
				gr_msg_queue_sptr             body_sisoctl_msgq,
				const std::vector<gr_complex>              &bpsk_sym_position, 
				const std::vector<unsigned char>           &bpsk_sym_value_out,
				const std::vector<gr_complex>              &qpsk_sym_position,
				const std::vector<unsigned char>           &qpsk_sym_value_out,
				const std::vector<gr_complex>    			&qam8_sym_position,
				const std::vector<unsigned char> 			&qam8_sym_value_out,				
				const std::vector<gr_complex>              &qam16_sym_position,
				const std::vector<unsigned char>           &qam16_sym_value_out,
				const std::vector<gr_complex>              &qam64_sym_position,
				const std::vector<unsigned char>           &qam64_sym_value_out,
				const std::vector<gr_complex>   			 &qam256_sym_position,
				const std::vector<unsigned char>			 &qam256_sym_value_out,			
				unsigned int fft_length, unsigned int occupied_tones, std::string tone_map,
				float phase_gain=0.25, float freq_gain=0.25*0.25/4.0, int num_known_symbols=0);

/*!
 * \brief Takes an OFDM symbol in, demaps it into bits and
 * confidences, and passes it along.
 *
 * input_items[0]: vector of gr_complex samples
 * input_items[1]: packet detect signal (char)
 *
 * output_items[0]: demodulator output (complex)
 * output_items[1]: header/body output select signal (unpacked char)
 * output_items[2]: demodulator angle output, normalized (float)
 *
 */
class flex_coded_ofdm_demod : public gr_block
{
  friend flex_coded_ofdm_demod_sptr 
  flex_make_coded_ofdm_demod (gr_msg_queue_sptr             body_sisoctl_msgq,
				const std::vector<gr_complex>              &bpsk_sym_position, 
				const std::vector<unsigned char>           &bpsk_sym_value_out,
				const std::vector<gr_complex>              &qpsk_sym_position,
				const std::vector<unsigned char>           &qpsk_sym_value_out,
				const std::vector<gr_complex>    &qam8_sym_position,
				const std::vector<unsigned char> &qam8_sym_value_out,					 
				const std::vector<gr_complex>    &qam16_sym_position,
				const std::vector<unsigned char> &qam16_sym_value_out,
				const std::vector<gr_complex>    &qam64_sym_position,
				const std::vector<unsigned char> &qam64_sym_value_out,
				const std::vector<gr_complex>    &qam256_sym_position,
				const std::vector<unsigned char> &qam256_sym_value_out,	
				unsigned int fft_length, unsigned int occupied_tones, std::string tone_map,
				float phase_gain, float freq_gain, int num_known_symbols);

 private:
  typedef enum coded_ofdm_demod_state {
    STATE_SYNC_SEARCH, 
    STATE_HAVE_SYNC, 
    STATE_CHECK_HEADER, 
    STATE_HAVE_HEADER
  } coded_ofdm_demod_state_t;

   	
  coded_ofdm_demod_state            d_state;
  unsigned int       d_header[CODED_OFDM_HEADERLEN_CODED_BYTES];		// header bits
  unsigned int		     d_headerbytelen_cnt;	// how many so far
  
  std::string d_tone_map;
  unsigned int       d_fft_length;
  unsigned int       d_occupied_carriers;

  unsigned int       d_byte_offset;
  unsigned int       d_partial_byte;
  unsigned int       d_carrier_index;

  int 		     d_packetlen;		// length of packet
  int                d_packet_whitener_offset;  // offset into whitener string to use
  int		     d_packetlen_cnt;		// how many so far

  std::vector<gr_complex>    d_sym_position;
  std::vector<unsigned char> d_sym_value_out;
  
  std::vector<gr_complex>    d_bpsk_sym_position;
  std::vector<unsigned char> d_bpsk_sym_value_out;
  std::vector<gr_complex>    d_qpsk_sym_position;
  std::vector<unsigned char> d_qpsk_sym_value_out;
  std::vector<gr_complex>    d_qam8_sym_position;
  std::vector<unsigned char> d_qam8_sym_value_out;
  std::vector<gr_complex>    d_qam16_sym_position;
  std::vector<unsigned char> d_qam16_sym_value_out;
  std::vector<gr_complex>    d_qam64_sym_position;
  std::vector<unsigned char> d_qam64_sym_value_out;
  std::vector<gr_complex>    d_qam256_sym_position;
  std::vector<unsigned char> d_qam256_sym_value_out; 
  
  std::vector<gr_complex>    d_dfe;
  std::vector<gr_complex>    d_dfe_pilot;
  unsigned int d_nbits;

  unsigned char d_resid;
  unsigned int d_nresid;
  float d_phase;
  float d_freq;
  float d_phase_gain;
  float d_freq_gain;
  float d_eq_gain;

  std::vector<int> d_subcarrier_map;
  //Veljko: do not use pilots for now
  //std::vector<int> d_pilot_map;
  //int d_pilotsyms[NUM_PILOTS];
  
  flex_coded_ofdm_rate_t d_rate;
  unsigned short d_curr_srcid;
  unsigned short d_curr_seqno;
  
  // inbound control messages:
  gr_msg_queue_sptr	d_hdrctl_msgq;

  // outbound control messages:
  gr_msg_queue_sptr d_good_hdrctl_msgq;
  //gr_msg_queue_sptr d_good_hdrctl_flush_msgq;
  gr_msg_queue_sptr d_body_sisoctl_msgq;

#define MAX_SYMBOLS 800*8*6*2
  float d_current_snr;  
  int d_num_known_symbols;
  int d_current_rate;
  unsigned char d_bits[MAX_SYMBOLS];
  void * last_hdrctl;
  
 protected:
  flex_coded_ofdm_demod (gr_msg_queue_sptr             body_sisoctl_msgq,
				const std::vector<gr_complex>              &bpsk_sym_position, 
				const std::vector<unsigned char>           &bpsk_sym_value_out,
				const std::vector<gr_complex>              &qpsk_sym_position,
				const std::vector<unsigned char>           &qpsk_sym_value_out,
				const std::vector<gr_complex>    			&qam8_sym_position,
				const std::vector<unsigned char> 			&qam8_sym_value_out,				
				const std::vector<gr_complex>              &qam16_sym_position,
				const std::vector<unsigned char>           &qam16_sym_value_out,
				const std::vector<gr_complex>              &qam64_sym_position,
				const std::vector<unsigned char>           &qam64_sym_value_out,
				const std::vector<gr_complex>   			 &qam256_sym_position,
				const std::vector<unsigned char>			 &qam256_sym_value_out,	
				unsigned int fft_length, unsigned int occupied_tones, std::string tone_map,
				float phase_gain, float freq_gain, int num_known_symbols);

  void enter_search();
  void enter_have_sync();
  void enter_check_header();
  void enter_have_header(unsigned int length);
  void send_sisoctl_msg(int arg, int numbits_valid, int num_bits_total);
  void flush_frame_sink();;
  
  unsigned char slicer(const gr_complex x);
  
  void slicer_with_distance(const gr_complex x, 
                            unsigned char *sym_out, 
                            float *dist_out);

  void slicer_with_confidence(const gr_complex x, 
                            char *sym_out, 
                            float *conf_out);

  unsigned int demapper(const gr_complex *in, gr_complex *output_demod,
			gr_complex *output_demod_derot,
			float *output_conf, char *output_bits);

  void set_rate(const flex_coded_ofdm_rate_t rate);

  void set_modulation(const std::vector<gr_complex> &sym_position, 
                      const std::vector<unsigned char> &sym_value_out);

 public:
  ~flex_coded_ofdm_demod();
  void reset_ofdm_params(std::string new_tone_map);   // linklab, reset carrier map
  void initialize();  // linklab, initialization function to set parameters 

  gr_msg_queue_sptr	hdrctl_msgq() const { 
    return d_hdrctl_msgq; 
  }

  gr_msg_queue_sptr	good_hdrctl_msgq() const { 
    return d_good_hdrctl_msgq; 
  }
  
  void forecast (int noutput_items, gr_vector_int &ninput_items_required);
  
  int general_work(int noutput_items,
     gr_vector_int &ninput_items,
	   gr_vector_const_void_star &input_items,
	   gr_vector_void_star &output_items);
};

#endif /* INCLUDED_FLEX_CODED_OFDM_DEMOD_H */
