#include "scene.h"
#include "errorhandling.h"
#include <libxml++/libxml++.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

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


void get_attribute_value(xmlpp::Element* elem,const std::string& name,unsigned int& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(),&c));
  if( c != attv.c_str() )
    value = tmpv;
}

src_object_t xml_read_src_object( xmlpp::Element* sne )
{
  src_object_t srcobj;
  get_attribute_value(sne,"start",srcobj.start);
  srcobj.name = sne->get_attribute_value("name");
  xmlpp::Node::NodeList subnodes = sne->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne ){
      if( sne->get_name() == "sound"){
        srcobj.sound.filename = sne->get_attribute_value("filename");
        get_attribute_value(sne,"gain",srcobj.sound.gain);
        get_attribute_value(sne,"channel",srcobj.sound.channel);
        get_attribute_value(sne,"loop",srcobj.sound.loop);
      }
      if( sne->get_name() == "position"){
        std::stringstream ptxt(xml_get_text(sne,""));
        while( !ptxt.eof() ){
          double t,x,y,z;
          ptxt >> t >> x >> y >> z;
          srcobj.position[t] = pos_t(x,y,z);
        }
      }
    }
  }
  return srcobj;
}

bg_amb_t xml_read_bg_amb( xmlpp::Element* sne )
{
  bg_amb_t bg;
  get_attribute_value(sne,"start",bg.start);
  bg.filename = sne->get_attribute_value("filename");
  get_attribute_value(sne,"gain",bg.gain);
  get_attribute_value(sne,"loop",bg.loop);
  return bg;
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
  s.description = xml_get_text(root,"description");
  xmlpp::Node::NodeList subnodes = root->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne ){
      if( sne->get_name() == "src_object" ){
        s.src.push_back(xml_read_src_object(sne));
      }
      if( sne->get_name() == "bg_amb" ){
      s.bg_amb.push_back(xml_read_bg_amb(sne));
      }
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
