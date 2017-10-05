#include "sourcemod.h"
#include "errorhandling.h"
#include <dlfcn.h>

using namespace TASCAR;

TASCAR_RESOLVER( sourcemod_base_t, xmlpp::Element* );

sourcemod_t::sourcemod_t( xmlpp::Element* cfg )
  : sourcemod_base_t( cfg ),
    lib( NULL ),
    libdata( NULL )
{
  get_attribute( "type", sourcetype );
  if( sourcetype.empty() )
    sourcetype = "omni";
  std::string libname( "tascarsource_" );
  libname += sourcetype + ".so";
  lib = dlopen(libname.c_str(), RTLD_NOW );
  if( !lib )
    throw TASCAR::ErrMsg("Unable to open source module \""+sourcetype+"\": "+dlerror());
  try{
    sourcemod_base_t_resolver( &libdata, cfg, lib, libname );
  }
  catch( ... ){
    dlclose(lib);
    throw;
  }
}

void sourcemod_t::write_xml()
{
  set_attribute("type",sourcetype);
  libdata->write_xml();
}

bool sourcemod_t::read_source(pos_t& prel, const std::vector<wave_t>& input, wave_t& output, sourcemod_base_t::data_t* data)
{
  return libdata->read_source( prel, input, output, data );
}

uint32_t sourcemod_t::get_num_channels()
{
  return libdata->get_num_channels();
}

std::string sourcemod_t::get_channel_postfix(uint32_t channel)
{
  return libdata->get_channel_postfix(channel);
}

std::vector<std::string> sourcemod_t::get_connections() const
{
  return libdata->get_connections();
}

void sourcemod_t::prepare(double srate,uint32_t fragsize)
{
  sourcemod_base_t::prepare(srate,fragsize);
  libdata->prepare(srate,fragsize);
}

void sourcemod_t::release()
{
  sourcemod_base_t::release();
  libdata->release();
}

sourcemod_base_t::data_t* sourcemod_t::create_data(double srate,uint32_t fragsize)
{
  return libdata->create_data(srate,fragsize);
}

sourcemod_t::~sourcemod_t()
{
  delete libdata;
  dlclose(lib);
}

sourcemod_base_t::sourcemod_base_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc)
{
}

void sourcemod_base_t::write_xml()
{
}

sourcemod_base_t::~sourcemod_base_t()
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

