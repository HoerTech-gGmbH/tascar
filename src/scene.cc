#include "scene.h"
#include "errorhandling.h"
#include <libxml++/libxml++.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "defs.h"
#include <string.h>
#include "sinks.h"
#include <locale.h>

using namespace TASCAR;
using namespace TASCAR::Scene;

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
    if( sne && ( sne->get_name() == "sndfile")){
      sndfiles.push_back(sndfile_info_t());
      sndfiles.back().read_xml(sne);
      sndfiles.back().parentname = get_name();
      sndfiles.back().starttime += starttime;
      sndfiles.back().objectchannel = sndfiles.size()-1;
    }
  }
}

void object_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  route_t::write_xml(e,help_comments);
  if( color.str() != "#000000" )
    e->set_attribute("color",color.str());
  if( !((starttime==0.0) && (endtime==0.0)) ){
    set_attribute_double(e,"start",starttime);
    set_attribute_double(e,"end",endtime);
  }
  if( location.size() ){
    location.write_xml( e->add_child("position") );
  }
  if( orientation.size() ){
    orientation.write_xml( e->add_child("orientation") );
  }
  for( std::vector<sndfile_info_t>::iterator it=sndfiles.begin();it!=sndfiles.end();++it)
    it->write_xml( e->add_child("sndfile") );
}

/*
 *src_diffuse_t
 */
src_diffuse_t::src_diffuse_t()
  : size(1,1,1),
    falloff(1.0),
    source(NULL)
{
}

void src_diffuse_t::read_xml(xmlpp::Element* e)
{
  object_t::read_xml(e);
  jack_port_t::read_xml(e);
  get_attribute_value(e,"size",size);
  get_attribute_value(e,"falloff",falloff);
}

void src_diffuse_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  object_t::write_xml(e,help_comments);
  jack_port_t::write_xml(e);
  set_attribute_value(e,"size",size);
  set_attribute_double(e,"falloff",falloff);
}

/*
 *src_door_t
 */
src_door_t::src_door_t()
  : width(1.0),
    height(2.0),
    falloff(1.0),
    distance(1.0),
    source(NULL)
{
}

void src_door_t::read_xml(xmlpp::Element* e)
{
  object_t::read_xml(e);
  jack_port_t::read_xml(e);
  get_attribute_value(e,"width",width);
  get_attribute_value(e,"height",height);
  get_attribute_value(e,"falloff",falloff);
  get_attribute_value(e,"distance",distance);
}

void src_door_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  object_t::write_xml(e,help_comments);
  jack_port_t::write_xml(e);
  set_attribute_double(e,"width",width);
  set_attribute_double(e,"height",height);
  set_attribute_double(e,"falloff",falloff);
}

src_door_t::~src_door_t()
{
  if( source )
    delete source;
}

void src_door_t::geometry_update(double t)
{
  if( source ){
    source->position = get_location(t);
    source->set(source->position,get_orientation(t),width,height);
  }
}


void src_door_t::prepare(double fs, uint32_t fragsize)
{
  if( source )
    delete source;
  source = new TASCAR::Acousticmodel::doorsource_t(fragsize);
  source->position = get_location(0);
  source->set(source->position,get_orientation(0),width,height);
  source->falloff = 1.0/std::max(falloff,1.0e-10);
  source->distance = distance;
}

/*
 * sound_t
 */
sound_t::sound_t(src_object_t* parent_)
  : local_position(0,0,0),
    chaindist(0),
    parent(parent_),
    direct(true),
    source(NULL)
{
}

sound_t::sound_t(const sound_t& src)
  : scene_node_base_t(src),
    jack_port_t(src),
    local_position(src.local_position),
    chaindist(src.chaindist),
    parent(NULL),
    direct(src.direct),
    source(NULL)
{
}

sound_t::~sound_t()
{
  if( source )
    delete source;
}

void sound_t::geometry_update(double t)
{
  if( source ){
    source->position = get_pos_global(t);
    source->direct = direct;
  }
}

void sound_t::prepare(double fs, uint32_t fragsize)
{
  if( source )
    delete source;
  source = new TASCAR::Acousticmodel::pointsource_t(fragsize);
}

src_diffuse_t::~src_diffuse_t()
{
  if( source )
    delete source;
}

void src_diffuse_t::geometry_update(double t)
{
  if( source ){
    source->center = get_location(t);
    source->orientation = get_orientation(t);
  }
}

void sink_object_t::geometry_update(double t)
{
  if( sink ){
    sink->position = get_location(t);
    sink->orientation = get_orientation(t);
    sink->diffusegain = diffusegain;
    sink->is_direct = is_direct;
    if( use_mask ){
      sink->mask.center = mask.get_location(t);
      sink->mask.orientation = mask.get_orientation(t);
    }
  }
}

void src_diffuse_t::prepare(double fs, uint32_t fragsize)
{
  if( source )
    delete source;
  source = new TASCAR::Acousticmodel::diffuse_source_t(fragsize);
  source->size = size;
  source->falloff = 1.0/std::max(falloff,1.0e-10);
  for( std::vector<sndfile_info_t>::iterator it=sndfiles.begin();it!=sndfiles.end();++it)
    if( it->channels != 4 )
      throw TASCAR::ErrMsg("Diffuse sources support only 4-channel (FOA) sound files ("+it->fname+").");
}

void sound_t::set_parent(src_object_t* ref)
{
  //DEBUG(ref);
  parent = ref;
}

std::string sound_t::get_parent_name() const
{
  if( parent )
    return parent->get_name();
  return "";
}

std::string sound_t::get_port_name() const
{
  if( parent )
    return parent->get_name() + "." + name;
  return name;
}

void jack_port_t::set_port_index(uint32_t port_index_)
{
  port_index = port_index_;
}

void sound_t::read_xml(xmlpp::Element* e)
{
  jack_port_t::read_xml(e);
  get_attribute_value(e,"x",local_position.x);
  get_attribute_value(e,"y",local_position.y);
  get_attribute_value(e,"z",local_position.z);
  get_attribute_value(e,"d",chaindist);
  get_attribute_value_bool(e,"direct",direct);
  name = e->get_attribute_value("name");
}

void sound_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  e->set_attribute("name",name);
  if( local_position.x != 0 )
    set_attribute_double(e,"x",local_position.x);
  if( local_position.y != 0 )
    set_attribute_double(e,"y",local_position.y);
  if( local_position.z != 0 )
    set_attribute_double(e,"z",local_position.z);
  if( chaindist != 0 )
    set_attribute_double(e,"d",chaindist);
  jack_port_t::write_xml(e);
}

bool sound_t::isactive(double t)
{
  return parent && parent->isactive(t);
}

pos_t sound_t::get_pos_global(double t) const
{
  pos_t rp(local_position);
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
src_object_t::src_object_t()
  : object_t(),
    //reference(reference_),
    startframe(0)
{
}

void src_object_t::geometry_update(double t)
{
  for(std::vector<sound_t>::iterator it=sound.begin();it!=sound.end();++it){
    it->geometry_update(t);
  }
}

void src_object_t::prepare(double fs, uint32_t fragsize)
{
  for(std::vector<sound_t>::iterator it=sound.begin();it!=sound.end();++it){
    it->prepare(fs,fragsize);
  }
  startframe = fs*starttime;
}

void src_object_t::read_xml(xmlpp::Element* e)
{
  object_t::read_xml(e);
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "sound" )){
      sound.push_back(sound_t(this));
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

sound_t* src_object_t::add_sound()
{
  sound.push_back(sound_t(this));
  return &sound.back();
}

scene_t::scene_t()
  : description(""),
    name(""),
    duration(60),mirrororder(1),
    guiscale(200),anysolo(0),loop(false)
{
}

scene_t::scene_t(const std::string& filename)
  : description(""),
    name(""),
    duration(60),mirrororder(1),
    guiscale(200),anysolo(0),loop(false)
{
  setlocale(LC_ALL,"C");
  if( filename.size() )
    read_xml(TASCAR::env_expand(filename));
}

void scene_t::geometry_update(double t)
{
  for(std::vector<src_object_t>::iterator it=object_sources.begin();it!=object_sources.end();++it)
    it->geometry_update(t);
  for(std::vector<src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it)
    it->geometry_update(t);
  for(std::vector<sink_object_t>::iterator it=sink_objects.begin();it!=sink_objects.end();++it)
    it->geometry_update(t);
  for(std::vector<face_object_t>::iterator it=faces.begin();it!=faces.end();++it)
    it->geometry_update(t);
  for(std::vector<mask_object_t>::iterator it=masks.begin();it!=masks.end();++it)
    it->geometry_update(t);
}

void scene_t::process_active(double t)
{
  //DEBUGS(anysolo);
  for(std::vector<src_object_t>::iterator it=object_sources.begin();it!=object_sources.end();++it)
    it->process_active(t,anysolo);
  for(std::vector<src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it)
    it->process_active(t,anysolo);
  for(std::vector<src_door_t>::iterator it=door_sources.begin();it!=door_sources.end();++it)
    it->process_active(t,anysolo);
  for(std::vector<sink_object_t>::iterator it=sink_objects.begin();it!=sink_objects.end();++it)
    it->process_active(t,anysolo);
  for(std::vector<face_object_t>::iterator it=faces.begin();it!=faces.end();++it)
    it->process_active(t,anysolo);
}

sink_object_t::sink_object_t()
  : sink_type(omni),
    falloff(-1.0),
    render_point(true),
    render_diffuse(true),
    is_direct(true),
    diffusegain(1.0),
    use_mask(false),
    use_global_mask(true),
    sink(NULL)
{
}

sink_object_t::sink_object_t(const sink_object_t& src)
  : object_t(src),
    jack_port_t(src),
    sink_type(src.sink_type),
    falloff(src.falloff),
    render_point(src.render_point),
    render_diffuse(src.render_diffuse),
    is_direct(src.is_direct),
    diffusegain(src.diffusegain),
    use_mask(src.use_mask),
    use_global_mask(src.use_global_mask),
    sink(NULL)
{
}

sink_object_t::~sink_object_t()
{
  if( sink )
    delete sink;
}

void sink_mask_t::read_xml(xmlpp::Element* e)
{
  object_t::read_xml(e);
  get_attribute_value(e,"size",size);
  get_attribute_value(e,"falloff",falloff);
}

sink_mask_t::sink_mask_t()
  : falloff(1.0)
{
}

void sink_mask_t::prepare(double fs, uint32_t fragsize)
{
}

void mask_object_t::read_xml(xmlpp::Element* e)
{
  object_t::read_xml(e);
  get_attribute_value(e,"size",xmlsize);
  get_attribute_value(e,"falloff",xmlfalloff);
  get_attribute_value_bool(e,"inside",mask_inner);
}

mask_object_t::mask_object_t()
{
}

void mask_object_t::prepare(double fs, uint32_t fragsize)
{
}

void mask_object_t::geometry_update(double t)
{
  shoebox_t::size.x = std::max(0.0,xmlsize.x-xmlfalloff);
  shoebox_t::size.y = std::max(0.0,xmlsize.y-xmlfalloff);
  shoebox_t::size.z = std::max(0.0,xmlsize.z-xmlfalloff);
  shoebox_t::center = get_location(t);
  shoebox_t::orientation = get_orientation(t);
  falloff = 1.0/std::max(xmlfalloff,1e-10);
}

void sink_object_t::read_xml(xmlpp::Element* e)
{
  jack_port_t::read_xml(e);
  object_t::read_xml(e);
  get_attribute_value(e,"size",size);
  get_attribute_value_bool(e,"point",render_point);
  get_attribute_value_bool(e,"diffuse",render_diffuse);
  get_attribute_value_bool(e,"isdirect",is_direct);
  get_attribute_value_bool(e,"globalmask",use_global_mask);
  get_attribute_value_db(e,"diffusegain",diffusegain);
  get_attribute_value(e,"falloff",falloff);
  std::string stype(e->get_attribute_value("type"));
  if( stype == "omni" )
    sink_type = omni;
  else if( stype == "cardioid" )
    sink_type = cardioid;
  else if( stype == "amb3h3v" )
    sink_type = amb3h3v;
  else if( stype == "amb3h0v" )
    sink_type = amb3h0v;
  else if( stype == "nsp" )
    sink_type = nsp;
  else 
    throw TASCAR::ErrMsg("Unupported sink type: \""+stype+"\"");
  //  
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "speaker" )){
      double az(0.0);
      double elev(0.0);
      double r(1.0);
      get_attribute_value(sne,"az",az);
      get_attribute_value(sne,"el",elev);
      get_attribute_value(sne,"r",r);
      pos_t spk;
      spk.set_sphere(r,az*DEG2RAD,elev*DEG2RAD);
      spkpos.push_back(spk);
    }
    if( sne && ( sne->get_name() == "mask" )){
      use_mask = true;
      mask.read_xml(sne);
    }
  }
}

void sink_object_t::write_xml(xmlpp::Element* e)
{
  object_t::write_xml(e);
  jack_port_t::write_xml(e);
  if( (size.x != 0) || (size.y != 0) || (size.z != 0))
    set_attribute_value(e,"size",size);
  set_attribute_bool(e,"point",render_point);
  set_attribute_bool(e,"diffuse",render_diffuse);
  set_attribute_bool(e,"isdirect",is_direct);
  if( diffusegain != 0 )
    set_attribute_db(e,"diffusegain",diffusegain);
  if( falloff != -1 )
    set_attribute_double(e,"falloff",falloff);
  switch( sink_type ){
  case omni:
    e->set_attribute("type", "omni");
    break;
  case cardioid:
    e->set_attribute("type", "cardioid");
    break;
  case amb3h3v :
    e->set_attribute("type", "amb3h3v");
    break;
  case amb3h0v :
    e->set_attribute("type", "amb3h0v");
    break;
  case nsp :
    e->set_attribute("type", "nsp");
    break;
  }
  //
  if( use_mask ){
    mask.write_xml(e->add_child("mask"));
  }
  for( std::vector<TASCAR::pos_t>::iterator it=spkpos.begin();it!=spkpos.end();++it){
    xmlpp::Element* sne(e->add_child("speaker"));
    set_attribute_double(sne,"az",it->azim()*RAD2DEG);
    set_attribute_double(sne,"el",it->elev()*RAD2DEG);
    set_attribute_double(sne,"r",it->norm());
  }
}

void sink_object_t::prepare(double fs, uint32_t fragsize)
{
  //DEBUG(fragsize);
  if( sink )
    delete sink;
  switch( sink_type ){
  case omni :
    sink = new TASCAR::Acousticmodel::sink_omni_t(fragsize,size,falloff,render_point,render_diffuse,
                                                  mask.size,
                                                  mask.falloff,
                                                  use_mask,use_global_mask);
    break;
  case cardioid :
    sink = new TASCAR::Acousticmodel::sink_cardioid_t(fragsize,size,falloff,render_point,render_diffuse,
                                                      mask.size,
                                                      mask.falloff,
                                                      use_mask,use_global_mask);
    break;
  case amb3h3v :
    sink = new TASCAR::Acousticmodel::sink_amb3h3v_t(fragsize,size,falloff,render_point,render_diffuse,
                                                     mask.size,
                                                     mask.falloff,
                                                     use_mask,use_global_mask);
    break;
  case amb3h0v :
    sink = new TASCAR::Acousticmodel::sink_amb3h0v_t(fragsize,size,falloff,render_point,render_diffuse,
                                                  mask.size,
                                                  mask.falloff,
                                                  use_mask,use_global_mask);
    break;
  case nsp :
    sink = new TASCAR::Acousticmodel::sink_nsp_t(fragsize,size,falloff,render_point,render_diffuse,
                                                  mask.size,
                                                  mask.falloff,
                                                  use_mask,use_global_mask,spkpos);
    break;
  }
}

void scene_t::write_xml(xmlpp::Element* e, bool help_comments)
{
  e->set_attribute("name",name);
  if( help_comments )
    e->add_child_comment(
      "\n"
      "A scene describes the spatial and acoustical information.\n"
      "Sub-tags are src_object, listener and diffuse_sources.\n");
  set_attribute_double(e,"duration",duration);
  set_attribute_double(e,"guiscale",guiscale);
  set_attribute_value(e,"guicenter",guicenter);
  set_attribute_bool(e,"loop",loop);
  if( description.size()){
    xmlpp::Element* description_node = e->add_child("description");
    description_node->add_child_text(description);
  }
  bool b_first(true);
  for(std::vector<range_t>::iterator it=ranges.begin();it!=ranges.end();++it){
    it->write_xml(e->add_child("range"),help_comments && b_first);
    b_first = false;
  }
  b_first = true;
  for(std::vector<src_object_t>::iterator it=object_sources.begin();it!=object_sources.end();++it){
    it->write_xml(e->add_child("src_object"),help_comments && b_first);
    b_first = false;
  }
  b_first = true;
  for(std::vector<src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it){
    it->write_xml(e->add_child("diffuse"),help_comments && b_first);
    b_first = false;
  }
  b_first = true;
  for(std::vector<sink_object_t>::iterator it=sink_objects.begin();it!=sink_objects.end();++it){
    it->write_xml(e->add_child("sink"));
    b_first = false;
  }
  b_first = true;
  for(std::vector<face_object_t>::iterator it=faces.begin();it!=faces.end();++it){
    it->write_xml(e->add_child("face"),help_comments && b_first);
    b_first = false;
  }
  for(std::vector<connection_t>::iterator it=connections.begin();it!=connections.end();++it){
    xmlpp::Element* sne(e->add_child("connect"));
    sne->set_attribute("src",it->src.c_str());
    sne->set_attribute("dest",it->dest.c_str());
  }
}

void scene_t::read_xml(xmlpp::Element* e)
{
  if( e->get_name() != "scene" )
    throw ErrMsg("Invalid file, XML root node should be \"scene\".");
  name = e->get_attribute_value("name");
  get_attribute_value(e,"mirrororder",mirrororder);
  get_attribute_value(e,"duration",duration);
  get_attribute_value(e,"guiscale",guiscale);
  get_attribute_value(e,"guicenter",guicenter);
  get_attribute_value_bool(e,"loop",loop);
  description = xml_get_text(e,"description");
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne ){
      if( sne->get_name() == "src_object" ){
        object_sources.push_back(src_object_t());
        object_sources.rbegin()->read_xml(sne);
      }
      if( sne->get_name() == "door" ){
        door_sources.push_back(src_door_t());
        door_sources.rbegin()->read_xml(sne);
      }
      if( sne->get_name() == "diffuse" ){
        diffuse_sources.push_back(src_diffuse_t());
        diffuse_sources.rbegin()->read_xml(sne);
      }
      if( sne->get_name() == "sink" ){
        sink_objects.push_back(sink_object_t());
        sink_objects.rbegin()->read_xml(sne);
      }
      if( sne->get_name() == "face" ){
        faces.push_back(face_object_t());
        faces.rbegin()->read_xml(sne);
      }
      if( sne->get_name() == "range" ){
        ranges.push_back(range_t());
        ranges.rbegin()->read_xml(sne);
      }
      if( sne->get_name() == "mask" ){
        masks.push_back(mask_object_t());
        masks.rbegin()->read_xml(sne);
      }
      if( sne->get_name() == "connect" ){
        connection_t c;
        c.src = sne->get_attribute_value("src");
        c.dest = sne->get_attribute_value("dest");
        connections.push_back(c);
      }
      if( sne->get_name() == "include" ){
        std::string fname(sne->get_attribute_value("name"));
        scene_t inc_scene(fname);
#define COPYOBJ(x) x.insert(x.end(),inc_scene.x.begin(),inc_scene.x.end())
        COPYOBJ(object_sources);
        COPYOBJ(diffuse_sources);
        COPYOBJ(door_sources);
        COPYOBJ(faces);
        COPYOBJ(sink_objects);
      }
    }
  }
}

src_object_t* scene_t::add_source()
{
  object_sources.push_back(src_object_t());
  return &object_sources.back();
}

void scene_t::read_xml(const std::string& filename)
{
  xmlpp::DomParser domp(TASCAR::env_expand(filename));
  xmlpp::Document* doc(domp.get_document());
  xmlpp::Element* root(doc->get_root_node());
  read_xml(root);
}

std::vector<sound_t*> scene_t::linearize_sounds()
{
  //DEBUGMSG("lin sounds");
  std::vector<sound_t*> r;
  for(std::vector<src_object_t>::iterator it=object_sources.begin();it!=object_sources.end();++it){
    //it->set_reference(&listener);
    for(std::vector<sound_t>::iterator its=it->sound.begin();its!=it->sound.end();++its){
      if( &(*its) ){
        (&(*its))->set_parent(&(*it));
        r.push_back(&(*its));
      }
    }
  }
  return r;
}

std::vector<object_t*> scene_t::get_objects()
{
  std::vector<object_t*> r;
  for(std::vector<src_object_t>::iterator it=object_sources.begin();it!=object_sources.end();++it)
    r.push_back(&(*it));
  for(std::vector<src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it)
    r.push_back(&(*it));
  for(std::vector<src_door_t>::iterator it=door_sources.begin();it!=door_sources.end();++it)
    r.push_back(&(*it));
  for(std::vector<sink_object_t>::iterator it=sink_objects.begin();it!=sink_objects.end();++it)
    r.push_back(&(*it));
  for(std::vector<face_object_t>::iterator it=faces.begin();it!=faces.end();++it)
    r.push_back(&(*it));
  return r;
}

void scene_t::prepare(double fs, uint32_t fragsize)
{
  if( !name.size() )
    throw TASCAR::ErrMsg("Invalid empty scene name (please set \"name\" attribute of scene node).");
  if( name.find(" ") != std::string::npos )
    throw TASCAR::ErrMsg("Spaces in scene name are not supported (\""+name+"\")");
  if( name.find(":") != std::string::npos )
    throw TASCAR::ErrMsg("Colons in scene name are not supported (\""+name+"\")");
  if( (object_sources.size() == 0) && (diffuse_sources.size() == 0) )
    throw TASCAR::ErrMsg("No sound source in scene \""+name+"\".");
  if( sink_objects.size() == 0 )
    throw TASCAR::ErrMsg("No receiver in scene \""+name+"\".");
  for(std::vector<src_object_t>::iterator it=object_sources.begin();it!=object_sources.end();++it){
    it->prepare(fs,fragsize);
  }
  for(std::vector<sink_object_t>::iterator it=sink_objects.begin();it!=sink_objects.end();++it){
    it->prepare(fs,fragsize);
  }
  for(std::vector<src_door_t>::iterator it=door_sources.begin();it!=door_sources.end();++it){
    it->prepare(fs,fragsize);
  }
  for(std::vector<src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it){
    it->prepare(fs,fragsize);
  }
  for(std::vector<face_object_t>::iterator it=faces.begin();it!=faces.end();++it){
    it->prepare(fs,fragsize);
  }
}

void scene_t::set_mute(const std::string& name,bool val)
{
  for(std::vector<src_object_t>::iterator it=object_sources.begin();it!=object_sources.end();++it)
    if( it->get_name() == name )
      it->set_mute(val);
  for(std::vector<face_object_t>::iterator it=faces.begin();it!=faces.end();++it)
    if( it->get_name() == name )
      it->set_mute(val);
  for(std::vector<sink_object_t>::iterator it=sink_objects.begin();it!=sink_objects.end();++it)
    if( it->get_name() == name )
      it->set_mute(val);
}

void scene_t::set_solo(const std::string& name,bool val)
{
  for(std::vector<src_object_t>::iterator it=object_sources.begin();it!=object_sources.end();++it)
    if( it->get_name() == name )
      it->set_solo(val,anysolo);
  for(std::vector<face_object_t>::iterator it=faces.begin();it!=faces.end();++it)
    if( it->get_name() == name )
      it->set_solo(val, anysolo);
  for(std::vector<sink_object_t>::iterator it=sink_objects.begin();it!=sink_objects.end();++it)
    if( it->get_name() == name )
      it->set_solo(val,anysolo);
}

void TASCAR::Scene::xml_write_scene(const std::string& filename, scene_t scene, const std::string& comment, bool help_comments)
{ 
  xmlpp::Document doc;
  if( comment.size() )
    doc.add_comment(comment);
  xmlpp::Element* root(doc.create_root_node("scene"));
  scene.write_xml(root);
  doc.write_to_file_formatted(TASCAR::env_expand(filename));
}

scene_t TASCAR::Scene::xml_read_scene(const std::string& filename)
{
  scene_t s;
  xmlpp::DomParser domp(TASCAR::env_expand(filename));
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

std::string sound_t::getlabel() const
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

std::string sound_t::get_name() const
{
  return name;
}

route_t::route_t()
  : mute(false),solo(false)
{
}

void route_t::read_xml(xmlpp::Element* e)
{
  name = e->get_attribute_value("name");
  get_attribute_value_bool(e,"mute",mute);
  get_attribute_value_bool(e,"solo",solo);
}

void route_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  e->set_attribute("name",name);
  set_attribute_bool(e,"mute",mute);
  set_attribute_bool(e,"solo",solo);
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

bool route_t::is_active(uint32_t anysolo)
{
  return ( !mute && ( (anysolo==0)||(solo) ) );    
}


bool object_t::is_active(uint32_t anysolo,double t)
{
  return (route_t::is_active(anysolo) && (t >= starttime) && ((t <= endtime)||(endtime <= starttime) ));
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
    height(1.0)
{
}

face_object_t::~face_object_t()
{
}

void face_object_t::geometry_update(double t)
{
  set(get_location(t),get_orientation(t),width,height);
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

TASCAR::Scene::range_t::range_t()
  : name(""),
    start(0),
    end(0)
{
}

void TASCAR::Scene::range_t::read_xml(xmlpp::Element* e)
{
  name = e->get_attribute_value("name");
  get_attribute_value(e,"start",start);
  get_attribute_value(e,"end",end);
}

void TASCAR::Scene::range_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  e->set_attribute("name",name);
  set_attribute_double(e,"start",start);
  set_attribute_double(e,"end",end);
}

void scene_t::set_source_position_offset(const std::string& srcname,pos_t position)
{
  for(std::vector<src_object_t>::iterator it=object_sources.begin();it != object_sources.end(); ++it){
    if( srcname == it->get_name() )
      it->dlocation = position;
  }
}

void scene_t::set_source_orientation_offset(const std::string& srcname,zyx_euler_t orientation)
{
  for(std::vector<src_object_t>::iterator it=object_sources.begin();it != object_sources.end(); ++it){
    if( srcname == it->get_name() )
      it->dorientation = orientation;
  }
}

void jack_port_t::read_xml(xmlpp::Element* e)
{
  connect = e->get_attribute_value("connect");
  get_attribute_value_db_float(e,"gain",gain);
}

void jack_port_t::set_gain_db( float g )
{
  gain = pow(10.0,0.05*g);
}

void jack_port_t::write_xml(xmlpp::Element* e)
{
  if( connect.size() )
    e->set_attribute("connect",connect);
  if( gain != 0.0 )
    set_attribute_db(e,"gain",gain);
}

jack_port_t::jack_port_t()
  : portname(""),
    connect(""),
    port_index(0),
    gain(1)
{
}



sndfile_info_t::sndfile_info_t()
  : fname(""),
    firstchannel(0),
    channels(1),
    objectchannel(0),
    starttime(0),
    loopcnt(1),
    gain(1.0)
{
}
 
void sndfile_info_t::read_xml(xmlpp::Element* e)
{
  fname = e->get_attribute_value("name");
  get_attribute_value(e,"firstchannel",firstchannel);
  get_attribute_value(e,"channels",channels);
  get_attribute_value(e,"loop",loopcnt);
  get_attribute_value(e,"starttime",starttime);
  get_attribute_value_db(e,"gain",gain);
  // ensure that no old timimng convection was used:
  if( e->get_attribute("start") != 0 )
    throw TASCAR::ErrMsg("Invalid attribute \"start\" found. Did you mean \"starttime\"? ("+fname+").");
  if( e->get_attribute("channel") != 0 )
    throw TASCAR::ErrMsg("Invalid attribute \"channel\" found. Did you mean \"firstchannel\"? ("+fname+").");
}

void sndfile_info_t::write_xml(xmlpp::Element* e,bool help_comments)
{
  e->set_attribute("name",fname.c_str());
  if( firstchannel != 0 )
    set_attribute_uint(e,"firstchannel",firstchannel);
  if( channels != 1 )
    set_attribute_uint(e,"channels",channels);
  if( loopcnt != 1 )
    set_attribute_uint(e,"loop",loopcnt);
  if( gain != 0.0 )
    set_attribute_db(e,"gain",gain);
}

void src_object_t::process_active(double t, uint32_t anysolo)
{
  bool a(is_active(anysolo,t));
  //DEBUGS(a);
  for(std::vector<sound_t>::iterator it=sound.begin();it!=sound.end();++it)
    if( it->get_source() )
      it->get_source()->active = a;
}

void src_diffuse_t::process_active(double t, uint32_t anysolo)
{
  bool a(is_active(anysolo,t));
  //DEBUGS(a);
  if( source )
    source->active = a;
}

void sink_object_t::process_active(double t, uint32_t anysolo)
{
  bool a(is_active(anysolo,t));
  //DEBUGS(a);
  if( sink )
    sink->active = a;
}

void face_object_t::process_active(double t, uint32_t anysolo)
{
  bool a(is_active(anysolo,t));
  //DEBUGS(a);
  active = a;
}

void src_door_t::process_active(double t, uint32_t anysolo)
{
  bool a(is_active(anysolo,t));
  //DEBUGS(a);
  if( source )
    source->active = a;
}


std::string jacknamer(const std::string& jackname,const std::string& scenename, const std::string& base)
{
  if( jackname.size() )
    return jackname;
  return base+scenename;
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
