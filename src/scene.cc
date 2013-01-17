#include "scene.h"
#include "errorhandling.h"
#include <libxml++/libxml++.h>
#include <stdio.h>
#include <stdlib.h>

using namespace TASCAR;

scene_t::scene_t()
  : description(""),
    name(""),
    lat(53.155473),
    lon(8.167249),
    elev(10)
{
}

listener_t::listener_t()
  : name("me")
{
}

src_object_t::src_object_t()
  : name(""),
    start(0)
{
}

bg_amb_t::bg_amb_t()
  : start(0),
    filename(""),
    gain(0),
    loop(1)
{
}

sound_t::sound_t()
  : filename(""),
    gain(0),
    channel(0),
    loop(1)
{
}

void set_attribute_uint(xmlpp::Element* elem,const std::string& name,unsigned int value)
{
  char ctmp[1024];
  sprintf(ctmp,"%d",value);
  elem->set_attribute(name,ctmp);
}

void set_attribute_double(xmlpp::Element* elem,const std::string& name,double value)
{
  char ctmp[1024];
  sprintf(ctmp,"%1.12g",value);
  elem->set_attribute(name,ctmp);
}

void TASCAR::xml_write_scene(const std::string& filename, scene_t scene, const std::string& comment)
{ 
  xmlpp::Document doc;
  if( comment.size() )
    doc.add_comment(comment);
  xmlpp::Element* root(doc.create_root_node("scene"));
  root->set_attribute("name",scene.name);
  set_attribute_double(root,"lat",scene.lat);
  set_attribute_double(root,"lon",scene.lon);
  set_attribute_double(root,"elev",scene.elev);
  if( scene.description.size()){
    xmlpp::Element* description_node = root->add_child("description");
    description_node->add_child_text(scene.description);
  }
  xmlpp::Element* listener_node = root->add_child("listener");
  listener_node->set_attribute("name",scene.listener.name);
  if( scene.listener.size() ){
    xmlpp::Element* listenpos_node = listener_node->add_child("position");
    scene.listener.export_to_xml_element( listenpos_node );
  }
  for( std::vector<src_object_t>::iterator src_it = scene.src.begin(); src_it != scene.src.end(); ++src_it){
    xmlpp::Element* src_node = root->add_child("src_object");
    src_node->set_attribute("name",src_it->name);
    set_attribute_double(src_node,"start",src_it->start);
    xmlpp::Element* sound_node = src_node->add_child("sound");
    sound_node->set_attribute("filename",src_it->sound.filename);
    set_attribute_double(sound_node,"gain",src_it->sound.gain);
    set_attribute_uint(sound_node,"channel",src_it->sound.channel);
    set_attribute_uint(sound_node,"loop",src_it->sound.loop);
    if( src_it->position.size() ){
      xmlpp::Element* pos_node = src_node->add_child("position");
      src_it->position.export_to_xml_element( pos_node );
    }
  }
  for( std::vector<bg_amb_t>::iterator bg_it = scene.bg_amb.begin(); bg_it != scene.bg_amb.end(); ++bg_it){
    xmlpp::Element* bg_node = root->add_child("bg_amb");
    set_attribute_double(bg_node,"start",bg_it->start);
    bg_node->set_attribute("filename",bg_it->filename);
    set_attribute_double(bg_node,"gain",bg_it->gain);
    set_attribute_uint(bg_node,"loop",bg_it->loop);
  }
  doc.write_to_file_formatted(filename);
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,double& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(),&c));
  if( c != attv.c_str() )
    value = tmpv;
}

scene_t TASCAR::xml_read_scene(const std::string& filename)
{
  scene_t s;
  xmlpp::DomParser domp(filename);
  xmlpp::Document* doc(domp.get_document());
  xmlpp::Element* root(doc->get_root_node());
  s.name = root->get_attribute_value("name");
  if( root->get_name() != "scene" )
    throw ErrMsg("Invalid file, XML root node should be \"scene\".");
  get_attribute_value(root,"lon",s.lon);
  get_attribute_value(root,"lat",s.lat);
  get_attribute_value(root,"elev",s.elev);
  xmlpp::Node::NodeList subnodes = root->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    if( (*sn)->get_name() == "description" ){
    }
  }
  return s;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
