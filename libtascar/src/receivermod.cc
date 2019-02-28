#include "receivermod.h"
#include "errorhandling.h"
#include <dlfcn.h>

using namespace TASCAR;

TASCAR_RESOLVER( receivermod_base_t, xmlpp::Element* );

TASCAR::receivermod_t::receivermod_t(xmlpp::Element* cfg)
  : receivermod_base_t(cfg),
    receivertype("omni"),
    lib(NULL),
    libdata(NULL)
{
  get_attribute("type",receivertype);
  receivertype = env_expand( receivertype );
  std::string libname("tascarreceiver_");
  libname += receivertype + ".so";
  lib = dlopen(libname.c_str(), RTLD_NOW );
  if( !lib )
    throw TASCAR::ErrMsg("Unable to open receiver module \""+receivertype+"\": "+dlerror());
  try{
    receivermod_base_t_resolver( &libdata, cfg, lib, libname );
  }
  catch( ... ){
    dlclose(lib);
    throw;
  }
}

void TASCAR::receivermod_t::add_pointsource(const pos_t& prel, double width, const wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t* data)
{
  libdata->add_pointsource(prel,width, chunk,output,data);
}

void TASCAR::receivermod_t::add_diffuse_sound_field(const amb1wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t* data)
{
  libdata->add_diffuse_sound_field(chunk,output,data);
}

void TASCAR::receivermod_t::postproc(std::vector<wave_t>& output)
{
  libdata->postproc(output);
}

uint32_t TASCAR::receivermod_t::get_num_channels()
{
  return libdata->get_num_channels();
}

std::string TASCAR::receivermod_t::get_channel_postfix(uint32_t channel)
{
  return libdata->get_channel_postfix(channel);
}

std::vector<std::string> TASCAR::receivermod_t::get_connections() const
{
  return libdata->get_connections();
}

void TASCAR::receivermod_t::add_variables( TASCAR::osc_server_t* srv )
{
  return libdata->add_variables( srv );
}

void TASCAR::receivermod_t::prepare( chunk_cfg_t& cf_ )
{
  receivermod_base_t::prepare( cf_ );
  libdata->prepare( cf_ );
}

void TASCAR::receivermod_t::release()
{
  receivermod_base_t::release();
  libdata->release();
}

TASCAR::receivermod_base_t::data_t* TASCAR::receivermod_t::create_data(double srate,uint32_t fragsize)
{
  return libdata->create_data(srate,fragsize);
}

TASCAR::receivermod_t::~receivermod_t()
{
  delete libdata;
  dlclose(lib);
}

TASCAR::receivermod_base_t::receivermod_base_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc)
{
}

TASCAR::receivermod_base_t::~receivermod_base_t()
{
}

TASCAR::receivermod_base_speaker_t::receivermod_base_speaker_t(xmlpp::Element* xmlsrc)
  : receivermod_base_t(xmlsrc),
    spkpos(xmlsrc)
{
}

void TASCAR::receivermod_base_speaker_t::add_variables( TASCAR::osc_server_t* srv )
{
  receivermod_base_t::add_variables(srv);
  srv->add_bool("/decorr",&(spkpos.decorr));
  srv->add_bool("/densitycorr",&(spkpos.densitycorr));
}
  
void TASCAR::receivermod_base_speaker_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  spkpos.add_diffuse_sound_field( chunk );
}

uint32_t TASCAR::receivermod_base_speaker_t::get_num_channels()
{
  return spkpos.size();
}

std::string TASCAR::receivermod_base_speaker_t::get_channel_postfix(uint32_t channel) const
{
  char ctmp[1024];
  sprintf(ctmp,".%d%s",channel,spkpos[channel].label.c_str());
  return ctmp;
}


std::vector<std::string> TASCAR::receivermod_base_speaker_t::get_connections() const
{
  return spkpos.connections;
}

void TASCAR::receivermod_base_speaker_t::postproc( std::vector<wave_t>& output )
{
  spkpos.render_diffuse( output );
  if( spkpos.delaycomp.size() == spkpos.size() ){
    for( uint32_t k=0;k<spkpos.size();++k){
      for( uint32_t f=0;f<output[k].n;++f ){
        output[k].d[f] = spkpos[k].gain*spkpos.delaycomp[k]( output[k].d[f] );
      }
    }
  }
  for( uint32_t k=0;k<spkpos.size();++k){
    if( spkpos[k].comp )
      spkpos[k].comp->process(output[k],output[k],false);
  }
}

void TASCAR::receivermod_base_speaker_t::prepare( chunk_cfg_t& cf_ )
{
  receivermod_base_t::prepare( cf_ );
  spkpos.prepare( cf_ );
}

void TASCAR::receivermod_base_speaker_t::release()
{
  receivermod_base_t::release( );
  spkpos.release( );
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
