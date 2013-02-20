#
# Copyright 2004,2005,2006,2008,2009 Free Software Foundation, Inc.
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

include $(top_srcdir)/Makefile.common

# C/C++ headers get installed in ${prefix}/include/gnuradio
grinclude_HEADERS =             \
	flex_ofdm_insert_preamble.h      \
	flex_ofdm_mapper_bcv.h             \
	flex_ofdm_frame_acquisition.h \
	flex_ofdm_cyclic_prefixer.h	\
	flex_ofdm_sampler.h	\
	flex_sample_and_hold_ff.h	\
	flex_peak_detector_fb.h		\
	flex_framed_message_sink.h	\
	flex_framed_message_source.h	\
	flex_ofdm_interleaver.h		\
	flex_coded_ofdm_demod2softin.h	\
	flex_coded_ofdm_demod.h	\
	flex_ofdm_header_decode_vbb.h	\
	flex_ofdm_deinterleaver.h	\
	flex_ofdm_constants.h		\
	flex_ofdm_struct_dot11a.h	\
	flex_fsm.h		\
	flex_null_mux.h		\
	flex_base.h

###################################
# SWIG Python interface and library

TOP_SWIG_IFILES = 		\
	flex.i

# Install so that they end up available as:
#   import gnuradio.wr
# This ends up at:
#   ${prefix}/lib/python${python_version}/site-packages/gnuradio
flex_pythondir_category =	\
	gnuradio

# additional arguments to the SWIG command
#wr_la_swig_args =		\
#	-g

# additional sources for the SWIG-generated library
flex_la_swig_sources =              \
	flex_ofdm_insert_preamble.cc      \
	flex_ofdm_mapper_bcv.cc             \
	flex_ofdm_frame_acquisition.cc     \
	flex_ofdm_cyclic_prefixer.cc	\
	flex_ofdm_sampler.cc	\
	flex_sample_and_hold_ff.cc	\
	flex_peak_detector_fb.cc	\
	flex_framed_message_sink.cc	\
	flex_framed_message_source.cc	\
	flex_ofdm_interleaver.cc	\
	flex_ofdm_deinterleaver.cc	\
	flex_coded_ofdm_demod2softin.cc	\
	flex_coded_ofdm_demod.cc	\
	flex_ofdm_header_decode_vbb.cc	\
	flex_fsm.cc		\
	flex_null_mux.cc		\
	flex_base.cc

# additional arguments to the SWIG command
#wr_la_swig_args =		\
#	$(SWIG_PYTHON_ARGS)

# additional libraries for linking with the SWIG-generated library
#wr_la_swig_libadd =		\
#	libm.dylib

# additional LD flags for linking the SWIG-generated library
#wr_la_swig_ldflags =	\
#	-L/opt/local/bin

# additional Python files to be installed along with the SWIG-generated one
#wr_python =			\
#	__init__.py

# additional SWIG files to be installed
#wr_swiginclude_headers =	\
#	wr.i

include $(top_srcdir)/Makefile.swig

# add some of the variables generated inside the Makefile.swig.gen
BUILT_SOURCES = $(swig_built_sources)

# Do not distribute the output of SWIG
no_dist_files = $(swig_built_sources)
