#include "scene.h"
#include "errorhandling.h"
#include <libxml++/libxml++.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "defs.h"
#include <string.h>
//#include "sinks.h"
#include <locale.h>
#include <libgen.h>
#include <unistd.h>
#include <fnmatch.h>

using namespace TASCAR;
using namespace TASCAR::Scene;

/*
 * object_t
 */
object_t::object_t(xmlpp::Element* src)
  : dynobject_t(src),
    route_t(src),
    endtime(0)
{
  dynobject_t::get_attribute("end",endtime);
  std::string scol;
  dynobject_t::get_attribute("color",scol);
  color = rgb_color_t(scol);
}

sndfile_object_t::sndfile_object_t(xmlpp::Element* xmlsrc)
  : object_t(xmlsrc)
{
  xmlpp::Node::NodeList subnodes = dynobject_t::e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "sndfile")){
      sndfiles.push_back(sndfile_info_t(sne));
      sndfiles.back().parentname = get_name();
      sndfiles.back().starttime += starttime;
      sndfiles.back().objectchannel = sndfiles.size()-1;
    }
  }
}

bool object_t::isactive(double time) const
{
  return (!get_mute())&&(time>=starttime)&&((starttime>=endtime)||(time<=endtime));
}

void object_t::write_xml()
{
  route_t::write_xml();
  if( color.str() != "#000000" )
    dynobject_t::set_attribute("color",color.str());
  dynobject_t::set_attribute("end",endtime);
}

void sndfile_object_t::write_xml()
{
  object_t::write_xml();
  for( std::vector<sndfile_info_t>::iterator it=sndfiles.begin();it!=sndfiles.end();++it)
    it->write_xml();
}


/*
 *src_diffuse_t
 */
src_diffuse_t::src_diffuse_t(xmlpp::Element* xmlsrc)
  : sndfile_object_t(xmlsrc),jack_port_t(xmlsrc),size(1,1,1),
    falloff(1.0),
    source(NULL)
{
  //object_t::read_xml(e);
  //jack_port_t::read_xml(e);
  dynobject_t::get_attribute("size",size);
  dynobject_t::get_attribute("falloff",falloff);
}

void src_diffuse_t::write_xml()
{
  object_t::write_xml();
  jack_port_t::write_xml();
  dynobject_t::set_attribute("size",size);
  dynobject_t::set_attribute("falloff",falloff);
}

/*
 *src_door_t
 */
src_door_t::src_door_t(xmlpp::Element* xmlsrc)
  : object_t(xmlsrc),jack_port_t(xmlsrc),width(1.0),
    height(2.0),
    falloff(1.0),
    distance(1.0),
    source(NULL)
{
  dynobject_t::get_attribute("width",width);
  dynobject_t::get_attribute("height",height);
  dynobject_t::get_attribute("falloff",falloff);
  dynobject_t::get_attribute("distance",distance);
}

void src_door_t::write_xml()
{
  object_t::write_xml();
  jack_port_t::write_xml();
  dynobject_t::set_attribute("width",width);
  dynobject_t::set_attribute("height",height);
  dynobject_t::set_attribute("falloff",falloff);
}

src_door_t::~src_door_t()
{
  if( source )
    delete source;
}

void src_door_t::geometry_update(double t)
{
  if( source ){
    dynobject_t::geometry_update(t);
    source->position = get_location();
    source->apply_rot_loc(source->position,get_orientation());
  }
}


void src_door_t::prepare(double fs, uint32_t fragsize)
{
  if( source )
    delete source;
  source = new TASCAR::Acousticmodel::doorsource_t(fragsize);
  geometry_update(0);
  source->falloff = 1.0/std::max(falloff,1.0e-10);
  source->distance = distance;
  source->nonrt_set_rect(width,height);
}

/*
 * sound_t
 */
sound_t::sound_t(xmlpp::Element* xmlsrc,src_object_t* parent_)
  : jack_port_t(xmlsrc),local_position(0,0,0),
    chaindist(0),
    parent(parent_),
    direct(true),
    source(NULL)
{
  get_attribute("x",local_position.x);
  get_attribute("y",local_position.y);
  get_attribute("z",local_position.z);
  get_attribute("d",chaindist);
  get_attribute_bool("direct",direct);
  get_attribute("name",name);
  if( name.empty() )
    throw TASCAR::ErrMsg("Invalid empty sound name.");
}

sound_t::sound_t(const sound_t& src)
  : jack_port_t(src),
    local_position(src.local_position),
    chaindist(src.chaindist),
    parent(NULL),
    name(src.name),
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
    dynobject_t::geometry_update(t);
    get_6dof(source->center,source->orientation);
  }
}

//void sink_object_t::geometry_update(double t)
//{
//  if( sink ){
//    dynobject_t::geometry_update(t);
//    get_6dof(sink->position,sink->orientation);
//    sink->diffusegain = diffusegain;
//    sink->is_direct = is_direct;
//    if( mask.active ){
//      mask.geometry_update(t);
//      mask.get_6dof(sink->mask.center,sink->mask.orientation);
//    }
//  }
//}
//
//std::vector<TASCAR::pos_t> sink_object_t::get_spkpos() const
//{
//  std::vector<TASCAR::pos_t> r;
//  for(std::vector<spk_pos_t>::const_iterator it=spkpos.begin();it!=spkpos.end();++it){
//    r.push_back(*it);
//  }
//  return r;
//}

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

void sound_t::write_xml()
{
  e->set_attribute("name",name);
  if( local_position.x != 0 )
    set_attribute("x",local_position.x);
  if( local_position.y != 0 )
    set_attribute("y",local_position.y);
  if( local_position.z != 0 )
    set_attribute("z",local_position.z);
  if( chaindist != 0 )
    set_attribute("d",chaindist);
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
src_object_t::src_object_t(xmlpp::Element* xmlsrc)
  : sndfile_object_t(xmlsrc),
    //reference(reference_),
    startframe(0)
{
  xmlpp::Node::NodeList subnodes = dynobject_t::e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "sound" )){
      sound.push_back(sound_t(sne,this));
    }
  }
}

void src_object_t::geometry_update(double t)
{
  dynobject_t::geometry_update(t);
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

void src_object_t::write_xml()
{
  object_t::write_xml();
  for(std::vector<sound_t>::iterator it=sound.begin();it!=sound.end();++it)
    it->write_xml();
}

sound_t* src_object_t::add_sound()
{
  sound.push_back(sound_t(dynobject_t::e->add_child("sound"),this));
  return &sound.back();
}

scene_t::scene_t(xmlpp::Element* xmlsrc)
  : scene_node_base_t(xmlsrc),
    description(""),
    name(""),
    mirrororder(1),
    guiscale(200),anysolo(0),
    scene_path("")
{
  try{
  get_attribute("name",name);
  get_attribute("mirrororder",mirrororder);
  get_attribute("guiscale",guiscale);
  get_attribute("guicenter",guicenter);
  description = xml_get_text(e,"description");
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne ){
      if( sne->get_name() == "src_object" )
        object_sources.push_back(new src_object_t(sne));
      else if( sne->get_name() == "door" )
        door_sources.push_back(new src_door_t(sne));
      else if( sne->get_name() == "diffuse" )
        diffuse_sources.push_back(new src_diffuse_t(sne));
      //else if( sne->get_name() == "sink" )
      //  sink_objects.push_back(sink_object_t(sne));
      else if( sne->get_name() == "sink" )
        sinkmod_objects.push_back(new sinkmod_object_t(sne));
      else if( sne->get_name() == "face" )
        faces.push_back(new face_object_t(sne));
      else if( sne->get_name() == "facegroup" )
        facegroups.push_back(new face_group_t(sne));
      else if( sne->get_name() == "mask" )
        masks.push_back(new mask_object_t(sne));
      else
        std::cerr << "Warning: Ignoring unrecognized xml node \"" << sne->get_name() << "\".\n";
    }
  }
  }
  catch(...){
    clean_children();
    throw;
  }
}

void scene_t::clean_children()
{
  std::vector<object_t*> objs(get_objects());
  for(std::vector<object_t*>::iterator it=objs.begin();it!=objs.end();++it)
    delete *it;
}

scene_t::~scene_t()
{
  clean_children();
}

void scene_t::write_xml()
{
  set_attribute("name",name);
  set_attribute("mirrororder",mirrororder);
  set_attribute("guiscale",guiscale);
  set_attribute("guicenter",guicenter);
  if( description.size()){
    xmlpp::Element* description_node = find_or_add_child("description");
    description_node->add_child_text(description);
  }
  std::vector<object_t*> objs(get_objects());
  for(std::vector<object_t*>::iterator it=objs.begin();it!=objs.end();++it)
    (*it)->write_xml();
}

void scene_t::geometry_update(double t)
{
  std::vector<object_t*> objs(get_objects());
  for(std::vector<object_t*>::iterator it=objs.begin();it!=objs.end();++it)
    (*it)->geometry_update( t );
}

void scene_t::process_active(double t)
{
  for(std::vector<src_object_t*>::iterator it=object_sources.begin();it!=object_sources.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<src_diffuse_t*>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<src_door_t*>::iterator it=door_sources.begin();it!=door_sources.end();++it)
    (*it)->process_active(t,anysolo);
  //for(std::vector<sink_object_t*>::iterator it=sink_objects.begin();it!=sink_objects.end();++it)
  //(*it)->process_active(t,anysolo);
  for(std::vector<sinkmod_object_t*>::iterator it=sinkmod_objects.begin();it!=sinkmod_objects.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<face_object_t*>::iterator it=faces.begin();it!=faces.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<face_group_t*>::iterator it=facegroups.begin();it!=facegroups.end();++it)
    (*it)->process_active(t,anysolo);
  
  //std::vector<object_t*> objs(get_objects());
  //for(std::vector<object_t*>::iterator it=objs.begin();it!=objs.end();++it){
  //  (*it)->process_active( t, anysolo );
  //}
}

spk_pos_t::spk_pos_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc)
{
  double az(0.0);
  double elev(0.0);
  double r(1.0);
  get_attribute("az",az);
  get_attribute("el",elev);
  get_attribute("r",r);
  set_sphere(r,az*DEG2RAD,elev*DEG2RAD);
}

void spk_pos_t::write_xml()
{
  if( azim() != 0.0 )
    set_attribute("az",azim()*RAD2DEG);
  if( elev() != 0.0 )
    set_attribute("el",elev()*RAD2DEG);
  if( norm() != 1.0 )
    set_attribute("r",norm());
}

//sink_object_t::sink_object_t(xmlpp::Element* xmlsrc)
//  : object_t(xmlsrc),jack_port_t(xmlsrc),
//    sink_type(omni),
//    falloff(-1.0),
//    render_point(true),
//    render_diffuse(true),
//    is_direct(true),
//    diffusegain(1.0),
//    use_global_mask(true),
//    mask(dynobject_t::find_or_add_child("mask")),
//    sink(NULL)
//{
//  dynobject_t::get_attribute("size",size);
//  dynobject_t::get_attribute_bool("point",render_point);
//  dynobject_t::get_attribute_bool("diffuse",render_diffuse);
//  dynobject_t::get_attribute_bool("isdirect",is_direct);
//  dynobject_t::get_attribute_bool("globalmask",use_global_mask);
//  dynobject_t::get_attribute_db("diffusegain",diffusegain);
//  dynobject_t::get_attribute("falloff",falloff);
//  std::string stype;
//  dynobject_t::get_attribute("type",stype);
//  if( stype == "omni" )
//    sink_type = omni;
//  else if( stype == "cardioid" )
//    sink_type = cardioid;
//  else if( stype == "amb3h3v" )
//    sink_type = amb3h3v;
//  else if( stype == "amb3h0v" )
//    sink_type = amb3h0v;
//  else if( stype == "nsp" )
//    sink_type = nsp;
//  else 
//    throw TASCAR::ErrMsg("Unupported sink type: \""+stype+"\"");
//  //  
//  xmlpp::Node::NodeList subnodes = dynobject_t::e->get_children();
//  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
//    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
//    if( sne && ( sne->get_name() == "speaker" )){
//      spkpos.push_back(spk_pos_t(sne));
//    }
//  }
//}
//
//sink_object_t::sink_object_t(const sink_object_t& src)
//  : object_t(src),
//    jack_port_t(src),
//    sink_type(src.sink_type),
//    spkpos(src.spkpos),
//    size(src.size),
//    falloff(src.falloff),
//    render_point(src.render_point),
//    render_diffuse(src.render_diffuse),
//    is_direct(src.is_direct),
//    diffusegain(src.diffusegain),
//    use_global_mask(src.use_global_mask),
//    mask(src.mask),
//    sink(NULL)
//{
//}
//
//sink_object_t::~sink_object_t()
//{
//  if( sink )
//    delete sink;
//}

mask_object_t::mask_object_t(xmlpp::Element* xmlsrc)
  : object_t(xmlsrc)
{
  dynobject_t::get_attribute("size",xmlsize);
  dynobject_t::get_attribute("falloff",xmlfalloff);
  dynobject_t::get_attribute_bool("inside",mask_inner);
}

void mask_object_t::write_xml()
{
  object_t::write_xml();
  dynobject_t::set_attribute("size",xmlsize);
  dynobject_t::set_attribute("falloff",xmlfalloff);
  dynobject_t::set_attribute_bool("inside",mask_inner);
}

void mask_object_t::prepare(double fs, uint32_t fragsize)
{
}

void mask_object_t::geometry_update(double t)
{
  dynobject_t::geometry_update(t);
  shoebox_t::size.x = std::max(0.0,xmlsize.x-xmlfalloff);
  shoebox_t::size.y = std::max(0.0,xmlsize.y-xmlfalloff);
  shoebox_t::size.z = std::max(0.0,xmlsize.z-xmlfalloff);
  get_6dof(shoebox_t::center,shoebox_t::orientation);
  falloff = 1.0/std::max(xmlfalloff,1e-10);
}

//void sink_object_t::write_xml()
//{
//  object_t::write_xml();
//  jack_port_t::write_xml();
//  if( (size.x != 0) || (size.y != 0) || (size.z != 0))
//    dynobject_t::set_attribute("size",size);
//  dynobject_t::set_attribute_bool("point",render_point);
//  dynobject_t::set_attribute_bool("diffuse",render_diffuse);
//  dynobject_t::set_attribute_bool("isdirect",is_direct);
//  if( diffusegain != 0 )
//    dynobject_t::set_attribute_db("diffusegain",diffusegain);
//  if( falloff != -1 )
//    dynobject_t::set_attribute("falloff",falloff);
//  switch( sink_type ){
//  case omni:
//    dynobject_t::e->set_attribute("type", "omni");
//    break;
//  case cardioid:
//    dynobject_t::e->set_attribute("type", "cardioid");
//    break;
//  case amb3h3v :
//    dynobject_t::e->set_attribute("type", "amb3h3v");
//    break;
//  case amb3h0v :
//    dynobject_t::e->set_attribute("type", "amb3h0v");
//    break;
//  case nsp :
//    dynobject_t::e->set_attribute("type", "nsp");
//    break;
//  }
//  //
//  if( mask.active )
//    mask.write_xml();
//  for( std::vector<spk_pos_t>::iterator it=spkpos.begin();it!=spkpos.end();++it)
//    it->write_xml();
//}

sinkmod_object_t::sinkmod_object_t(xmlpp::Element* xmlsrc)
  : object_t(xmlsrc), jack_port_t(xmlsrc), sink_t(xmlsrc)
{
}

void sinkmod_object_t::write_xml()
{
  object_t::write_xml();
  jack_port_t::write_xml();
  sinkmod_t::write_xml();
}

void sinkmod_object_t::prepare(double fs, uint32_t fragsize)
{
  TASCAR::Acousticmodel::sink_t::prepare(fs,fragsize);
}

void sinkmod_object_t::geometry_update(double t)
{
  dynobject_t::geometry_update(t);
  TASCAR::Acousticmodel::sink_t::position = c6dof.p;
  TASCAR::Acousticmodel::sink_t::orientation = c6dof.o;
}

void sinkmod_object_t::process_active(double t,uint32_t anysolo)
{
  TASCAR::Acousticmodel::sink_t::active = is_active(anysolo,t);
}

//void sink_object_t::prepare(double fs, uint32_t fragsize)
//{
//  bool use_mask(mask.active);
//  if( sink )
//    delete sink;
//  switch( sink_type ){
//  case omni :
//    sink = new TASCAR::Acousticmodel::sink_omni_t(fragsize,size,falloff,render_point,render_diffuse,
//                                                  mask.size,
//                                                  mask.falloff,
//                                                  use_mask,use_global_mask);
//    break;
//  case cardioid :
//    sink = new TASCAR::Acousticmodel::sink_cardioid_t(fragsize,size,falloff,render_point,render_diffuse,
//                                                      mask.size,
//                                                      mask.falloff,
//                                                      use_mask,use_global_mask);
//    break;
//  case amb3h3v :
//    sink = new TASCAR::Acousticmodel::sink_amb3h3v_t(fragsize,size,falloff,render_point,render_diffuse,
//                                                     mask.size,
//                                                     mask.falloff,
//                                                     use_mask,use_global_mask);
//    break;
//  case amb3h0v :
//    sink = new TASCAR::Acousticmodel::sink_amb3h0v_t(fragsize,size,falloff,render_point,render_diffuse,
//                                                  mask.size,
//                                                  mask.falloff,
//                                                  use_mask,use_global_mask);
//    break;
//  case nsp :
//    sink = new TASCAR::Acousticmodel::sink_nsp_t(fragsize,size,falloff,render_point,render_diffuse,
//                                                 mask.size,
//                                                 mask.falloff,
//                                                 use_mask,use_global_mask,get_spkpos());
//    break;
//  }
//}

src_object_t* scene_t::add_source()
{
  object_sources.push_back(new src_object_t(e->add_child("src_object")));
  return object_sources.back();
}

std::vector<sound_t*> scene_t::linearize_sounds()
{
  std::vector<sound_t*> r;
  for(std::vector<src_object_t*>::iterator it=object_sources.begin();it!=object_sources.end();++it){
    //it->set_reference(&listener);
    for(std::vector<sound_t>::iterator its=(*it)->sound.begin();its!=(*it)->sound.end();++its){
      if( &(*its) ){
        (&(*its))->set_parent((*it));
        r.push_back(&(*its));
      }
    }
  }
  return r;
}

std::vector<object_t*> scene_t::get_objects()
{
  std::vector<object_t*> r;
  for(std::vector<src_object_t*>::iterator it=object_sources.begin();it!=object_sources.end();++it)
    r.push_back(*it);
  for(std::vector<src_diffuse_t*>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it)
    r.push_back(*it);
  for(std::vector<src_door_t*>::iterator it=door_sources.begin();it!=door_sources.end();++it)
    r.push_back(*it);
  //for(std::vector<sink_object_t>::iterator it=sink_objects.begin();it!=sink_objects.end();++it)
  //  r.push_back(&(*it));
  for(std::vector<sinkmod_object_t*>::iterator it=sinkmod_objects.begin();it!=sinkmod_objects.end();++it)
    r.push_back(*it);
  for(std::vector<face_object_t*>::iterator it=faces.begin();it!=faces.end();++it)
    r.push_back(*it);
  for(std::vector<face_group_t*>::iterator it=facegroups.begin();it!=facegroups.end();++it)
    r.push_back(*it);
  for(std::vector<mask_object_t*>::iterator it=masks.begin();it!=masks.end();++it)
    r.push_back(*it);
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
  if( sinkmod_objects.size() == 0 )
    throw TASCAR::ErrMsg("No receiver in scene \""+name+"\".");
  std::vector<object_t*> objs(get_objects());
  for(std::vector<object_t*>::iterator it=objs.begin();it!=objs.end();++it)
    (*it)->prepare( fs, fragsize );
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
  unsigned int c(((unsigned int)(round(r*255.0)) << 16) + 
                 ((unsigned int)(round(g*255.0)) << 8) + 
                 ((unsigned int)(round(b*255.0))));
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

route_t::route_t(xmlpp::Element* xmlsrc)
  : scene_node_base_t(xmlsrc),mute(false),solo(false)
{
  get_attribute("name",name);
  get_attribute_bool("mute",mute);
  get_attribute_bool("solo",solo);
}

void route_t::write_xml()
{
  set_attribute("name",name);
  set_attribute_bool("mute",mute);
  set_attribute_bool("solo",solo);
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

face_object_t::face_object_t(xmlpp::Element* xmlsrc)
  : object_t(xmlsrc),width(1.0),
    height(1.0)
{
  dynobject_t::get_attribute("width",width);
  dynobject_t::get_attribute("height",height);
  dynobject_t::get_attribute("reflectivity",reflectivity);
  dynobject_t::get_attribute("damping",damping);
  dynobject_t::get_attribute("vertices",vertices);
  if( vertices.size() > 2 )
    nonrt_set(vertices);
  else
    nonrt_set_rect(width,height);
}

face_object_t::~face_object_t()
{
}

void face_object_t::geometry_update(double t)
{
  dynobject_t::geometry_update(t);
  apply_rot_loc(get_location(),get_orientation());
}

void face_object_t::prepare(double fs, uint32_t fragsize)
{
}

void face_object_t::write_xml()
{
  object_t::write_xml();
  dynobject_t::set_attribute("width",width);
  dynobject_t::set_attribute("height",height);
  dynobject_t::set_attribute("reflectivity",reflectivity);
  dynobject_t::set_attribute("damping",damping);
  dynobject_t::set_attribute("vertices",vertices);
}

jack_port_t::jack_port_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc),portname(""),
    connect(""),
    port_index(0),
    gain(1)
{
  get_attribute("connect",connect);
  get_attribute_db_float("gain",gain);
}

void jack_port_t::set_gain_db( float g )
{
  gain = pow(10.0,0.05*g);
}

void jack_port_t::write_xml()
{
  if( connect.size() )
    e->set_attribute("connect",connect);
  if( gain != 0.0 )
    set_attribute_db("gain",gain);
}

sndfile_info_t::sndfile_info_t(xmlpp::Element* xmlsrc)
  : scene_node_base_t(xmlsrc),fname(""),
    firstchannel(0),
    channels(1),
    objectchannel(0),
    starttime(0),
    loopcnt(1),
    gain(1.0)
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

void sndfile_info_t::write_xml()
{
  e->set_attribute("name",fname.c_str());
  if( firstchannel != 0 )
    set_attribute_uint(e,"firstchannel",firstchannel);
  if( channels != 1 )
    set_attribute_uint(e,"channels",channels);
  if( loopcnt != 1 )
    set_attribute_uint(e,"loop",loopcnt);
  if( gain != 0.0 )
    set_attribute_db("gain",gain);
}

void src_object_t::process_active(double t, uint32_t anysolo)
{
  bool a(is_active(anysolo,t));
  for(std::vector<sound_t>::iterator it=sound.begin();it!=sound.end();++it)
    if( it->get_source() )
      it->get_source()->active = a;
}

void src_diffuse_t::process_active(double t, uint32_t anysolo)
{
  bool a(is_active(anysolo,t));
  if( source )
    source->active = a;
}

void face_object_t::process_active(double t, uint32_t anysolo)
{
  active = is_active(anysolo,t);
}

void src_door_t::process_active(double t, uint32_t anysolo)
{
  bool a(is_active(anysolo,t));
  if( source )
    source->active = a;
}


std::string jacknamer(const std::string& scenename, const std::string& base)
{
  if( scenename.empty() )
    return base+"tascar";
  return base+scenename;
}

std::vector<TASCAR::Scene::object_t*> TASCAR::Scene::scene_t::find_object(const std::string& pattern)
{
  std::vector<TASCAR::Scene::object_t*> retv;
  std::vector<TASCAR::Scene::object_t*> objs(get_objects());
  for(std::vector<TASCAR::Scene::object_t*>::iterator it=objs.begin();it!=objs.end();++it)
    if( fnmatch(pattern.c_str(),(*it)->get_name().c_str(),FNM_PATHNAME) == 0 )
      retv.push_back(*it);
  return retv;
}

face_group_t::face_group_t(xmlpp::Element* xmlsrc)
  : object_t(xmlsrc)
{
  dynobject_t::GET_ATTRIBUTE(reflectivity);
  dynobject_t::GET_ATTRIBUTE(damping);
  std::stringstream txtmesh(TASCAR::xml_get_text(xmlsrc,""));
  while(!txtmesh.eof() ){
    std::string meshline;
    getline(txtmesh,meshline,'\n');
    if( !meshline.empty() ){
      //DEBUGS(meshline);
      TASCAR::Acousticmodel::reflector_t* p_reflector(new TASCAR::Acousticmodel::reflector_t());
      p_reflector->nonrt_set(TASCAR::str2vecpos(meshline));
      reflectors.push_back(p_reflector);
    }
  }
}

face_group_t::~face_group_t()
{
  for(std::vector<TASCAR::Acousticmodel::reflector_t*>::iterator it=reflectors.begin();it!=reflectors.end();++it)
    delete *it;
}

void face_group_t::prepare(double fs, uint32_t fragsize)
{
}

void face_group_t::write_xml()
{
  object_t::write_xml();
  dynobject_t::SET_ATTRIBUTE(reflectivity);
  dynobject_t::SET_ATTRIBUTE(damping);
  std::stringstream txtmesh;
  for(std::vector<TASCAR::Acousticmodel::reflector_t*>::iterator it=reflectors.begin();it!=reflectors.end();++it){
    txtmesh << (*it)->print(" ") << std::endl;
  }
  dynobject_t::e->add_child_text(txtmesh.str());
}
 
void face_group_t::geometry_update(double t)
{
  dynobject_t::geometry_update(t);
  for(std::vector<TASCAR::Acousticmodel::reflector_t*>::iterator it=reflectors.begin();it!=reflectors.end();++it){
    (*it)->apply_rot_loc(c6dof.p,c6dof.o);
    (*it)->reflectivity = reflectivity;
    (*it)->damping = damping;
  }
}
 
void face_group_t::process_active(double t,uint32_t anysolo)
{
  bool a(is_active(anysolo,t));
  for(std::vector<TASCAR::Acousticmodel::reflector_t*>::iterator it=reflectors.begin();it!=reflectors.end();++it){
    (*it)->active = a;
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
