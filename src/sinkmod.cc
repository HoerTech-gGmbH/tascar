#include "sinkmod.h"
#include "errorhandling.h"
#include <dlfcn.h>

static void sinkmod_error(std::string errmsg)
{
  throw TASCAR::ErrMsg("Sink module error: "+errmsg);
}

#define RESOLVE(x) x ## _cb = (sinkmod_ ## x ## _t)dlsym(lib,"sinkmod_" #x);if(!x ## _cb) throw TASCAR::ErrMsg("Unable to resolve \"sinkmod_" #x "\" in sink module \""+sinktype+"\".")

TASCAR::sinkmod_t::sinkmod_t(xmlpp::Element* xmlsrc)
  : sinkmod_base_t(xmlsrc),
    lib(NULL),
    libdata(NULL),
    create_cb(NULL),
    destroy_cb(NULL),
    write_xml_cb(NULL)
{
  get_attribute("type",sinktype);
  std::string libname("tascarsink_");
  libname += sinktype + ".so";
  lib = dlopen(libname.c_str(), RTLD_NOW );
  if( !lib )
    throw TASCAR::ErrMsg("Unable to open sink module \""+sinktype+"\": "+dlerror());
  try{
    RESOLVE(create);
    RESOLVE(destroy);
    RESOLVE(write_xml);
    RESOLVE(add_pointsource);
    RESOLVE(add_diffusesource);
    RESOLVE(postproc);
    RESOLVE(get_num_channels);
    RESOLVE(get_channel_postfix);
    RESOLVE(create_data);
    libdata = create_cb(xmlsrc,sinkmod_error);
  }
  catch( ... ){
    dlclose(lib);
    throw;
  }
}

void TASCAR::sinkmod_t::write_xml()
{
  set_attribute("type",sinktype);
  if( write_xml_cb )
    write_xml_cb(libdata,sinkmod_error);
}

void TASCAR::sinkmod_t::add_pointsource(const pos_t& prel, const wave_t& chunk, std::vector<wave_t>& output, sinkmod_base_t::data_t* data)
{
  add_pointsource_cb(libdata,prel,chunk,output,data,sinkmod_error);
}

void TASCAR::sinkmod_t::add_diffusesource(const pos_t& prel, const amb1wave_t& chunk, std::vector<wave_t>& output, sinkmod_base_t::data_t* data)
{
  add_diffusesource_cb(libdata,prel,chunk,output,data,sinkmod_error);
}

void TASCAR::sinkmod_t::postproc()
{
  postproc_cb(libdata,sinkmod_error);
}

uint32_t TASCAR::sinkmod_t::get_num_channels()
{
  return get_num_channels_cb(libdata,sinkmod_error);
}

std::string TASCAR::sinkmod_t::get_channel_postfix(uint32_t channel)
{
  return get_channel_postfix_cb(libdata,channel,sinkmod_error);
}

TASCAR::sinkmod_base_t::data_t* TASCAR::sinkmod_t::create_data(double srate,uint32_t fragsize)
{
  return create_data_cb(libdata,srate,fragsize,sinkmod_error);
}

TASCAR::sinkmod_t::~sinkmod_t()
{
  destroy_cb(libdata,sinkmod_error);
  dlclose(lib);
}

TASCAR::sinkmod_base_t::sinkmod_base_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc)
{
}

void TASCAR::sinkmod_base_t::write_xml()
{
}

TASCAR::sinkmod_base_t::~sinkmod_base_t()
{
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
