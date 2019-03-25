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
#include <set>

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

/*
 *diffuse_info_t
 */
diffuse_info_t::diffuse_info_t(xmlpp::Element* xmlsrc)
  : sndfile_object_t(xmlsrc),
    audio_port_t(xmlsrc,true),size(1,1,1),
    falloff(1.0),
    layers(0xffffffff),
    source(NULL)
{
  dynobject_t::get_attribute("size",size);
  dynobject_t::get_attribute("falloff",falloff);
  dynobject_t::GET_ATTRIBUTE_BITS(layers);
}

diffuse_info_t::~diffuse_info_t()
{
  if( source )
    delete source;
}

void diffuse_info_t::geometry_update(double t)
{
  if( source ){
    dynobject_t::geometry_update(t);
    get_6dof(source->center,source->orientation);
    source->layers = layers;
  }
}

void diffuse_info_t::prepare( chunk_cfg_t& cf_ )
{
  cf_.n_channels = 4;
  sndfile_object_t::prepare( cf_ );
  if( source )
    delete source;
  reset_meters();
  addmeter( f_sample );
  source = new TASCAR::Acousticmodel::diffuse_t( dynobject_t::e, n_fragment, *(rmsmeter[0]), get_name() );
  source->size = size;
  source->falloff = 1.0/std::max(falloff,1.0e-10);
  for( std::vector<sndfile_info_t>::iterator it=sndfiles.begin();it!=sndfiles.end();++it)
    if( it->channels != 4 )
      throw TASCAR::ErrMsg("Diffuse sources support only 4-channel (FOA) sound files ("+it->fname+").");
  source->prepare( cf_ );
}

audio_port_t::~audio_port_t()
{
}

void audio_port_t::set_port_index(uint32_t port_index_)
{
  port_index = port_index_;
}

void audio_port_t::set_inv( bool inv )
{
  if( inv )
    gain = -fabsf(gain);
  else
    gain = fabsf(gain);
}

/*
 * src_object_t
 */
src_object_t::src_object_t(xmlpp::Element* xmlsrc)
  : sndfile_object_t(xmlsrc),
    startframe(0)
{
  if( get_name().empty() )
    set_name("in");
  xmlpp::Node::NodeList subnodes = dynobject_t::e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne ){
      if( sne->get_name() == "sound" )
        add_sound(sne);
      else if( (sne->get_name() != "creator") && 
               (sne->get_name() != "sndfile") && 
               (sne->get_name() != "include") && 
               (sne->get_name() != "position") && 
               (sne->get_name() != "orientation") )
        TASCAR::add_warning("Invalid sub-node \""+sne->get_name()+"\".",sne);
    }
  }
}

void src_object_t::add_sound(xmlpp::Element* src)
{
  if( !src )
    src = dynobject_t::e->add_child("sound");
  sound.push_back(new sound_t(src,this));
}

src_object_t::~src_object_t()
{
  for(std::vector<sound_t*>::iterator s=sound.begin();s!=sound.end();++s)
    delete *s;
}

std::string src_object_t::next_sound_name() const
{
  std::set<std::string> names;
  for(std::vector<sound_t*>::const_iterator it=sound.begin();it!=sound.end();++it)
    names.insert((*it)->get_name());
  char ctmp[1024];
  uint32_t n(0);
  sprintf(ctmp,"%d",n);
  while( names.find(ctmp) != names.end() ){
    ++n;
    sprintf(ctmp,"%d",n);
  }
  return ctmp;
}

void src_object_t::validate_attributes(std::string& msg) const
{
  dynobject_t::validate_attributes(msg);
  for(std::vector<sound_t*>::const_iterator it=sound.begin();it!=sound.end();++it)
    (*it)->validate_attributes(msg);
}

void src_object_t::geometry_update(double t)
{
  dynobject_t::geometry_update(t);
  for(std::vector<sound_t*>::iterator it=sound.begin();it!=sound.end();++it){
    (*it)->geometry_update(t);
  }
}

void src_object_t::prepare( chunk_cfg_t& cf_ )
{
  cf_.n_channels = 1;
  sndfile_object_t::prepare( cf_ );
  reset_meters();
  for(std::vector<sound_t*>::iterator it=sound.begin();it!=sound.end();++it){
    for(uint32_t k=0;k<(*it)->get_num_channels();++k){
      addmeter( f_sample );
      (*it)->add_meter(rmsmeter.back());
    }
    (*it)->prepare( cf_ );
    //(*it)->get_source()->add_rmslevel((rmsmeter.back()));
  }
  startframe = f_sample * starttime;
}

void src_object_t::release()
{
  sndfile_object_t::release();
  for(std::vector<sound_t*>::iterator it=sound.begin();it!=sound.end();++it)
    (*it)->release();
}

scene_t::scene_t(xmlpp::Element* xmlsrc)
  : scene_node_base_t(xmlsrc),
    description(""),
    name(""),
    c(340.0),
    ismorder(1),
    guiscale(200),
    guitrackobject(NULL),
    anysolo(0),
    scene_path("")
{
  try{
    GET_ATTRIBUTE(name);
    if( name.empty() )
      name = "scene";
    if( has_attribute("mirrororder") ){
      uint32_t mirrororder(1);
      GET_ATTRIBUTE(mirrororder);
      ismorder = mirrororder;
      TASCAR::add_warning("Attribute \"mirrororder\" is deprecated and has been replaced by \"ismorder\".");
    }
    GET_ATTRIBUTE(ismorder);
    GET_ATTRIBUTE(guiscale);
    GET_ATTRIBUTE(guicenter);
    GET_ATTRIBUTE(c);
    description = xml_get_text(e,"description");
    xmlpp::Node::NodeList subnodes = e->get_children();
    for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
      xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
      if( sne ){
        // rename old "sink" to "receiver":
        if( sne->get_name() == "sink" ){
          sne->set_name("receiver");
          add_warning( "Deprecated element \"sink\", use \"receiver\" instead.", sne );
        }
        if( sne->get_name() == "src_object" ){
          object_sources.push_back(new src_object_t(sne));
          add_warning( "Deprecated element \"src_object\", use \"source\" instead.", sne );
        }
        // parse nodes:
        if( sne->get_name() == "source" )
          object_sources.push_back(new src_object_t(sne));
        else if( sne->get_name() == "diffuse" )
          diffuse_sound_field_infos.push_back(new diffuse_info_t(sne));
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
        else if( sne->get_name() != "include" )
          add_warning("Unrecognized xml element \""+ sne->get_name() +"\".",sne );
      }
    }
    for(std::vector<src_object_t*>::iterator it=object_sources.begin();it!=object_sources.end();++it){
      for(std::vector<sound_t*>::iterator its=(*it)->sound.begin();its!=(*it)->sound.end();++its){
        sounds.push_back( *its );
      }
    }
    std::string guitracking;
    GET_ATTRIBUTE(guitracking);
    std::vector<object_t*> objs(find_object(guitracking));
    if( objs.size() )
      guitrackobject = objs[0];
  }
  catch(...){
    clean_children();
    throw;
  }
}

void scene_t::configure_meter( float tc, TASCAR::levelmeter_t::weight_t w )
{
  std::vector<object_t*> objs(get_objects());
  for(std::vector<object_t*>::iterator it=objs.begin(); it!=objs.end(); ++it)
    (*it)->configure_meter( tc, w );
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

void scene_t::geometry_update(double t)
{
  for(std::vector<object_t*>::iterator it=all_objects.begin();it!=all_objects.end();++it)
    (*it)->geometry_update( t );
}

void scene_t::process_active(double t)
{
  for(std::vector<src_object_t*>::iterator it=object_sources.begin();it!=object_sources.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<diffuse_info_t*>::iterator it=diffuse_sound_field_infos.begin();it!=diffuse_sound_field_infos.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<receivermod_object_t*>::iterator it=receivermod_objects.begin();it!=receivermod_objects.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<face_object_t*>::iterator it=faces.begin();it!=faces.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<face_group_t*>::iterator it=facegroups.begin();it!=facegroups.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<obstacle_group_t*>::iterator it=obstaclegroups.begin();it!=obstaclegroups.end();++it)
    (*it)->process_active(t,anysolo);
  for(std::vector<mask_object_t*>::iterator it=masks.begin();it!=masks.end();++it)
    (*it)->process_active(t,anysolo);
}

mask_object_t::mask_object_t(xmlpp::Element* xmlsrc)
  : object_t(xmlsrc)
{
  dynobject_t::get_attribute("size",xmlsize);
  dynobject_t::get_attribute("falloff",xmlfalloff);
  dynobject_t::get_attribute_bool("inside",mask_inner);
}

void mask_object_t::geometry_update(double t)
{
  dynobject_t::geometry_update(t);
  shoebox_t::size.x = std::max(0.0,xmlsize.x-xmlfalloff);
  shoebox_t::size.y = std::max(0.0,xmlsize.y-xmlfalloff);
  shoebox_t::size.z = std::max(0.0,xmlsize.z-xmlfalloff);
  dynobject_t::get_6dof(shoebox_t::center,shoebox_t::orientation);
  inv_falloff = 1.0/std::max(xmlfalloff,1e-10);
}

void mask_object_t::process_active(double t,uint32_t anysolo)
{
  mask_object_t::active = is_active(anysolo,t);
}

receivermod_object_t::receivermod_object_t(xmlpp::Element* xmlsrc)
  : object_t(xmlsrc), audio_port_t(xmlsrc,false), receiver_t(xmlsrc, default_name("out"))
{
  // test if this is a speaker-based receiver module:
  TASCAR::receivermod_base_speaker_t* spk(dynamic_cast<TASCAR::receivermod_base_speaker_t*>(libdata));
  double maxage(TASCAR::config("tascar.spkcalib.maxage",30));
  if( spk ){
    if( spk->spkpos.has_caliblevel ){
      if( has_caliblevel )
        TASCAR::add_warning("Caliblevel is defined in receiver \""+get_name()+
                            "\" and in layout file \""+spk->spkpos.layout+"\". Will use the value from layout file.");
      caliblevel = spk->spkpos.caliblevel;
    }
    if( spk->spkpos.has_diffusegain ){
      if( has_diffusegain )
        TASCAR::add_warning("Diffusegain is defined in receiver \""+get_name()+
                            "\" and in layout file \""+spk->spkpos.layout+"\". Will use the value from layout file.");
      diffusegain = spk->spkpos.diffusegain;
    }
    if( spk->spkpos.has_caliblevel || spk->spkpos.has_diffusegain || spk->spkpos.has_calibdate ){
      if( spk->spkpos.calibage > maxage )
        TASCAR::add_warning("Calibration of layout file \""+spk->spkpos.layout+"\" is " +
                            TASCAR::days_to_string(spk->spkpos.calibage) +
                            " old (calibrated: "+spk->spkpos.calibdate+", receiver \""+get_name()+"\").",xmlsrc);

    }
  }
}

void receivermod_object_t::prepare( chunk_cfg_t& cf_ )
{
  cf_.n_channels = get_num_channels();
  TASCAR::Acousticmodel::receiver_t::prepare( cf_ );
  object_t::prepare( cf_ );
  reset_meters();
  for(uint32_t k=0;k<get_num_channels();k++)
    addmeter( TASCAR::Acousticmodel::receiver_t::f_sample );
}

void receivermod_object_t::release()
{
  TASCAR::Acousticmodel::receiver_t::release();
  object_t::release();
}

void receivermod_object_t::postproc(std::vector<wave_t>& output)
{
  starttime_samples = TASCAR::Acousticmodel::receiver_t::f_sample*starttime;
  TASCAR::Acousticmodel::receiver_t::postproc(output);
  if( output.size() != rmsmeter.size() ){
    DEBUG(output.size());
    DEBUG(rmsmeter.size());
    throw TASCAR::ErrMsg("Programming error");
  }
  for(uint32_t k=0;k<output.size();k++)
    rmsmeter[k]->update(output[k]);
}

void receivermod_object_t::geometry_update(double t)
{
  dynobject_t::geometry_update(t);
  *(c6dof_t*)this = c6dof;
  boundingbox.geometry_update(t);
}

void receivermod_object_t::process_active(double t,uint32_t anysolo)
{
  TASCAR::Acousticmodel::receiver_t::active = is_active(anysolo,t);
}

src_object_t* scene_t::add_source()
{
  object_sources.push_back(new src_object_t(e->add_child("source")));
  return object_sources.back();
}

std::vector<object_t*> scene_t::get_objects()
{
  std::vector<object_t*> r;
  for(std::vector<src_object_t*>::iterator it=object_sources.begin();it!=object_sources.end();++it)
    r.push_back(*it);
  for(std::vector<diffuse_info_t*>::iterator it=diffuse_sound_field_infos.begin();it!=diffuse_sound_field_infos.end();++it)
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

void scene_t::prepare( chunk_cfg_t& cf_ )
{
  scene_node_base_t::prepare( cf_ );
  if( !name.size() )
    throw TASCAR::ErrMsg("Invalid empty scene name (please set \"name\" attribute of scene node).");
  if( name.find(" ") != std::string::npos )
    throw TASCAR::ErrMsg("Spaces in scene name are not supported (\""+name+"\")");
  if( name.find(":") != std::string::npos )
    throw TASCAR::ErrMsg("Colons in scene name are not supported (\""+name+"\")");
  all_objects = get_objects();
  for(auto it=all_objects.begin();it!=all_objects.end();++it)
    (*it)->prepare( cf_ );
}

void scene_t::release()
{
  scene_node_base_t::release();
  all_objects = get_objects();
  for(auto it=all_objects.begin();it!=all_objects.end();++it)
    (*it)->release();
}

void scene_t::add_licenses( licensehandler_t* session )
{
  for( auto it=sounds.begin();it!=sounds.end();++it)
    (*it)->plugins.add_licenses( session );
  for( std::vector<src_object_t*>::iterator it=object_sources.begin();it!=object_sources.end();++it)
    for( std::vector<sndfile_info_t>::iterator iFile=(*it)->sndfiles.begin();iFile!=(*it)->sndfiles.end();++iFile)
      session->add_license( iFile->license, iFile->attribution, TASCAR::tscbasename( TASCAR::env_expand( iFile->fname ) ) );
  for( std::vector<diffuse_info_t*>::iterator it=diffuse_sound_field_infos.begin();it!=diffuse_sound_field_infos.end();++it)
    for( std::vector<sndfile_info_t>::iterator iFile=(*it)->sndfiles.begin();iFile!=(*it)->sndfiles.end();++iFile)
      session->add_license( iFile->license, iFile->attribution, TASCAR::tscbasename( TASCAR::env_expand( iFile->fname ) ) );
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

void route_t::reset_meters()
{
  rmsmeter.clear(); 
  meterval.clear();
}

std::string route_t::get_type() const
{
  if( dynamic_cast<const TASCAR::Scene::face_object_t*>(this))
    return "face";
  if( dynamic_cast<const TASCAR::Scene::face_group_t*>(this))
    return "facegroup";
  if( dynamic_cast<const TASCAR::Scene::obstacle_group_t*>(this))
    return "obstacle";
  if( dynamic_cast<const TASCAR::Scene::src_object_t*>(this))
    return "source";
  if( dynamic_cast<const TASCAR::Scene::diffuse_info_t*>(this))
    return "diffuse";
  if( dynamic_cast<const TASCAR::Scene::receivermod_object_t*>(this))
    return "receiver";
  return "unknwon";
}

void route_t::addmeter( float fs )
{
  rmsmeter.push_back(new TASCAR::levelmeter_t(fs,meter_tc,meter_weight));
  meterval.push_back(0);
}

route_t::~route_t()
{
  for(uint32_t k=0;k<rmsmeter.size();++k)
    delete rmsmeter[k];
}

void route_t::configure_meter( float tc, TASCAR::levelmeter_t::weight_t w )
{
  meter_tc = tc;
  meter_weight = w;
}

const std::vector<float>& route_t::readmeter()
{
  for(uint32_t k=0;k<rmsmeter.size();k++)
    meterval[k] = rmsmeter[k]->spldb();
  return meterval;
}

float route_t::read_meter_max()
{
  float rv(-HUGE_VAL);
  for(uint32_t k=0;k<rmsmeter.size();k++){
    float l(rmsmeter[k]->spldb());
    if( !(l<rv) )
      rv = l;
  }
  return rv;
}

float sound_t::read_meter()
{
  if( meter.size() > 0 )
    if( meter[0] )
      return meter[0]->spldb();
  return -HUGE_VAL;
}

route_t::route_t(xmlpp::Element* xmlsrc)
  : scene_node_base_t(xmlsrc),mute(false),solo(false),
    meter_tc(2),
    meter_weight(TASCAR::levelmeter_t::Z),
    targetlevel(0)
{
  get_attribute("name",name);
  get_attribute_bool("mute",mute);
  get_attribute_bool("solo",solo);
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

audio_port_t::audio_port_t(xmlpp::Element* xmlsrc, bool is_input_)
  : xml_element_t(xmlsrc),ctlname(""),
    connect(""),
    port_index(0),
    is_input(is_input_),
    gain(1),
    caliblevel(50000.0)
{
  get_attribute("connect",connect);
  get_attribute_db_float("gain",gain);
  has_caliblevel = has_attribute("caliblevel");
  get_attribute_db_float("caliblevel",caliblevel);
  bool inv(false);
  get_attribute_bool("inv",inv);
  set_inv(inv);
}

void audio_port_t::set_gain_db( float g )
{
  if( gain < 0.0f )
    gain = -pow(10.0,0.05*g);
  else
    gain = pow(10.0,0.05*g);
}

void audio_port_t::set_gain_lin( float g )
{
  gain = g;
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
  get_license_info( e, fname, license, attribution );
}

void src_object_t::process_active(double t, uint32_t anysolo)
{
  bool a(is_active(anysolo,t));
  for(std::vector<sound_t*>::iterator it=sound.begin();it!=sound.end();++it)
    (*it)->active = a;
}

void diffuse_info_t::release()
{
  sndfile_object_t::release();
  if( source )
    source->release();
}

void diffuse_info_t::process_active(double t, uint32_t anysolo)
{
  bool a(is_active(anysolo,t));
  if( source )
    source->active = a;
}

void face_object_t::process_active(double t, uint32_t anysolo)
{
  active = is_active(anysolo,t);
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

void TASCAR::Scene::scene_t::validate_attributes(std::string& msg) const
{
  scene_node_base_t::validate_attributes(msg);
  for(std::vector<src_object_t*>::const_iterator it=object_sources.begin();it!=object_sources.end();++it)
    (*it)->validate_attributes(msg);
  for(std::vector<diffuse_info_t*>::const_iterator it=diffuse_sound_field_infos.begin();it!=diffuse_sound_field_infos.end();++it)
    (*it)->dynobject_t::validate_attributes(msg);
  for(std::vector<face_object_t*>::const_iterator it=faces.begin();it!=faces.end();++it)
    (*it)->dynobject_t::validate_attributes(msg);
  for(std::vector<face_group_t*>::const_iterator it=facegroups.begin();it!=facegroups.end();++it)
    (*it)->dynobject_t::validate_attributes(msg);
  for(std::vector<obstacle_group_t*>::const_iterator it=obstaclegroups.begin();it!=obstaclegroups.end();++it)
    (*it)->dynobject_t::validate_attributes(msg);
  for(std::vector<receivermod_object_t*>::const_iterator it=receivermod_objects.begin();it!=receivermod_objects.end();++it)
    (*it)->dynobject_t::validate_attributes(msg);
  for(std::vector<mask_object_t*>::const_iterator it=masks.begin();it!=masks.end();++it)
    (*it)->dynobject_t::validate_attributes(msg);
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
  dynobject_t::GET_ATTRIBUTE(shoeboxwalls);
  if( !shoeboxwalls.is_null() ){
    TASCAR::pos_t sb(shoeboxwalls);
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
  }
  if( !importraw.empty() ){
    std::ifstream rawmesh( TASCAR::env_expand(importraw).c_str() );
    if( !rawmesh.good() )
      throw TASCAR::ErrMsg("Unable to open mesh file \""+TASCAR::env_expand(importraw)+"\".");
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

void face_group_t::geometry_update(double t)
{
  dynobject_t::geometry_update(t);
  for(std::vector<TASCAR::Acousticmodel::reflector_t*>::iterator it=reflectors.begin();it!=reflectors.end();++it){
    (*it)->apply_rot_loc(c6dof.position,c6dof.orientation);
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
    std::ifstream rawmesh( TASCAR::env_expand(importraw).c_str() );
    if( !rawmesh.good() )
      throw TASCAR::ErrMsg("Unable to open mesh file \""+TASCAR::env_expand(importraw)+"\".");
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

void obstacle_group_t::geometry_update(double t)
{
  dynobject_t::geometry_update(t);
  for(std::vector<TASCAR::Acousticmodel::obstacle_t*>::iterator it=obstacles.begin();it!=obstacles.end();++it){
    (*it)->apply_rot_loc(c6dof.position,c6dof.orientation);
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

sound_name_t::sound_name_t( xmlpp::Element* xmlsrc, src_object_t* parent_ )
  : xml_element_t( xmlsrc )
{
  GET_ATTRIBUTE(name);
  if( parent_ && name.empty() )
    name = parent_->next_sound_name();
  if( name.empty() )
    throw TASCAR::ErrMsg("Invalid (empty) sound name.");
  if( parent_ )
    parentname = parent_->get_name();
}

sound_t::sound_t( xmlpp::Element* xmlsrc, src_object_t* parent_ )
  : sound_name_t( xmlsrc, parent_ ),
    source_t(xmlsrc, get_name(), get_parent_name() ),
    audio_port_t(xmlsrc,true),
    parent(parent_),
    chaindist(0),
    gain_(1)
{
  source_t::get_attribute("x",local_position.x);
  source_t::get_attribute("y",local_position.y);
  source_t::get_attribute("z",local_position.z);
  source_t::get_attribute("d",chaindist);
  // parse plugins:
  xmlpp::Node::NodeList subnodes = source_t::e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && (sne->get_name()!="plugins")){
      // add_warning:
      char ctmp[1024];
      sprintf(ctmp,"%d",sne->get_line());
      TASCAR::add_warning("Ignoring entry \""+sne->get_name()+"\" in sound \""+get_fullname()+"\".",sne);
      //TASCAR::add_warning( "Audio plugins in sounds should be placed within the <plugins></plugins> element.\n  (plugin \""+sne->get_name()+"\" in sound \""+get_fullname()+"\", line "+std::string(ctmp)+")");
    }
  }
}

sound_t::~sound_t()
{
}

void sound_t::geometry_update( double t )
{
  pos_t rp(local_position);
  if( parent ){
    TASCAR::pos_t ppos;
    TASCAR::zyx_euler_t por;
    parent->get_6dof(ppos,por);
    if( chaindist != 0 ){
      double tp(t - parent->starttime);
      tp = parent->location.get_time(parent->location.get_dist(tp)-chaindist);
      ppos = parent->location.interp(tp);
    }
    rp *= por;
    rp += ppos;
    orientation = por;
  }
  position = rp;
}

void sound_t::process_plugins( const TASCAR::transport_t& tp )
{
  TASCAR::transport_t ltp(tp);
  if( parent ){
      ltp.object_time_seconds = ltp.session_time_seconds - parent->starttime;
      ltp.object_time_samples = ltp.session_time_samples - f_sample * parent->starttime;
  }
  source_t::process_plugins( ltp );
}

void sound_t::validate_attributes(std::string& msg) const
{
  TASCAR::Acousticmodel::source_t::validate_attributes(msg);
  plugins.validate_attributes( msg );
}

void sound_t::add_meter( TASCAR::levelmeter_t* m )
{
  meter.push_back( m );
}

void sound_t::apply_gain()
{
  double dg((get_gain() - gain_)*t_inc);
  uint32_t channels(inchannels.size());
  for(uint32_t k=0;k<inchannels[0].n;++k){
    gain_ += dg;
    for(uint32_t c=0;c<channels;++c)
      inchannels[c].d[k] *= gain_;
  }
  for(uint32_t k=0;k<get_num_channels();++k)
    meter[k]->update(inchannels[k]);
}

rgb_color_t sound_t::get_color() const
{
  if( parent )
    return parent->color;
  else
    return rgb_color_t();
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
