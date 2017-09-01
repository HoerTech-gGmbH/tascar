#include "receivermod.h"
#include "errorhandling.h"
#include <dlfcn.h>

using namespace TASCAR;

TASCAR_RESOLVER( receivermod_base_t, xmlpp::Element* );

TASCAR::receivermod_t::receivermod_t(xmlpp::Element* cfg)
  : receivermod_base_t(cfg),
    lib(NULL),
    libdata(NULL)
{
  get_attribute("type",receivertype);
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

void TASCAR::receivermod_t::write_xml()
{
  set_attribute("type",receivertype);
  libdata->write_xml();
}

void TASCAR::receivermod_t::add_pointsource(const pos_t& prel, double width, const wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t* data)
{
  libdata->add_pointsource(prel,width, chunk,output,data);
}

void TASCAR::receivermod_t::add_diffusesource(const amb1wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t* data)
{
  libdata->add_diffusesource(chunk,output,data);
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

void TASCAR::receivermod_t::configure(double srate,uint32_t fragsize)
{
  libdata->configure(srate,fragsize);
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

void TASCAR::receivermod_base_t::write_xml()
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

void TASCAR::receivermod_base_speaker_t::write_xml()
{
  receivermod_base_t::write_xml();
  spkpos.write_xml();
}

std::vector<std::string> TASCAR::receivermod_base_speaker_t::get_connections() const
{
  return spkpos.connections;
}

void TASCAR::receivermod_base_speaker_t::postproc(std::vector<wave_t>& output)
{
  if( spkpos.delaycomp.size() == spkpos.size() ){
    for( uint32_t k=0;k<spkpos.size();++k){
      for( uint32_t f=0;f<output[k].n;++f ){
        output[k].d[f] = spkpos[k].gain*spkpos.delaycomp[k]( output[k].d[f] );
      }
    }
  }
  for( uint32_t k=0;k<spkpos.size();++k){
    if( spkpos[k].comp )
      spkpos[k].comp->filter(&(output[k]),&(output[k]));
  }
}

void TASCAR::receivermod_base_speaker_t::configure(double srate,uint32_t fragsize)
{
  receivermod_base_t::configure(srate,fragsize);
  spkpos.configure(srate,fragsize);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
