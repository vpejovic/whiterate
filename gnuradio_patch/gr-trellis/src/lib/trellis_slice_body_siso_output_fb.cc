/* -*- c++ -*- */
/*
 * Copyright 2004,2006 Free Software Foundation, Inc.
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

#include <trellis_slice_body_siso_output_fb.h>
#include <gr_io_signature.h>
#include <stdio.h>

#define VERBOSE 0



static unsigned char IDEAL_PKT[] = {
255, 63, 0, 16, 0, 12, 0, 5, 192, 3, 16, 1, 204, 0, 85, 192, 63, 16, 16, 12, 12, 5, 197, 195, 19, 17, 205, 204, 85, 149, 255, 47, 0, 28, 0, 9, 192, 6, 208, 2, 220, 1, 153, 192, 106, 208, 47, 28, 28, 9, 201, 198, 214, 210, 222, 221, 152, 89, 170, 186, 255, 51, 0, 21, 192, 15, 16, 4, 12, 3, 69, 193, 243, 16, 69, 204, 51, 21, 213, 207, 31, 20, 8, 15, 70, 132, 50, 227, 85, 137, 255, 38, 192, 26, 208, 11, 28, 7, 73, 194, 182, 209, 182, 220, 118, 217, 230, 218, 202, 219, 23, 27, 78, 139, 116, 103, 103, 106, 170, 175, 63, 60, 16, 17, 204, 12, 85, 197, 255, 19, 0, 13, 192, 5, 144, 3, 44, 1, 221, 192, 89, 144, 58, 236, 19, 13, 205, 197, 149, 147, 47, 45, 220, 29, 153, 201, 170, 214, 255, 30, 192, 8, 80, 6, 188, 2, 241, 193, 132, 80, 99, 124, 41, 225, 222, 200, 88, 86, 186, 190, 243, 48, 69, 212, 51, 31, 85, 200, 63, 22, 144, 14, 236, 4, 77, 195, 117, 145, 231, 44, 74, 157, 247, 41, 134, 158, 226, 232, 73, 142, 182, 228, 118, 203, 102, 215, 106, 222, 175, 24, 124, 10, 161, 199, 56, 82, 146, 189, 173, 177, 189, 180, 113, 183, 100, 118, 171, 102, 255, 106, 192, 47, 16, 28, 12, 9, 197, 198, 211, 18, 221, 205, 153, 149, 0, 255, 63, 0, 16, 0, 12, 0, 5, 192, 3, 16, 1, 204, 0, 85, 192, 63, 16, 16, 12, 12, 5, 197, 195, 19, 17, 205, 204, 85, 149, 255, 47, 0, 28, 0, 9, 192, 6, 208, 2, 220, 1, 153, 192, 106, 208, 47, 28, 28, 9, 201, 198, 214, 210, 222, 221, 152, 89, 170, 186, 255, 51, 0, 21, 192, 15, 16, 4, 12, 3, 69, 193, 243, 16, 69, 204, 51, 21, 213, 207, 31, 20, 8, 15, 70, 132, 50, 227, 85, 137, 255, 38, 192, 26, 208, 11, 28, 7, 73, 194, 182, 209, 182, 220, 118, 217, 230, 218, 202, 219, 23, 27, 78, 139, 116, 103, 103, 106, 170, 175, 63, 60, 16, 17, 204, 12, 85, 197, 255, 19, 0, 13, 192, 5, 144, 3, 44, 1, 221, 192, 89, 144, 58, 236, 19, 13, 205, 197, 149, 147, 47, 45, 220, 29, 153, 201, 170, 214, 255, 30, 192, 8, 80, 6, 188, 2, 241, 193, 132, 80, 99, 124, 41, 225, 222, 200, 88, 86, 186, 190, 243, 48, 69, 212, 51, 31, 85, 200, 63, 22, 144, 14, 236, 4, 77, 195, 117, 145, 231, 44, 74, 157, 247, 41, 134, 158, 226, 232, 73, 142, 182, 228, 118, 203, 102, 215, 106, 222, 175, 24, 124, 10, 161, 199, 56, 82, 146, 189, 173, 177, 189, 180, 113, 183, 100, 118, 171, 102, 255, 106, 192, 47, 16, 28, 12, 9, 197, 198, 211, 18, 221, 205, 153, 149, 0, 255, 63, 0, 16, 0, 12, 0, 5, 192, 3, 16, 1, 204, 0, 85, 192, 63, 16, 16, 12, 12, 5, 197, 195, 19, 17, 205, 204, 85, 149, 255, 47, 0, 28, 0, 9, 192, 6, 208, 2, 220, 1, 153, 192, 106, 208, 47, 28, 28, 9, 201, 198, 214, 210, 222, 221, 152, 89, 170, 186, 255, 51, 0, 21, 192, 15, 16, 4, 12, 3, 69, 193, 243, 16, 69, 204, 51, 21, 213, 207, 31, 20, 8, 15, 70, 132, 50, 227, 85, 137, 255, 38, 192, 26, 208, 11, 28, 7, 73, 194, 182, 209, 182, 220, 118, 217, 230, 218, 202, 219, 23, 27, 78, 139, 116, 103, 103, 106, 170, 175, 63, 60, 16, 17, 204, 12, 85, 197, 255, 19, 0, 13, 192, 201, 1, 77, 178, 63, 0, 0, 0, 0, 78, 98, 178, 63, 76, 202, 177, 63};


trellis_slice_body_siso_output_fb_sptr
trellis_make_slice_body_siso_output_fb (unsigned int D,
                                   const std::vector<unsigned char> &tbl, gr_msg_queue_sptr body_siso_msgq, gr_msg_queue_sptr hdrctl_msgq, gr_msg_queue_sptr recv_msgq, char * dump_file) {
  return trellis_slice_body_siso_output_fb_sptr (new trellis_slice_body_siso_output_fb (D, tbl, body_siso_msgq, hdrctl_msgq, recv_msgq, dump_file));
}

trellis_slice_body_siso_output_fb
  ::trellis_slice_body_siso_output_fb (unsigned int D, 
                                  const std::vector<unsigned char> &tbl,
				  gr_msg_queue_sptr body_siso_msgq, gr_msg_queue_sptr hdrctl_msgq, gr_msg_queue_sptr recv_msgq, char * dump_file)
    : gr_sync_decimator ("slice_body_siso_output_fb",
        gr_make_io_signature (1, 1, sizeof(float)),
        gr_make_io_signature (1, 1, sizeof(char)), 
        1), // decimation factor is D
      d_D(D), d_tbl(tbl), d_body_siso_msgq(body_siso_msgq), d_hdrctl_msgq(hdrctl_msgq), d_recv_msgq(recv_msgq), d_file(dump_file) 
{ 
  assert(tbl.size() == D);
#if VERBOSE
  printf("trellis_slice_body_siso_output_fb: ");
  for (unsigned int i = 0; i < d_tbl.size(); i++)
    printf("d_tbl[%d]=%hx ", i, d_tbl[i]);
  printf("\n");
#endif
  //printf("Dump filename is %s\n", d_file);
  //d_fp = fopen(d_file, "a");
}

unsigned int
trellis_get_bit_be1_unpacked_to_packed_bb (const unsigned char *in_vector,unsigned int bit_addr, unsigned int bits_per_chunk) {
  unsigned int byte_addr = (int)bit_addr/bits_per_chunk;
  unsigned char x = in_vector[byte_addr];
  unsigned int residue = bit_addr - byte_addr * bits_per_chunk;
  //printf("Bit addr %d  byte addr %d  residue %d  val  %d\n",bit_addr,byte_addr,residue,(x>>(bits_per_chunk-1-residue))&1);
  return (x >> (bits_per_chunk-1-residue))&1;
}


trellis_slice_body_siso_output_fb::~trellis_slice_body_siso_output_fb()
{
	//fclose(d_fp);
}

int
trellis_slice_body_siso_output_fb::work (int noutput_items,
			     gr_vector_const_void_star &input_items,
			     gr_vector_void_star &output_items)
{
  //const float *in = (const float *) input_items[0];
  unsigned char *out = (unsigned char *) output_items[0];

  float interference_metric=0;
  float prev_interference_metric=0;
  double max_int_diff = 0;
  int d_num_subcarriers = 384;
  int subcarrier_count =0;

#if VERBOSE
  printf("trellis_slice_body_siso_output_fb::work: noutput_items=%d\n", noutput_items);
#endif
 
  
  gr_message_sptr body_siso_msg = d_body_siso_msgq->delete_head_nowait();
  gr_message_sptr hdrctl_msg = d_hdrctl_msgq->delete_head_nowait();

  int num_items = body_siso_msg->length()/(sizeof(float));
  assert(num_items%2==0);
  unsigned char slice_out[num_items>>1];
  unsigned char packet[num_items/16]; //Veljko: why was it +1?
  unsigned int packetlen = num_items/16;// + 1;
  int bit_ptr=0, byte_ptr=0;
  double BER=0;

  gr_coded_ofdm_hdrctl * hdr = (gr_coded_ofdm_hdrctl *) hdrctl_msg->msg();
  
  if(!body_siso_msg || !hdrctl_msg)
    return noutput_items;
  else
  {
    float * in = (float *)body_siso_msg->msg();
	float gaussian_factor = 0;
	int d_nbits = 0;

    switch (hdr->rate)
	{
        case CODED_OFDM_RATE_BPSK_1_2:
			d_nbits = 1;
			gaussian_factor = 2*.67*.67;
			break;	        
        case CODED_OFDM_RATE_QPSK_1_2:
			d_nbits = 2;
			gaussian_factor = 2*.67*.67;
			break;	        
    	case CODED_OFDM_RATE_8QAM_1_2:
			d_nbits = 3;
			gaussian_factor = 2*.67*.67;
			break;	        			
    	case CODED_OFDM_RATE_16QAM_1_2:
			d_nbits = 4;
			gaussian_factor = 2*.67*.67;
			break;	   
    	case CODED_OFDM_RATE_64QAM_1_2:
			d_nbits = 6;
			gaussian_factor = 2*.67*.67;
			break;	        			     
    	case CODED_OFDM_RATE_256QAM_1_2:
			d_nbits = 8;
			gaussian_factor = 2*.67*.67;
			break;	   

        case CODED_OFDM_RATE_BPSK_3_4:
			d_nbits = 1;
			gaussian_factor = 2*.5*.5;
			break;	        

        case CODED_OFDM_RATE_QPSK_3_4:
			d_nbits = 2;
			gaussian_factor = 2*.5*.5;
			break;	        

        case CODED_OFDM_RATE_8QAM_3_4:
			d_nbits = 3;
			gaussian_factor = 2*.5*.5;
			break;	        

        case CODED_OFDM_RATE_16QAM_3_4:
			d_nbits = 4;
			gaussian_factor = 2*.5*.5;
			break;	        

        case CODED_OFDM_RATE_64QAM_3_4:
			d_nbits = 6;
			gaussian_factor = 2*.5*.5;
			break;	   
			
        case CODED_OFDM_RATE_256QAM_3_4:
			d_nbits = 8;
			gaussian_factor = 2*.5*.5;
			break;	        			     
			     
		case CODED_OFDM_RATE_64QAM_2_3: //Veljko TODO: this is not true
			d_nbits = 6;
			gaussian_factor = 2*.67*.67;
			break;	 
			default:
              assert(0);
	}
	
	assert(gaussian_factor != 0);
	assert(d_nbits != 0);

    for (int i = 0; i < num_items>>1; i++) 
	{
	    float max = -(float)INFINITY;
	    int maxd = 0;
 
	    for (unsigned int d = 0; d < d_D; d++) 
		{
	      	if (in[i*d_D + d] > max) 
			{
	        	maxd = d;
	        	max = in[i*d_D + d];
      		}
      //MV
      //printf("iter=%d bit=%u prob=%.3f\n", i, d, in[i*d_D + d]);

    	}
	
#if VERBOSE > 1 
    	if(noutput_items > 48) 
		{
    	  printf("trellis_slice_body_siso_output_fb::work: maxd=%d for i=%d -> %hx maxprob=%.3f\n", 
		     maxd, i, d_tbl[maxd], max);
    	}
#endif
		if(max > 20)
			max = 20;

		/* Calculate the Softrate BER here */
		/* Depending on the rate BER calculation will vary */

		BER += 1000/(1+exp((double)max/(gaussian_factor)));
		
		if(subcarrier_count < d_num_subcarriers)
		{
			interference_metric += 1/(1+exp((double)max/(gaussian_factor)));
		}
		else
		if(subcarrier_count == d_num_subcarriers)
		{
			if(prev_interference_metric != 0)
			{
				max_int_diff = std::max(fabs(prev_interference_metric - interference_metric), max_int_diff);
			}
			prev_interference_metric = interference_metric;
			subcarrier_count = 0;
		}

    	slice_out[i] = d_tbl[maxd];
  	}

  }

  if(subcarrier_count != 0)
  {
	if(prev_interference_metric != 0)
	{
		float cur_metric = (interference_metric/(1.0*subcarrier_count))*384; 
		max_int_diff = std::max(fabs(prev_interference_metric - cur_metric), max_int_diff);
	}
	prev_interference_metric = interference_metric;
	subcarrier_count = 0;
  }


  #define BITS_PER_TYPE sizeof(unsigned char) * 8
  unsigned char tmp=0;
  int index_tmp = 0, d_bits_per_chunk = 1;

 
  for(int i=0; i<num_items/16; i++) 
  {
	unsigned long tmp=0;
	for(unsigned int j=0; j<BITS_PER_TYPE; j++) 
	{
	  tmp = (tmp>>1)| (trellis_get_bit_be1_unpacked_to_packed_bb(slice_out,index_tmp,d_bits_per_chunk)<<(BITS_PER_TYPE-1));
	  index_tmp++;
	}
	packet[i] = tmp;
  }

  bool ok = true;
  int num_errors = 0;
  //for(int i=0; i<num_items/16 - 50; i++)
#if VERBOSE	
  for(int i=0; i<packetlen; i++)
  {
	/*if(packet[i] != IDEAL_PKT[i])
	{
		ok = false;
		num_errors++;
	}*/	
	printf("%x, ", packet[i]);
  }
#endif  
  gr_message_sptr msg = gr_make_message(0, 0, 0, packetlen);
  memcpy(msg->msg(), packet, packetlen);
  d_recv_msgq->insert_tail(msg);		// send it
  msg.reset();  				// free it up
  
  
  if(num_errors <= 10)
	ok = true;
  //fprintf(stderr, "seqno %d\t orig_rate %d\t rate %d\t correct %d\t BER: %f\t num_known_symbols %d\t max diff %f\t snr %f\n", hdr->seqno, hdr->orig_rate, hdr->rate, ok, BER/((num_items>>1)), hdr->num_known_symbols, max_int_diff, hdr->snr);

  //d_fp = fopen(d_file, "a");
#if VERBOSE
  fprintf(stderr, "%d\t %d\t %d\t %d\t %f\t %d\t %f\t %f\n", hdr->seqno, hdr->orig_rate, hdr->rate, ok, BER/((num_items>>1)), hdr->num_known_symbols, max_int_diff, hdr->snr);
#endif
  //fclose(d_fp);

  body_siso_msg.reset();
  return noutput_items;
}
