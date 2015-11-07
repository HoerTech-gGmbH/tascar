#include "scene.h"
#include "errorhandling.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <libgen.h>
#include <unistd.h>
#include <fnmatch.h>
#include <fstream>
#include <complex.h>
#include <algorithm>

using namespace TASCAR;
using namespace TASCAR::Scene;

#define RMSLEN 2.0

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

void object_t::write_xml()
{
  route_t::write_xml();
  dynobject_t::write_xml();
  if( color.str() != "#000000" )
    dynobject_t::set_attribute("color",color.str());
  dynobject_t::set_attribute("end",endtime);
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
    wnd_sqrt(false),
    maxdist(3700),
    sincorder(0),
    source(NULL)
{
  dynobject_t::get_attribute("width",width);
  dynobject_t::get_attribute("height",height);
  dynobject_t::get_attribute("falloff",falloff);
  dynobject_t::get_attribute("distance",distance);
  dynobject_t::get_attribute_bool("wndsqrt",wnd_sqrt);
  dynobject_t::GET_ATTRIBUTE(maxdist);
  dynobject_t::GET_ATTRIBUTE(sincorder);
}

void src_door_t::write_xml()
{
  object_t::write_xml();
  jack_port_t::write_xml();
  dynobject_t::set_attribute("width",width);
  dynobject_t::set_attribute("height",height);
  dynobject_t::set_attribute("falloff",falloff);
  dynobject_t::set_attribute("distance",distance);
  dynobject_t::set_attribute_bool("wndsqrt",wnd_sqrt);
  dynobject_t::SET_ATTRIBUTE(maxdist);
  dynobject_t::SET_ATTRIBUTE(sincorder);
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
    source->falloff = 1.0/std::max(falloff,1.0e-10);
    source->distance = distance;
    source->wnd_sqrt = wnd_sqrt;
    source->position = get_location();
    source->apply_rot_loc(source->position,get_orientation());
  }
}


void src_door_t::prepare(double fs, uint32_t fragsize)
{
  if( source )
    delete source;
  reset_meters();
  addmeter(RMSLEN*fs);
  source = new TASCAR::Acousticmodel::doorsource_t(fragsize,maxdist,sincorder);
  source->add_rmslevel(&(rmsmeter[0]));
  geometry_update(0);
  source->nonrt_set_rect(width,height);
}

/*
 * sound_t
 */
sound_t::sound_t(xmlpp::Element* xmlsrc,src_object_t* parent_)
  : jack_port_t(xmlsrc),local_position(0,0,0),
    chaindist(0),
    maxdist(3700),
    sincorder(0),
    parent(parent_),
    direct(true),
    source(NULL)
{
  get_attribute("x",local_position.x);
  get_attribute("y",local_position.y);
  get_attribute("z",local_position.z);
  get_attribute("d",chaindist);
  GET_ATTRIBUTE(maxdist);
  GET_ATTRIBUTE(sincorder);
  get_attribute_bool("direct",direct);
  get_attribute("name",name);
  if( name.empty() )
    throw TASCAR::ErrMsg("Invalid empty sound name.");
}

sound_t::sound_t(const sound_t& src)
  : jack_port_t(src),
    local_position(src.local_position),
    chaindist(src.chaindist),
    maxdist(src.maxdist),
    sincorder(src.sincorder),
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
  source = new TASCAR::Acousticmodel::pointsource_t(fragsize,maxdist,sincorder);
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

void src_diffuse_t::prepare(double fs, uint32_t fragsize)
{
  if( source )
    delete source;
  reset_meters();
  addmeter(RMSLEN*fs);
  source = new TASCAR::Acousticmodel::diffuse_source_t(fragsize,rmsmeter[0]);
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
  SET_ATTRIBUTE(maxdist);
  SET_ATTRIBUTE(sincorder);
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
  reset_meters();
  for(std::vector<sound_t>::iterator it=sound.begin();it!=sound.end();++it){
    addmeter(RMSLEN*fs);
    it->prepare(fs,fragsize);
    it->get_source()->add_rmslevel(&(rmsmeter.back()));
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
    c(340.0),
    mirrororder(1),
    guiscale(200),anysolo(0),
    scene_path("")
{
  try{
  GET_ATTRIBUTE(name);
  GET_ATTRIBUTE(mirrororder);
  GET_ATTRIBUTE(guiscale);
  GET_ATTRIBUTE(guicenter);
  GET_ATTRIBUTE(c);
  description = xml_get_text(e,"description");
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne ){
      // rename old "sink" to "receiver":
      if( sne->get_name() == "sink" )
        sne->set_name("receiver");
      // parse nodes:
      if( sne->get_name() == "src_object" )
        object_sources.push_back(new src_object_t(sne));
      else if( sne->get_name() == "door" )
        door_sources.push_back(new src_door_t(sne));
      else if( sne->get_name() == "diffuse" )
        diffuse_sources.push_back(new src_diffuse_t(sne));
      else if( sne->get_name() == "receiver" )
        receivermod_objects.push_back(new receivermod_object_t(sne));
      else if( sne->get_name() == "face" )
        faces.push_back(new face_object_t(sne));
      else if( sne->get_name() == "facegroup" )
        facegroups.push_back(new face_group_t(sne));
      else if( sne->get_name() == "obstacle" )
        obstaclegroups.push_back(new obstacle_group_t(sne));
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

/**
   \brief Update geometry of all objects within a scene based on current transport time
   \param t Transport time
   \ingroup callgraph
 */
void scene_t::geometry_update(double t)
{
  std::vector<object_t*> objs(get_objects());
  for(std::vector<object_t*>::iterator it=objs.begin();it!=objs.end();++it)
    (*it)->geometry_update( t );
}

/**
   \brief Update activity flags based on current transport time
   \param t Transport time
   \ingroup callgraph
 */
void scene_t::process_active(double t)
{
  for(std::vector<src_object_t*>::iterator it=object_sources.begin();it!=object_sources.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<src_diffuse_t*>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<src_door_t*>::iterator it=door_sources.begin();it!=door_sources.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<receivermod_object_t*>::iterator it=receivermod_objects.begin();it!=receivermod_objects.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<face_object_t*>::iterator it=faces.begin();it!=faces.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<face_group_t*>::iterator it=facegroups.begin();it!=facegroups.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<obstacle_group_t*>::iterator it=obstaclegroups.begin();it!=obstaclegroups.end();++it)
    (*it)->process_active(t,anysolo);
}

spk_pos_t::spk_pos_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc),
    az(0.0),
    el(0.0),
    r(1.0),
    gain(1.0),
    dr(0.0),      
    d_w(0.0f),
    d_x(0.0f),
    d_y(0.0f),
    d_z(0.0f)
{
  GET_ATTRIBUTE_DEG(az);
  GET_ATTRIBUTE_DEG(el);
  GET_ATTRIBUTE(r);
  GET_ATTRIBUTE(label);
  GET_ATTRIBUTE(connect);
  set_sphere(r,az,el);
  unitvector = normal();
  update_foa_decoder(1.0f);
}

void spk_pos_t::update_foa_decoder(float gain)
{
  // update of FOA decoder matrix:
  TASCAR::pos_t px(1,0,0);
  TASCAR::pos_t py(0,1,0);
  TASCAR::pos_t pz(0,0,1);
  d_w = sqrtf(0.5f) * gain;
  d_x = dot_prod(px,unitvector) * gain;
  d_y = dot_prod(py,unitvector) * gain;
  d_z = dot_prod(pz,unitvector) * gain;
}

void spk_pos_t::write_xml()
{
  if( az != 0.0 )
    SET_ATTRIBUTE_DEG(az);
  if( el != 0.0 )
    SET_ATTRIBUTE_DEG(el);
  if( r != 1.0 )
    SET_ATTRIBUTE(r);
  if( !label.empty() )
    SET_ATTRIBUTE(label);
  if( !connect.empty() )
    SET_ATTRIBUTE(connect);
}

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
  dynobject_t::get_6dof(shoebox_t::center,shoebox_t::orientation);
  falloff = 1.0/std::max(xmlfalloff,1e-10);
}

receivermod_object_t::receivermod_object_t(xmlpp::Element* xmlsrc)
  : object_t(xmlsrc), jack_port_t(xmlsrc), receiver_t(xmlsrc)
{
}

void receivermod_object_t::write_xml()
{
  object_t::write_xml();
  jack_port_t::write_xml();
  receiver_t::write_xml();
}

void receivermod_object_t::prepare(double fs, uint32_t fragsize)
{
  TASCAR::Acousticmodel::receiver_t::prepare(fs,fragsize);
  reset_meters();
  for(uint32_t k=0;k<get_num_channels();k++)
    addmeter(RMSLEN*fs);
}

void receivermod_object_t::postproc(std::vector<wave_t>& output)
{
  TASCAR::Acousticmodel::receiver_t::postproc(output);
  if( output.size() != rmsmeter.size() ){
    DEBUG(output.size());
    DEBUG(rmsmeter.size());
    throw TASCAR::ErrMsg("Programming error");
  }
  for(uint32_t k=0;k<output.size();k++)
    rmsmeter[k].append(output[k]);
}

void receivermod_object_t::geometry_update(double t)
{
  dynobject_t::geometry_update(t);
  TASCAR::Acousticmodel::receiver_t::position = c6dof.p;
  TASCAR::Acousticmodel::receiver_t::orientation = c6dof.o;
  boundingbox.geometry_update(t);
}

void receivermod_object_t::process_active(double t,uint32_t anysolo)
{
  TASCAR::Acousticmodel::receiver_t::active = is_active(anysolo,t);
}

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
  for(std::vector<receivermod_object_t*>::iterator it=receivermod_objects.begin();it!=receivermod_objects.end();++it)
    r.push_back(*it);
  for(std::vector<face_object_t*>::iterator it=faces.begin();it!=faces.end();++it)
    r.push_back(*it);
  for(std::vector<face_group_t*>::iterator it=facegroups.begin();it!=facegroups.end();++it)
    r.push_back(*it);
  for(std::vector<obstacle_group_t*>::iterator it=obstaclegroups.begin();it!=obstaclegroups.end();++it)
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
  if( (object_sources.size() == 0) && (diffuse_sources.size() == 0) && (door_sources.size() == 0) )
    throw TASCAR::ErrMsg("No sound source in scene \""+name+"\".");
  if( receivermod_objects.size() == 0 )
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

void route_t::addmeter(uint32_t frames)
{
  rmsmeter.push_back(wave_t(frames));
  meterval.push_back(0);
}

const std::vector<float>& route_t::readmeter()
{
  for(uint32_t k=0;k<rmsmeter.size();k++)
    meterval[k] = rmsmeter[k].spldb();
  return meterval;
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
  dynobject_t::get_attribute_bool("edgereflection",edgereflection);
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
  dynobject_t::set_attribute_bool("edgereflection",edgereflection);
}

jack_port_t::jack_port_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc),portname(""),
    connect(""),
    port_index(0),
    gain(1),
    caliblevel(50000.0)
{
  get_attribute("connect",connect);
  get_attribute_db_float("gain",gain);
  get_attribute_db_float("caliblevel",caliblevel);
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
  set_attribute_db("caliblevel",caliblevel);
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
  : object_t(xmlsrc),
    reflectivity(1.0),
    damping(0.0),
    edgereflection(true)
{
  dynobject_t::GET_ATTRIBUTE(reflectivity);
  dynobject_t::GET_ATTRIBUTE(damping);
  dynobject_t::GET_ATTRIBUTE(importraw);
  dynobject_t::get_attribute_bool("edgereflection",edgereflection);
  dynobject_t::GET_ATTRIBUTE(shoebox);
  if( !shoebox.is_null() ){
    TASCAR::pos_t sb(shoebox);
    sb *= 0.5;
    std::vector<TASCAR::pos_t> verts;
    verts.resize(4);
    TASCAR::Acousticmodel::reflector_t* p_reflector(NULL);
    // face1:
    verts[0] = TASCAR::pos_t(sb.x,-sb.y,-sb.z);
    verts[1] = TASCAR::pos_t(sb.x,-sb.y,sb.z);
    verts[2] = TASCAR::pos_t(sb.x,sb.y,sb.z);
    verts[3] = TASCAR::pos_t(sb.x,sb.y,-sb.z);
    p_reflector = new TASCAR::Acousticmodel::reflector_t();
    p_reflector->nonrt_set(verts);
    reflectors.push_back(p_reflector);
    // face2:
    verts[0] = TASCAR::pos_t(-sb.x,-sb.y,-sb.z);
    verts[1] = TASCAR::pos_t(-sb.x,sb.y,-sb.z);
    verts[2] = TASCAR::pos_t(-sb.x,sb.y,sb.z);
    verts[3] = TASCAR::pos_t(-sb.x,-sb.y,sb.z);
    p_reflector = new TASCAR::Acousticmodel::reflector_t();
    p_reflector->nonrt_set(verts);
    reflectors.push_back(p_reflector);
    // face3:
    verts[0] = TASCAR::pos_t(-sb.x,-sb.y,-sb.z);
    verts[1] = TASCAR::pos_t(-sb.x,-sb.y,sb.z);
    verts[2] = TASCAR::pos_t(sb.x,-sb.y,sb.z);
    verts[3] = TASCAR::pos_t(sb.x,-sb.y,-sb.z);
    p_reflector = new TASCAR::Acousticmodel::reflector_t();
    p_reflector->nonrt_set(verts);
    reflectors.push_back(p_reflector);
    // face4:
    verts[0] = TASCAR::pos_t(-sb.x,sb.y,-sb.z);
    verts[1] = TASCAR::pos_t(sb.x,sb.y,-sb.z);
    verts[2] = TASCAR::pos_t(sb.x,sb.y,sb.z);
    verts[3] = TASCAR::pos_t(-sb.x,sb.y,sb.z);
    p_reflector = new TASCAR::Acousticmodel::reflector_t();
    p_reflector->nonrt_set(verts);
    reflectors.push_back(p_reflector);
    // face5:
    verts[0] = TASCAR::pos_t(-sb.x,-sb.y,sb.z);
    verts[1] = TASCAR::pos_t(-sb.x,sb.y,sb.z);
    verts[2] = TASCAR::pos_t(sb.x,sb.y,sb.z);
    verts[3] = TASCAR::pos_t(sb.x,-sb.y,sb.z);
    p_reflector = new TASCAR::Acousticmodel::reflector_t();
    p_reflector->nonrt_set(verts);
    reflectors.push_back(p_reflector);
    // face6:
    verts[0] = TASCAR::pos_t(-sb.x,-sb.y,-sb.z);
    verts[1] = TASCAR::pos_t(sb.x,-sb.y,-sb.z);
    verts[2] = TASCAR::pos_t(sb.x,sb.y,-sb.z);
    verts[3] = TASCAR::pos_t(-sb.x,sb.y,-sb.z);
    p_reflector = new TASCAR::Acousticmodel::reflector_t();
    p_reflector->nonrt_set(verts);
    reflectors.push_back(p_reflector);
  }
  if( !importraw.empty() ){
    std::ifstream rawmesh(importraw.c_str());
    if( !rawmesh.good() )
      throw TASCAR::ErrMsg("Unable to open mesh file \""+importraw+"\".");
    while(!rawmesh.eof() ){
      std::string meshline;
      getline(rawmesh,meshline,'\n');
      if( !meshline.empty() ){
        TASCAR::Acousticmodel::reflector_t* p_reflector(new TASCAR::Acousticmodel::reflector_t());
        p_reflector->nonrt_set(TASCAR::str2vecpos(meshline));
        reflectors.push_back(p_reflector);
      }
    }
  }
  std::stringstream txtmesh(TASCAR::xml_get_text(xmlsrc,"faces"));
  while(!txtmesh.eof() ){
    std::string meshline;
    getline(txtmesh,meshline,'\n');
    if( !meshline.empty() ){
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
  dynobject_t::SET_ATTRIBUTE(importraw);
  dynobject_t::set_attribute_bool("edgereflection",edgereflection);
}
 
void face_group_t::geometry_update(double t)
{
  dynobject_t::geometry_update(t);
  for(std::vector<TASCAR::Acousticmodel::reflector_t*>::iterator it=reflectors.begin();it!=reflectors.end();++it){
    (*it)->apply_rot_loc(c6dof.p,c6dof.o);
    (*it)->reflectivity = reflectivity;
    (*it)->damping = damping;
    (*it)->edgereflection = edgereflection;
  }
}
 
void face_group_t::process_active(double t,uint32_t anysolo)
{
  bool a(is_active(anysolo,t));
  for(std::vector<TASCAR::Acousticmodel::reflector_t*>::iterator it=reflectors.begin();it!=reflectors.end();++it){
    (*it)->active = a;
  }
}

// obstacles:
obstacle_group_t::obstacle_group_t(xmlpp::Element* xmlsrc)
  : object_t(xmlsrc),
    transmission(0)
{
  dynobject_t::GET_ATTRIBUTE(transmission);
  dynobject_t::GET_ATTRIBUTE(importraw);
  if( !importraw.empty() ){
    std::ifstream rawmesh(importraw.c_str());
    if( !rawmesh.good() )
      throw TASCAR::ErrMsg("Unable to open mesh file \""+importraw+"\".");
    while(!rawmesh.eof() ){
      std::string meshline;
      getline(rawmesh,meshline,'\n');
      if( !meshline.empty() ){
        TASCAR::Acousticmodel::obstacle_t* p_obstacle(new TASCAR::Acousticmodel::obstacle_t());
        p_obstacle->nonrt_set(TASCAR::str2vecpos(meshline));
        obstacles.push_back(p_obstacle);
      }
    }
  }
  std::stringstream txtmesh(TASCAR::xml_get_text(xmlsrc,"faces"));
  while(!txtmesh.eof() ){
    std::string meshline;
    getline(txtmesh,meshline,'\n');
    if( !meshline.empty() ){
      TASCAR::Acousticmodel::obstacle_t* p_obstacle(new TASCAR::Acousticmodel::obstacle_t());
      p_obstacle->nonrt_set(TASCAR::str2vecpos(meshline));
      obstacles.push_back(p_obstacle);
    }
  }
}

obstacle_group_t::~obstacle_group_t()
{
  for(std::vector<TASCAR::Acousticmodel::obstacle_t*>::iterator it=obstacles.begin();it!=obstacles.end();++it)
    delete *it;
}

void obstacle_group_t::prepare(double fs, uint32_t fragsize)
{
}

void obstacle_group_t::write_xml()
{
  object_t::write_xml();
  dynobject_t::SET_ATTRIBUTE(transmission);
  dynobject_t::SET_ATTRIBUTE(importraw);
}
 
void obstacle_group_t::geometry_update(double t)
{
  dynobject_t::geometry_update(t);
  for(std::vector<TASCAR::Acousticmodel::obstacle_t*>::iterator it=obstacles.begin();it!=obstacles.end();++it){
    (*it)->apply_rot_loc(c6dof.p,c6dof.o);
    (*it)->transmission = transmission;
  }
}
 
void obstacle_group_t::process_active(double t,uint32_t anysolo)
{
  bool a(is_active(anysolo,t));
  for(std::vector<TASCAR::Acousticmodel::obstacle_t*>::iterator it=obstacles.begin();it!=obstacles.end();++it){
    (*it)->active = a;
    (*it)->transmission = transmission;
  }
}



// speaker array:

spk_array_t::spk_array_t(xmlpp::Element* e)
  : xml_element_t(e),
    rmax(0),
    rmin(0)
{
  read_xml(e);
  if( !empty() ){
    rmax = operator[](0).r;
    rmin = rmax;
    for(uint32_t k=1;k<size();k++){
      if( rmax < operator[](k).r )
        rmax = operator[](k).r;
      if( rmin > operator[](k).r )
        rmin = operator[](k).r;
    }
    for(uint32_t k=0;k<size();k++){
      operator[](k).gain = rmax/operator[](k).r;
      operator[](k).dr = rmax-operator[](k).r;
    }
  }
  if( empty() )
    throw TASCAR::ErrMsg("Invalid empty speaker array.");
  didx.resize(size());
  for(uint32_t k=0;k<size();k++){
    operator[](k).update_foa_decoder(1.0f/size());
    connections.push_back(operator[](k).connect);
  }
}

void spk_array_t::read_xml(xmlpp::Element* e)
{
  clear();
  std::string importsrc(e->get_attribute_value("layout"));
  if( !importsrc.empty() )
    import_file(importsrc);
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "speaker" )){
      push_back(TASCAR::spk_pos_t(sne));
    }
  }
}

void spk_array_t::write_xml()
{
}

void spk_array_t::import_file(const std::string& fname)
{
  xmlpp::DomParser domp;
  domp.parse_file(TASCAR::env_expand(fname));
  xmlpp::Element* r(domp.get_document()->get_root_node());
  if( r->get_name() != "layout" )
    throw TASCAR::ErrMsg("Invalid root node name. Expected \"layout\", got "+r->get_name()+".");
  read_xml(r);
}

double spk_pos_t::get_rel_azim(double az_src) const
{
  return carg(cexp(I*(az_src - az)));
}

double spk_pos_t::get_cos_adist(pos_t src_unit) const
{
  return dot_prod( src_unit, unitvector );
}

bool sort_didx(const spk_array_t::didx_t& a,const spk_array_t::didx_t& b)
{
  return (a.d > b.d);
}

const std::vector<spk_array_t::didx_t>& spk_array_t::sort_distance(const pos_t& psrc)
{
  for(uint32_t k=0;k<size();++k){
    didx[k].idx = k;
    didx[k].d = dot_prod(psrc,operator[](k).unitvector);
  }
  std::sort(didx.begin(),didx.end(),sort_didx);
  return didx;
}

void spk_array_t::foa_decode(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output)
{
  uint32_t channels(size());
  if( output.size() != channels ){
    throw TASCAR::ErrMsg("Invalid size of speaker array");
  }
  uint32_t N(chunk.size());
  for( uint32_t t=0; t<N; ++t ){
    for( uint32_t ch=0; ch<channels; ++ch ){
      output[ch][t] += operator[](ch).d_w * chunk.w()[t];
      output[ch][t] += operator[](ch).d_x * chunk.x()[t];
      output[ch][t] += operator[](ch).d_y * chunk.y()[t];
      output[ch][t] += operator[](ch).d_z * chunk.z()[t];
    }
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
