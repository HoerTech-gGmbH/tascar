#ifndef ALSAMIDICC_H
#define ALSAMIDICC_H

#include "serviceclass.h"
#include <alsa/asoundlib.h>
#include <alsa/seq_event.h>
#include <string>

namespace TASCAR {


  /**
     \brief Simple MIDI controller base class
  */
  class midi_ctl_t : public TASCAR::service_t  {
  public:
    /**
       \param cname Client name
    */
    midi_ctl_t(const std::string& cname);
    virtual ~midi_ctl_t();
    /**
       \param client Source client
       \param port Source port
    */
    void connect_input(int client,int port);
    void connect_input( const std::string& name);
    /**
       \param client Destination client
       \param port Destination port
    */
    void connect_output(int client,int port);
    void connect_output( const std::string& name);
    /**
       \brief Send a CC event to output
       \param channel MIDI channel number
       \param param MIDI parameter
       \param value MIDI value
    */
    void send_midi(int channel, int param, int value);
  protected:
    void service();
    /**
       \brief Callback to be called for incoming MIDI events
    */
    virtual void emit_event(int channel, int param, int value) = 0;
  private:
    // MIDI sequencer:
    snd_seq_t* seq;
    // input port:
    snd_seq_addr_t port_in;
    // output/feedback port:
    snd_seq_addr_t port_out;
  };

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
