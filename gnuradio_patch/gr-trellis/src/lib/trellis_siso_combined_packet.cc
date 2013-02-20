/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
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

#include <trellis_siso_combined_packet.h>
#include <gr_ofdm_struct_dot11a.h>
#include <gr_io_signature.h>
#include <stdexcept>
#include <assert.h>
#include <iostream>
#include <cstdio>

#define VERBOSE 0
  
static const float INF = 1.0e9;

trellis_siso_combined_packet_sptr 
trellis_make_siso_combined_packet (
    unsigned char tag,
    const fsm &FSM,
    int S0,
    int SK,
    bool POSTI,
    bool POSTO,
    trellis_siso_type_t SISO_TYPE,
    int D,
    const std::vector<float> &TABLE,
    trellis_metric_type_t TYPE)
{
  return trellis_siso_combined_packet_sptr (new trellis_siso_combined_packet (tag,FSM,S0,SK,POSTI,POSTO,SISO_TYPE,D,TABLE,TYPE));
}

trellis_siso_combined_packet::trellis_siso_combined_packet (
    unsigned char tag,
    const fsm &FSM,
    int S0,
    int SK,
    bool POSTI,
    bool POSTO,
    trellis_siso_type_t SISO_TYPE,
    int D,
    const std::vector<float> &TABLE,
    trellis_metric_type_t TYPE)
  : gr_block ("siso_combined_packet",
			  gr_make_io_signature (1, -1, sizeof (float)),
			  gr_make_io_signature (1, -1, sizeof (float))),  
  d_FSM (FSM),
  d_S0 (S0),
  d_SK (SK),
  d_POSTI (POSTI),
  d_POSTO (POSTO),
  d_SISO_TYPE (SISO_TYPE),
  d_D (D),
  d_TABLE (TABLE),
  d_TYPE (TYPE),
  d_body_siso_msgq(gr_make_msg_queue(1)),
  //d_alpha(FSM.S()*(K+1)),
  //d_beta(FSM.S()*(K+1))
  d_sisoctl_msgq(gr_make_msg_queue(4)),
  d_input_ready(false),
    d_tag(tag),
    d_flush(false)
{
    if (d_POSTI && d_POSTO) 
        d_multiple = d_FSM.I()+d_FSM.O();
    else if(d_POSTI)
        d_multiple = d_FSM.I();
    else if(d_POSTO)
        d_multiple = d_FSM.O();
    else
        throw std::runtime_error ("Not both POSTI and POSTO can be false.");
    //printf("constructor: Multiple = %d\n",d_multiple);
    //set_output_multiple (d_K*d_multiple);
    //what is the meaning of relative rate for a block with 2 inputs?
    //set_relative_rate ( d_multiple / ((double) d_FSM.I()) );
    // it turns out that the above gives problems in the scheduler, so 
    // let's try (assumption O>I)
    //set_relative_rate ( d_multiple / ((double) d_FSM.O()) );
    // I am tempted to automate like this
    if(d_FSM.I() <= d_D) {
      set_relative_rate ( d_multiple / ((double) d_D) );
#if VERBOSE
      printf("Setting relative rate d_multiple/d_D %d/%d\n", d_multiple, d_D);
#endif
    }
    else {
      set_relative_rate ( d_multiple / ((double) d_FSM.I()) ); 
#if VERBOSE
      printf("Setting relative rate d_multiple/num fsm inputs %d/%d\n", d_multiple, d_FSM.I());
#endif
    }

	/* Set by Souvik */
  	num_oldinput[0] = num_oldinput[1] = 0;
	memset(&old_input[0][0], 0x0, 176000*sizeof(float));
	memset(&old_input[1][0], 0x0, 176000*sizeof(float));
}

void
trellis_siso_combined_packet::forecast (int noutput_items, gr_vector_int &ninput_items_required)
{
#if VERBOSE > 2
  printf("trellis_siso_combined_packet(0x%02d) forecast: d_multiple = %d\n", 
         d_tag, d_multiple); 
#endif
  //assert (noutput_items % (d_K*d_multiple) == 0);
  //int input_required1 =  d_FSM.I() * (noutput_items/d_multiple) ;
  //int input_required2 =  d_D * (noutput_items/d_multiple) ;
#if VERBOSE > 1
  /*if (d_input_ready) {
    printf("trellis_siso_combined_packet(0x%02d) forecast: Output requirements: %d\n",
           d_tag, noutput_items);
    printf("trellis_siso_combined_packet(0x%02d) forecast: Input requirements: %d, %d\n",
           d_tag, input_required1,input_required2);
  }*/
#endif
  unsigned ninputs = ninput_items_required.size();
  assert(ninputs % 2 == 0);
  for (unsigned int i = 0; i < ninputs/2; i++) {
    ninput_items_required[2*i] = noutput_items; //input_required1;
    ninput_items_required[2*i+1] = noutput_items; //input_required2;
  }
}

inline float min(float a, float b)
{
  return a <= b ? a : b;
}

inline float min_star(float a, float b)
{
  return (a <= b ? a : b)-log(1+exp(a <= b ? a-b : b-a));
}

extern void siso_algorithm_combined(unsigned char tag, int I, int S, int O, 
             const std::vector<int> &NS,
             const std::vector<int> &OS,
             const std::vector< std::vector<int> > &PS,
             const std::vector< std::vector<int> > &PI,
             int K,
             int S0,int SK,
             bool POSTI, bool POSTO,
             float (*p2mymin)(float,float),
             int D,
             const std::vector<float> &TABLE,
             trellis_metric_type_t TYPE,
             const float *priori, const float *observations, 
             float *post
             ); 



void trellis_siso_combined_packet::consume_old_samples(int num_samples, unsigned char which_input)
{

	memcpy(old_input[which_input], old_input[which_input] + num_samples, (num_oldinput[which_input] - num_samples)*sizeof(float));
	num_oldinput[which_input] -= num_samples;
	return;

}

void trellis_siso_combined_packet::store_samples(float * new_samples, int num_samples, unsigned char which_input)
{

	memcpy(&old_input[which_input][0] + num_oldinput[which_input], new_samples, num_samples*sizeof(float));
	num_oldinput[which_input] += num_samples;
	return;
}



int
trellis_siso_combined_packet::general_work (int noutput_items,
                        gr_vector_int &ninput_items,
                        gr_vector_const_void_star &input_items,
                        gr_vector_void_star &output_items)
{

	int num_future_items0 =0;
	int num_future_items1 =0;

  //consume(0, ninput_items[0]);
  //consume(1, ninput_items[1]);

	if(d_flush) 
	{

		if(num_oldinput[0] + ninput_items[0] < d_flush_input0)
		{
			store_samples((float *)input_items[0], ninput_items[0], 0);
			consume(0, ninput_items[0]);
			return 0;
		}
		else
		{
			//store_samples((float **) input_items, d_flush_input0 - num_oldinput[0], 0x1);
			assert(d_flush_input0 >= num_oldinput[0]);
			consume(0, d_flush_input0 - num_oldinput[0]);
			num_oldinput[0] = 0;
		}							

		if(num_oldinput[1] + ninput_items[1] < d_flush_input1)
		{
			store_samples((float *) input_items[1], ninput_items[1], 1);
			consume(1, ninput_items[1]);
			return 0;
		}
		else
		{
			//store_samples((float **) input_items, d_flush_input1 - num_oldinput[1], 0x1);
			assert(d_flush_input1 >= num_oldinput[1]);
			consume(1, d_flush_input1 - num_oldinput[1]);
			num_oldinput[1] = 0;
		}							

    	/*if (ninput_items[1] < d_flush_input1 ||
			ninput_items[0] < d_flush_input0) 
		{
#if VERBOSE
      printf("trellis_siso_combined_packet: not enough input to flush. reqd: %d %d, avlbl: %d %d\n",
	     d_flush_input0,d_flush_input1,ninput_items[0], ninput_items[1] );
#endif
			return 0;
	    }

    	consume(0, d_flush_input0);
    	consume(1, d_flush_input1);

#if VERBOSE 
      	printf("trellis_siso_combined_packet: flushed inputs %d %d, avlbl %d %d\n",
	     	d_flush_input0,d_flush_input1,ninput_items[0], ninput_items[1]);
#endif
	    */
      	d_flush = false;
      	return 0;

  	}

  	if (!d_input_ready) 
	{
    	// Wait for the demodulator to enqueue a control message, and set
    	// our block length d_K accordingly.
    	gr_message_sptr sisoctl_msg = d_sisoctl_msgq->delete_head_nowait();
    	if (sisoctl_msg) 
		{
      		gr_coded_ofdm_sisoctl_t *sisoctl = (gr_coded_ofdm_sisoctl_t *)sisoctl_msg->msg();
      

      		if(sisoctl->arg == 1) 
			{ //flush

				assert (input_items.size() == 2*output_items.size());
				int nstreams = output_items.size();
				int len = sisoctl->length_total; 
	
				d_flush_input0 = d_FSM.I() * len * d_FSM.I() / d_FSM.O();
				d_flush_input1 = d_D * len * d_FSM.I() / d_FSM.O();
				d_flush = true;
	
#if VERBOSE 
	printf("trellis_siso_combined_packet(0x%02d): got FLUSH message len=%d in0=%d in1=%d nstreams=%d avlbl in0=%d in1=%d\n", 
	       d_tag, len, d_flush_input0, d_flush_input1, nstreams, ninput_items[0], ninput_items[1]);
#endif
				return 0;

      		} //end flush

      		//not flush message, proceed further

      		// K is the block size in trellis steps.
      		// sisoctl->length is in output (coded) bits.
      		d_K = sisoctl->length_valid * d_FSM.I() / d_FSM.O();
      		d_K_total = sisoctl->length_total * d_FSM.I() / d_FSM.O();
      		d_input_ready = true;
			
			
#if VERBOSE
      printf("trellis_siso_combined_packet(0x%02d): got sisoctl msg, length_valid=%d, ninput_items[1]=%d, set d_K=%d, d_multiple=%d d_K_total=%d, d_FSM.I()=%d, d_FSM.O()=%d\n", 
	      d_tag, sisoctl->length_valid, ninput_items[1], d_K, d_multiple, d_K_total, d_FSM.I(), d_FSM.O());
#endif
			if(num_oldinput[0] + ninput_items[0] < d_FSM.I()*(d_K))
			{
				//store_samples((float *) input_items[0], ninput_items[0], 0x0);
				consume(0, ninput_items[0]);
			}
			else
			{
				//store_samples((float *) input_items[0], d_FSM.I()*(d_K) - num_oldinput[0], 0x0);
				consume(0, ninput_items[0]);
			}							

			if(num_oldinput[1] + ninput_items[1] < d_D*d_K_total)
			{
				store_samples((float *) input_items[1], ninput_items[1], 0x1);
				consume(1, ninput_items[1]);
				num_future_items1 = 0;
				return 0; 
			}
			else
			{
#if VERBOSE
				printf("Storing partial samples\n");
#endif      
				store_samples((float *) input_items[1], ninput_items[1], 0x1);
				num_future_items1 = num_oldinput[1] - d_D*d_K_total;
				consume(1, ninput_items[1]);
			}							

      		set_output_multiple(std::min(d_K*d_multiple>>3, 510));
	  
		} 
//#if VERBOSE > 2
//    	else
//      		printf("trellis_siso_combined_packet(0x%02d): no control msg\n", d_tag);
//#endif
		else
			return 0;
	}// if the input is not ready

  // BPSK: D=4 <=> 2 bits, 1 bit/sym
  // QPSK: D=2 <=> 2 bits, 2 bits/sym 
  //if (ninput_items[1] < d_K_total*d_D) {
  if(num_oldinput[1] < d_K_total*d_D){
#if VERBOSE
    printf("trellis_siso_combined_packet(0x%02d): input not finished (d_K_total=%d, d_D=%d, ninput_items[1]=%d, num_oldinput[1]=%d)\n", d_tag, d_K_total, d_D, ninput_items[1], num_oldinput[1]);
#endif
	if(num_oldinput[0] + ninput_items[0] < d_FSM.I()*(d_K_total))
	{
		//store_samples((float *) input_items[0], ninput_items[0], 0x0);
		consume(0, ninput_items[0]);
	}
	else
	{
		//store_samples((float *) input_items[0], d_FSM.I()*(d_K_total) - num_oldinput[0], 0x0);
		consume(0, ninput_items[0]);
	}							

	if(num_oldinput[1] + ninput_items[1] < d_D*d_K_total)
	{
		store_samples((float *) input_items[1], ninput_items[1], 0x1);
		consume(1, ninput_items[1]);
		num_future_items1=0;
		return 0;
	}
	else
	{
#if VERBOSE
		printf("Storing partial samples outside\n");
#endif		
		store_samples((float *) input_items[1], ninput_items[1], 0x1);
		num_future_items1 = num_oldinput[1] - d_D*d_K_total;
		consume(1, ninput_items[1]);
	}							

    //return 0;
  }

  /* At this point we have exact number of old samples which is required for decoding purpose */
  assert (input_items.size() == 2*output_items.size());
  int nstreams = output_items.size();

  //assert (noutput_items % (d_K*d_multiple) == 0);
  //assert (num_oldinput[1] % (d_D*d_K_total) == 0);
  
  int noutput_items_now = d_K*d_multiple; //one pkt at a time, so we can trash junk at end correctly

  int temp_out = 0;
  int temp_d_K = std::min(4080, noutput_items_now)/d_multiple;

#if VERBOSE
  printf("trellis_siso_combined_packet(0x%02d): noutput_items=%d, \
ninput_items[0]=%d, ninput_items[1]=%d, streams=%d, d_multiple=%d, \
d_D=%d, d_K=%d, noutput_items_now=%d\n", d_tag, noutput_items, ninput_items[0], 
    ninput_items[1], nstreams, d_multiple, d_D, d_K, noutput_items_now);
#endif


  float (*p2min)(float, float) = NULL; 
  if(d_SISO_TYPE == TRELLIS_MIN_SUM)
    p2min = &min;
  else if(d_SISO_TYPE == TRELLIS_SUM_PRODUCT)
    p2min = &min_star;
  
  
  float out[noutput_items_now];
  for (int m=0;m<nstreams;m++) {
    const float *in1 = (const float *) &old_input[0][0]; //input_items[2*m];
    const float *in2 = (const float *) &old_input[1][0]; //input_items[2*m+1];
	//for(int i=0; i<d_D*d_K_total; i++)
		//printf("%f, %f\n", old_input[0][i], old_input[1][i]);
	//float *out = (float *) output_items[m];
	
    int n = 0; //packet offset in stream, one pkt for now.
	while(temp_out < noutput_items_now)
	{
		
    	siso_algorithm_combined(d_tag, d_FSM.I(),d_FSM.S(),d_FSM.O(),
			    d_FSM.NS(),d_FSM.OS(),d_FSM.PS(),d_FSM.PI(),
			    temp_d_K,d_S0,d_SK,
			    d_POSTI,d_POSTO,
			    p2min,
			    d_D,d_TABLE,d_TYPE,
			    &(in1[n*d_K*d_FSM.I()]), &(in2[n*d_K*d_D]), // a priori, observations
			    &(out[n*d_K*d_multiple]) + temp_out                      // a posteriori
			    );
		temp_out += 4080;
		temp_d_K = std::min(4080, noutput_items_now-temp_out)/d_multiple;
    }
  }
  
  gr_message_sptr packet_out_ptr = 
    gr_make_message(0, 0, 0, noutput_items_now*sizeof(float));
  float * out_pkt = (float *) packet_out_ptr->msg();
  memcpy(out_pkt, &out[0], noutput_items_now*sizeof(float));
  //for(int ptr=0; ptr < noutput_items_now; ptr++)
	//	printf("%f\n", out_pkt[ptr]);
    
  d_body_siso_msgq->insert_tail(packet_out_ptr);
  packet_out_ptr.reset();
  /*int consume_in0 = d_FSM.I() * d_K;
  int consume_in1 = d_D * d_K_total;
  
  for (unsigned int i = 0; i < input_items.size()/2; i++) {
    consume(2*i, consume_in0);
    consume(2*i+1,consume_in1 );
  }*/
  

  memcpy(&old_input[1][0], &old_input[1][0] + d_D*d_K_total, (num_oldinput[1]-d_D*d_K_total)*sizeof(float));
  num_oldinput[1] = num_oldinput[1]-d_D*d_K_total;
  num_future_items1 = 0;
  //num_oldinput[0] = num_oldinput[1] = 0;
  
#if VERBOSE
  printf("trellis_siso_combined_packet(0x%02d): output_now %d items, output %d items\n", 
         d_tag, noutput_items_now, noutput_items);
#endif
  d_input_ready = false;
  set_output_multiple(1);
  return noutput_items;
}
