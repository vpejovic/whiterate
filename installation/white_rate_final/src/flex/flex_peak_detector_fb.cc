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

// WARNING: this file is machine generated.  Edits will be over written

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <flex_peak_detector_fb.h>
#include <gr_io_signature.h>
#include <string.h>
#include <cstdio>
flex_peak_detector_fb_sptr
flex_make_peak_detector_fb (float threshold_factor_rise,
		     float threshold_factor_fall,
		     int look_ahead, float alpha)
{
  return flex_peak_detector_fb_sptr (new flex_peak_detector_fb (threshold_factor_rise, 
				  threshold_factor_fall,
				  look_ahead, alpha));
}

flex_peak_detector_fb::flex_peak_detector_fb (float threshold_factor_rise, 
		float threshold_factor_fall,
		int look_ahead, float alpha)
  : gr_sync_block ("peak_detector_fb",
		   gr_make_io_signature2 (2, 2, sizeof (float), sizeof (float)),
		   //gr_make_io_signature (1, 1, sizeof (float)),	//TODO: why is this different than XX file?
		   gr_make_io_signature (1, 1, sizeof (char))),
    d_threshold_factor_rise(threshold_factor_rise), 
    d_threshold_factor_fall(threshold_factor_fall),
    d_look_ahead(look_ahead), d_avg_alpha(alpha),
    d_avg(-1), // linklab
    d_sinr(0), // linklab
    d_first(true)
{
}

int
flex_peak_detector_fb::work (int noutput_items,
	      gr_vector_const_void_star &input_items,
	      gr_vector_void_star &output_items)
{
  float *iptr = (float *) input_items[0];
  float *ippw = (float *) input_items[1];// linklab, input sig power
  char *optr = (char *) output_items[0];

  memset(optr, 0, noutput_items*sizeof(char));

  float peak_val = -(float)INFINITY;
  int peak_ind = 0;
  unsigned char state = 0;
  int i = 0;

  //printf("noutput_items %d\n",noutput_items);
  while(i < noutput_items) {
    if (iptr[i]!=iptr[i]){
	//This is nan, so we set it to -1 to prevent the bug
	iptr[i] = -1;
    }	
    d_sig_power = sqrt(ippw[i]); // TODO: how can we use this? linklab, get signal power
    if(state == 0) {  // below threshold
      if (iptr[i] > -1*d_threshold_factor_rise) {
	//printf("Candidate: %d, value %f, avg: %f\n", i, iptr[i], d_avg);	
      }	
      if(iptr[i] > d_avg*d_threshold_factor_rise){ //&& d_sig_power>0.0000001) {
	//printf("Index %d\n",i);		
	state = 1;
      }
      else {
	d_avg = (d_avg_alpha)*iptr[i] + (1-d_avg_alpha)*d_avg;
	i++;
      }
    }
    else if(state == 1) {  // above threshold, have not found peak
      //printf("Entered State 1: %f  i: %d  noutput_items: %d\n", iptr[i], i, noutput_items);
      if(iptr[i] > peak_val) {
	peak_val = iptr[i];
	peak_ind = i;
	d_avg = (d_avg_alpha)*iptr[i] + (1-d_avg_alpha)*d_avg;
	i++;
      }
      // linklab, add the leaked peak value near the end and wipe out the false peak value at the start
      // else if (iptr[i] > d_avg*d_threshold_factor_fall) {
	  // Veljko: not only the first index, but up to first 50000
      else if ((iptr[i] > d_avg*d_threshold_factor_fall) && (i < noutput_items-1) || (peak_ind == 0)){
      //else if ((iptr[i] > d_avg*d_threshold_factor_fall) && (i < noutput_items-1) || (peak_ind < 5000)){
        d_avg = (d_avg_alpha)*iptr[i] + (1-d_avg_alpha)*d_avg;
        i++;
      }
      else {
	//Veljko: comment this and erase the first peak: optr[peak_ind] = 1;
	//if (!d_first){ 	  
	optr[peak_ind] = 1;
	//}
	//else {
	//  d_first = false;	
	//}

	// calculate the sinr in time domain, linklab
        if (peak_val < 0)
            d_sinr = 10*log(1/(1/(sqrt(peak_val+1))-1))/log(10);
        else
            d_sinr = (float)INFINITY; 
	state = 0;
	//printf("Leaving  State 1: Peak: %f Avg: %f Peak Ind: %d   i: %d  noutput_items: %d\n", 
	//peak_val, d_avg, peak_ind, i, noutput_items);
	peak_val = -(float)INFINITY;
      }
    }
  }

  if(state == 0) {
    //printf("Leave in State 0, produced %d\n",noutput_items);
    return noutput_items;
  }
  else {   // only return up to passing the threshold
    //printf("Leave in State 1, only produced %d of %d\n",peak_ind,noutput_items);
    // TODO: consume the peak or not?
    return peak_ind+1;
  }
}
