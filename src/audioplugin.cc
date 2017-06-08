#include "audioplugin.h"
#include "errorhandling.h"
#include <dlfcn.h>

using namespace TASCAR;

transport_t::transport_t()
  : session_time_samples(0), session_time_seconds(0), 
    object_time_samples(0), object_time_seconds(0), rolling(false)
{
}

audioplugin_base_t::audioplugin_base_t( const audioplugin_cfg_t& cfg )
  : xml_element_t(cfg.xmlsrc),
    f_sample(1),
    f_fragment(1),
    t_sample(1),
    t_fragment(1),
    n_fragment(1),
    name(cfg.name),
    modname(cfg.modname),
    prepared(false)
{
}

void audioplugin_base_t::prepare_( double srate, uint32_t fragsize )
{
  release_();
  f_sample = srate;
  f_fragment = srate/(double)fragsize;
  t_sample = 1.0/srate;
  t_fragment = 1.0/f_fragment;
  n_fragment = fragsize;
  prepare(srate,fragsize);
  prepared = true;
}

void audioplugin_base_t::release_()
{
  if( prepared )
    release();
  prepared = false;
}

audioplugin_base_t::~audioplugin_base_t()
{
  release_();
}

TASCAR_RESOLVER( audioplugin_base_t, const audioplugin_cfg_t& )


TASCAR::audioplugin_t::audioplugin_t( const audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    lib(NULL),
    libdata(NULL)
{
  plugintype = e->get_name();
  if( plugintype == "plugin" )
    get_attribute("type",plugintype);
  std::string libname("tascar_ap_");
  libname += plugintype + ".so";
  modname = plugintype;
  audioplugin_cfg_t lcfg(cfg);
  lcfg.modname = modname;
  lib = dlopen(libname.c_str(), RTLD_NOW );
  if( !lib )
    throw TASCAR::ErrMsg("Unable to open module \""+plugintype+"\": "+dlerror());
  try{
    audioplugin_base_t_resolver( &libdata, lcfg, lib, libname );
  }
  catch( ... ){
    dlclose(lib);
    throw;
  }
}

void TASCAR::audioplugin_t::write_xml()
{
  set_attribute("type",plugintype);
  libdata->write_xml();
}

void TASCAR::audioplugin_t::ap_process( wave_t& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp )
{
  libdata->ap_process( chunk, pos, tp );
}

void TASCAR::audioplugin_t::prepare(double srate,uint32_t fragsize)
{
  libdata->prepare_( srate, fragsize );
}

void TASCAR::audioplugin_t::release()
{
  libdata->release_();
}

void TASCAR::audioplugin_t::add_variables(TASCAR::osc_server_t* srv)
{
  libdata->add_variables( srv );
}

TASCAR::audioplugin_t::~audioplugin_t()
{
  release();
  delete libdata;
  dlclose(lib);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
