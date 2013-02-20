/* -*- c++ -*- */
/*
 * Copyright 2007,2008 Free Software Foundation, Inc.
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

#include <flex_coded_ofdm_demod.h>
#include <gr_io_signature.h>
#include <flex_ofdm_struct_dot11a.h>
#include <gr_expj.h>
#include <gr_math.h>
#include <math.h>
#include <cstdio>
#include <stdexcept>
#include <iostream>
#include <string.h>

#define VERBOSE 0 

using namespace std;

inline void
flex_coded_ofdm_demod::enter_search()
{
  if (VERBOSE)
    fprintf(stderr, "@ enter_search\n");

  d_state = STATE_SYNC_SEARCH;
  d_carrier_index = 0;
}
    
inline void
flex_coded_ofdm_demod::enter_have_sync()
{
  if (VERBOSE)
    fprintf(stderr, "@ enter_have_sync\n");

  d_state = STATE_HAVE_SYNC;

  set_modulation(d_bpsk_sym_position, d_bpsk_sym_value_out);
  
  // clear state of demapper
  d_byte_offset = 0;
  d_partial_byte = 0;
  d_carrier_index = 0;
  d_headerbytelen_cnt = 0;

  // Resetting PLL
  d_freq = 0.0;
  d_phase = 0.0;
  fill(d_dfe.begin(), d_dfe.end(), gr_complex(1.0,0.0));
  //Veljko: do not use pilots
  //fill(d_dfe_pilot.begin(), d_dfe_pilot.end(), gr_complex(1.0,0.0));
}

inline void
flex_coded_ofdm_demod::enter_check_header()
{
  d_state = STATE_CHECK_HEADER;

  if (VERBOSE)
    fprintf(stderr, "@ enter_check_header\n");
}


inline void
flex_coded_ofdm_demod::enter_have_header(unsigned int length)
{
  d_state = STATE_HAVE_HEADER;

  d_packetlen = length;
  //d_packet_whitener_offset = (d_header >> 28) & 0x000f;
  d_packetlen_cnt = 0;

  if (VERBOSE)
    fprintf(stderr, "@ enter_have_header (payload_len = %d)\n", d_packetlen);
}

inline void
flex_coded_ofdm_demod::send_sisoctl_msg(int arg, int numbits_valid, int numbits_total) {
	
  // Send a control message to the body SISO decoder:
  gr_message_sptr sisoctl_msg = gr_make_message(0, 0, 0, sizeof(flex_coded_ofdm_sisoctl_t));
  flex_coded_ofdm_sisoctl_t *sisoctl = (flex_coded_ofdm_sisoctl_t *)sisoctl_msg->msg();
  // packet CODED length in bits (number of trellis steps for SISO)
  
  sisoctl->arg = arg;

  switch (d_rate) {
  case CODED_OFDM_RATE_BPSK_1_2:
  case CODED_OFDM_RATE_QPSK_1_2:
  case CODED_OFDM_RATE_8QAM_1_2:
  case CODED_OFDM_RATE_16QAM_1_2:
  case CODED_OFDM_RATE_64QAM_1_2:  
  case CODED_OFDM_RATE_256QAM_1_2:
    sisoctl->length_valid = numbits_valid; 
    sisoctl->length_total = numbits_total; 
    break;

  case CODED_OFDM_RATE_BPSK_3_4:
  case CODED_OFDM_RATE_QPSK_3_4:
  case CODED_OFDM_RATE_8QAM_3_4:  
  case CODED_OFDM_RATE_16QAM_3_4:
  case CODED_OFDM_RATE_64QAM_3_4:
  case CODED_OFDM_RATE_256QAM_3_4:  
    sisoctl->length_valid = numbits_valid * 3/2; //adjust for padding depunctured bytes later on
    sisoctl->length_total = numbits_total * 3/2; //adjust for padding depunctured bytes later on
    break;

  case CODED_OFDM_RATE_64QAM_2_3:
    sisoctl->length_valid = numbits_valid * 4/3; //adjust for padding depunctured bytes later on
    sisoctl->length_total = numbits_total * 4/3; //adjust for padding depunctured bytes later on
    break;
    
  default:
    assert(0);
  }

  if (d_body_sisoctl_msgq->full_p()){
    fprintf(stderr, "flex_coded_ofdm_demod: sisoctl_msgq FULL!!!\n");
    assert(0);		
  }

  d_body_sisoctl_msgq->insert_tail(sisoctl_msg);

  if (VERBOSE)
     fprintf(stderr, "flex_coded_ofdm_demod: sent siso control msg to body decoder. pkt len=%d,%d arg=%d rate=%x\n",
	 sisoctl->length_valid, sisoctl->length_total, sisoctl->arg, d_rate);
  sisoctl_msg.reset(); // free up message we created
}

unsigned char flex_coded_ofdm_demod::slicer(const gr_complex x){
  unsigned int table_size = d_sym_value_out.size();
  unsigned int min_index = 0;
  float min_euclid_dist = norm(x - d_sym_position[0]);
  float euclid_dist = 0;
  
  for (unsigned int j = 1; j < table_size; j++){
    euclid_dist = norm(x - d_sym_position[j]);
    if (euclid_dist < min_euclid_dist){
      min_euclid_dist = euclid_dist;
      min_index = j;
    }
  }
  return d_sym_value_out[min_index];
}

void flex_coded_ofdm_demod::slicer_with_distance(const gr_complex x, unsigned char *sym_out, float *dist_out) {
  unsigned int table_size = d_sym_value_out.size();
  unsigned int min_index = 0;
  float min_euclid_norm = norm(x - d_sym_position[0]);
  float euclid_norm = 0;
  float gridsize = sqrt(norm(d_sym_position[1] - d_sym_position[0]))/2;


  for (unsigned int j = 1; j < table_size; j++){
    euclid_norm = norm(x - d_sym_position[j]);
    if (euclid_norm < min_euclid_norm){
      min_euclid_norm = euclid_norm;
      min_index = j;
    }
  }

  *sym_out = d_sym_value_out[min_index];
  *dist_out = (gridsize-sqrt(min_euclid_norm));
}

void flex_coded_ofdm_demod::slicer_with_confidence(const gr_complex x, char *sym_out, float *conf_out) {
  unsigned int table_size = d_sym_value_out.size();
  unsigned int min_index = 0;
  float min_euclid_norm = norm(x - d_sym_position[0]);
  float euclid_norm = 0, dist1, dist2;
  float gridsize = sqrt(norm(d_sym_position[1] - d_sym_position[0]))/2;

  for (unsigned int j = 1; j < table_size; j++){
    euclid_norm = norm(x - d_sym_position[j]);
    if (euclid_norm < min_euclid_norm){
      min_euclid_norm = euclid_norm;
      min_index = j;
    }
  }

  *sym_out = d_sym_value_out[min_index];
  dist1 = sqrt(min_euclid_norm);

  //computing second closest constellation
  if(min_index == 0)
    min_euclid_norm = norm(x - d_sym_position[1]);
  else 
    min_euclid_norm = norm(x - d_sym_position[0]);

  for (unsigned int j = 1; j < table_size; j++){
    if(j != min_index) {
      euclid_norm = norm(x - d_sym_position[j]);
      if (euclid_norm < min_euclid_norm) min_euclid_norm = euclid_norm;
    }
  }
  
  dist2 = sqrt(min_euclid_norm);

if (VERBOSE>1)
    fprintf(stderr,"flex_coded_ofdm_demod: slicer with distance dist1=%f dist2=%f\n", dist1, dist2);

  *conf_out = (dist2 - dist1);//*(dist2-dist1);
}

inline float sigmoid(float angle) {

  return angle;
}

unsigned int flex_coded_ofdm_demod::demapper(const gr_complex *in, gr_complex *output_demod,
			gr_complex *output_demod_derot,
			float *output_conf, char *output_bits)
{
  unsigned int symbols_produced = 0;
  bool got_header = false, got_packet = false;
	
  //unsigned int i=0, bytes_produced=0;
  
  gr_complex accum_error = 0.0;
  
  // Veljko: Sen et al method for PPL:
  //PLL calculations using pilots, at start of symbol
  /*for(int p=0; p < NUM_PILOTS; p++) {
    gr_complex actual_pilot = d_sym_position[d_pilotsyms[p] % d_sym_position.size()];
    gr_complex rcvd_pilot = in[d_pilot_map[p]];
    gr_complex corrected_pilot = in[d_pilot_map[p]]*carrier*d_dfe_pilot[p];
    gr_complex this_accum_error = corrected_pilot * conj(actual_pilot);
    gr_complex this_accum_error2 = rcvd_pilot * conj(actual_pilot);

	if (VERBOSE) 
	  fprintf(stderr,"flex_coded_ofdm_demod: pilot %d (actual %d), sent %+.3f%+.3fj, rcvd  %+.3f%+.3fj, corrected  %+.3f%+.3fj, error %.5f error2 %.5f\n",
	   p, d_pilot_map[p], actual_pilot.real(), actual_pilot.imag(), rcvd_pilot.real(), rcvd_pilot.imag(), corrected_pilot.real(), corrected_pilot.imag(),
	   arg(this_accum_error), arg(this_accum_error2));
	   
    accum_error += this_accum_error;
    d_dfe_pilot[p] +=  gr_complex(0.05,0)*(actual_pilot/corrected_pilot-d_dfe_pilot[p]);
  }

  float angle = arg(accum_error);
  float angle_abs = fabs(angle);

  if(angle_abs < 0.2) 
    d_phase_gain = 0.3;
  else if(angle_abs < 0.3)
    d_phase_gain = 0.5;
  else 
    d_phase_gain = 0.8;
    
  d_freq_gain = d_phase_gain*d_phase_gain/4;
  
  d_freq = d_freq - d_freq_gain*angle;
  d_phase = d_phase + d_freq - d_phase_gain*angle;
  if (d_phase >= 2*M_PI) d_phase -= 2*M_PI;
  if (d_phase <0) d_phase += 2*M_PI;
  carrier=gr_expj(d_phase); 
  
  if (VERBOSE){ 
	  fprintf(stderr,"flex_coded_ofdm_demod: PLL: angle = %+.5f gain=%.5f,%.5f d_freq=%.5f  d_phase = %.5f\n", angle, d_freq_gain, d_phase_gain, d_freq, d_phase);
      for(int p=0; p < NUM_PILOTS; p++) fprintf(stderr,"flex_coded_ofdm_demod: dfe_pilot %d norm %.5f arg %.5f\n", p, norm(d_dfe_pilot[p]), arg(d_dfe_pilot[p]));
	  fprintf(stderr,"flex_coded_ofdm_demod: start new sym, offset %d, resid bits %d, subcarrier index %d\n",
           d_byte_offset, d_nresid, d_carrier_index); 
  }
  accum_error = 0.0; */ 
  

  //Veljko: previous method  
  gr_complex carrier;
  carrier=gr_expj(d_phase);
  
  
  assert(d_state == STATE_HAVE_SYNC || d_state == STATE_HAVE_HEADER);

  
  while(d_carrier_index < d_subcarrier_map.size()) {
    if(d_nresid > 0) {
      d_partial_byte |= d_resid;
      d_byte_offset += d_nresid;
      d_nresid = 0;
      d_resid = 0;
    }
    
 	char bits;
  	float symbol_confidence;
	gr_complex sigrot;
	gr_complex closest_sym;
	
    while((d_byte_offset < 8) && (d_carrier_index < d_subcarrier_map.size())) {
      //gr_complex sigrot = in[i]*carrier*d_dfe[i];
      
      output_demod_derot[symbols_produced] = in[d_subcarrier_map[d_carrier_index]];
      sigrot = in[d_subcarrier_map[d_carrier_index]]*carrier*d_dfe[d_carrier_index];
      
      slicer_with_confidence(sigrot, &bits, &symbol_confidence);
      
	  closest_sym = d_sym_position[bits];
	  accum_error += sigrot * conj(closest_sym);
      
      output_demod[symbols_produced] = sigrot;
      output_bits[symbols_produced] = bits;
      output_conf[symbols_produced] = symbol_confidence;
      symbols_produced++;

	  if (VERBOSE>2) 
	    fprintf(stderr,"flex_coded_ofdm_demod: recv=%+.3f%+.3fj, sym=%+.3f%+.3fj, dist=%.3f\n",
	     sigrot.real(), sigrot.imag(), closest_sym.real(), closest_sym.imag(), symbol_confidence);

      	// TODO: I don't get this (Veljko): FIX THE FOLLOWING STATEMENT
      if (d_state == STATE_HAVE_HEADER)
	  {
		if(norm(sigrot)> 0.001) 
		{
	  	  d_dfe[d_carrier_index] +=  d_eq_gain*(closest_sym/sigrot-d_dfe[d_carrier_index]);
		  if (VERBOSE>2) fprintf(stderr,"flex_coded_ofdm_demod: carrier %d dfe %+.3f%+.3fj norm_sigrot %f\n", d_carrier_index, d_dfe[d_carrier_index].real(), d_dfe[d_carrier_index].imag(), norm(sigrot));
		  
		}
      }
      
      d_carrier_index++;

      if((8 - d_byte_offset) >= d_nbits) {
		d_partial_byte |= bits << (d_byte_offset);
		d_byte_offset += d_nbits;
      }
      else {
		d_nresid = d_nbits-(8-d_byte_offset);
		int mask = ((1<<(8-d_byte_offset))-1);
		d_partial_byte |= (bits & mask) << d_byte_offset;
		d_resid = bits >> (8-d_byte_offset);
		d_byte_offset += (d_nbits - d_nresid);
      }
      //printf("demod symbol: %.4f + j%.4f   bits: %x   partial_byte: %x   byte_offset: %d   resid: %x   nresid: %d\n", 
      //     in[i-1].real(), in[i-1].imag(), bits, d_partial_byte, d_byte_offset, d_resid, d_nresid);
    }

    if(d_byte_offset == 8) {
	  switch (d_state)
	  {
        case STATE_HAVE_SYNC:
	  	  if(!got_header) 
		  {
	   		if(d_headerbytelen_cnt < CODED_OFDM_HEADERLEN_CODED_BYTES) 
			{
			  //if (VERBOSE) fprintf(stderr,"flex_coded_ofdm_demod: demod header byte %d/%d: %x\n\n", d_headerbytelen_cnt, CODED_OFDM_HEADERLEN_CODED_BYTES - 1, d_partial_byte);
              d_header[d_headerbytelen_cnt++] = d_partial_byte;
	          if (d_headerbytelen_cnt == CODED_OFDM_HEADERLEN_CODED_BYTES) {
				got_header = true;
				if(VERBOSE) fprintf(stderr,"Got header\n");
		  	  }
	        }
	  	  }
	  	  else //got_header
	    	d_headerbytelen_cnt++;
	  
          break;
	  
        case STATE_HAVE_HEADER:
	  	  if(!got_packet) 
		  {
			if (d_packetlen_cnt < d_packetlen) 
			{
			  /* Need to capture the channel here */
			  //if (VERBOSE) fprintf(stderr,"flex_coded_ofdm_demod (havee header): demod byte %d/%d: %x\n\n", d_packetlen_cnt, d_packetlen - 1, d_partial_byte);
			  d_packetlen_cnt++;
			  if (d_packetlen_cnt == d_packetlen)
		  	  {
			    got_packet = true;
				/* Need to map the captured channel to various datarates */

				/* Send the last header to the slice block */
  				//flex_coded_ofdm_hdrctl_t hdrctl = {0,(flex_coded_ofdm_rate_t)0,0,0,0}; //0 length hdrctl
				((flex_coded_ofdm_hdrctl_t *) last_hdrctl)->orig_rate = (flex_coded_ofdm_rate_t) d_current_rate;
				((flex_coded_ofdm_hdrctl_t *) last_hdrctl)->num_known_symbols = -1;
				((flex_coded_ofdm_hdrctl_t *) last_hdrctl)->snr = d_current_snr;						
  				gr_message_sptr msg = gr_make_message(0, 0, 0, sizeof(flex_coded_ofdm_hdrctl_t)); 
  				memcpy(msg->msg(), (flex_coded_ofdm_hdrctl_t *)last_hdrctl, sizeof(flex_coded_ofdm_hdrctl_t));
				if (d_good_hdrctl_msgq->full_p())  {
					fprintf(stderr, "flex_coded_ofdm_demod -- msg queue #2 full\n");
					assert(0);
				}
  				//fprintf(stderr, "flex_coded_ofdm_demod -- put into good_hdrctl_msgq\n");
				d_good_hdrctl_msgq->insert_tail(msg);
  				msg.reset();
		  	  }
	    	}
	  	  }
	  	  else 
		  {
	    	d_packetlen_cnt++;
		  }
          break;
	    default:;
	  }	

      d_byte_offset = 0;
      d_partial_byte = 0;
    }
    
    if(got_header || got_packet) //go on if got_packet
      break;
  }


  //Veljko: adjusting PLL Linknet method:
  float angle = arg(accum_error);
  d_freq = d_freq - d_freq_gain*angle;
  d_phase = d_phase + d_freq - d_phase_gain*angle;
  if (d_phase >= 2*M_PI) d_phase -= 2*M_PI;
  if (d_phase <0) d_phase += 2*M_PI;
   
 
  //printf("Byte offset: %d symbols: %d\n", d_byte_offset, symbols_produced);
  if(got_header) {
    assert(d_nresid == 0);
    assert(d_byte_offset == 0);
    d_partial_byte = 0;
    d_resid = 0;
  }

  if(got_packet){
    d_byte_offset = 0;
    d_nresid = 0;
    d_partial_byte = 0;
    d_resid = 0;		
  }


  assert(d_carrier_index <= d_subcarrier_map.size());
  consume_each(1);
  d_carrier_index = 0;
  
  if(got_header)
    enter_check_header();
  else if(got_packet) 
  {
    send_sisoctl_msg(0, d_packetlen*8, d_packetlen_cnt*8); //tell sisoctl that packet successfully recvd
    enter_search();
  }  
  return symbols_produced;
}


flex_coded_ofdm_demod_sptr
flex_make_coded_ofdm_demod(gr_msg_queue_sptr body_sisoctl_msgq,
					 const std::vector<gr_complex>    &bpsk_sym_position, 
					 const std::vector<unsigned char> &bpsk_sym_value_out,
					 const std::vector<gr_complex>    &qpsk_sym_position,
					 const std::vector<unsigned char> &qpsk_sym_value_out,
					 const std::vector<gr_complex>    &qam8_sym_position,
					 const std::vector<unsigned char> &qam8_sym_value_out,					 
					 const std::vector<gr_complex>    &qam16_sym_position,
					 const std::vector<unsigned char> &qam16_sym_value_out,
					 const std::vector<gr_complex>    &qam64_sym_position,
					 const std::vector<unsigned char> &qam64_sym_value_out,
					 const std::vector<gr_complex>    &qam256_sym_position,
					 const std::vector<unsigned char> &qam256_sym_value_out,					 
					 unsigned int fft_length, unsigned int occupied_carriers, std::string tone_map,
				     float phase_gain, float freq_gain, int num_known_symbols)
{
  return flex_coded_ofdm_demod_sptr(new flex_coded_ofdm_demod(body_sisoctl_msgq,
					 bpsk_sym_position, 
					 bpsk_sym_value_out,
					 qpsk_sym_position,
					 qpsk_sym_value_out,
					 qam8_sym_position,
					 qam8_sym_value_out,
					 qam16_sym_position,
					 qam16_sym_value_out,
					 qam64_sym_position,
					 qam64_sym_value_out,
					 qam256_sym_position,
					 qam256_sym_value_out,
					 fft_length, occupied_carriers, tone_map,
				     phase_gain, freq_gain, num_known_symbols));
}


flex_coded_ofdm_demod::flex_coded_ofdm_demod(
					 gr_msg_queue_sptr body_sisoctl_msgq,
					 const std::vector<gr_complex>    &bpsk_sym_position, 
					 const std::vector<unsigned char> &bpsk_sym_value_out,
					 const std::vector<gr_complex>    &qpsk_sym_position,
					 const std::vector<unsigned char> &qpsk_sym_value_out,
					 const std::vector<gr_complex>    &qam8_sym_position,
					 const std::vector<unsigned char> &qam8_sym_value_out,					 
					 const std::vector<gr_complex>    &qam16_sym_position,
					 const std::vector<unsigned char> &qam16_sym_value_out,
					 const std::vector<gr_complex>    &qam64_sym_position,
					 const std::vector<unsigned char> &qam64_sym_value_out,
					 const std::vector<gr_complex>    &qam256_sym_position,
					 const std::vector<unsigned char> &qam256_sym_value_out,					 
					 unsigned int fft_length, unsigned int occupied_carriers, std::string tone_map,
				     float phase_gain, float freq_gain, int num_known_symbols)
  : gr_block ("coded_ofdm_demod",
	gr_make_io_signature3 (3, 3, sizeof(gr_complex)*occupied_carriers, sizeof(char), sizeof(float)),
	gr_make_io_signature4 (5, 5, sizeof(gr_complex), sizeof(gr_complex), sizeof(float), sizeof(char))),
    d_occupied_carriers(occupied_carriers), d_fft_length(fft_length), d_tone_map(tone_map),
    d_byte_offset(0), d_partial_byte(0), d_carrier_index(0),
    d_bpsk_sym_position(bpsk_sym_position),
    d_bpsk_sym_value_out(bpsk_sym_value_out),
    d_qpsk_sym_position(qpsk_sym_position),
    d_qpsk_sym_value_out(qpsk_sym_value_out),
    d_qam8_sym_position(qam8_sym_position),
    d_qam8_sym_value_out(qam8_sym_value_out),
    d_qam16_sym_position(qam16_sym_position),
    d_qam16_sym_value_out(qam16_sym_value_out),
    d_qam64_sym_position(qam64_sym_position),
    d_qam64_sym_value_out(qam64_sym_value_out),
    d_qam256_sym_position(qam256_sym_position),
    d_qam256_sym_value_out(qam256_sym_value_out),
    d_resid(0), d_nresid(0),d_phase(0),d_freq(0),d_phase_gain(phase_gain),d_freq_gain(freq_gain),
    d_eq_gain(0.05),
    d_hdrctl_msgq(gr_make_msg_queue(1)),
	d_num_known_symbols(num_known_symbols),
    d_good_hdrctl_msgq(gr_make_msg_queue(4)),
    d_body_sisoctl_msgq(body_sisoctl_msgq)
{
  initialize();
  
  d_bpsk_sym_position = bpsk_sym_position;
  d_bpsk_sym_value_out = bpsk_sym_value_out;
  d_qpsk_sym_position = qpsk_sym_position;
  d_qpsk_sym_value_out = qpsk_sym_value_out;
  d_qam8_sym_position = qam8_sym_position;
  d_qam8_sym_value_out = qam8_sym_value_out;
  d_qam16_sym_position = qam16_sym_position;
  d_qam16_sym_value_out = qam16_sym_value_out;
  d_qam64_sym_position = qam64_sym_position;
  d_qam64_sym_value_out = qam64_sym_value_out;
  d_qam256_sym_position = qam256_sym_position;
  d_qam256_sym_value_out = qam256_sym_value_out;

  set_modulation(d_bpsk_sym_position, d_bpsk_sym_value_out);
  
  enter_search();
  
  d_num_known_symbols=0;
  d_current_rate=0;
  memset(d_bits, 0, MAX_NUM_SYMBOLS*sizeof(unsigned char));
  last_hdrctl = new flex_coded_ofdm_hdrctl_t();
  d_current_snr = 0;
}

flex_coded_ofdm_demod::~flex_coded_ofdm_demod ()
{
  delete last_hdrctl;
}

void
flex_coded_ofdm_demod::initialize()
{
  // A bit hacky to fill out carriers to occupied_carriers length
  int diff = (d_occupied_carriers - d_tone_map.length()); 
  int diff_left = (int)ceil((float)diff/2.0f);  // number of carriers to put on the left side
  int diff_right = diff - diff_left;        // number of carriers to put on the right side
  while(diff_left > 0) {
    d_tone_map.insert(0, "1");
    diff_left --;
  }
  
  while(diff_right > 0) {
    d_tone_map.insert(d_tone_map.length(), "1");
    diff_right --;
  }

  // linklab, print subcarrier usage: '|' is used, '.' is not used
  unsigned int i,j,k;
  d_subcarrier_map.clear();
  printf("\nUsing subcarriers: \n");
  for(i = 0; i < d_occupied_carriers; i++) {
    const char c = d_tone_map[i];
    if(c == '1') {
      d_subcarrier_map.push_back(i);
      printf("|"); 
    }
    else
      printf("."); 
  }
  printf("\n\n");
  
  // make sure we stay in the limit currently imposed by the occupied_carriers
  if(d_subcarrier_map.size() > d_occupied_carriers) {
    throw std::invalid_argument("flex_coded_ofdm_demod: subcarriers allocated exceeds size of occupied carriers");
  }

  //d_bytes_out = new unsigned char[d_occupied_carriers];
  d_dfe.resize(d_occupied_carriers);
  fill(d_dfe.begin(), d_dfe.end(), gr_complex(1.0,0.0));
}

void
flex_coded_ofdm_demod::set_rate(const flex_coded_ofdm_rate_t rate) {
	
  d_current_rate = rate;
  switch(rate) {
    case CODED_OFDM_RATE_BPSK_1_2:
    case CODED_OFDM_RATE_BPSK_3_4:
	if (VERBOSE)
		fprintf(stderr,"flex_coded_ofdm_demod: set BPSK modulation\n");

      set_modulation(d_bpsk_sym_position, d_bpsk_sym_value_out);
      break;

    case CODED_OFDM_RATE_QPSK_1_2:
    case CODED_OFDM_RATE_QPSK_3_4:
	if (VERBOSE)
		fprintf(stderr,"flex_coded_ofdm_demod: set QPSK modulation\n");

      set_modulation(d_qpsk_sym_position, d_qpsk_sym_value_out);
      break;

    case CODED_OFDM_RATE_8QAM_1_2:
    case CODED_OFDM_RATE_8QAM_3_4:
	if (VERBOSE)
		fprintf(stderr,"flex_coded_ofdm_demod: set QAM8 modulation\n");

      set_modulation(d_qam8_sym_position, d_qam8_sym_value_out);
      break;

    case CODED_OFDM_RATE_16QAM_1_2:
    case CODED_OFDM_RATE_16QAM_3_4:
	if (VERBOSE)
		fprintf(stderr,"flex_coded_ofdm_demod: set QAM16 modulation\n");

      set_modulation(d_qam16_sym_position, d_qam16_sym_value_out);
      break;

    case CODED_OFDM_RATE_64QAM_1_2:
    case CODED_OFDM_RATE_64QAM_2_3:
    case CODED_OFDM_RATE_64QAM_3_4:
	if (VERBOSE)
		fprintf(stderr,"flex_coded_ofdm_demod: set QAM64 modulation\n");

      set_modulation(d_qam64_sym_position, d_qam64_sym_value_out);
      break;

    case CODED_OFDM_RATE_256QAM_1_2:
    case CODED_OFDM_RATE_256QAM_3_4:
	if (VERBOSE)
		fprintf(stderr,"flex_coded_ofdm_demod: set QAM256 modulation\n");

      set_modulation(d_qam256_sym_position, d_qam256_sym_value_out);
      break;
  }
    d_rate = rate;
}

void
flex_coded_ofdm_demod::set_modulation(const std::vector<gr_complex> &sym_position, 
				      const std::vector<unsigned char> &sym_value_out)
{
  assert(sym_position.size() >= 2); 
  assert(sym_position.size() == sym_value_out.size());

  d_sym_position  = sym_position;
  d_sym_value_out = sym_value_out;
  d_nbits = (unsigned long)ceil(log10(d_sym_value_out.size()) / log10(2.0));
}

void 
flex_coded_ofdm_demod::forecast (int noutput_items, 
                            gr_vector_int &ninput_items_required) {
  int nrequired = (int)
    ceilf((float)noutput_items / (float)d_nbits / (float)d_subcarrier_map.size()); //(float)d_occupied_carriers);
	if (VERBOSE>1)
		fprintf(stderr,"flex_coded_ofdm_demod: forecast %d input for %d output (d_nbits=%d)\n",
         nrequired, noutput_items, d_nbits);
  ninput_items_required[0] = ninput_items_required[1] = nrequired;
}

void
flex_coded_ofdm_demod::reset_ofdm_params(std::string new_tone_map)
{
  d_tone_map = new_tone_map;
  //d_occupied_carriers = new_occupied_carriers;
  //std::cout << d_carrier_map << std::endl;
  initialize();
  set_modulation(d_bpsk_sym_position, d_bpsk_sym_value_out);
  enter_search();
  
  d_num_known_symbols=0;
  d_current_rate=0;
  memset(d_bits, 0, MAX_NUM_SYMBOLS*sizeof(unsigned char));
  d_current_snr = 0;
}

int
flex_coded_ofdm_demod::general_work (int noutput_items, gr_vector_int &ninput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
{
  const gr_complex      *in = (const gr_complex *) input_items[0];
  //const char         *pkt_detect_sig = (const char *) input_items[1]; // We dont use this now
  const char         *corr_found_sig = (const char *) input_items[1];
  const float		 * snr = (const float *) input_items[2];
  gr_complex 		*output_demod = (gr_complex *)output_items[0];
  gr_complex 		*output_demod_derot = (gr_complex *)output_items[1];
  float 			*output_conf = (float *)output_items[2];
  char 				*output_bits = (char *)output_items[3];
  char 				*output_sel = (char *)output_items[4];

if (VERBOSE>1)
    fprintf(stderr,"flex_coded_ofdm_demod: noutput_items=%d, ninput_items[0]=%d, ninput_items[1]=%d\n",
         noutput_items, ninput_items[0], ninput_items[1]);

  unsigned int symbols_produced = 0, oi;
  gr_message_sptr hdrctl_msg;
  flex_coded_ofdm_hdrctl_t *hdrctl;

  
  if (VERBOSE>1)
    fprintf(stderr,">>> Entering state machine with %x\n",corr_found_sig[0] );

  switch(d_state) {
      
  case STATE_SYNC_SEARCH:    // Look for flag indicating beginning of pkt
    if (VERBOSE>1)
      fprintf(stderr,"SYNC Search, noutput=%d\n", noutput_items);
    
    if (corr_found_sig[0]) {
      enter_have_sync();
    }
    d_current_snr = .5*d_current_snr + .5*snr[0];
    consume_each(1);
    //return 1;	
    break;

  case STATE_HAVE_SYNC:
	  if (corr_found_sig[0] ) {
        // TODO: estimate SINR of new packet and possibly lock on to that
        // instead.
		if (VERBOSE)
		  fprintf(stderr," flex_coded_ofdm_demod: warning, found SYNC in HAVE_SYNC\n");

		// Sen: 
		enter_search();
		// linknet implementation: enter_have_sync();
        //break;
      }
      else {
		symbols_produced = demapper(&in[0], output_demod, output_demod_derot, output_conf, output_bits);
		for (oi = 0; oi < symbols_produced; oi++) {
			output_sel[oi] = CODED_OFDM_DEMOD_HDR_SEL;
			if (VERBOSE>1) fprintf(stderr,"flex_coded_ofdm_demod: output in 2 %x\n", output_sel[oi]);
		}
      }
      break;  
  
  case STATE_CHECK_HEADER: 
      //fprintf(stderr,"flex_coded_ofdm_demod: checking hdrctl_msgq\n");

      hdrctl_msg = d_hdrctl_msgq->delete_head_nowait();
      
      //fprintf(stderr,"flex_coded_ofdm_demod: checed hdrctl_msgq\n");

      if (hdrctl_msg) {
        hdrctl = (flex_coded_ofdm_hdrctl_t *)hdrctl_msg->msg();
		memcpy((flex_coded_ofdm_hdrctl_t*) last_hdrctl, hdrctl, sizeof(flex_coded_ofdm_hdrctl_t));
		if (VERBOSE)
			fprintf(stderr,"flex_coded_ofdm_demod: header ok=%d, length=%d, rate=%02hx, seqno=%d, srcid=%d\n",
               hdrctl->good, hdrctl->length, hdrctl->rate, hdrctl->seqno, hdrctl->srcid);
		d_curr_seqno = 1; //hdrctl->seqno;
		d_curr_srcid = 1; //hdrctl->srcid;

        if (hdrctl->good) 
		{
          set_rate(hdrctl->rate);
          switch (hdrctl->rate) 
		  {
            case CODED_OFDM_RATE_BPSK_1_2:
            case CODED_OFDM_RATE_QPSK_1_2:
            case CODED_OFDM_RATE_8QAM_1_2:
            case CODED_OFDM_RATE_16QAM_1_2:
            case CODED_OFDM_RATE_64QAM_1_2: 
            case CODED_OFDM_RATE_256QAM_1_2:
              enter_have_header(hdrctl->length*2); // CODED bits length
	      //enter_search();
              break;

            case CODED_OFDM_RATE_BPSK_3_4:
            case CODED_OFDM_RATE_QPSK_3_4:
            case CODED_OFDM_RATE_8QAM_3_4:
            case CODED_OFDM_RATE_16QAM_3_4:
            case CODED_OFDM_RATE_64QAM_3_4:
            case CODED_OFDM_RATE_256QAM_3_4:            
              enter_have_header(hdrctl->length*4/3); // CODED bits length
              break;

            case CODED_OFDM_RATE_64QAM_2_3:
              enter_have_header(hdrctl->length*3/2); // CODED bits length
              break;

            default:
              assert(0);
		  }

		  if (VERBOSE)
		    fprintf(stderr,"flex_coded_ofdm_demod: got correct header, length=%d, rate=%02hx\n", hdrctl->length, hdrctl->rate);
	    } 
		else {

	  fprintf(stderr,"flex_coded_ofdm_demod: got incorrect header\n");
	  //symbols_produced = 1;		
          enter_search();
        }
        hdrctl_msg.reset();
	if (VERBOSE) fprintf(stderr,"flex_coded_ofdm_demod: got rid of the header\n");
	  }
      break;
      
  case STATE_HAVE_HEADER:
	  if (corr_found_sig[0] ) {
        // TODO: estimate SINR of new packet and possibly lock on to that instead.
		if (VERBOSE)
		  fprintf(stderr,"flex_coded_ofdm_demod: warning, found SYNC in HAVE_HEADER. abort packet %d from source %d\n\n", d_curr_seqno, d_curr_srcid);
		send_sisoctl_msg(1, d_packetlen_cnt*8, d_packetlen_cnt*8); //tell sisoctl to throw away stuff so sent far
		//flush_frame_sink();
		
		//Sen: 
		enter_search();
		// In the linklab implementation:
		//enter_have_sync();
        //break; 
      }
      else {
		symbols_produced = demapper(&in[0], output_demod, output_demod_derot, output_conf, output_bits);
    	for (oi = 0; oi < symbols_produced; oi++)
		{
			//printf("%d, ", output_bits[oi]);
		  	output_sel[oi] = d_rate;
		  	if (VERBOSE>1) fprintf(stderr,"flex_coded_ofdm_demod: output in 3 %x\n", output_sel[oi]);
		}
        
      }
      break;      
      
    
  default:
    assert(0);
    
  } // switch
  
  if (VERBOSE && symbols_produced)
    fprintf(stderr,"flex_coded_ofdm_demod: produced %u symbols\n", symbols_produced);
  else {
	  if (VERBOSE>1) fprintf(stderr,"flex_coded_ofdm_demod: no symbols produced\n");
  }

  
  //consume(0, 1);
  //consume(1, d_occupied_carriers);	
  
  //consume(2, d_occupied_carriers);	
  
  //consume(3, d_occupied_carriers);	
  
  //consume_each(1);
  //return 1;	
  return symbols_produced;
}
