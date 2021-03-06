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

GR_SWIG_BLOCK_MAGIC(trellis,siso_combined_packet);

trellis_siso_combined_packet_sptr trellis_make_siso_combined_packet (
    unsigned char tag,
    const fsm &FSM,
    int S0,
    int SK,
    bool POSTI,
    bool POSTO,
    trellis_siso_type_t SISO_TYPE,
    int D,
    const std::vector<float> &TABLE,
    trellis_metric_type_t TYPE);


class trellis_siso_combined_packet : public gr_block
{
private:
  trellis_siso_combined_packet (
    unsigned char tag,
    const fsm &FSM,
    int S0,
    int SK,
    bool POSTI,
    bool POSTO,
    trellis_siso_type_t SISO_TYPE,
    int D,
    const std::vector<float> &TABLE,
    trellis_metric_type_t TYPE);

public:
    fsm FSM () const { return d_FSM; }
    int K () const { return d_K; }
    int S0 () const { return d_S0; }
    int SK () const { return d_SK; }
    bool POSTI () const { return d_POSTI; }
    bool POSTO () const { return d_POSTO; }
    trellis_siso_type_t SISO_TYPE () const { return d_SISO_TYPE; }
    int D () const { return d_D; }
    std::vector<float> TABLE () const { return d_TABLE; }
    gr_msg_queue_sptr sisoctl_msgq() const { return d_sisoctl_msgq; }
    trellis_metric_type_t TYPE () const { return d_TYPE; }
    gr_msg_queue_sptr	body_siso_msgq() const { return d_body_siso_msgq; }

    
};
