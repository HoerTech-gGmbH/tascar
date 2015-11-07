#include "receivermod.h"
#include "errorhandling.h"
#include <dlfcn.h>

static void receivermod_error(std::string errmsg)
{
  throw TASCAR::ErrMsg("Receiver module error: "+errmsg);
}

#define RESOLVE(x) x ## _cb = (receivermod_ ## x ## _t)dlsym(lib,"receivermod_" #x);if(!x ## _cb) throw TASCAR::ErrMsg("Unable to resolve \"receivermod_" #x "\" in receiver module \""+receivertype+"\".")

TASCAR::receivermod_t::receivermod_t(xmlpp::Element* xmlsrc)
  : receivermod_base_t(xmlsrc),
    lib(NULL),
    libdata(NULL),
    create_cb(NULL),
    destroy_cb(NULL),
    write_xml_cb(NULL),
    add_pointsource_cb(NULL),
    add_diffusesource_cb(NULL),
    postproc_cb(NULL),
    get_num_channels_cb(NULL),
    get_channel_postfix_cb(NULL),
    get_connections_cb(NULL),
    configure_cb(NULL),
    create_data_cb(NULL)
{
  get_attribute("type",receivertype);
  std::string libname("tascarreceiver_");
  libname += receivertype + ".so";
  lib = dlopen(libname.c_str(), RTLD_NOW );
  if( !lib )
    throw TASCAR::ErrMsg("Unable to open receiver module \""+receivertype+"\": "+dlerror());
  try{
    RESOLVE(create);
    RESOLVE(destroy);
    RESOLVE(write_xml);
    RESOLVE(add_pointsource);
    RESOLVE(add_diffusesource);
    RESOLVE(postproc);
    RESOLVE(get_num_channels);
    RESOLVE(get_channel_postfix);
    RESOLVE(get_connections);
    RESOLVE(configure);
    RESOLVE(create_data);
    libdata = create_cb(xmlsrc,receivermod_error);
  }
  catch( ... ){
    dlclose(lib);
    throw;
  }
}

void TASCAR::receivermod_t::write_xml()
{
  set_attribute("type",receivertype);
  if( write_xml_cb )
    write_xml_cb(libdata,receivermod_error);
}

void TASCAR::receivermod_t::add_pointsource(const pos_t& prel, const wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t* data)
{
  add_pointsource_cb(libdata,prel,chunk,output,data,receivermod_error);
}

void TASCAR::receivermod_t::add_diffusesource(const amb1wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t* data)
{
  add_diffusesource_cb(libdata,chunk,output,data,receivermod_error);
}

void TASCAR::receivermod_t::postproc(std::vector<wave_t>& output)
{
  postproc_cb(libdata,output,receivermod_error);
}

uint32_t TASCAR::receivermod_t::get_num_channels()
{
  return get_num_channels_cb(libdata,receivermod_error);
}

std::string TASCAR::receivermod_t::get_channel_postfix(uint32_t channel)
{
  return get_channel_postfix_cb(libdata,channel,receivermod_error);
}

std::vector<std::string> TASCAR::receivermod_t::get_connections() const
{
  //DEBUG(1);
  return get_connections_cb(libdata,receivermod_error);
}

void TASCAR::receivermod_t::configure(double srate,uint32_t fragsize)
{
  configure_cb(libdata,srate,fragsize,receivermod_error);
}

TASCAR::receivermod_base_t::data_t* TASCAR::receivermod_t::create_data(double srate,uint32_t fragsize)
{
  return create_data_cb(libdata,srate,fragsize,receivermod_error);
}

TASCAR::receivermod_t::~receivermod_t()
{
  destroy_cb(libdata,receivermod_error);
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
  //DEBUG(1);
  return spkpos.connections;
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
