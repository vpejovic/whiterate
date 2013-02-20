/* -*- c++ -*- */
/*
 * Copyright 2006,2007,2008 Free Software Foundation, Inc.
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

#include <flex_ofdm_mapper_flow.h>
#include <flex_ofdm_constants.h>
#include <gr_io_signature.h>
#include <stdexcept>
#include <string.h>
#include <stdio.h>

#define VERBOSE 0 

flex_ofdm_mapper_flow_sptr
flex_make_ofdm_mapper_flow (const std::vector<gr_complex> &constellation, 
							const std::vector<gr_complex> &header_constellation, 
							unsigned int msgq_limit, 
							unsigned int occupied_carriers, 
							unsigned int fft_length, 
							std::string tone_map, 
							const std::vector<float> &scale)
{
  return flex_ofdm_mapper_flow_sptr (new flex_ofdm_mapper_flow (
								constellation,
								header_constellation,
								msgq_limit, 
								occupied_carriers, 
								fft_length, 
								tone_map, 
								scale));
}

// Consumes 1 packet and produces as many OFDM symbols of fft_length to hold the full packet
flex_ofdm_mapper_flow::flex_ofdm_mapper_flow (const std::vector<gr_complex> &constellation, 
											const std::vector<gr_complex> &header_constellation, 
											unsigned int msgq_limit, 
											unsigned int occupied_carriers, 
											unsigned int fft_length, 
											std::string tone_map, 
											const std::vector<float> &scale)
  : gr_sync_block ("flex_ofdm_mapper_flow",
		   gr_make_io_signature (0, 0, 0),
		   gr_make_io_signature2 (1, 2, sizeof(gr_complex)*fft_length, sizeof(char))),
	d_header_constellation(header_constellation),
    d_body_constellation(constellation),
    d_constellation(header_constellation), // start with header constellation
    d_msgq(gr_make_msg_queue(msgq_limit)), d_msg_offset(0), d_eof(false),
    d_occupied_carriers(occupied_carriers),
    d_fft_length(fft_length),
    d_tone_map(tone_map),
    d_scale(scale),
    d_bit_offset(0),
    d_pending_flag(0),
    d_resid(0),
    d_nresid(0),
	d_mapping_header(true)
{
    // linklab, initialize subcarrier allocation 
    initialize();
}

// linklab, initialize subcarrier allocation 
void flex_ofdm_mapper_flow::initialize()
{
  if (!(d_occupied_carriers <= d_fft_length))
    throw std::invalid_argument("flex_ofdm_mapper_flow: occupied carriers must be <= fft_length");
  std::string carriers = d_tone_map;
  int diff = (d_occupied_carriers - 4*carriers.length()); 
  while(diff > 7) {
    carriers.insert(0, "f");
    carriers.insert(carriers.length(), "f");
    diff -= 8;
  }  

  int diff_left=0;
  int diff_right=0;

  char abc[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  if(diff > 0) {
    char c[2] = {0,0};
    diff_left = (int)ceil((float)diff/2.0f);   // number of carriers to put on the left side
    c[0] = abc[(1 << diff_left) - 1];          // convert to bits and move to ASCI integer
    carriers.insert(0, c);   
    diff_right = diff - diff_left;	       // number of carriers to put on the right side
    c[0] = abc[0xF^((1 << diff_right) - 1)];   // convert to bits and move to ASCI integer
    carriers.insert(carriers.length(), c);
  }  

  // find out how many zeros to pad on the sides; the difference between the fft length and the subcarrier
  // mapping size in chunks of four. This is the number to pack on the left and this number plus any 
  // residual nulls (if odd) will be packed on the right. 
  //diff = (d_fft_length/4 - carriers.length())/2; 
  //to avoid uncentered sub-carrier assignment, linklab
  diff = d_fft_length/4 - carriers.length(); 

  printf("\nUsing subcarriers: \n");
  unsigned int i,j,k;
  d_subcarrier_map.clear(); // linklab, clear the subcarrier map
  for(i = 0; i < carriers.length(); i++) {
    char c = carriers[i];                            // get the current hex character from the string
    for(j = 0; j < 4; j++) {                         // walk through all four bits
      k = (strtol(&c, NULL, 16) >> (3-j)) & 0x1;     // convert to int and extract next bit
      if(k) {                                        // if bit is a 1, 
		d_subcarrier_map.push_back(4*i+2*diff + j);  // use this subcarrier
		printf("|");   // link, "|" means the subcarrier is used
      }
      else printf("."); //linklab, "." means the subcarrier is not used
    }
  }
  printf("\n\n");

  if(d_subcarrier_map.size() > d_occupied_carriers) {
    throw std::invalid_argument("flex_ofdm_mapper_flow: subcarriers allocated exceeds size of occupied carriers");
  }
  
  /*
   * Veljko: for mixed modulations 
  for (i = 0; i < d_subcarrier_map.size(); i++){
	d_nbits_pkt.push_back((unsigned long)ceil(log10(d_constellations[d_modulation_map_pkt[i]].size()) / log10(2.0)));
	d_nbits_pkt_sum += d_nbits_pkt[i];
  }
  
  for (i = 0; i < d_subcarrier_map.size(); i++){
    d_nbits_base.push_back((unsigned long)ceil(log10(d_constellations[d_modulation_map_base[i]].size()) / log10(2.0)));
	d_nbits_base_sum += d_nbits_base[i];
  }*/
  
  d_nbodybits = (unsigned long)ceil(log10(d_body_constellation.size()) / log10(2.0));
  d_nheaderbits = (unsigned long)ceil(log10(d_header_constellation.size()) / log10(2.0));
  // start mapping header bits
  d_nbits = d_nheaderbits;
  d_constellation = d_header_constellation;
}


flex_ofdm_mapper_flow::~flex_ofdm_mapper_flow(void)
{
}

int flex_ofdm_mapper_flow::randsym()
{
  return (rand() % d_constellation.size());
}

// TODO: FIX THIS
void flex_ofdm_mapper_flow::reset_ofdm_params(const std::vector<gr_complex> &new_constellation, std::string new_tone_map)
{
   d_tone_map = new_tone_map;
   d_body_constellation = new_constellation;
   initialize();
}

int
flex_ofdm_mapper_flow::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
{
  // The header should be coded with the base modulation.
  // What is the header length? Should be passed as a parameter?	
  // For now, just a dummy variable

  gr_complex *out = (gr_complex *)output_items[0];
  
  unsigned int i=0;
  
  //printf("OFDM BPSK Mapper:  ninput_items: %d   noutput_items: %d\n", ninput_items[0], noutput_items);

  if(d_eof) {
    return -1;
  }
  
  if(!d_msg) {
    if (d_msgq->empty_p())
    {
          memset(out, 0, d_fft_length*sizeof(gr_complex));
          return 1;
    }
    d_msg = d_msgq->delete_head();	   // block, waiting for a message
    d_msg_offset = 0;
    d_bit_offset = 0;
    d_pending_flag = 1;			   // new packet, write start of packet flag
    
    if((d_msg->length() == 0) && (d_msg->type() == 1)) {
      d_msg.reset();
      return -1;		// We're done; no more messages coming.
    }
  }

  char *out_flag = 0;
  if(output_items.size() == 2)
    out_flag = (char *) output_items[1]; 
  
  // Build a single symbol: initialize all bins to 0 to set unused carriers
  memset(out, 0, d_fft_length*sizeof(gr_complex));
  
  i = 0;
  unsigned char bits = 0;

  // We fill the symbol as long as:
  // - we have more header data (stop before reaching the payload data) 
  // - we have more payload data
  while( ( (d_mapping_header && d_msg_offset < CODED_OFDM_HEADERLEN_CODED_BYTES)
	  || (!d_mapping_header && d_msg_offset < d_msg->length()) ) 
	 && (i < d_subcarrier_map.size()) ) { 

    // need new data to process
    if(d_bit_offset == 0) {
      d_msgbytes = d_msg->msg()[d_msg_offset];
    }

    if(d_nresid > 0) {
      // take the residual bits, fill out nbits with info from the new byte, and put them in the symbol
      d_resid |= (((1 << d_nresid)-1) & d_msgbytes) << (d_nbits - d_nresid);
      bits = d_resid;

      out[d_subcarrier_map[i]] = d_scale[i]*d_constellation[bits];
      
      i++;
      d_bit_offset += d_nresid;
      d_nresid = 0;
      d_resid = 0;
#if VERBOSE > 1
      printf("mod bit(r): %x   resid: %x   nresid: %d    bit_offset: %d\n", 
           bits, d_resid, d_nresid, d_bit_offset);
 #endif
    }
    else {
      if((8 - d_bit_offset) >= d_nbits) {  // test to make sure we can fit nbits
		// take the nbits number of bits at a time from the byte to add to the symbol
		bits = ((1 << d_nbits)-1) & (d_msgbytes >> d_bit_offset);
		d_bit_offset += d_nbits;
		
		out[d_subcarrier_map[i]] = d_scale[i]*d_constellation[bits];
		i++;

      }
      else {  // if we can't fit nbits, store them for the next 
		// saves d_nresid bits of this message where d_nresid < nbits
		unsigned int extra = 8-d_bit_offset;
		d_resid = ((1 << extra)-1) & (d_msgbytes >> d_bit_offset);
		d_bit_offset += extra;
		d_nresid = d_nbits - extra;
      }      
    }
            
    if(d_bit_offset == 8) {
      d_bit_offset = 0;
      d_msg_offset++;
    }   
  }


  // Ran out of data to put in symbol
  if( (d_mapping_header && d_msg_offset == CODED_OFDM_HEADERLEN_CODED_BYTES)
      || (!d_mapping_header && d_msg_offset == d_msg->length())) {

    if(d_nresid > 0) {
      d_resid |= 0x00;
      bits = d_resid;
      d_nresid = 0;
      d_resid = 0;
    }
    
    while(i < d_subcarrier_map.size()) {   // finish filling out the symbol
      //if(!d_mapping_header)	printf("gr_ofdm_mapper: WARNING: filling randoms symbol\n");
      out[d_subcarrier_map[i]] = -0.1; //d_scale[i]*d_constellation[randsym()]; //-0.1;
#if VERBOSE > 1
      printf("subcarrier %d (actual %d) rand symbol %f + %f j\n", i, d_subcarrier_map[i],
	     out[d_subcarrier_map[i]].real(), out[d_subcarrier_map[i]].imag() );
#endif
      i++;
    }
    
    if(d_mapping_header) { //last header symbol
      d_nbits = d_nbodybits;
      d_constellation = d_body_constellation;
      d_mapping_header = false;
#if VERBOSE
      printf("gr_ofdm_mapper: last header symbol sent\n");
#endif
    }
    else {//last data symbol

      if (d_msg->type() == 1) d_eof = true; // type == 1 sets EOF
      d_msg.reset();   			// finished packet, free message
      assert(d_bit_offset == 0);
            
      // get ready for next packet
      d_nbits = d_nheaderbits;
      d_constellation = d_header_constellation;
      d_mapping_header = true;

#if VERBOSE
      printf("gr_ofdm_mapper: last data symbol sent\n");
#endif
    }
  }
  
  if (out_flag) out_flag[0] = d_pending_flag;
  d_pending_flag = 0;
  
  return 1;  // produced symbol
}
