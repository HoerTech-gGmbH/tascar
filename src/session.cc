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
  if( get_name() != "session" )
    throw TASCAR::ErrMsg("Invalid root node name. Expected \"session\", got "+get_name()+".");
}

TASCAR::session_t::~session_t()
{
}

TASCAR::Scene::scene_t& TASCAR::session_t::add_scene(xmlpp::Element* src)
{
  if( !src )
    src = e->add_child("scene");
  scenes.push_back(TASCAR::Scene::scene_t(src));
  return scenes.back();
}

TASCAR::Scene::range_t& TASCAR::session_t::add_range(xmlpp::Element* src)
{
  if( !src )
    src = e->add_child("range");
  ranges.push_back(TASCAR::Scene::range_t(src));
  return ranges.back();
}

TASCAR::Scene::connection_t& TASCAR::session_t::add_connection(xmlpp::Element* src)
{
  if( !src )
    src = e->add_child("connection");
  connections.push_back(TASCAR::Scene::connection_t(src));
  return connections.back();
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
