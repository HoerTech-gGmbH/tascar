#include "session.h"
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include "errorhandling.h"

TASCAR::xml_doc_t::xml_doc_t()
  : doc(NULL)
{
  doc = new xmlpp::Document();
  doc->create_root_node("session");
}

TASCAR::xml_doc_t::xml_doc_t(const std::string& filename)
  : doc(NULL)
{
  xmlpp::DomParser domp(TASCAR::env_expand(filename));
  doc = domp.get_document();
  if( !doc )
    throw TASCAR::ErrMsg("Unable to parse document.");
}

TASCAR::session_t::session_t()
  : xml_element_t(doc->get_root_node())
{
  char c_respath[PATH_MAX];
  session_path = getcwd(c_respath,PATH_MAX);
}

TASCAR::session_t::session_t(const std::string& filename)
  : xml_doc_t(filename),xml_element_t(doc->get_root_node())
{
  char c_fname[filename.size()+1];
  char c_respath[PATH_MAX];
  memcpy(c_fname,filename.c_str(),filename.size()+1);
  session_path = realpath(dirname(c_fname),c_respath);
  if( get_element_name() != "session" )
    throw TASCAR::ErrMsg("Invalid root node name. Expected \"session\", got "+get_element_name()+".");
}

void TASCAR::session_t::write_xml()
{
  for( std::vector<TASCAR::scene_player_t>::iterator it=player.begin();it!=player.end();++it)
    it->write_xml();
  for( std::vector<TASCAR::range_t>::iterator it=ranges.begin();it!=ranges.end();++it)
    it->write_xml();
  for( std::vector<TASCAR::connection_t>::iterator it=connections.begin();it!=connections.end();++it)
    it->write_xml();
}

TASCAR::session_t::~session_t()
{
}

TASCAR::Scene::scene_t& TASCAR::session_t::add_scene(xmlpp::Element* src)
{
  if( !src )
    src = e->add_child("scene");
  player.push_back(TASCAR::scene_player_t(src));
  return player.back();
}

TASCAR::range_t& TASCAR::session_t::add_range(xmlpp::Element* src)
{
  if( !src )
    src = e->add_child("range");
  ranges.push_back(TASCAR::range_t(src));
  return ranges.back();
}

TASCAR::connection_t& TASCAR::session_t::add_connection(xmlpp::Element* src)
{
  if( !src )
    src = e->add_child("connection");
  connections.push_back(TASCAR::connection_t(src));
  return connections.back();
}

void TASCAR::session_t::start()
{
  for(std::vector<TASCAR::scene_player_t>::iterator ipl=player.begin();ipl!=player.end();++ipl)
    ipl->start();
  jackc_portless_t jc(name+"_session");
  jc.activate();
  for(std::vector<TASCAR::connection_t>::iterator icon=connections.begin();icon!=connections.end();++icon)
    jc.connect(icon->src,icon->dest);
  jc.deactivate();
  //for(uint32_t k=0;k<connections.size();k++)
  //  connect(connections[k].src,connections[k].dest,true);
}

void TASCAR::session_t::stop()
{
  for(std::vector<TASCAR::scene_player_t>::iterator ipl=player.begin();ipl!=player.end();++ipl)
    ipl->stop();
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

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
