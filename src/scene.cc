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

/*
 * object_t
 */
object_t::object_t()
  : name("object"),
    starttime(0)
{
}

void object_t::read_xml(xmlpp::Element* e)
{
  name = e->get_attribute_value("name");
  get_attribute_value(e,"starttime",starttime);
  color = rgb_color_t(e->get_attribute_value("color"));
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "position")){
      location.read_xml(sne);
    }
    if( sne && ( sne->get_name() == "orientation")){
      orientation.read_xml(sne);
    }
    if( sne && (sne->get_name() == "creator")){
      xmlpp::Node::NodeList subnodes = sne->get_children();
      location.edit(subnodes);
    }
  }
}

void object_t::write_xml(xmlpp::Element* e,bool help_comments)
{
//  if( help_comments )
//    listener_node->add_child_comment(
//      "\n"
//      "The listener tag describes position and orientation of the\n"
//      "listener relative to the scene center. If no listener tag is\n"
//      "provided, then the position is in the origin and the orientation\n"
//      "is parallel to the x-axis.\n");
  e->set_attribute("name",name);
  e->set_attribute("color",color.str());
  set_attribute_double(e,"starttime",starttime);
  if( location.size() ){
    location.write_xml( e->add_child("position") );
  }
  if( orientation.size() ){
    orientation.write_xml( e->add_child("orientation") );
  }
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

/*
 *soundfile_t
 */
soundfile_t::soundfile_t(unsigned int channels_)
  : async_sndfile_t(channels_),
    filename(""),
    gain(0),
    loop(1),
    starttime(0),
    firstchannel(0),
    channels(channels_)
{
}

void soundfile_t::read_xml(xmlpp::Element* e)
{
  get_attribute_value(e,"starttime",starttime);
  filename = e->get_attribute_value("filename");
  get_attribute_value(e,"gain",gain);
  get_attribute_value(e,"loop",loop);
}

void soundfile_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  e->set_attribute("filename",filename);
  set_attribute_double(e,"gain",gain);
  set_attribute_uint(e,"loop",loop);
  set_attribute_double(e,"starttime",starttime);
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

void soundfile_t::prepare(double fs)
{
  //DEBUG("pre-service");
  start_service();
  //DEBUG("post-service");
  open(filename,firstchannel,(uint32_t)(starttime*fs),pow(10.0,0.05*gain),loop);
  //DEBUG("post-open");
}

/*
 * sound_t
 */
sound_t::sound_t(object_t* parent_,object_t* reference_)
  : soundfile_t(1),
    loc(0,0,0),
    parent(parent_),
    reference(reference_),
    fs_(1)
{
}

void sound_t::prepare(double fs)
{
  fs_ = fs;
  soundfile_t::prepare(fs);
}

void sound_t::request_data( uint32_t firstframe, uint32_t n, uint32_t channels, float** buf )
{
  if( parent )
    firstframe -= parent->starttime*fs_;
  soundfile_t::request_data( firstframe-starttime*fs_, n, channels, buf );
}

void sound_t::set_reference(object_t* ref)
{
  reference = ref;
}

void sound_t::set_parent(object_t* ref)
{
  parent = ref;
}

void sound_t::read_xml(xmlpp::Element* e)
{
  soundfile_t::read_xml(e);
  get_attribute_value(e,"channel",firstchannel);
  get_attribute_value(e,"x",loc.x);
  get_attribute_value(e,"y",loc.y);
  get_attribute_value(e,"z",loc.z);
}

void sound_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  soundfile_t::write_xml(e,help_comments);
  set_attribute_double(e,"x",loc.x);
  set_attribute_double(e,"y",loc.y);
  set_attribute_double(e,"z",loc.z);
  set_attribute_uint(e,"channel",firstchannel);
}

std::string sound_t::print(const std::string& prefix)
{
  std::stringstream r;
  r << soundfile_t::print(prefix);
  r << prefix << "Location: " << loc.print_cart() << "\n";
  r << prefix << "Channel: " << firstchannel << "\n";
  return r.str();
}

pos_t sound_t::get_pos(double t) const
{
  pos_t rp(loc);
  if( parent ){
    rp *= parent->orientation.interp(t - parent->starttime);
    rp += parent->location.interp(t - parent->starttime);
  }
  if( reference ){
    rp -= reference->location.interp(t - reference->starttime);
    rp /= reference->orientation.interp(t - reference->starttime);
  }
  return rp;
}

pos_t sound_t::get_pos_global(double t) const
{
  pos_t rp(loc);
  if( parent ){
    t -= parent->starttime;
    zyx_euler_t o(parent->orientation.interp(t));
    //DEBUG(o.print());
    rp *= o;
    rp += parent->location.interp(t);
  }
  return rp;
}


/*
 * src_object_t
 */
src_object_t::src_object_t(object_t* reference_)
  : object_t(),
    reference(reference_)
{
}

void src_object_t::prepare(double fs)
{
  for(std::vector<sound_t>::iterator it=sound.begin();
      it!=sound.end();++it){
    it->prepare(fs);
  }
}

void src_object_t::set_reference(object_t* ref)
{
  reference = ref;
  for(std::vector<sound_t>::iterator it=sound.begin();
      it!=sound.end();++it){
    it->set_reference(ref);
    it->set_parent(this);
  }
}

void src_object_t::read_xml(xmlpp::Element* e)
{
  object_t::read_xml(e);
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "sound" )){
      sound.push_back(sound_t(this,reference));
      sound.rbegin()->read_xml(sne);
    }
  }
}

void src_object_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  object_t::write_xml(e,help_comments);
  bool b_first(true);
  for(std::vector<sound_t>::iterator it=sound.begin();
      it!=sound.end();++it){
    it->write_xml(e->add_child("sound"),help_comments && b_first);
    b_first = false;
  }
}

std::string src_object_t::print(const std::string& prefix)
{
  std::stringstream r;
  r << object_t::print(prefix);
  for(std::vector<sound_t>::iterator it=sound.begin();
      it!=sound.end();++it)
    r << it->print(prefix+"  ");
  return r.str();
}

sound_t* src_object_t::add_sound()
{
  sound.push_back(sound_t(this,reference));
  return &sound.back();
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

bg_amb_t::bg_amb_t()
  : soundfile_t(4)
{
}

void scene_t::write_xml(xmlpp::Element* e, bool help_comments)
{
  e->set_attribute("name",name);
  if( help_comments )
    e->add_child_comment(
      "\n"
      "A scene describes the spatial and acoustical information.\n"
      "Sub-tags are src_object, listener and bg_amb.\n");
  set_attribute_double(e,"lat",lat);
  set_attribute_double(e,"lon",lon);
  set_attribute_double(e,"elev",elev);
  if( description.size()){
    xmlpp::Element* description_node = e->add_child("description");
    description_node->add_child_text(description);
  }
  bool b_first(true);
  for(std::vector<src_object_t>::iterator it=src.begin();it!=src.end();++it){
    it->write_xml(e->add_child("src_object"),help_comments && b_first);
    b_first = false;
  }
  b_first = true;
  for(std::vector<bg_amb_t>::iterator it=bg_amb.begin();it!=bg_amb.end();++it){
    it->write_xml(e->add_child("bg_amb"),help_comments && b_first);
    b_first = false;
  }
  listener.write_xml(e->add_child("listener"),help_comments);
}

void scene_t::read_xml(xmlpp::Element* e)
{
  if( e->get_name() != "scene" )
    throw ErrMsg("Invalid file, XML root node should be \"scene\".");
  name = e->get_attribute_value("name");
  get_attribute_value(e,"lon",lon);
  get_attribute_value(e,"lat",lat);
  get_attribute_value(e,"elev",elev);
  get_attribute_value(e,"duration",duration);
  description = xml_get_text(e,"description");
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne ){
      if( sne->get_name() == "src_object" ){
        src.push_back(src_object_t(&listener));
        src.rbegin()->read_xml(sne);
      }
      if( sne->get_name() == "bg_amb" ){
        bg_amb.push_back(bg_amb_t());
        bg_amb.rbegin()->read_xml(sne);
      }
      if( sne->get_name() == "listener" ){
        listener.read_xml(sne);
      }
    }
  }
}

src_object_t* scene_t::add_source()
{
  src.push_back(src_object_t(&listener));
  return &src.back();
}

void scene_t::read_xml(const std::string& filename)
{
  xmlpp::DomParser domp(filename);
  xmlpp::Document* doc(domp.get_document());
  xmlpp::Element* root(doc->get_root_node());
  read_xml(root);
}

std::vector<sound_t*> scene_t::linearize()
{
  std::vector<sound_t*> r;
  for(std::vector<src_object_t>::iterator it=src.begin();it!=src.end();++it){
    it->set_reference(&listener);
    for(std::vector<sound_t>::iterator its=it->sound.begin();its!=it->sound.end();++its){
      r.push_back(&(*its));
    }
  }
  return r;
}

void scene_t::prepare(double fs)
{
  for(std::vector<src_object_t>::iterator it=src.begin();it!=src.end();++it)
    it->prepare(fs);
}

void TASCAR::xml_write_scene(const std::string& filename, scene_t scene, const std::string& comment, bool help_comments)
{ 
  xmlpp::Document doc;
  if( comment.size() )
    doc.add_comment(comment);
  xmlpp::Element* root(doc.create_root_node("scene"));
  scene.write_xml(root);
  doc.write_to_file_formatted(filename);
}

scene_t TASCAR::xml_read_scene(const std::string& filename)
{
  scene_t s;
  xmlpp::DomParser domp(filename);
  xmlpp::Document* doc(domp.get_document());
  xmlpp::Element* root(doc->get_root_node());
  s.read_xml(root);
  return s;
}

rgb_color_t::rgb_color_t(const std::string& webc)
  : r(0),g(0),b(0)
{
  if( (webc.size() == 7) && (webc[0] == '#') ){
    int c(0);
    sscanf(webc.c_str(),"#%x",&c);
    r = ((c >> 16) & 0xff)/255.0;
    g = ((c >> 8) & 0xff)/255.0;
    b = (c & 0xff)/255.0;
  }
}

std::string rgb_color_t::str()
{
  char ctmp[64];
  unsigned int c(((unsigned int)(r*255) << 16) + 
                 ((unsigned int)(g*255) << 8) + 
                 ((unsigned int)(b*255)));
  sprintf(ctmp,"#%06x",c);
  return ctmp;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
