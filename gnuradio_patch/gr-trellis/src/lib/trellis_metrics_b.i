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

// WARNING: this file is machine generated.  Edits will be over written

GR_SWIG_BLOCK_MAGIC(trellis,metrics_b);

trellis_metrics_b_sptr trellis_make_metrics_b (int O, int D, const std::vector<unsigned char> &TABLE, trellis_metric_type_t TYPE);

class trellis_metrics_b : public gr_block
{
private:
  trellis_metrics_b (int O, int D, const std::vector<unsigned char> &TABLE, trellis_metric_type_t TYPE);

public:
  int O () const { return d_O; }
  int D () const { return d_D; }
  trellis_metric_type_t TYPE () const { return d_TYPE; }
  void set_TABLE (const std::vector<unsigned char> &table);
  std::vector<unsigned char> TABLE () const { return d_TABLE; }
};
