#ifndef TASCARPLUGIN_H

#include "../build/tascarver.h"
#include <string.h>

#define TASCAR_PLUGIN( baseclass, cfgclass, pluginclass ) \
  using namespace TASCAR;\
  extern "C" baseclass* baseclass ## _factory( cfgclass cfg, std::string& emsg ) { try{ return new pluginclass( cfg ); } catch( const std::exception& e ){ emsg = e.what(); return NULL; } }; \
  extern "C" const char* baseclass ## _tascar_version() { return TASCARVER; };

#define TASCAR_RESOLVER( baseclass, cfgclass )    \
  void baseclass ## _resolver( baseclass** instance, cfgclass cfg, void* lib, const std::string& libname ){\
  typedef const char* (*baseclass ## _tascar_version_t)(); \
  baseclass ## _tascar_version_t tscver((baseclass ## _tascar_version_t)dlsym(lib, #baseclass "_tascar_version"));\
  if( !tscver ) throw TASCAR::ErrMsg("Unable to resolve tascar version function\n(module: "+libname+")." );\
  if( strncmp( TASCARVER, tscver(), std::min(strlen(TASCARVER),strlen(tscver())))!= 0) \
      throw TASCAR::ErrMsg("Invalid plugin version "+ std::string(tscver()) + "\n(module: "+libname+", expected version "+std::string(TASCARVER)+")."); \
  typedef baseclass* (*baseclass ## factory_t)( cfgclass, std::string& );      \
  baseclass ## factory_t factory((baseclass ## factory_t)dlsym(lib, #baseclass "_factory"));\
    if(!factory) throw TASCAR::ErrMsg("Unable to resolve factory of " + std::string(#baseclass) + "\n(module: "+libname+")." );\
  std::string emsg;\
  *instance = factory( cfg, emsg );                  \
  if(!(*instance)) throw TASCAR::ErrMsg("Error while loading \""+libname+"\": "+emsg); \
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
