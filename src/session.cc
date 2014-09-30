#include "session.h"
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include "errorhandling.h"
#include <dlfcn.h>
#include <fnmatch.h>

static void module_error(std::string errmsg)
{
  throw TASCAR::ErrMsg("Submodule error: "+errmsg);
}

TASCAR::module_t::module_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session)
  : xml_element_t(xmlsrc),
    lib(NULL),
    libdata(NULL),
    create_cb(NULL),
    destroy_cb(NULL),
    write_xml_cb(NULL)
{
  get_attribute("name",name);
  std::string libname("tascar_");
  libname += name + ".so";
  lib = dlopen(libname.c_str(), RTLD_NOW );
  if( !lib )
    throw TASCAR::ErrMsg("Unable to open module \""+name+"\": "+dlerror());
  try{
    create_cb = (module_create_t)dlsym(lib,"tascar_create");
    if( !create_cb )
      throw TASCAR::ErrMsg("Unable to resolve \"tascar_create\" in module \""+name+"\".");
    destroy_cb = (module_destroy_t)dlsym(lib,"tascar_destroy");
    if( !destroy_cb )
      throw TASCAR::ErrMsg("Unable to resolve \"tascar_destroy\" in module \""+name+"\".");
    write_xml_cb = (module_write_xml_t)dlsym(lib,"tascar_write_xml");
    //DEBUG(1);
    libdata = create_cb(xmlsrc,session,module_error);
    //DEBUG(libdata);
  }
  catch( ... ){
    dlclose(lib);
    throw;
  }
}

void TASCAR::module_t::write_xml()
{
  //DEBUG(write_xml_cb);
  if( write_xml_cb )
    write_xml_cb(libdata,module_error);
}

TASCAR::module_t::~module_t()
{
  //DEBUG(libdata);
  destroy_cb(libdata,module_error);
  dlclose(lib);
}

xmlpp::Element* assert_element(xmlpp::Element* e)
{
  //DEBUG(e);
  if( !e )
    throw TASCAR::ErrMsg("NULL pointer element");
  return e;
}

const std::string& debug_str(const std::string& s)
{
  //DEBUG(s);
  return s;
}

TASCAR::xml_doc_t::xml_doc_t()
  : doc(NULL)
{
  //DEBUG(1);
  doc = new xmlpp::Document();
  //DEBUG(1);
  doc->create_root_node("session");
  //DEBUG(1);
}

TASCAR::xml_doc_t::xml_doc_t(const std::string& filename)
  : domp(TASCAR::env_expand(filename)),doc(NULL)
{
  //DEBUG(1);
  //DEBUG(1);
  doc = domp.get_document();
  //DEBUG(1);
  if( !doc )
    throw TASCAR::ErrMsg("Unable to parse document.");
  //DEBUG(1);
}

TASCAR::session_t::session_t()
  : xml_element_t(doc->get_root_node()),
    jackc_portless_t(jacknamer(e->get_attribute_value("name"),"session.")),
    name("tascar"),
    duration(60),
    loop(false)
{
  //DEBUG(1);
  char c_respath[PATH_MAX];
  session_path = getcwd(c_respath,PATH_MAX);
  if( get_element_name() != "session" )
    throw TASCAR::ErrMsg("Invalid root node name. Expected \"session\", got "+get_element_name()+".");
  read_xml();
}

TASCAR::session_t::session_t(const std::string& filename)
  : xml_doc_t(filename),
    xml_element_t(doc->get_root_node()),
    jackc_portless_t(jacknamer(e->get_attribute_value("name"),"session.")),
    name("tascar"),
    duration(60),
    loop(false)
{
  //DEBUG(1);
  char c_fname[filename.size()+1];
  char c_respath[PATH_MAX];
  memcpy(c_fname,filename.c_str(),filename.size()+1);
  session_path = realpath(dirname(c_fname),c_respath);
  if( get_element_name() != "session" )
    throw TASCAR::ErrMsg("Invalid root node name. Expected \"session\", got "+get_element_name()+".");
  read_xml();
}

void TASCAR::session_t::read_xml()
{
  get_attribute("name",name);
  get_attribute("duration",duration);
  get_attribute_bool("loop",loop);
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "scene"))
      add_scene(sne);
    if( sne && ( sne->get_name() == "range"))
      add_range(sne);
    if( sne && ( sne->get_name() == "connect"))
      add_connection(sne);
    if( sne && ( sne->get_name() == "module"))
      add_module(sne);
  }
}


void TASCAR::session_t::save(const std::string& filename)
{
  write_xml();
  xml_doc_t::save(filename);
}

void TASCAR::session_t::write_xml()
{
  //DEBUG(1);
  for( std::vector<TASCAR::scene_player_t*>::iterator it=player.begin();it!=player.end();++it)
    (*it)->write_xml();
  for( std::vector<TASCAR::range_t*>::iterator it=ranges.begin();it!=ranges.end();++it)
    (*it)->write_xml();
  for( std::vector<TASCAR::connection_t*>::iterator it=connections.begin();it!=connections.end();++it)
    (*it)->write_xml();
  for( std::vector<TASCAR::module_t*>::iterator it=modules.begin();it!=modules.end();++it)
    (*it)->write_xml();
  //DEBUG(1);
}

TASCAR::session_t::~session_t()
{
  for( std::vector<TASCAR::scene_player_t*>::iterator it=player.begin();it!=player.end();++it)
    delete (*it);
  for( std::vector<TASCAR::range_t*>::iterator it=ranges.begin();it!=ranges.end();++it)
    delete (*it);
  for( std::vector<TASCAR::connection_t*>::iterator it=connections.begin();it!=connections.end();++it)
    delete (*it);
  for( std::vector<TASCAR::module_t*>::iterator it=modules.begin();it!=modules.end();++it)
    delete (*it);
}

TASCAR::Scene::scene_t* TASCAR::session_t::add_scene(xmlpp::Element* src)
{
  if( !src )
    src = e->add_child("scene");
  player.push_back(new TASCAR::scene_player_t(src));
  return player.back();
}

TASCAR::range_t* TASCAR::session_t::add_range(xmlpp::Element* src)
{
  if( !src )
    src = e->add_child("range");
  ranges.push_back(new TASCAR::range_t(src));
  return ranges.back();
}

TASCAR::connection_t* TASCAR::session_t::add_connection(xmlpp::Element* src)
{
  if( !src )
    src = e->add_child("connect");
  connections.push_back(new TASCAR::connection_t(src));
  return connections.back();
}

TASCAR::module_t* TASCAR::session_t::add_module(xmlpp::Element* src)
{
  if( !src )
    src = e->add_child("module");
  modules.push_back(new TASCAR::module_t(src,this));
  return modules.back();
}

void TASCAR::session_t::start()
{
  activate();
  for(std::vector<TASCAR::scene_player_t*>::iterator ipl=player.begin();ipl!=player.end();++ipl)
    (*ipl)->start();
  for(std::vector<TASCAR::connection_t*>::iterator icon=connections.begin();icon!=connections.end();++icon){
    try{
      connect((*icon)->src,(*icon)->dest);
    }
    catch(const std::exception& e){
      std::cerr << "Warning: " << e.what() << std::endl;
    }
  }
}

void TASCAR::session_t::stop()
{
  for(std::vector<TASCAR::scene_player_t*>::iterator ipl=player.begin();ipl!=player.end();++ipl)
    (*ipl)->stop();
  deactivate();
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
  for( std::vector<TASCAR::scene_player_t*>::const_iterator it=player.begin();it!=player.end();++it)
    rv += (*it)->active_pointsources;
  return rv;
}

uint32_t TASCAR::session_t::get_total_pointsources() const
{
  uint32_t rv(0);
  for( std::vector<TASCAR::scene_player_t*>::const_iterator it=player.begin();it!=player.end();++it)
    rv += (*it)->total_pointsources;
  return rv;
}

uint32_t TASCAR::session_t::get_active_diffusesources() const
{
  uint32_t rv(0);
  for( std::vector<TASCAR::scene_player_t*>::const_iterator it=player.begin();it!=player.end();++it)
    rv += (*it)->active_diffusesources;
  return rv;
}

uint32_t TASCAR::session_t::get_total_diffusesources() const
{
  uint32_t rv(0);
  for( std::vector<TASCAR::scene_player_t*>::const_iterator it=player.begin();it!=player.end();++it)
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

TASCAR::module_base_t::module_base_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session_)
  : xml_element_t(xmlsrc),session(session_)
{
}

void TASCAR::module_base_t::write_xml()
{
}

TASCAR::module_base_t::~module_base_t()
{
}

std::vector<TASCAR::named_object_t> TASCAR::session_t::find_objects(const std::string& pattern)
{
  std::vector<TASCAR::named_object_t> retv;
  for(std::vector<TASCAR::scene_player_t*>::iterator sit=player.begin();sit!=player.end();++sit){
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

const std::string& TASCAR::session_t::get_session_path() const
{
  return session_path;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
