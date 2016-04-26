#include "audioplugin.h"
#include "errorhandling.h"
#include <dlfcn.h>

using namespace TASCAR;

audioplugin_base_t::audioplugin_base_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname)
  : xml_element_t(xmlsrc),
    f_sample(1),
    f_fragment(1),
    t_sample(1),
    t_fragment(1),
    n_fragment(1),
    prepared(false)
{
}

void audioplugin_base_t::prepare_(double srate,uint32_t fragsize)
{
  if( prepared )
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

static void audioplugin_error(std::string errmsg)
{
  throw TASCAR::ErrMsg("Receiver module error: "+errmsg);
}

#define RESOLVE(x) x ## _cb = (audioplugin_ ## x ## _t)dlsym(lib,"audioplugin_" #x);if(!x ## _cb) throw TASCAR::ErrMsg("Unable to resolve \"audioplugin_" #x "\" in audio plugin \""+plugintype+"\".")

TASCAR::audioplugin_t::audioplugin_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parent_name )
  : audioplugin_base_t(xmlsrc,name,parent_name),
    lib(NULL),
    libdata(NULL),
    create_cb(NULL),
    destroy_cb(NULL),
    write_xml_cb(NULL),
    process_cb(NULL),
    prepare_cb(NULL),
    release_cb(NULL)
{
  get_attribute("type",plugintype);
  std::string libname("tascar_ap_");
  libname += plugintype + ".so";
  lib = dlopen(libname.c_str(), RTLD_NOW );
  if( !lib )
    throw TASCAR::ErrMsg("Unable to open receiver module \""+plugintype+"\": "+dlerror());
  try{
    RESOLVE(create);
    RESOLVE(destroy);
    RESOLVE(write_xml);
    RESOLVE(process);
    RESOLVE(prepare);
    RESOLVE(release);
    libdata = create_cb(xmlsrc,name,parent_name,audioplugin_error);
  }
  catch( ... ){
    dlclose(lib);
    throw;
  }
}

void TASCAR::audioplugin_t::write_xml()
{
  set_attribute("type",plugintype);
  if( write_xml_cb )
    write_xml_cb(libdata,audioplugin_error);
}

void TASCAR::audioplugin_t::process(wave_t& chunk, const TASCAR::pos_t& pos)
{
  process_cb( libdata, chunk, pos, audioplugin_error );
}

void TASCAR::audioplugin_t::prepare(double srate,uint32_t fragsize)
{
  prepare_cb(libdata,srate,fragsize,audioplugin_error);
}

void TASCAR::audioplugin_t::release()
{
  release_cb(libdata,audioplugin_error);
}

TASCAR::audioplugin_t::~audioplugin_t()
{
  destroy_cb(libdata,audioplugin_error);
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
