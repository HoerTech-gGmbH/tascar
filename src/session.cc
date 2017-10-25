#include "session.h"
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include "errorhandling.h"
#include <dlfcn.h>
#include <fnmatch.h>
#include <locale.h>

using namespace TASCAR;

TASCAR_RESOLVER( module_base_t, const module_cfg_t& )

TASCAR::module_t::module_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t(cfg),
    lib(NULL),
    libdata(NULL),
    is_configured(false)
{
  name = e->get_name();
  if( name == "module" ){
    std::cerr << "Deprecated session file, line " << e->get_line() << ": Use modules within <modules>...</modules> section.\n";
    get_attribute("name",name);
  }
  std::string libname("tascar_");
  libname += name + ".so";
  lib = dlopen(libname.c_str(), RTLD_NOW );
  if( !lib )
    throw TASCAR::ErrMsg("Unable to open module \""+name+"\": "+dlerror());
  try{
    module_base_t_resolver( &libdata, cfg, lib, libname );
  }
  catch( ... ){
    dlclose(lib);
    throw;
  }
}

void TASCAR::module_t::write_xml()
{
  libdata->write_xml();
}

void TASCAR::module_t::update(uint32_t frame,bool running)
{
  if( is_configured)
    libdata->update( frame, running );
}

void TASCAR::module_t::prepare( chunk_cfg_t& cf_ )
{
  module_base_t::prepare( cf_ );
  libdata->prepare( cf_ );
  is_configured = true;
}

void TASCAR::module_t::release()
{
  module_base_t::release();
  is_configured = false;
  libdata->release();
}

TASCAR::module_t::~module_t()
{
  delete libdata;
  dlclose(lib);
}

TASCAR::module_cfg_t::module_cfg_t(xmlpp::Element* xmlsrc_, TASCAR::session_t* session_ )
  : session(session_),xmlsrc(xmlsrc_)
{ 
}

xmlpp::Element* assert_element(xmlpp::Element* e)
{
  if( !e )
    throw TASCAR::ErrMsg("NULL pointer element");
  return e;
}

const std::string& debug_str(const std::string& s)
{
  return s;
}

TASCAR::session_t::session_t()
  : TASCAR::session_oscvars_t(tsc_reader_t::e),
    jackc_transport_t(jacknamer(name,"session.")),
    osc_server_t(srv_addr,srv_port),
    duration(60),
    loop(false),
    levelmeter_tc(2.0),
    levelmeter_weight(TASCAR::levelmeter_t::Z),
    period_time(1.0/(double)srate),
    started_(false)
{
  pthread_mutex_init( &mtx, NULL );
  add_output_port("sync_out");
  jackc_transport_t::activate();
  read_xml();
  add_transport_methods();
  osc_server_t::activate();
}

TASCAR::session_oscvars_t::session_oscvars_t( xmlpp::Element* src )
  : xml_element_t(src)
{
  GET_ATTRIBUTE(srv_port);
  GET_ATTRIBUTE(srv_addr);
  GET_ATTRIBUTE(name);
  if( name.empty() )
    name = "tascar";
  if( srv_port.empty() )
    srv_port = "9877";
  if( srv_port == "none" )
    srv_port = "";
}

TASCAR::session_t::session_t(const std::string& filename_or_data,load_type_t t,const std::string& path)
  : TASCAR::tsc_reader_t(filename_or_data,t,path),
    session_oscvars_t(tsc_reader_t::e),
    jackc_transport_t(jacknamer(name,"session.")),
    osc_server_t(srv_addr,srv_port),
    duration(60),
    loop(false),
    levelmeter_tc(2.0),
    levelmeter_weight(TASCAR::levelmeter_t::Z),
    period_time(1.0/(double)srate),
    started_(false)
{
  pthread_mutex_init( &mtx, NULL );
  add_output_port("sync_out");
  jackc_transport_t::activate();
  // parse XML:
  read_xml();
  add_transport_methods();
  osc_server_t::activate();
}

void TASCAR::session_t::read_xml()
{
  try{
    tsc_reader_t::GET_ATTRIBUTE(duration);
    tsc_reader_t::GET_ATTRIBUTE_BOOL(loop);
    tsc_reader_t::GET_ATTRIBUTE(levelmeter_tc);
    tsc_reader_t::GET_ATTRIBUTE(levelmeter_weight);
    TASCAR::tsc_reader_t::read_xml();
  }
  catch( ... ){
    if( lock_vars() ){
      //for( std::vector<TASCAR::module_t*>::iterator it=modules.begin();it!=modules.end();++it){
      //  DEBUG(*it);
      //  delete (*it);
      //}
      for( std::vector<TASCAR::scene_render_rt_t*>::iterator it=scenes.begin();it!=scenes.end();++it)
        delete (*it);
      for( std::vector<TASCAR::range_t*>::iterator it=ranges.begin();it!=ranges.end();++it)
        delete (*it);
      for( std::vector<TASCAR::connection_t*>::iterator it=connections.begin();it!=connections.end();++it)
        delete (*it);
      unlock_vars();
    }
    throw;
  }
}

void TASCAR::session_t::write_xml()
{
  tsc_reader_t::SET_ATTRIBUTE(duration);
  tsc_reader_t::SET_ATTRIBUTE_BOOL(loop);
  tsc_reader_t::SET_ATTRIBUTE(levelmeter_tc);
  tsc_reader_t::SET_ATTRIBUTE(levelmeter_weight);
  for( std::vector<TASCAR::scene_render_rt_t*>::iterator it=scenes.begin();it!=scenes.end();++it)
    (*it)->write_xml();
  for( std::vector<TASCAR::range_t*>::iterator it=ranges.begin();it!=ranges.end();++it)
    (*it)->write_xml();
  for( std::vector<TASCAR::connection_t*>::iterator it=connections.begin();it!=connections.end();++it)
    (*it)->write_xml();
  for( std::vector<TASCAR::module_t*>::iterator it=modules.begin();it!=modules.end();++it)
    (*it)->write_xml();
}

void TASCAR::session_t::unload_modules()
{
  if( started_ )
    stop();
  if( lock_vars() ){
    std::vector<TASCAR::module_t*> lmodules(modules);  
    modules.clear();
    for( std::vector<TASCAR::module_t*>::iterator it=lmodules.begin();it!=lmodules.end();++it)
      (*it)->release();
    for( std::vector<TASCAR::module_t*>::iterator it=lmodules.begin();it!=lmodules.end();++it)
      delete (*it);
    for( std::vector<TASCAR::scene_render_rt_t*>::iterator it=scenes.begin();it!=scenes.end();++it)
      delete (*it);
    scenes.clear();
    for( std::vector<TASCAR::range_t*>::iterator it=ranges.begin();it!=ranges.end();++it)
      delete (*it);
    ranges.clear();
    for( std::vector<TASCAR::connection_t*>::iterator it=connections.begin();it!=connections.end();++it)
      delete (*it);
    connections.clear();
    unlock_vars();
  }
}

bool TASCAR::session_t::lock_vars()
{
  return (pthread_mutex_lock( &mtx ) == 0);
}

void TASCAR::session_t::unlock_vars()
{
  pthread_mutex_unlock( &mtx );
}

bool TASCAR::session_t::trylock_vars()
{
  return (pthread_mutex_trylock( &mtx ) == 0);
}

TASCAR::session_t::~session_t()
{
  osc_server_t::deactivate();
  jackc_transport_t::deactivate();
  unload_modules();
  pthread_mutex_trylock( &mtx );
  pthread_mutex_unlock( &mtx );
  pthread_mutex_destroy( &mtx );
}

std::vector<std::string> TASCAR::session_t::get_render_output_ports() const
{
  std::vector<std::string> ports;
  for( std::vector<TASCAR::scene_render_rt_t*>::const_iterator it=scenes.begin();it!=scenes.end();++it){
    std::vector<std::string> pports((*it)->get_output_ports());
    ports.insert(ports.end(),pports.begin(),pports.end());
  }
  return ports;
}

void TASCAR::session_t::add_scene(xmlpp::Element* src)
{
  if( !src )
    src = tsc_reader_t::e->add_child("scene");
  scenes.push_back(new TASCAR::scene_render_rt_t(src));
  scenes.back()->configure_meter( levelmeter_tc, levelmeter_weight );
  scenes.back()->add_child_methods(this);
}

void TASCAR::session_t::add_range(xmlpp::Element* src)
{
  if( !src )
    src = tsc_reader_t::e->add_child("range");
  ranges.push_back(new TASCAR::range_t(src));
}

void TASCAR::session_t::add_connection(xmlpp::Element* src)
{
  if( !src )
    src = tsc_reader_t::e->add_child("connect");
  connections.push_back(new TASCAR::connection_t(src));
}

void TASCAR::session_t::add_module(xmlpp::Element* src)
{
  if( !src )
    src = tsc_reader_t::e->add_child("module");
  modules.push_back(new TASCAR::module_t( TASCAR::module_cfg_t(src,this)));
}

void TASCAR::session_t::start()
{
  started_ = true;
  for(std::vector<TASCAR::scene_render_rt_t*>::iterator ipl=scenes.begin();ipl!=scenes.end();++ipl)
    (*ipl)->start();
  for(std::vector<TASCAR::module_t*>::iterator imod=modules.begin();imod!=modules.end();++imod){
    chunk_cfg_t cf( srate, fragsize );
    (*imod)->prepare( cf );
  }
  for(std::vector<TASCAR::connection_t*>::iterator icon=connections.begin();icon!=connections.end();++icon){
    try{
      connect((*icon)->src,(*icon)->dest);
    }
    catch(const std::exception& e){
      std::cerr << "Warning: " << e.what() << std::endl;
    }
  }
  for(std::vector<TASCAR::scene_render_rt_t*>::iterator ipl=scenes.begin();ipl!=scenes.end();++ipl){
    try{
      connect(get_client_name()+":sync_out",(*ipl)->get_client_name()+":sync_in");
    }
    catch(const std::exception& e){
      std::cerr << "Warning: " << e.what() << std::endl;
    }
  }
}

int TASCAR::session_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_rolling)
{
  double t(period_time*(double)tp_frame);
  uint32_t next_tp_frame(tp_frame);
  if( tp_rolling )
    next_tp_frame += fragsize;
  if( started_ )
    for(std::vector<TASCAR::module_t*>::iterator imod=modules.begin();imod!=modules.end();++imod)
      (*imod)->update(next_tp_frame,tp_rolling);
  if( t >= duration ){
    if( loop )
      tp_locate(0u);
    else
      tp_stop();
  }
  return 0;
}

void TASCAR::session_t::stop()
{
  started_ = false;
  for(std::vector<TASCAR::scene_render_rt_t*>::iterator ipl=scenes.begin();ipl!=scenes.end();++ipl)
    (*ipl)->stop();
  //jackc_transport_t::deactivate();
}

void del_whitespace( xmlpp::Node* node )
{
    xmlpp::TextNode* nodeText = dynamic_cast<xmlpp::TextNode*>(node);
    if( nodeText && nodeText->is_white_space()){
	nodeText->get_parent()->remove_child(node);
    }else{
	xmlpp::Element* nodeElement = dynamic_cast<xmlpp::Element*>(node);
	if( nodeElement ){
	    xmlpp::Node::NodeList children = nodeElement->get_children();
	    for(xmlpp::Node::NodeList::iterator nita=children.begin();nita!=children.end();++nita){
		del_whitespace( *nita );
	    }
	}
    }
}

void TASCAR::xml_doc_t::save(const std::string& filename)
{
  if( doc ){
    del_whitespace( doc->get_root_node());
    doc->write_to_file_formatted(filename);
  }
}

void TASCAR::session_t::run(bool &b_quit)
{
  start();
  while( !b_quit ){
    usleep( 50000 );
    getchar();
    if( feof( stdin ) )
      b_quit = true;
  }
  stop();
}

uint32_t TASCAR::session_t::get_active_pointsources() const
{
  uint32_t rv(0);
  for( std::vector<TASCAR::scene_render_rt_t*>::const_iterator it=scenes.begin();it!=scenes.end();++it)
    rv += (*it)->active_pointsources;
  return rv;
}

uint32_t TASCAR::session_t::get_total_pointsources() const
{
  uint32_t rv(0);
  for( std::vector<TASCAR::scene_render_rt_t*>::const_iterator it=scenes.begin();it!=scenes.end();++it)
    rv += (*it)->total_pointsources;
  return rv;
}

uint32_t TASCAR::session_t::get_active_diffusesources() const
{
  uint32_t rv(0);
  for( std::vector<TASCAR::scene_render_rt_t*>::const_iterator it=scenes.begin();it!=scenes.end();++it)
    rv += (*it)->active_diffusesources;
  return rv;
}

uint32_t TASCAR::session_t::get_total_diffusesources() const
{
  uint32_t rv(0);
  for( std::vector<TASCAR::scene_render_rt_t*>::const_iterator it=scenes.begin();it!=scenes.end();++it)
    rv += (*it)->total_diffusesources;
  return rv;
}

TASCAR::range_t::range_t(xmlpp::Element* xmlsrc)
  : scene_node_base_t(xmlsrc),
    name(""),
    start(0),
    end(0)
{
  name = e->get_attribute_value("name");
  get_attribute("start",start);
  get_attribute("end",end);
}

void TASCAR::range_t::write_xml()
{
  e->set_attribute("name",name);
  set_attribute("start",start);
  set_attribute("end",end);
}

TASCAR::connection_t::connection_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc)
{
  get_attribute("src",src);
  get_attribute("dest",dest);
}

void TASCAR::connection_t::write_xml()
{
  set_attribute("src",src);
  set_attribute("dest",dest);
}

TASCAR::module_base_t::module_base_t( const TASCAR::module_cfg_t& cfg )
  : xml_element_t(cfg.xmlsrc),session(cfg.session)
{
}

void TASCAR::module_base_t::write_xml()
{
}

void TASCAR::module_base_t::update(uint32_t frame,bool running)
{
}

TASCAR::module_base_t::~module_base_t()
{
}

std::vector<TASCAR::named_object_t> TASCAR::session_t::find_objects(const std::string& pattern)
{
  std::vector<TASCAR::named_object_t> retv;
  for(std::vector<TASCAR::scene_render_rt_t*>::iterator sit=scenes.begin();sit!=scenes.end();++sit){
    std::vector<TASCAR::Scene::object_t*> objs((*sit)->get_objects());
    std::string base("/"+(*sit)->name+"/");
    for(std::vector<TASCAR::Scene::object_t*>::iterator it=objs.begin();it!=objs.end();++it){
      std::string name(base+(*it)->get_name());
      if( fnmatch(pattern.c_str(),name.c_str(),FNM_PATHNAME) == 0 )
        retv.push_back(TASCAR::named_object_t(*it,name));
    }
  }
  return retv;
}

TASCAR::actor_module_t::actor_module_t( const TASCAR::module_cfg_t& cfg, bool fail_on_empty)
  : module_base_t( cfg )
{
  GET_ATTRIBUTE(actor);
  obj = session->find_objects(actor);
  if( fail_on_empty && obj.empty() )
    throw TASCAR::ErrMsg("No object matches actor pattern \""+actor+"\".");
}

TASCAR::actor_module_t::~actor_module_t()
{
}

void TASCAR::actor_module_t::write_xml()
{
  SET_ATTRIBUTE(actor);
}

void TASCAR::actor_module_t::set_location(const TASCAR::pos_t& l)
{
  for(std::vector<TASCAR::named_object_t>::iterator it=obj.begin();it!=obj.end();++it)
    it->obj->dlocation = l;
}

void TASCAR::actor_module_t::set_orientation(const TASCAR::zyx_euler_t& o)
{
  for(std::vector<TASCAR::named_object_t>::iterator it=obj.begin();it!=obj.end();++it)
    it->obj->dorientation = o;
}

void TASCAR::actor_module_t::add_location(const TASCAR::pos_t& l)
{
  for(std::vector<TASCAR::named_object_t>::iterator it=obj.begin();it!=obj.end();++it)
    it->obj->dlocation += l;
}

void TASCAR::actor_module_t::add_orientation(const TASCAR::zyx_euler_t& o)
{
  for(std::vector<TASCAR::named_object_t>::iterator it=obj.begin();it!=obj.end();++it)
    it->obj->dorientation += o;
}

namespace OSCSession {

  int _addtime(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 1) && (types[0] == 'f') ){
      double cur_time(std::max(0.0,std::min(((TASCAR::session_t*)user_data)->duration,((TASCAR::session_t*)user_data)->tp_get_time())));
      ((TASCAR::session_t*)user_data)->tp_locate(cur_time+argv[0]->f);
      return 0;
    }
    return 1;
  }


  int _locate(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 1) && (types[0] == 'f') ){
      ((TASCAR::session_t*)user_data)->tp_locate(argv[0]->f);
      return 0;
    }
    return 1;
  }

  int _locatei(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 1) && (types[0] == 'i') ){
      ((TASCAR::session_t*)user_data)->tp_locate((uint32_t)(argv[0]->i));
      return 0;
    }
    return 1;
  }

  int _stop(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 0) ){
      ((TASCAR::session_t*)user_data)->tp_stop();
      return 0;
    }
    return 1;
  }

  int _start(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 0) ){
      ((TASCAR::session_t*)user_data)->tp_start();
      return 0;
    }
    return 1;
  }

  int _unload_modules(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 0) ){
      ((TASCAR::session_t*)user_data)->unload_modules();
      return 0;
    }
    return 0;
  }

}


void TASCAR::session_t::add_transport_methods()
{
  osc_server_t::add_method("/transport/locate","f",OSCSession::_locate,this);
  osc_server_t::add_method("/transport/locatei","i",OSCSession::_locatei,this);
  osc_server_t::add_method("/transport/addtime","f",OSCSession::_addtime,this);
  osc_server_t::add_method("/transport/start","",OSCSession::_start,this);
  osc_server_t::add_method("/transport/stop","",OSCSession::_stop,this);
  osc_server_t::add_method("/transport/unload","",OSCSession::_unload_modules,this);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
