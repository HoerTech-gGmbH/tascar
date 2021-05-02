/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
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

#include "session.h"
#include "jackclient.h"

#define SQRT12 0.70710678118654757274f

const std::complex<double> i(0.0, 1.0);

class matrix_vars_t : public TASCAR::module_base_t {
public:
  matrix_vars_t( const TASCAR::module_cfg_t& cfg );
  ~matrix_vars_t();
protected:
  std::string id;
  std::string decoder;
  bool own_outputs;
  std::string outname;
};

class matrix_t : public matrix_vars_t, public jackc_t {
public:
  matrix_t( const TASCAR::module_cfg_t& cfg );
  ~matrix_t();
  void release();
  virtual int process(jack_nframes_t, const std::vector<float*>&, const std::vector<float*>&);
  void configure( );
private:
  TASCAR::spk_array_t outputs;
  TASCAR::spk_array_t inputs;
  std::vector<std::vector<float> > m;
};

matrix_vars_t::matrix_vars_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    own_outputs(true),
    outname("output")
{
  GET_ATTRIBUTE_(id);
  GET_ATTRIBUTE_(decoder);
  if( has_attribute("layout") ){
    own_outputs = false;
    outname = "speaker";
  }
}

matrix_t::matrix_t( const TASCAR::module_cfg_t& cfg )
  : matrix_vars_t( cfg ),
    jackc_t(id),
    outputs( cfg.xmlsrc, own_outputs, outname ),
    inputs( cfg.xmlsrc, true, "input" )
{
  m.resize( outputs.size() );
  for(uint32_t k=0;k<outputs.size();++k)
    outputs[k].get_attribute( "m", m[k], "", "undocumented" );
  if( decoder == "maxre2d" ){
    uint32_t amborder((inputs.size()-1)/2);
    uint32_t maxc(2*amborder+1);
    for(uint32_t k=0;k<outputs.size();++k){
      m[k].resize(maxc);
      double ordergain(sqrt(0.5));
      double channelgain(1.0/maxc);
      m[k][0] = channelgain*ordergain;
        for(uint32_t o=1;o<amborder+1;++o){
          ordergain = cos(o*M_PI/(2.0*amborder+2.0));
          std::complex<double> cw(std::pow(std::exp(-i*outputs[k].azim()),o));
          // ACN!
          m[k][2*o] = channelgain*ordergain*cw.real();
          m[k][2*o-1] = channelgain*ordergain*cw.imag();
        }
    }
  }
  for(uint32_t k=0;k<inputs.size();++k){
    char ctmp[1024];
    if( inputs[k].label.empty() )
      sprintf(ctmp,"in.%d",k);
    else
      sprintf(ctmp,"%s",inputs[k].label.c_str());
    add_input_port(ctmp);
  }
  for(uint32_t k=0;k<outputs.size();++k){
    char ctmp[1024];
    if( outputs[k].label.empty() )
      sprintf(ctmp,"out.%d",k);
    else
      sprintf(ctmp,"%s",outputs[k].label.c_str());
    add_output_port(ctmp);
  }
  activate();
}

matrix_vars_t::~matrix_vars_t()
{
}

void matrix_t::release()
{
  module_base_t::release();
  inputs.release();
  outputs.release();
  deactivate();
}

matrix_t::~matrix_t()
{
}

void matrix_t::configure( )
{
  module_base_t::configure( );
  chunk_cfg_t cf(cfg());
  outputs.prepare( cf );
  cf = cfg();
  inputs.prepare( cf );
  // connect output ports:
  for(uint32_t kc=0;kc<outputs.connections.size();++kc)
    if( outputs.connections[kc].size() )
      connect_out(kc,outputs.connections[kc],true);
  for(uint32_t kc=0;kc<inputs.connections.size();++kc)
    if( inputs.connections[kc].size() )
      connect_in(kc,inputs.connections[kc],true);
}

int matrix_t::process(jack_nframes_t n, const std::vector<float*>& sIn, const std::vector<float*>& sOut)
{
  for(uint32_t ko=0;ko<sOut.size();++ko){
    memset(sOut[ko],0,sizeof(float)*n);
    for(uint32_t ki=0;ki<std::min(sIn.size(),m[ko].size());++ki)
      if( m[ko][ki] != 0.0f )
        for(uint32_t k=0;k<n;++k)
          sOut[ko][k] += m[ko][ki]*sIn[ki][k];
  }
  if( outputs.delaycomp.size() == outputs.size() ){
    for( uint32_t k=0;k<outputs.size();++k){
      for( uint32_t f=0;f<n;++f ){
        sOut[k][f] = outputs[k].gain*outputs.delaycomp[k]( sOut[k][f] );
      }
    }
  }
  for( uint32_t k=0;k<outputs.size();++k){
    if( outputs[k].comp ){
      TASCAR::wave_t wOut(n,sOut[k]);
      outputs[k].comp->process(wOut,wOut,false);
    }
  }
  return 0;
}

REGISTER_MODULE(matrix_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

