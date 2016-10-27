#include "alsamidicc.h"
#include "errorhandling.h"

TASCAR::midi_ctl_t::midi_ctl_t(const std::string& cname)
  : seq(NULL)
{
  //DEBUG(1);
  if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK) < 0)
    throw TASCAR::ErrMsg("Unable to open MIDI sequencer.");
  snd_seq_set_client_name(seq, cname.c_str());
  snd_seq_drop_input(seq);
  snd_seq_drop_input_buffer(seq);
  snd_seq_drop_output(seq);
  snd_seq_drop_output_buffer(seq);
  port_in.port = 
    snd_seq_create_simple_port(seq, "control",
			       SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
			       SND_SEQ_PORT_TYPE_APPLICATION);
  port_out.port = 
    snd_seq_create_simple_port(seq, "feedback",
			       SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
			       SND_SEQ_PORT_TYPE_APPLICATION);
  port_in.client = snd_seq_client_id(seq);
  port_out.client = snd_seq_client_id(seq);
  // todo: error handling!
}

void TASCAR::midi_ctl_t::connect_input(int src_client,int src_port)
{
  //DEBUG(src_client);
  //DEBUG(src_port);
  snd_seq_addr_t src_port_;
  src_port_.client = src_client;
  src_port_.port = src_port;
  snd_seq_port_subscribe_t *subs;
  snd_seq_port_subscribe_alloca(&subs);
  snd_seq_port_subscribe_set_sender(subs, &src_port_);
  snd_seq_port_subscribe_set_dest(subs, &port_in);
  snd_seq_port_subscribe_set_queue(subs, 1);
  snd_seq_port_subscribe_set_time_update(subs, 1);
  snd_seq_port_subscribe_set_time_real(subs, 1);
  snd_seq_subscribe_port(seq, subs);
}

void TASCAR::midi_ctl_t::connect_output(int src_client,int src_port)
{
  //DEBUG(src_client);
  //DEBUG(src_port);
  snd_seq_addr_t src_port_;
  src_port_.client = src_client;
  src_port_.port = src_port;
  snd_seq_port_subscribe_t *subs;
  snd_seq_port_subscribe_alloca(&subs);
  snd_seq_port_subscribe_set_sender(subs, &port_out);
  snd_seq_port_subscribe_set_dest(subs, &src_port_);
  snd_seq_port_subscribe_set_queue(subs, 1);
  snd_seq_port_subscribe_set_time_update(subs, 1);
  snd_seq_port_subscribe_set_time_real(subs, 1);
  snd_seq_subscribe_port(seq, subs);
}

void TASCAR::midi_ctl_t::service(){
  snd_seq_drop_input(seq);
  snd_seq_drop_input_buffer(seq);
  snd_seq_drop_output(seq);
  snd_seq_drop_output_buffer(seq);
  //DEBUG(b_run_service);
  snd_seq_event_t *ev;
  while( run_service ){
    //while( snd_seq_event_input_pending(seq,0) ){
    while( snd_seq_event_input(seq, &ev) >= 0 ){
      if( ev->type == SND_SEQ_EVENT_CONTROLLER ){
	//DEBUG(ev->data.control.channel);
	//DEBUG(ev->data.control.param);
	//DEBUG(ev->data.control.value);
	emit_event(ev->data.control.channel,ev->data.control.param, ev->data.control.value);
      }
    }
    usleep(50);
  }
  //DEBUG(b_run_service);
}

TASCAR::midi_ctl_t::~midi_ctl_t()
{
  //DEBUG(b_run_service);
  //DEBUG(seq);
  snd_seq_close(seq);
  //DEBUG(seq);
}

void TASCAR::midi_ctl_t::send_midi(int channel, int param, int value)
{
  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, port_out.port);
  snd_seq_ev_set_subs(&ev);
  snd_seq_ev_set_direct( &ev );
  ev.type = SND_SEQ_EVENT_CONTROLLER;
  ev.data.control.channel = (unsigned char)(channel);
  ev.data.control.param = (unsigned char)(param);
  ev.data.control.value = (unsigned char)(value);
  snd_seq_event_output_direct(seq, &ev);
  snd_seq_drain_output(seq);
  snd_seq_sync_output_queue(seq);
}

void TASCAR::midi_ctl_t::connect_input(const std::string& src)
{
  snd_seq_addr_t sender;
  if( snd_seq_parse_address( seq, &sender, src.c_str()) == 0)
    connect_input(sender.client,sender.port);
  else
    throw TASCAR::ErrMsg("Invalid MIDI address "+src);
}


void TASCAR::midi_ctl_t::connect_output(const std::string& src)
{
  snd_seq_addr_t sender;
  if( snd_seq_parse_address( seq, &sender, src.c_str()) == 0)
    connect_output(sender.client,sender.port);
  else
    throw TASCAR::ErrMsg("Invalid MIDI address "+src);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
