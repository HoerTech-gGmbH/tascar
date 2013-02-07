/**
  \file jackclient.cc
  \ingroup libtascar
  \brief "jackclient" is a C++ wrapper for the JACK API.
  \author  Giso Grimm
  \date 2011
  
  \section license License (LGPL)
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; version 2 of the License.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/
#include "jackclient.h"
#include <stdio.h>
#include <iostream>
#include "errorhandling.h"

static std::string errmsg("");


jackc_portless_t::jackc_portless_t(const std::string& clientname)
  : srate(0)
{
  jack_status_t jstat;
  jc = jack_client_open(clientname.c_str(),JackUseExactName,&jstat);
  if( !jc ){
    throw TASCAR::ErrMsg("unable to open jack client");
  }
  srate = jack_get_sample_rate(jc);
  fragsize = jack_get_buffer_size(jc);
  //jack_set_process_callback(jc,process_,this);
}

jackc_t::jackc_t(const std::string& clientname)
  : jackc_portless_t(clientname)
{
  jack_set_process_callback(jc,process_,this);
}

jackc_t::~jackc_t()
{
  for(unsigned int k=0;k<inPort.size();k++)
    jack_port_unregister(jc,inPort[k]);
  for(unsigned int k=0;k<outPort.size();k++)
    jack_port_unregister(jc,outPort[k]);
}

jackc_portless_t::~jackc_portless_t()
{
  jack_client_close(jc);
}

int jackc_t::process_(jack_nframes_t nframes, void *arg)
{
  return ((jackc_t*)(arg))->process_(nframes);
}

int jackc_t::process_(jack_nframes_t nframes)
{
  for(unsigned int k=0;k<inBuffer.size();k++)
    inBuffer[k] = (float*)(jack_port_get_buffer(inPort[k],nframes));
  for(unsigned int k=0;k<outBuffer.size();k++)
    outBuffer[k] = (float*)(jack_port_get_buffer(outPort[k],nframes));
  return process(nframes,inBuffer,outBuffer);
}

void jackc_t::add_input_port(const std::string& name)
{
  jack_port_t* p;
  p = jack_port_register(jc,name.c_str(),JACK_DEFAULT_AUDIO_TYPE,JackPortIsInput,0);
  if( !p )
    throw TASCAR::ErrMsg("unable to register port");
  inPort.push_back(p);
  inBuffer.push_back(NULL);
}

void jackc_t::add_output_port(const std::string& name)
{
  jack_port_t* p;
  p = jack_port_register(jc,name.c_str(),JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput,0);
  if( !p )
    throw TASCAR::ErrMsg("unable to register port");
  outPort.push_back(p);
  outBuffer.push_back(NULL);
}

void jackc_portless_t::activate()
{
  jack_activate(jc);
}

void jackc_portless_t::deactivate()
{
  jack_deactivate(jc);
}

void jackc_t::connect_in(unsigned int port,const std::string& pname)
{
  if( jack_connect(jc,pname.c_str(),jack_port_name(inPort[port])) != 0 ){
    errmsg = std::string("unable to connect port '")+pname + "' to '" + jack_port_name(inPort[port]) + "'.";
    throw TASCAR::ErrMsg(errmsg.c_str());
  }
}

void jackc_t::connect_out(unsigned int port,const std::string& pname)
{
  if( jack_connect(jc,jack_port_name(outPort[port]),pname.c_str()) != 0 ){
    errmsg = std::string("unable to connect port '")+jack_port_name(outPort[port]) + "' to '" + pname + "'.";
    throw TASCAR::ErrMsg(errmsg.c_str());
  }
}

jackc_transport_t::jackc_transport_t(const std::string& clientname)
  : jackc_t(clientname)
{
}

int jackc_transport_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer)
{ 
  jack_position_t pos;
  jack_transport_state_t jstate;
  jstate = jack_transport_query(jc,&pos);
  return process(nframes,inBuffer,outBuffer,pos.frame,jstate == JackTransportRolling);
}

void jackc_portless_t::tp_locate(double p)
{
  jack_transport_locate(jc,p*srate); 
}

void jackc_portless_t::tp_locate(uint32_t p)
{
  jack_transport_locate(jc,p); 
}

void jackc_portless_t::tp_start()
{
  jack_transport_start(jc); 
}

void jackc_portless_t::tp_stop()
{
  jack_transport_stop(jc); 
}


std::string jackc_portless_t::get_client_name()
{
  return jack_get_client_name(jc);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

