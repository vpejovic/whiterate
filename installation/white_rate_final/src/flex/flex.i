/* -*- c++ -*- */
/*
 * Copyright 2005 Free Software Foundation, Inc.
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

%include "gnuradio.i"			// the common stuff

%{
#include "flex_ofdm_cyclic_prefixer.h"
#include "flex_ofdm_frame_acquisition.h"
#include "flex_ofdm_insert_preamble.h"
#include "flex_ofdm_mapper_bcv.h"
#include "flex_ofdm_sampler.h"
#include "flex_sample_and_hold_ff.h"
#include "flex_peak_detector_fb.h"
#include "flex_framed_message_sink.h"
#include "flex_framed_message_source.h"
#include "flex_ofdm_interleaver.h"
#include "flex_ofdm_deinterleaver.h"
#include "flex_coded_ofdm_demod2softin.h"
#include "flex_coded_ofdm_demod.h"
#include "flex_ofdm_header_decode_vbb.h"
#include "flex_ofdm_struct_dot11a.h"
#include "flex_ofdm_constants.h"
#include "flex_fsm.h"
#include "flex_base.h"
#include "flex_null_mux.h"
%}


// ----------------------------------------------------------------
GR_SWIG_BLOCK_MAGIC(flex,ofdm_cyclic_prefixer)

flex_ofdm_cyclic_prefixer_sptr 
flex_make_ofdm_cyclic_prefixer (size_t input_size, size_t output_size);

class flex_ofdm_cyclic_prefixer : public gr_sync_interpolator
{
 protected:
  flex_ofdm_cyclic_prefixer (size_t input_size, size_t output_size);

 public:
};
// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(flex,ofdm_insert_preamble);

flex_ofdm_insert_preamble_sptr
flex_make_ofdm_insert_preamble(int fft_length,
			     const std::vector<std::vector<gr_complex> > &preamble);


class flex_ofdm_insert_preamble : public gr_block
{
 protected:
  flex_ofdm_insert_preamble(int fft_length,
			  const std::vector<std::vector<gr_complex> > &preamble);
 public: 
  // linklab, reset preamble
  void reset_preamble(std::vector<std::vector<gr_complex> > new_preamble);
};
//----------------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(flex,ofdm_mapper_bcv);

flex_ofdm_mapper_bcv_sptr
flex_make_ofdm_mapper_bcv (const std::vector<gr_complex> &constellation, const std::vector<gr_complex> &header_constellation, unsigned msgq_limit,
                           unsigned occupied_carriers, unsigned int fft_length, std::string tone_map, const std::vector<float> &scale);

class flex_ofdm_mapper_bcv : public gr_sync_block
{
 protected:
  flex_ofdm_mapper_bcv (const std::vector<gr_complex> &constellation, const std::vector<gr_complex> &header_constellation, unsigned msgq_limit,
                      unsigned occupied_carriers, unsigned int fft_length, std::string tone_map, const std::vector<float> &scale);

 public:
  gr_msg_queue_sptr msgq();

  int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

  void initialize();

  void reset_ofdm_params(const std::vector<gr_complex> &new_constellation, std::string new_tone_map);
};

//-----------------------------------------------------------------
GR_SWIG_BLOCK_MAGIC(flex,ofdm_frame_acquisition);

flex_ofdm_frame_acquisition_sptr 
flex_make_ofdm_frame_acquisition (unsigned int occupied_carriers, 
				unsigned int fft_length,
				unsigned int cplen,
				const std::vector<gr_complex> &known_symbol, 
				unsigned int max_fft_shift_len=4);

class flex_ofdm_frame_acquisition : public gr_sync_decimator
{
 protected:
  flex_ofdm_frame_acquisition (unsigned int occupied_carriers,
			     unsigned int fft_length,
			     unsigned int cplen,
			     const std::vector<gr_complex> &known_symbol,
			     unsigned int max_fft_shift_len);

 public:
  float snr() { return d_snr_est; }
  float d_sinr;                  // linklab, SINR estimation
  float d_gain;                  // Veljko, avg channel gain
  //std::vector<float> d_gain_full; // Veljko, full channel gain
  const std::vector<float> & gain_full () const { return d_gain_full; }
  std::string d_ch;              // linklab, channel gain
  int d_coarse_freq;             // linklab, search distance in number of bins
  void reset_known_symbol(std::vector<gr_complex> new_known_symbol); //linklab, reset known symbols
  void initialize(); //linklab, initialization function
  int general_work(int noutput_items,
		   gr_vector_int &ninput_items,
		   gr_vector_const_void_star &input_items,
		   gr_vector_void_star &output_items);
};
//-----------------------------------------------------------------
GR_SWIG_BLOCK_MAGIC(flex,ofdm_sampler)

  flex_ofdm_sampler_sptr flex_make_ofdm_sampler (unsigned int fft_length, 
					     unsigned int symbol_length,
					     unsigned int timeout=1000);

class flex_ofdm_sampler : public gr_sync_block
{
 private:
  flex_ofdm_sampler (unsigned int fft_length,
		   unsigned int symbol_length,
		   unsigned int timeout);
};


//-----------------------------------------------------------------
GR_SWIG_BLOCK_MAGIC(flex,peak_detector_fb)

flex_peak_detector_fb_sptr flex_make_peak_detector_fb (float threshold_factor_rise = 0.25,
				 float threshold_factor_fall = 0.40, 
				 int look_ahead = 10,
				 float alpha=0.001); 

class flex_peak_detector_fb : public gr_sync_block
{
 private:
  flex_peak_detector_fb (float threshold_factor_rise, 
	  float threshold_factor_fall,
	  int look_ahead, float alpha);  // linklab, add carrier_num

 public:
  float d_sinr;      //linklab, sinr estimation
  float d_sig_power; //linklab, signal power estimation
  void set_threshold_factor_rise(float thr) { d_threshold_factor_rise = thr; }
  void set_threshold_factor_fall(float thr) { d_threshold_factor_fall = thr; }
  void set_look_ahead(int look) { d_look_ahead = look; }
  void set_alpha(int alpha) { d_avg_alpha = alpha; }

  float threshold_factor_rise() { return d_threshold_factor_rise; } 
  float threshold_factor_fall() { return d_threshold_factor_fall; }
  int look_ahead() { return d_look_ahead; }
  float alpha() { return d_avg_alpha; }
};
//-----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(flex,framed_message_sink);

flex_framed_message_sink_sptr flex_make_framed_message_sink (size_t itemsize,
					   gr_msg_queue_sptr msgq,
					   bool dont_block);

class flex_framed_message_sink : public gr_sync_block
{
 protected:
  flex_framed_message_sink (size_t itemsize, gr_msg_queue_sptr msgq, bool dont_block);

 public:
  ~flex_framed_message_sink ();
};

//-----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(flex,framed_message_source);

flex_framed_message_source_sptr flex_make_framed_message_source (size_t itemsize, int msgq_limit=0);

class flex_framed_message_source : public gr_sync_block
{
 protected:
  flex_framed_message_source (size_t itemsize, int msgq_limit);

 public:
  ~flex_framed_message_source ();

  gr_msg_queue_sptr msgq() const;
};

//-----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(flex,ofdm_interleaver);

flex_ofdm_interleaver_sptr 
flex_make_ofdm_interleaver(int occ_tones, int bits_per_sym, int skip_bits);

class flex_ofdm_interleaver : public gr_block
{
protected:
  flex_ofdm_interleaver(int occ_tones, int bits_per_sym, int skip_bits);
  
public:
  ~flex_ofdm_interleaver();
};

//-----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(flex,coded_ofdm_demod2softin);

flex_coded_ofdm_demod2softin_sptr 
flex_make_coded_ofdm_demod2softin(int tag);

class flex_coded_ofdm_demod2softin : public gr_block
{
protected:
  flex_coded_ofdm_demod2softin(int tag);
  
public:
  ~flex_coded_ofdm_demod2softin();
};

//-----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(flex,coded_ofdm_demod);

flex_coded_ofdm_demod_sptr 
flex_make_coded_ofdm_demod(gr_msg_queue_sptr             body_sisoctl_msgq,
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

class flex_coded_ofdm_demod : public gr_block
{
protected:
  flex_coded_ofdm_demod(gr_msg_queue_sptr             body_sisoctl_msgq,
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
  
public:
  gr_msg_queue_sptr	hdrctl_msgq() const;
  gr_msg_queue_sptr	good_hdrctl_msgq() const;
  void reset_ofdm_params(std::string new_tone_map);
  ~flex_coded_ofdm_demod();
};
//-----------------------------------------------------------------
GR_SWIG_BLOCK_MAGIC(flex,ofdm_header_decode_vbb);

flex_ofdm_header_decode_vbb_sptr 
flex_make_ofdm_header_decode_vbb(gr_msg_queue_sptr headerctl_msgq,
			       int _footerlen);

class flex_ofdm_header_decode_vbb : public gr_sync_block
{
 protected:
  flex_ofdm_header_decode_vbb(gr_msg_queue_sptr headerctl_msgq,
			    int _footerlen);

 public:
  ~flex_ofdm_header_decode_vbb();
};
//-----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(flex,ofdm_deinterleaver);

flex_ofdm_deinterleaver_sptr 
flex_make_ofdm_deinterleaver(int occ_tones, int itemsize);

class flex_ofdm_deinterleaver : public gr_block
{
protected:
  flex_ofdm_deinterleaver(int occ_tones, int itemsize);
  
public:
  ~flex_ofdm_deinterleaver();
};
//-----------------------------------------------------------------
GR_SWIG_BLOCK_MAGIC(flex,sample_and_hold_ff)

flex_sample_and_hold_ff_sptr flex_make_sample_and_hold_ff ();

class flex_sample_and_hold_ff : public gr_sync_block
{
 private:
  flex_sample_and_hold_ff ();
 public:
  float d_data; //linklab
};

//-----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(flex,null_mux)

flex_null_mux_sptr flex_make_null_mux(size_t itemsize, float ampl);

class flex_null_mux : public gr_block
{
 private:
  flex_null_mux(size_t itemsize, float ampl);

};

//-----------------------------------------------------------------


class flex_fsm {
private:
  int d_I;
  int d_S;
  int d_O;
  std::vector<int> d_NS;
  std::vector<int> d_OS;
  std::vector< std::vector<int> > d_PS;
  std::vector< std::vector<int> > d_PI;
  std::vector<int> d_TMi;
  std::vector<int> d_TMl;
  void generate_PS_PI ();
  void generate_TM ();
public:
  flex_fsm();
  flex_fsm(const flex_fsm &FSM);
  flex_fsm(int I, int S, int O, const std::vector<int> &NS, const std::vector<int> &OS);
  flex_fsm(const char *name);
  flex_fsm(int k, int n, const std::vector<int> &G);
  flex_fsm(int mod_size, int ch_length);
  int I () const { return d_I; }
  int S () const { return d_S; }
  int O () const { return d_O; }
  const std::vector<int> & NS () const { return d_NS; }
  const std::vector<int> & OS () const { return d_OS; }
  // disable these accessors until we find out how to swig them
  //const std::vector< std::vector<int> > & PS () const { return d_PS; }
  //const std::vector< std::vector<int> > & PI () const { return d_PI; }
  const std::vector<int> & TMi () const { return d_TMi; }
  const std::vector<int> & TMl () const { return d_TMl; }
  void flex_fsm::write_trellis_svg(std::string filename ,int number_stages);
  void flex_fsm::write_fsm_txt(std::string filename);
};

%include <flex_ofdm_constants.h>

