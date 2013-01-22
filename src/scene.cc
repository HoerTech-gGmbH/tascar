#include "scene.h"
#include "errorhandling.h"
#include <libxml++/libxml++.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "defs.h"

using namespace TASCAR;

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

object_t::object_t()
  : name("object"),
    starttime(0)
{
}

void object_t::read_xml(xmlpp::Element* e)
{
}

void object_t::write_xml(xmlpp::Element* e,bool help_comments)
{
}

std::string object_t::print(const std::string& prefix)
{
  std::stringstream r;
  r << prefix << "Name: \"" << name << "\"\n";
  r << prefix << "Starttime: " << starttime << " s\n";
  r << prefix << "Trajectory center: " << location.center().print_cart() << " m\n";
  r << prefix << "trajectory length: " << location.length() << " m\n";
  r << prefix << "Trajectory duration: " << location.duration() << " s (";
  r << location.t_min()+starttime << " - " << location.t_max()+starttime << "s)\n";
  //euler_track_t orientation;
  return r.str();
}

soundfile_t::soundfile_t()
{
}

void soundfile_t::read_xml(xmlpp::Element* e)
{
}

void soundfile_t::write_xml(xmlpp::Element* e,bool help_comments)
{
}

std::string soundfile_t::print(const std::string& prefix)
{
  std::stringstream r;
  r << prefix << "Filename: \"" << filename << "\"\n";
  r << prefix << "Gain: " << gain << " dB\n";
  r << prefix << "Loop: " << loop << "\n";
  r << prefix << "Starttime: " << starttime << " s\n";
  return r.str();
}

scene_t::scene_t()
  : description(""),
    name(""),
    duration(60),
    lat(53.155473),
    lon(8.167249),
    elev(10)
{
}


std::string scene_t::print(const std::string& prefix)
{
  std::stringstream r;
  r << "scene: \"" << name << "\"\n  duration: " << duration << " s\n";
  r << "  geo center at " << fabs(lat) << ((lat>=0)?"N ":"S ");
  r << fabs(lon) << ((lon>=0)?"E, ":"W, ");
  r << fabs(elev) << ((elev>=0)?" m above":"m below");
  r << " sea level.\n";
  r << "  " << src.size() << " source objects\n";
  r << "  " << bg_amb.size() << " backgrounds\n";
  for(std::vector<src_object_t>::iterator it=src.begin();it!=src.end();++it)
    r << it->print(prefix+"  ");
  for(std::vector<bg_amb_t>::iterator it=bg_amb.begin();it != bg_amb.end();++it)
    r << it->print(prefix+"  ");
  r << listener.print(prefix+"  ");
  return r.str();
}

listener_t::listener_t()
{
}

//std::string listener_t::print()
//{
//  std::stringstream r;
//  r << "  listener: \"" << name << "\"\n";
//  r << "    start: " << start << " s\n";
//  r << "    trajectory center: " << position.center().print_cart() << " m\n";
//  r << "    trajectory length: " << position.length() << " m\n";
//  r << "    trajectory duration: " << position.duration() << " s (";
//  r << position.t_min()+start << " - " << position.t_max()+start << "s)\n";
//  return r.str();
//}

src_object_t::src_object_t()
{
}

std::string src_object_t::print(const std::string& prefix)
{
  std::stringstream r;
  r << object_t::print(prefix);
  for( std::vector<sound_t>::iterator it=sound.begin();it!=sound.end();++it){
    r << prefix << "--sound--\n";
    r << it->print(prefix+"  ");
  }
  return r.str();
}

bg_amb_t::bg_amb_t()
{
}

sound_t::sound_t()
{
}

void scene_t::write_xml(xmlpp::Element* e, bool help_comments)
{
}

void TASCAR::xml_write_scene(const std::string& filename, scene_t scene, const std::string& comment, bool help_comments)
{ 
  xmlpp::Document doc;
  if( comment.size() )
    doc.add_comment(comment);
  xmlpp::Element* root(doc.create_root_node("scene"));
  scene.write_xml(root);
//  root->set_attribute("name",scene.name);
//  if( help_comments )
//    root->add_child_comment(
//      "\n"
//      "A scene describes the spatial and acoustical information.\n"
//      "Sub-tags are src_object, listener and bg_amb.\n");
//  set_attribute_double(root,"lat",scene.lat);
//  set_attribute_double(root,"lon",scene.lon);
//  set_attribute_double(root,"elev",scene.elev);
//  if( scene.description.size()){
//    xmlpp::Element* description_node = root->add_child("description");
//    description_node->add_child_text(scene.description);
//  }
//  xmlpp::Element* listener_node = root->add_child("listener");
//  listener_node->set_attribute("name",scene.listener.name);
//  set_attribute_double(listener_node,"start",scene.listener.start);
//  if( help_comments )
//    listener_node->add_child_comment(
//      "\n"
//      "The listener tag describes position and orientation of the\n"
//      "listener relative to the scene center. If no listener tag is\n"
//      "provided, then the position is in the origin and the orientation\n"
//      "is parallel to the x-axis.\n");
//  if( scene.listener.size() ){
//    xmlpp::Element* listenpos_node = listener_node->add_child("position");
//    scene.listener.export_to_xml_element( listenpos_node );
//  }
//  bool first_object(true);
//  for( std::vector<src_object_t>::iterator src_it = scene.src.begin(); src_it != scene.src.end(); ++src_it){
//    xmlpp::Element* src_node = root->add_child("src_object");
//    if( help_comments && first_object )
//      src_node->add_child_comment(
//        "\n"
//        "A src_object defines a sound source in a scene. Children\n"
//        "are the sound file (acoustic parameters) and the position\n"
//        "(geometrical parameters).\n");
//    first_object = false;
//    src_node->set_attribute("name",src_it->name);
//    set_attribute_double(src_node,"start",src_it->start);
//    xmlpp::Element* sound_node = src_node->add_child("sound");
//    sound_node->set_attribute("filename",src_it->sound.filename);
//    set_attribute_double(sound_node,"gain",src_it->sound.gain);
//    set_attribute_uint(sound_node,"channel",src_it->sound.channel);
//    set_attribute_uint(sound_node,"loop",src_it->sound.loop);
//    if( src_it->position.size() ){
//      xmlpp::Element* pos_node = src_node->add_child("position");
//      src_it->position.export_to_xml_element( pos_node );
//    }
//  }
//  first_object = true;
//  for( std::vector<bg_amb_t>::iterator bg_it = scene.bg_amb.begin(); bg_it != scene.bg_amb.end(); ++bg_it){
//    xmlpp::Element* bg_node = root->add_child("bg_amb");
//    if( help_comments && first_object )
//      bg_node->add_child_comment(
//        "\n"
//        "A background can be a sound file in first order ambisonics\n"
//        "format, Furse-Malham-normalization. Multiple background objects\n"
//        "are allowed.\n");
//    first_object = false;
//    set_attribute_double(bg_node,"start",bg_it->start);
//    bg_node->set_attribute("filename",bg_it->filename);
//    set_attribute_double(bg_node,"gain",bg_it->gain);
//    set_attribute_uint(bg_node,"loop",bg_it->loop);
//  }
//  doc.write_to_file_formatted(filename);
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

src_object_t xml_read_src_object( xmlpp::Element* msne )
{
  src_object_t srcobj;
//  get_attribute_value(msne,"start",srcobj.start);
//  srcobj.name = msne->get_attribute_value("name");
//  xmlpp::Node::NodeList subnodes = msne->get_children();
//  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
//    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
//    if( sne ){
//      if( sne->get_name() == "sound"){
//        srcobj.sound.filename = sne->get_attribute_value("filename");
//        get_attribute_value(sne,"gain",srcobj.sound.gain);
//        get_attribute_value(sne,"channel",srcobj.sound.channel);
//        get_attribute_value(sne,"loop",srcobj.sound.loop);
//      }
//      if( sne->get_name() == "position"){
//        std::stringstream ptxt(xml_get_text(sne,""));
//        while( !ptxt.eof() ){
//          double t,x,y,z;
//          ptxt >> t >> x >> y >> z;
//          srcobj.position[t] = pos_t(x,y,z);
//        }
//      }
//      if( sne->get_name() == "creator"){
//        xmlpp::Node::NodeList subnodes = sne->get_children();
//        srcobj.position.edit(subnodes);
//      }
//    }
//  }
  return srcobj;
}

bg_amb_t xml_read_bg_amb( xmlpp::Element* sne )
{
  bg_amb_t bg;
  //get_attribute_value(sne,"start",bg.start);
  //bg.filename = sne->get_attribute_value("filename");
  //get_attribute_value(sne,"gain",bg.gain);
  //get_attribute_value(sne,"loop",bg.loop);
  return bg;
}

scene_t TASCAR::xml_read_scene(const std::string& filename)
{
  scene_t s;
//  xmlpp::DomParser domp(filename);
//  xmlpp::Document* doc(domp.get_document());
//  xmlpp::Element* root(doc->get_root_node());
//  s.name = root->get_attribute_value("name");
//  if( root->get_name() != "scene" )
//    throw ErrMsg("Invalid file, XML root node should be \"scene\".");
//  get_attribute_value(root,"lon",s.lon);
//  get_attribute_value(root,"lat",s.lat);
//  get_attribute_value(root,"elev",s.elev);
//  get_attribute_value(root,"duration",s.duration);
//  s.description = xml_get_text(root,"description");
//  xmlpp::Node::NodeList subnodes = root->get_children();
//  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
//    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
//    if( sne ){
//      if( sne->get_name() == "src_object" ){
//        s.src.push_back(xml_read_src_object(sne));
//      }
//      if( sne->get_name() == "bg_amb" ){
//        s.bg_amb.push_back(xml_read_bg_amb(sne));
//      }
//      if( sne->get_name() == "listener" ){
//        s.listener.name = sne->get_attribute_value("name");
//        get_attribute_value(sne,"elev",s.listener.start);
//        for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
//          xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
//          if( sne ){
//            if( sne->get_name() == "position"){
//              std::stringstream ptxt(xml_get_text(sne,""));
//              while( !ptxt.eof() ){
//                double t,x,y,z;
//                ptxt >> t >> x >> y >> z;
//                s.listener.position[t] = pos_t(x,y,z);
//              }
//            }
//          }
//        }
//      }
//    }
//  }
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
