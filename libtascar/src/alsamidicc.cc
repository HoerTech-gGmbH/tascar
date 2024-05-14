/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include "alsamidicc.h"
#include "errorhandling.h"
#include "tscconfig.h"

TASCAR::midi_ctl_t::midi_ctl_t(const std::string& cname) : seq(NULL)
{
  if(snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK) < 0)
    throw TASCAR::ErrMsg("Unable to open MIDI sequencer.");
  snd_seq_set_client_name(seq, cname.c_str());
  snd_seq_drop_input(seq);
  snd_seq_drop_input_buffer(seq);
  snd_seq_drop_output(seq);
  snd_seq_drop_output_buffer(seq);
  port_in.port = snd_seq_create_simple_port(
      seq, "control", SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
      SND_SEQ_PORT_TYPE_APPLICATION);
  port_out.port = snd_seq_create_simple_port(
      seq, "feedback", SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
      SND_SEQ_PORT_TYPE_APPLICATION);
  port_in.client = snd_seq_client_id(seq);
  port_out.client = snd_seq_client_id(seq);
  // todo: error handling!
}

void TASCAR::midi_ctl_t::connect_input(int src_client, int src_port)
{
  snd_seq_addr_t src_port_;
  src_port_.client = src_client;
  src_port_.port = src_port;
  snd_seq_port_subscribe_t* subs;
  snd_seq_port_subscribe_alloca(&subs);
  snd_seq_port_subscribe_set_sender(subs, &src_port_);
  snd_seq_port_subscribe_set_dest(subs, &port_in);
  snd_seq_port_subscribe_set_queue(subs, 1);
  snd_seq_port_subscribe_set_time_update(subs, 1);
  snd_seq_port_subscribe_set_time_real(subs, 1);
  snd_seq_subscribe_port(seq, subs);
}

void TASCAR::midi_ctl_t::connect_output(int src_client, int src_port)
{
  snd_seq_addr_t src_port_;
  src_port_.client = src_client;
  src_port_.port = src_port;
  snd_seq_port_subscribe_t* subs;
  snd_seq_port_subscribe_alloca(&subs);
  snd_seq_port_subscribe_set_sender(subs, &port_out);
  snd_seq_port_subscribe_set_dest(subs, &src_port_);
  snd_seq_port_subscribe_set_queue(subs, 1);
  snd_seq_port_subscribe_set_time_update(subs, 1);
  snd_seq_port_subscribe_set_time_real(subs, 1);
  snd_seq_subscribe_port(seq, subs);
}

void TASCAR::midi_ctl_t::service()
{
  snd_seq_drop_input(seq);
  snd_seq_drop_input_buffer(seq);
  snd_seq_drop_output(seq);
  snd_seq_drop_output_buffer(seq);
  snd_seq_event_t* ev = NULL;
  while(run_service) {
    // while( snd_seq_event_input_pending(seq,0) ){
    while(snd_seq_event_input(seq, &ev) >= 0) {
      if(ev && (ev->type == SND_SEQ_EVENT_CONTROLLER)) {
        emit_event(ev->data.control.channel, ev->data.control.param,
                   ev->data.control.value);
      }
      if(ev && ((ev->type == SND_SEQ_EVENT_NOTE) ||
                (ev->type == SND_SEQ_EVENT_NOTEON) ||
                (ev->type == SND_SEQ_EVENT_NOTEOFF))) {
        emit_event_note(ev->data.note.channel, ev->data.note.note,
                        ev->data.note.velocity);
      }
    }
    usleep(10);
  }
}

TASCAR::midi_ctl_t::~midi_ctl_t()
{
  snd_seq_close(seq);
}

void TASCAR::midi_ctl_t::send_midi(int channel, int param, int value)
{
  snd_seq_event_t ev;
  memset(&ev, 0, sizeof(ev));
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, port_out.port);
  snd_seq_ev_set_subs(&ev);
  snd_seq_ev_set_direct(&ev);
  ev.type = SND_SEQ_EVENT_CONTROLLER;
  ev.data.control.channel = (unsigned char)(channel);
  ev.data.control.param = (unsigned char)(param);
  ev.data.control.value = (unsigned char)(value);
  snd_seq_event_output_direct(seq, &ev);
  snd_seq_drain_output(seq);
  snd_seq_sync_output_queue(seq);
}

void TASCAR::midi_ctl_t::send_midi_note(int channel, int param, int value)
{
  snd_seq_event_t ev;
  memset(&ev, 0, sizeof(ev));
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, port_out.port);
  snd_seq_ev_set_subs(&ev);
  snd_seq_ev_set_direct(&ev);
  ev.type = SND_SEQ_EVENT_NOTEON;
  ev.data.note.channel = (unsigned char)(channel);
  ev.data.note.note = (unsigned char)(param);
  ev.data.note.velocity = (unsigned char)(value);
  snd_seq_event_output_direct(seq, &ev);
  snd_seq_drain_output(seq);
  snd_seq_sync_output_queue(seq);
}

void TASCAR::midi_ctl_t::connect_input(const std::string& src,
                                       bool warn_on_fail)
{
  snd_seq_addr_t sender;
  memset(&sender, 0, sizeof(sender));
  if(snd_seq_parse_address(seq, &sender, src.c_str()) == 0)
    connect_input(sender.client, sender.port);
  else {
    if(warn_on_fail)
      TASCAR::add_warning("Invalid MIDI address " + src);
    else
      throw TASCAR::ErrMsg("Invalid MIDI address " + src);
  }
}

void TASCAR::midi_ctl_t::connect_output(const std::string& src,
                                        bool warn_on_fail)
{
  snd_seq_addr_t sender;
  memset(&sender, 0, sizeof(sender));
  if(snd_seq_parse_address(seq, &sender, src.c_str()) == 0)
    connect_output(sender.client, sender.port);
  else {
    if(warn_on_fail)
      TASCAR::add_warning("Invalid MIDI address " + src);
    else
      throw TASCAR::ErrMsg("Invalid MIDI address " + src);
  }
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
