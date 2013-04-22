#include "scene.h"
#include "errorhandling.h"
#include <libxml++/libxml++.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "defs.h"
#include <string.h>

using namespace TASCAR;

void set_attribute_uint(xmlpp::Element* elem,const std::string& name,unsigned int value)
{
  char ctmp[1024];
  sprintf(ctmp,"%d",value);
  elem->set_attribute(name,ctmp);
}

void set_attribute_bool(xmlpp::Element* elem,const std::string& name,bool value)
{
  if( value )
    elem->set_attribute(name,"true");
  else
    elem->set_attribute(name,"false");
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

void get_attribute_value_deg(xmlpp::Element* elem,const std::string& name,double& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(),&c));
  if( c != attv.c_str() )
    value = DEG2RAD*tmpv;
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,unsigned int& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(),&c));
  if( c != attv.c_str() )
    value = tmpv;
}


void get_attribute_value(xmlpp::Element* elem,const std::string& name,bool& value)
{
  std::string attv(elem->get_attribute_value(name));
  value = (attv == "true");
}

/*
 * object_t
 */
object_t::object_t()
  : starttime(0),
    endtime(0)
{
}

bool object_t::isactive(double time) const
{
  return (!get_mute())&&(time>=starttime)&&((starttime>=endtime)||(time<=endtime));
}

pos_t object_t::get_location(double time) const
{
  pos_t p(location.interp(time-starttime));
  return (p+=dlocation);
}

zyx_euler_t object_t::get_orientation(double time) const
{
  zyx_euler_t o(orientation.interp(time-starttime));
  return (o+=dorientation);
}

void object_t::read_xml(xmlpp::Element* e)
{
  route_t::read_xml(e);
  get_attribute_value(e,"start",starttime);
  get_attribute_value(e,"end",endtime);
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
      TASCAR::track_t::iterator it_old=location.end();
      double old_azim(0);
      double new_azim(0);
      for(TASCAR::track_t::iterator it=location.begin();it!=location.end();++it){
        if( it_old != location.end() ){
          pos_t p=it->second;
          p -= it_old->second;
          new_azim = p.azim();
          while( new_azim - old_azim > M_PI )
            new_azim -= 2*M_PI;
          while( new_azim - old_azim < -M_PI )
            new_azim += 2*M_PI;
          orientation[it_old->first] = zyx_euler_t(new_azim,0,0);
          old_azim = new_azim;
        }
        if( TASCAR::distance(it->second,it_old->second) > 0 )
          it_old = it;
      }
    }
  }
  //if( starttime < endtime ){
  //  TASCAR::track_t ntrack;
  //  for(TASCAR::track_t::iterator it=location.begin();it!=location.end();++it){
  //    if( (it->first >= 0) && (it->first <= endtime-starttime) )
  //      ntrack[it->first] = it->second;
  //  }
  //  location = ntrack;
  //}
}

void object_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  route_t::write_xml(e,help_comments);
  e->set_attribute("color",color.str());
  set_attribute_double(e,"start",starttime);
  set_attribute_double(e,"end",endtime);
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
  r << route_t::print(prefix);
  r << prefix << "Starttime: " << starttime << " s\n";
  r << prefix << "Trajectory center: " << location.center().print_cart() << " m\n";
  r << prefix << "trajectory length: " << location.length() << " m\n";
  r << prefix << "Trajectory duration: " << location.duration() << " s (";
  r << location.t_min()+starttime << " - " << location.t_max()+starttime << "s)\n";
  //euler_track_t orientation;
  return r.str();
}

/*
 *bg_amb_t
 */
bg_amb_t::bg_amb_t()
  : async_sndfile_t(4,1<<18,4096),
    filename(""),
    gain(0),
    loop(1),
    starttime(0),
    firstchannel(0)
{
}

void bg_amb_t::read_xml(xmlpp::Element* e)
{
  route_t::read_xml(e);
  get_attribute_value(e,"start",starttime);
  filename = e->get_attribute_value("filename");
  get_attribute_value(e,"gain",gain);
  get_attribute_value(e,"loop",loop);
}

void bg_amb_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  route_t::write_xml(e,help_comments);
  e->set_attribute("filename",filename);
  set_attribute_double(e,"gain",gain);
  set_attribute_uint(e,"loop",loop);
  set_attribute_double(e,"start",starttime);
}

std::string bg_amb_t::print(const std::string& prefix)
{
  std::stringstream r;
  r << prefix << "Filename: \"" << filename << "\"\n";
  r << prefix << "Gain: " << gain << " dB\n";
  r << prefix << "Loop: " << loop << "\n";
  r << prefix << "Starttime: " << starttime << " s\n";
  return r.str();
}

void bg_amb_t::prepare(double fs, uint32_t fragsize)
{
  //DEBUG("pre-service");
  open(filename,firstchannel,(uint32_t)(starttime*fs),pow(10.0,0.05*gain),loop);
  start_service();
  //DEBUG("post-service");
  //open(filename,firstchannel,0,pow(10.0,0.05*gain),loop);
  //DEBUG("post-open");
}

/*
 * sound_t
 */
sound_t::sound_t(src_object_t* parent_,object_t* reference_)
  : //bg_amb_t(1),
    loc(0,0,0),
    chaindist(0),
    parent(parent_),
    reference(reference_),
    fs_(1)
{
}

void sound_t::prepare(double fs, uint32_t fragsize)
{
  fs_ = fs;
  //DEBUG(parent);
  if( parent )
    input = parent->get_input(name);
  else
    input = NULL;
  //DEBUG(input);
  //bg_amb_t::prepare(fs,fragsize);
}

//void sound_t::request_data( int32_t firstframe, uint32_t n, uint32_t channels, float** buf )
//{
//  if( parent )
//    firstframe -= parent->starttime*fs_;
//  //bg_amb_t::request_data( firstframe-starttime*fs_, n, channels, buf );
//  //bg_amb_t::request_data( firstframe, n, channels, buf );
//}

float* sound_t::get_buffer( uint32_t n )
{
  if( input )
    return input->get_buffer(n);
  return NULL;
}

void sound_t::set_reference(object_t* ref)
{
  //DEBUG(ref);
  reference = ref;
}

void sound_t::set_parent(src_object_t* ref)
{
  //DEBUG(ref);
  parent = ref;
}

void sound_t::read_xml(xmlpp::Element* e)
{
  //bg_amb_t::read_xml(e);
  //get_attribute_value(e,"channel",firstchannel);
  get_attribute_value(e,"x",loc.x);
  get_attribute_value(e,"y",loc.y);
  get_attribute_value(e,"z",loc.z);
  get_attribute_value(e,"d",chaindist);
  name = e->get_attribute_value("name");
}

void sound_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  //bg_amb_t::write_xml(e,help_comments);
  e->set_attribute("name",name);
  if( loc.x != 0 )
    set_attribute_double(e,"x",loc.x);
  if( loc.y != 0 )
    set_attribute_double(e,"y",loc.y);
  if( loc.z != 0 )
    set_attribute_double(e,"z",loc.z);
  if( chaindist != 0 )
    set_attribute_double(e,"d",chaindist);
}

std::string sound_t::print(const std::string& prefix)
{
  std::stringstream r;
  //r << bg_amb_t::print(prefix);
  r << prefix << "Location: " << loc.print_cart() << "\n";
  //r << prefix << "Channel: " << firstchannel << "\n";
  return r.str();
}

bool sound_t::isactive(double t)
{
  return parent && parent->isactive(t);
}

pos_t sound_t::get_pos(double t) const
{
  pos_t rp(get_pos_global(t));
  if( reference ){
    rp -= reference->dlocation;
    rp -= reference->location.interp(t - reference->starttime);
    zyx_euler_t o(reference->dorientation);
    o += reference->orientation.interp(t - reference->starttime);
    rp /= o;
  }
  return rp;
}

pos_t sound_t::get_pos_global(double t) const
{
  pos_t rp(loc);
  if( parent ){
    double tp(t - parent->starttime);
    if( chaindist != 0 ){
      tp = parent->location.get_time(parent->location.get_dist(tp)-chaindist);
    }
    zyx_euler_t o(parent->dorientation);
    o += parent->orientation.interp(tp);
    rp *= o;
    rp += parent->location.interp(tp);
    rp += parent->dlocation;
  }
  return rp;
}


/*
 * src_object_t
 */
src_object_t::src_object_t(object_t* reference_)
  : object_t(),
    reference(reference_),
    startframe(0)
{
}

void src_object_t::prepare(double fs, uint32_t fragsize)
{
  //DEBUG(this);
  for(std::vector<Input::base_t*>::iterator it=inputs.begin();it!=inputs.end();++it){
    //DEBUG(*it);
    if( *it )
      (*it)->prepare(fs,fragsize);
  }
  //DEBUG(this);
  for(std::vector<sound_t>::iterator it=sound.begin();it!=sound.end();++it){
    it->prepare(fs,fragsize);
  }
  startframe = fs*starttime;
}

void src_object_t::set_reference(object_t* ref)
{
  //DEBUG(ref);
  reference = ref;
  for(std::vector<sound_t>::iterator it=sound.begin();it!=sound.end();++it){
    //DEBUGMSG("set reference/parent");
    it->set_reference(ref);
    it->set_parent(this);
  }
}

Input::base_t* src_object_t::get_input(const std::string& name)
{
  for( std::vector<TASCAR::Input::base_t*>::iterator it=inputs.begin();it!=inputs.end();++it)
    if( (*it)->name == name )
      return *it;
  return NULL;
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
    if( sne && (sne->get_name() == "inputs") ){
      xmlpp::Node::NodeList insubnodes = sne->get_children();
      for(xmlpp::Node::NodeList::iterator insn=insubnodes.begin();insn!=insubnodes.end();++insn){
        xmlpp::Element* insne(dynamic_cast<xmlpp::Element*>(*insn));
        if( insne ){
          if( insne->get_name() == "file" ){
            inputs.push_back(new TASCAR::Input::file_t());
            (*inputs.rbegin())->read_xml(insne);
          }
          if( insne->get_name() == "jack" ){
            inputs.push_back(new TASCAR::Input::jack_t(get_name()));
            (*inputs.rbegin())->read_xml(insne);
          }
        }
      }
    }
  }
}

void src_object_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  object_t::write_xml(e,help_comments);
  if( inputs.size()){
    xmlpp::Element* in_node = e->add_child("inputs");
    for( std::vector<TASCAR::Input::base_t*>::iterator it=inputs.begin();it!=inputs.end();++it){
      (*it)->write_xml(in_node->add_child((*it)->get_tag()));
    }
  }
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

void src_object_t::fill(int32_t tp_firstframe, bool tp_running)
{
  for( std::vector<TASCAR::Input::base_t*>::iterator it=inputs.begin();it!=inputs.end();++it){
    (*it)->fill(tp_firstframe-startframe,tp_running);
  }
}

scene_t::scene_t()
  : description(""),
    name(""),
    duration(60),
    lat(53.155473),
    lon(8.167249),
    elev(10),
    guiscale(200),anysolo(0)
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
  r << "  " << srcobjects.size() << " source objects\n";
  r << "  " << bg_amb.size() << " backgrounds\n";
  for(std::vector<src_object_t>::iterator it=srcobjects.begin();it!=srcobjects.end();++it)
    r << it->print(prefix+"  ");
  for(std::vector<bg_amb_t>::iterator it=bg_amb.begin();it != bg_amb.end();++it)
    r << it->print(prefix+"  ");
  r << listener.print(prefix+"  ");
  return r.str();
}

listener_t::listener_t()
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
  set_attribute_double(e,"guiscale",guiscale);
  set_attribute_double(e,"duration",duration);
  if( description.size()){
    xmlpp::Element* description_node = e->add_child("description");
    description_node->add_child_text(description);
  }
  bool b_first(true);
  for(std::vector<src_object_t>::iterator it=srcobjects.begin();it!=srcobjects.end();++it){
    it->write_xml(e->add_child("src_object"),help_comments && b_first);
    b_first = false;
  }
  b_first = true;
  for(std::vector<bg_amb_t>::iterator it=bg_amb.begin();it!=bg_amb.end();++it){
    it->write_xml(e->add_child("bg_amb"),help_comments && b_first);
    b_first = false;
  }
  b_first = true;
  for(std::vector<diffuse_reverb_t>::iterator it=reverbs.begin();it!=reverbs.end();++it){
    it->write_xml(e->add_child("reverb"),help_comments && b_first);
    b_first = false;
  }
  b_first = true;
  for(std::vector<face_object_t>::iterator it=faces.begin();it!=faces.end();++it){
    it->write_xml(e->add_child("face"),help_comments && b_first);
    b_first = false;
  }
  b_first = true;
  for(std::vector<range_t>::iterator it=ranges.begin();it!=ranges.end();++it){
    it->write_xml(e->add_child("range"),help_comments && b_first);
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
  get_attribute_value(e,"guiscale",guiscale);
  description = xml_get_text(e,"description");
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne ){
      if( sne->get_name() == "src_object" ){
        srcobjects.push_back(src_object_t(&listener));
        srcobjects.rbegin()->read_xml(sne);
      }
      if( sne->get_name() == "bg_amb" ){
        bg_amb.push_back(bg_amb_t());
        bg_amb.rbegin()->read_xml(sne);
      }
      if( sne->get_name() == "listener" ){
        listener.read_xml(sne);
      }
      if( sne->get_name() == "reverb" ){
        reverbs.push_back(diffuse_reverb_t());
        reverbs.rbegin()->read_xml(sne);
      }
      if( sne->get_name() == "face" ){
        faces.push_back(face_object_t());
        faces.rbegin()->read_xml(sne);
      }
      if( sne->get_name() == "range" ){
        ranges.push_back(range_t());
        ranges.rbegin()->read_xml(sne);
      }
    }
  }
}

src_object_t* scene_t::add_source()
{
  srcobjects.push_back(src_object_t(&listener));
  return &srcobjects.back();
}

void scene_t::read_xml(const std::string& filename)
{
  xmlpp::DomParser domp(filename);
  xmlpp::Document* doc(domp.get_document());
  xmlpp::Element* root(doc->get_root_node());
  read_xml(root);
}

std::vector<sound_t*> scene_t::linearize_sounds()
{
  //DEBUGMSG("lin sounds");
  std::vector<sound_t*> r;
  for(std::vector<src_object_t>::iterator it=srcobjects.begin();it!=srcobjects.end();++it){
    it->set_reference(&listener);
    for(std::vector<sound_t>::iterator its=it->sound.begin();its!=it->sound.end();++its){
      if( &(*its) )
        r.push_back(&(*its));
    }
  }
  return r;
}

std::vector<Input::base_t*> scene_t::linearize_inputs()
{
  //DEBUGMSG("lin inputs");
  std::vector<Input::base_t*> r;
  for(std::vector<src_object_t>::iterator it=srcobjects.begin();it!=srcobjects.end();++it){
    it->set_reference(&listener);
    for(std::vector<Input::base_t*>::iterator its=it->inputs.begin();its!=it->inputs.end();++its){
      if( (*its ) )
        r.push_back((*its));
    }
  }
  return r;
}


std::vector<Input::jack_t*> scene_t::get_jack_inputs()
{
  std::vector<Input::jack_t*> r;
  for(std::vector<src_object_t>::iterator it=srcobjects.begin();it!=srcobjects.end();++it){
    for(std::vector<Input::base_t*>::iterator its=it->inputs.begin();its!=it->inputs.end();++its){
      if( (*its ) ){
        Input::jack_t* ji(dynamic_cast<TASCAR::Input::jack_t*>(*its));
        if( ji )
          r.push_back(ji);
      }
    }
  }
  return r;
}

void scene_t::prepare(double fs, uint32_t fragsize)
{
  //DEBUG(1);
  for(std::vector<src_object_t>::iterator it=srcobjects.begin();it!=srcobjects.end();++it){
    //DEBUG(it->name);
    it->prepare(fs,fragsize);
  }
  //DEBUG(1);
  for(std::vector<bg_amb_t>::iterator it=bg_amb.begin();it!=bg_amb.end();++it)
    it->prepare(fs,fragsize);
}

void scene_t::set_mute(const std::string& name,bool val)
{
  for(std::vector<src_object_t>::iterator it=srcobjects.begin();it!=srcobjects.end();++it)
    if( it->get_name() == name )
      it->set_mute(val);
  for(std::vector<bg_amb_t>::iterator it=bg_amb.begin();it!=bg_amb.end();++it)
    if( it->get_name() == name )
      it->set_mute(val);
  for(std::vector<diffuse_reverb_t>::iterator it=reverbs.begin();it!=reverbs.end();++it)
    if( it->get_name() == name )
      it->set_mute(val);
  for(std::vector<face_object_t>::iterator it=faces.begin();it!=faces.end();++it)
    if( it->get_name() == name )
      it->set_mute(val);
  if( listener.get_name() == name )
    listener.set_mute(val);
}

void scene_t::set_solo(const std::string& name,bool val)
{
  for(std::vector<src_object_t>::iterator it=srcobjects.begin();it!=srcobjects.end();++it)
    if( it->get_name() == name )
      it->set_solo(val,anysolo);
  for(std::vector<bg_amb_t>::iterator it=bg_amb.begin();it!=bg_amb.end();++it)
    if( it->get_name() == name )
      it->set_solo(val,anysolo);
  for(std::vector<diffuse_reverb_t>::iterator it=reverbs.begin();it!=reverbs.end();++it)
    if( it->get_name() == name )
      it->set_solo(val,anysolo);
  for(std::vector<face_object_t>::iterator it=faces.begin();it!=faces.end();++it)
    if( it->get_name() == name )
      it->set_solo(val, anysolo);
  if( listener.get_name() == name )
    listener.set_solo(val,anysolo);
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

std::string sound_t::getlabel()
{
  std::string r;
  if( parent ){
    r = parent->get_name() + ".";
  }else{
    r = "<NULL>.";
  }
  r += name;
  return r;
}


/*
 *TASCAR::Input::file_t
 */
TASCAR::Input::file_t::file_t()
  : async_sndfile_t(1,1<<18,4096),
    filename(""),
    gain(0),
    loop(1),
    starttime(0),
    firstchannel(0)
{
}

void TASCAR::Input::file_t::read_xml(xmlpp::Element* e)
{
  base_t::read_xml(e);
  get_attribute_value(e,"start",starttime);
  filename = e->get_attribute_value("filename");
  get_attribute_value(e,"gain",gain);
  get_attribute_value(e,"loop",loop);
  get_attribute_value(e,"channel",firstchannel);
}

void TASCAR::Input::file_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  base_t::write_xml(e,help_comments);
  e->set_attribute("filename",filename);
  set_attribute_double(e,"gain",gain);
  set_attribute_uint(e,"loop",loop);
  set_attribute_double(e,"start",starttime);
  set_attribute_uint(e,"channel",firstchannel);
}

std::string TASCAR::Input::file_t::print(const std::string& prefix)
{
  std::stringstream r;
  r << prefix << "Filename: \"" << filename << "\"\n";
  r << prefix << "Gain: " << gain << " dB\n";
  r << prefix << "Loop: " << loop << "\n";
  r << prefix << "Starttime: " << starttime << " s\n";
  return r.str();
}

void TASCAR::Input::file_t::prepare(double fs, uint32_t fragsize)
{
  base_t::prepare(fs,fragsize);
  open(filename,firstchannel,(uint32_t)(starttime*fs),pow(10.0,0.05*gain),loop);
  start_service();
}

void TASCAR::Input::file_t::fill(int32_t tp_firstframe, bool tp_running)
{
  memset( data, 0, sizeof(float)*size );
  if( tp_running ){
    request_data( tp_firstframe, size, 1, &data );
  }else{
    request_data( tp_firstframe, 0, 1, &data );
  }
}


TASCAR::Input::base_t::base_t()
  : size(0),data(NULL)
{
}

TASCAR::Input::base_t::~base_t()
{
  if( data )
    delete [] data;
}

void TASCAR::Input::base_t::prepare(double fs, uint32_t fragsize)
{
  if( data )
    delete [] data;
  data = new float[fragsize];
  size = fragsize;
  memset(data,0,sizeof(float)*size);
}

 
void TASCAR::Input::base_t::read_xml(xmlpp::Element* e)
{
  name = e->get_attribute_value("name");
}

void TASCAR::Input::base_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  e->set_attribute("name",name);
}

float* TASCAR::Input::base_t::get_buffer(uint32_t n)
{
  if( n == size )
    return data;
  return NULL;
}


diffuse_reverb_t::diffuse_reverb_t()
  : size(2,2,2)
{
}

double diffuse_reverb_t::border_distance(pos_t p)
{
  p -= center;
  p /= orientation;
  p.x = fabs(p.x);
  p.y = fabs(p.y);
  p.z = fabs(p.z);
  pos_t size2(size);
  size2 *= 0.5;
  p -= size2;
  p.x = std::max(0.0,p.x);
  p.y = std::max(0.0,p.y);
  p.z = std::max(0.0,p.z);
  return p.norm();
}

void diffuse_reverb_t::read_xml(xmlpp::Element* e)
{
  route_t::read_xml(e);
  get_attribute_value(e,"center_x",center.x);
  get_attribute_value(e,"center_y",center.y);
  get_attribute_value(e,"center_z",center.z);
  get_attribute_value(e,"size_x",size.x);
  get_attribute_value(e,"size_y",size.y);
  get_attribute_value(e,"size_z",size.z);
  get_attribute_value_deg(e,"orientation_x",orientation.x);
  get_attribute_value_deg(e,"orientation_y",orientation.y);
  get_attribute_value_deg(e,"orientation_z",orientation.z);
}

void diffuse_reverb_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  route_t::write_xml(e,help_comments);
  set_attribute_double(e,"center_x",center.x);
  set_attribute_double(e,"center_y",center.y);
  set_attribute_double(e,"center_z",center.z);
  set_attribute_double(e,"size_x",size.x);
  set_attribute_double(e,"size_y",size.y);
  set_attribute_double(e,"size_z",size.z);
  set_attribute_double(e,"orientation_x",RAD2DEG*orientation.x);
  set_attribute_double(e,"orientation_y",RAD2DEG*orientation.y);
  set_attribute_double(e,"orientation_z",RAD2DEG*orientation.z);
}


std::string diffuse_reverb_t::print(const std::string& prefix)
{
  std::stringstream r;
  r << route_t::print(prefix);
  r << prefix << "Center: " << center.print_cart() << " s\n";
  r << prefix << "Size: " << size.print_cart() << " s\n";
  r << prefix << "Orientation: " << orientation.print() << " s\n";
  return r.str();
}

route_t::route_t()
  : mute(false),solo(false)
{
}

void route_t::read_xml(xmlpp::Element* e)
{
  name = e->get_attribute_value("name");
  get_attribute_value(e,"mute",mute);
  get_attribute_value(e,"solo",solo);
}

void route_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  e->set_attribute("name",name);
  set_attribute_bool(e,"mute",mute);
  set_attribute_bool(e,"solo",solo);
}

std::string route_t::print(const std::string& prefix)
{
  std::stringstream r;
  r << prefix << "Name: \"" << name << "\"\n";
  return r.str();
}

void route_t::set_solo(bool b,uint32_t& anysolo)
{
  if( b != solo ){
    if( b ){
      anysolo++;
    }else{
      if( anysolo )
        anysolo--;
    }
    solo=b;
  }
}

bool sound_t::get_mute() const 
{
  if( parent ) 
    return parent->get_mute();
  return false;
}

bool sound_t::get_solo() const 
{
  if( parent ) 
    return parent->get_solo();
  return false;
}

bool scene_t::get_playsound(const sound_t* s)
{
  if( s->get_mute() )
    return false;
  if( !anysolo )
    return true;
  return s->get_solo();
}

face_object_t::face_object_t()
  : width(1.0),
    height(1.0),
    reflectivity(1.0),
    damping(0.0)
{
}

pos_t face_object_t::get_closest_point(double t, pos_t p)
{
  pos_t center(location.interp(t));
  center += dlocation;
  zyx_euler_t o(orientation.interp(t));
  o += dorientation;
  p -= center;
  p /= o;
  p.x = 0;
  double w1(0.5*width);
  double h1(0.5*height);
  p.y = std::min(std::max(p.y,-w1),w1);
  p.z = std::min(std::max(p.z,-h1),h1);
  p *= o;
  p += center;
  return p;
}

mirror_t face_object_t::get_mirror(double t, pos_t src)
{
  pos_t cp(src);
  pos_t center(location.interp(t));
  center += dlocation;
  zyx_euler_t o(orientation.interp(t));
  o += dorientation;
  cp -= center;
  cp /= o;
  double w1(0.5*width);
  double h1(0.5*height);
  mirror_t mirror;
  mirror.p = cp;
  mirror.p.x *= -1.0;
  cp.y = std::min(std::max(cp.y,-w1),w1);
  cp.z = std::min(std::max(cp.z,-h1),h1);
  cp.x = 0;
  cp *= o;
  cp += center;
  mirror.p *= o;
  mirror.p += center;
  cp -= src;
  cp *= -1.0/cp.norm();
  pos_t n(1,0,0);
  n *= o;
  mirror.c1 = std::max(n.x*cp.x+n.y*cp.y+n.z*cp.z,0.0);
  mirror.c2 = std::min(0.9999,(1.0-(mirror.c1*(1.0-damping))));
  mirror.c1 *= mirror.c1;
  mirror.c1 *= reflectivity;
  return mirror;
}

void face_object_t::prepare(double fs, uint32_t fragsize)
{
}

void face_object_t::read_xml(xmlpp::Element* e)
{
  object_t::read_xml(e);
  get_attribute_value(e,"width",width);
  get_attribute_value(e,"height",height);
  get_attribute_value(e,"reflectivity",reflectivity);
  get_attribute_value(e,"damping",damping);
}

void face_object_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  object_t::write_xml(e,help_comments);
  set_attribute_double(e,"width",width);
  set_attribute_double(e,"height",height);
  set_attribute_double(e,"reflectivity",reflectivity);
  set_attribute_double(e,"damping",damping);
}

void TASCAR::Input::jack_t::write(uint32_t n,float* b)
{
  memcpy(data,b,sizeof(float)*std::min(n,size));
}

void TASCAR::Input::jack_t::read_xml(xmlpp::Element* e)
{
  base_t::read_xml(e);
  connections.clear();
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "connection")){
      connections.push_back(sne->get_attribute_value("port"));
    }
  }
}

void TASCAR::Input::jack_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  base_t::write_xml(e,help_comments);
  for( std::vector<std::string>::iterator it=connections.begin();it!=connections.end();++it){
    xmlpp::Element* se(e->add_child("connection"));
    se->set_attribute("port",*it);
  }
}

std::string TASCAR::Input::jack_t::print(const std::string& prefix)
{
  std::stringstream r;
  r << prefix << "Name: \"" << name << "\"\n";
  return r.str();
}

TASCAR::Input::jack_t::jack_t(const std::string& parent_name_)
  : parent_name(parent_name_)
{
}

TASCAR::range_t::range_t()
  : name(""),
    start(0),
    end(0)
{
}

void TASCAR::range_t::read_xml(xmlpp::Element* e)
{
  name = e->get_attribute_value("name");
  get_attribute_value(e,"start",start);
  get_attribute_value(e,"end",end);
}

void TASCAR::range_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  e->set_attribute("name",name);
  set_attribute_double(e,"start",start);
  set_attribute_double(e,"end",end);
}

void scene_t::set_source_position_offset(const std::string& srcname,pos_t position)
{
  for(std::vector<src_object_t>::iterator it=srcobjects.begin();it != srcobjects.end(); ++it){
    if( srcname == it->get_name() )
      it->dlocation = position;
  }
}

void scene_t::set_source_orientation_offset(const std::string& srcname,zyx_euler_t orientation)
{
  for(std::vector<src_object_t>::iterator it=srcobjects.begin();it != srcobjects.end(); ++it){
    if( srcname == it->get_name() )
      it->dorientation = orientation;
  }
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
