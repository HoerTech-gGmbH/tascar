#include "audioplayer.h"
#include <string.h>
#include <unistd.h>

using namespace TASCAR;
using namespace TASCAR::Scene;

std::string strrep(std::string s,const std::string& pat, const std::string& rep)
{
  std::string out_string("");
  std::string::size_type len = pat.size(  );
  std::string::size_type pos;
  while( (pos = s.find(pat)) < s.size() ){
    out_string += s.substr(0,pos);
    out_string += rep;
    s.erase(0,pos+len);
  }
  s = out_string + s;
  return s;
}

scene_container_t::scene_container_t(const std::string& xmlfile)
  : own_pointer(true)
{
  xmlpp::DomParser domp(TASCAR::env_expand(xmlfile));
  xmldoc = domp.get_document();
  if( !xmldoc )
    throw TASCAR::ErrMsg("Unable to parse document \""+xmlfile+"\".");
  xmlpp::Element* scene_node(NULL);
  xmlpp::Element* root_node(xmldoc->get_root_node());
  if( !root_node )
    throw TASCAR::ErrMsg("No root node in document \""+xmlfile+"\".");
  if( root_node->get_name() != "session" )
    throw TASCAR::ErrMsg("Expected \"session\" root node, got \""+root_node->get_name()+"\" in document \""+xmlfile+"\".");
  xmlpp::Node::NodeList subnodes = root_node->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "scene" )){
      scene_node = sne;
      break;
    }
  }
  if( !scene_node )
    throw TASCAR::ErrMsg("No \"scene\" node in document \""+xmlfile+"\".");
  scene = new TASCAR::Scene::scene_t(scene_node);
}

scene_container_t::scene_container_t(TASCAR::Scene::scene_t* scenesrc)
  : xmldoc(NULL),scene(scenesrc),own_pointer(false)
{
}

scene_container_t::~scene_container_t()
{
  if( own_pointer && scene )
    delete scene;
}

TASCAR::audioplayer_t::audioplayer_t(const std::string& xmlfile)
  : scene_container_t(xmlfile),jackc_transport_t(jacknamer(scene->name,"player."))
{
}

TASCAR::audioplayer_t::audioplayer_t(TASCAR::Scene::scene_t* scenesrc)
  : scene_container_t(scenesrc),jackc_transport_t(jacknamer(scene->name,"player."))
{
}

TASCAR::audioplayer_t::~audioplayer_t()
{
  if( active )
    deactivate();
}

int TASCAR::audioplayer_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_running)
{
  for(uint32_t ch=0;ch<outBuffer.size();ch++)
    memset(outBuffer[ch],0,nframes*sizeof(float));
  for(uint32_t k=0;k<files.size();k++){
    uint32_t numchannels(infos[k].channels);
    float* dp[numchannels];
    for(uint32_t ch=0;ch<numchannels;ch++)
      dp[ch] = outBuffer[portno[k]+ch];
    files[k].request_data(tp_frame,nframes*tp_running,numchannels,dp);
  }
  return 0;
}

void TASCAR::audioplayer_t::open_files()
{
  if( !scene )
    throw TASCAR::ErrMsg("Invalid scene pointer");
  for(std::vector<TASCAR::Scene::src_object_t>::iterator it=scene->object_sources.begin();it!=scene->object_sources.end();++it){
    infos.insert(infos.end(),it->sndfiles.begin(),it->sndfiles.end());
  }
  for(std::vector<TASCAR::Scene::src_diffuse_t>::iterator it=scene->diffuse_sources.begin();it!=scene->diffuse_sources.end();++it){
    infos.insert(infos.end(),it->sndfiles.begin(),it->sndfiles.end());
  }
  for(std::vector<TASCAR::Scene::sndfile_info_t>::iterator it=infos.begin();it!=infos.end();++it){
    files.push_back(TASCAR::async_sndfile_t(it->channels,1<<18,get_fragsize()));
  }
  portno.clear();
  for(uint32_t k=0;k<files.size();k++){
    files[k].open(infos[k].fname,infos[k].firstchannel,infos[k].starttime*get_srate(),
                  infos[k].gain,infos[k].loopcnt);
    portno.push_back(get_num_output_ports());
    if( infos[k].channels != 1 ){
      for(uint32_t ch=0;ch<infos[k].channels;ch++){
        char pname[1024];
        sprintf(pname,"%s.%d.%d",infos[k].parentname.c_str(),infos[k].objectchannel,ch);
        //DEBUG(pname);
        add_output_port(pname);
      }
    }else{
      char pname[1024];
      sprintf(pname,"%s.%d",infos[k].parentname.c_str(),infos[k].objectchannel);
      //DEBUG(pname);
      add_output_port(pname);
    }
  }
}

void TASCAR::audioplayer_t::start()
{
  if( !scene )
    throw TASCAR::ErrMsg("Invalid (NULL) scene pointer.");
  //DEBUG(get_srate());
  //DEBUG(get_fragsize());
  //DEBUG(scene);
  // first prepare all nodes for audio processing:
  scene->prepare(get_srate(), get_fragsize());
  //DEBUG(1);
  open_files();
  //DEBUG(1);
  for(uint32_t k=0;k<files.size();k++)
    files[k].start_service();
  jackc_t::activate();
  //DEBUG(1);
}


void TASCAR::audioplayer_t::stop()
{
  jackc_t::deactivate();
  for(uint32_t k=0;k<files.size();k++)
    files[k].stop_service();
}

void TASCAR::audioplayer_t::run(bool & b_quit)
{
  start();
  while( !b_quit ){
    usleep( 50000 );
    getchar();
    if( feof( stdin ) )
      b_quit = true;
  }
  stop();
}

TASCAR::renderer_t::renderer_t(xmlpp::Element* xmlsrc)
  : osc_scene_t(xmlsrc),
    jackc_transport_t(jacknamer(name,"render.")),
    world(NULL),active_pointsources(0),active_diffusesources(0),
    total_pointsources(0),
    total_diffusesources(0)
{
}

TASCAR::renderer_t::~renderer_t()
{
  if( active )
    jackc_t::deactivate();
  if( world )
    delete world;
}


int TASCAR::renderer_t::process(jack_nframes_t nframes,
                                const std::vector<float*>& inBuffer,
                                const std::vector<float*>& outBuffer, 
                                uint32_t tp_frame, bool tp_rolling)
{
  //security/stability:
  for(uint32_t ch=0;ch<inBuffer.size();ch++)
    for(uint32_t k=0;k<nframes;k++)
      make_friendly_number_limited(inBuffer[ch][k]);
  double tp_time((double)tp_frame/(double)srate);
  time = tp_time;
  // mute output:
  for(unsigned int k=0;k<outBuffer.size();k++)
    memset(outBuffer[k],0,sizeof(float)*nframes);
  for(unsigned int k=0;k<sinkmod_objects.size();k++){
    sinkmod_objects[k]->clear_output();
    //TASCAR::Acousticmodel::sink_t* psink(sink_objects[k].get_sink());
    //psink->clear();
  }
  geometry_update(tp_time);
  process_active(tp_time);
  // fill inputs:
  for(unsigned int k=0;k<sounds.size();k++){
    TASCAR::Acousticmodel::pointsource_t* psrc(sounds[k]->get_source());
    psrc->audio.copy(inBuffer[sounds[k]->get_port_index()],nframes,sounds[k]->get_gain());
  }
  for(uint32_t k=0;k<door_sources.size();k++){
    TASCAR::Acousticmodel::pointsource_t* psrc(door_sources[k].get_source());
    psrc->audio.copy(inBuffer[door_sources[k].get_port_index()],nframes,door_sources[k].get_gain());
  }
  for(std::vector<src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it){
    TASCAR::Acousticmodel::diffuse_source_t* psrc(it->get_source());
    float gain(it->get_gain());
    psrc->audio.w().copy(inBuffer[it->get_port_index()],nframes,gain);
    psrc->audio.x().copy(inBuffer[it->get_port_index()+1],nframes,gain);
    psrc->audio.y().copy(inBuffer[it->get_port_index()+2],nframes,gain);
    psrc->audio.z().copy(inBuffer[it->get_port_index()+3],nframes,gain);
  }
  // process world:
  if( world ){
    world->process();
    active_pointsources = world->get_active_pointsource();
    active_diffusesources = world->get_active_diffusesource();
  }else{
    active_pointsources = 0;
    active_diffusesources = 0;
  }
  // copy sink output:
  for(unsigned int k=0;k<sinkmod_objects.size();k++){
    //TASCAR::Acousticmodel::sink_t* psink(sink_objects[k].get_sink());
    float gain(sinkmod_objects[k]->get_gain());
    for(uint32_t ch=0;ch<sinkmod_objects[k]->get_num_channels();ch++)
      sinkmod_objects[k]->outchannels[ch].copy_to(outBuffer[sinkmod_objects[k]->get_port_index()+ch],nframes,gain);
  }
  //security/stability:
  for(uint32_t ch=0;ch<outBuffer.size();ch++)
    for(uint32_t k=0;k<nframes;k++)
      make_friendly_number_limited(outBuffer[ch][k]);
  return 0;
}

void TASCAR::renderer_t::start()
{
  // first prepare all nodes for audio processing:
  prepare(get_srate(), get_fragsize());
  sounds = linearize_sounds();
  sources.clear();
  diffusesources.clear();
  for(std::vector<sound_t*>::iterator it=sounds.begin();it!=sounds.end();++it){
    sources.push_back((*it)->get_source());
    (*it)->set_port_index(get_num_input_ports());
    //DEBUG((*it)->get_port_name());
    add_input_port((*it)->get_port_name());
  }
  for(std::vector<src_door_t>::iterator it=door_sources.begin();it!=door_sources.end();++it){
    sources.push_back(it->get_source());
    it->set_port_index(get_num_input_ports());
    //DEBUG(it->get_name());
    add_input_port(it->get_name());
  }
  for(std::vector<src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it){
    diffusesources.push_back(it->get_source());
  }
  for(std::vector<src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it){
    it->set_port_index(get_num_input_ports());
    for(uint32_t ch=0;ch<4;ch++){
      char ctmp[32];
      const char* stmp("wxyz");
      sprintf(ctmp,".%d%c",(ch>0),stmp[ch]);
      //DEBUG(it->get_name()+ctmp);
      add_input_port(it->get_name()+ctmp);
    }
  }
  sinks.clear();
  for(std::vector<sinkmod_object_t*>::iterator it=sinkmod_objects.begin();it!=sinkmod_objects.end();++it){
    TASCAR::Acousticmodel::newsink_t* sink(*it);
    sinks.push_back(sink);
    (*it)->set_port_index(get_num_output_ports());
    for(uint32_t ch=0;ch<sink->get_num_channels();ch++){
      //DEBUG(ch);
      //DEBUG(it->get_name()+sink->get_channel_postfix(ch));
      add_output_port((*it)->get_name()+sink->get_channel_postfix(ch));
    }
  }
  reflectors.clear();
  for(std::vector<face_object_t>::iterator it=faces.begin();it!=faces.end();++it){
    reflectors.push_back(&(*it));
  }
  pmasks.clear();
  for(std::vector<mask_object_t>::iterator it=masks.begin();it!=masks.end();++it){
    pmasks.push_back(&(*it));
  }
  // create the world, before first process callback is called:
  world = new Acousticmodel::world_t(get_srate(),get_fragsize(),sources,diffusesources,reflectors,sinks,pmasks,mirrororder);
  total_pointsources = world->get_total_pointsource();
  total_diffusesources = world->get_total_diffusesource();
  //
  // activate repositioning services for each object:
  add_child_methods();
  jackc_t::activate();
  osc_server_t::activate();
  // connect jack ports of point sources:
  for(unsigned int k=0;k<sounds.size();k++){
    std::string cn(sounds[k]->get_connect());
    if( cn.size() ){
      cn = strrep(cn,"@","player."+name+":"+sounds[k]->get_parent_name());
      connect_in(sounds[k]->get_port_index(),cn,true);
    }
  }
  // connect jack ports of point sources:
  for(unsigned int k=0;k<door_sources.size();k++){
    std::string cn(door_sources[k].get_connect());
    if( cn.size() ){
      cn = strrep(cn,"@","player."+name+":"+door_sources[k].get_name());
      connect_in(door_sources[k].get_port_index(),cn,true);
    }
  }
  // todo: connect diffuse ports.
  // connect sink ports:
  for(unsigned int k=0;k<sinkmod_objects.size();k++){
    std::string cn(sinkmod_objects[k]->get_connect());
    if( cn.size() ){
      cn = strrep(cn,"@","player."+name+":"+sinkmod_objects[k]->get_name());
      for(uint32_t ch=0;ch<sinkmod_objects[k]->get_num_channels();ch++)
        connect_out(sinkmod_objects[k]->get_port_index()+ch,cn+sinkmod_objects[k]->get_channel_postfix(ch),true);
    }
  }
  //for(uint32_t k=0;k<connections.size();k++)
  //  connect(connections[k].src,connections[k].dest,true);
}

void TASCAR::renderer_t::stop()
{
  osc_server_t::deactivate();
  jackc_t::deactivate();
  if( world )
    delete world;
  world = NULL;
}

void TASCAR::renderer_t::run(bool& b_quit)
{
  start();
  while( !b_quit ){
    usleep( 50000 );
    getchar();
    if( feof( stdin ) )
      b_quit = true;
  }
  stop();
}


TASCAR::scene_player_t::scene_player_t(xmlpp::Element* xmlsrc)
  : renderer_t(xmlsrc),
    player(dynamic_cast<TASCAR::Scene::scene_t*>(this))
{
}

void TASCAR::scene_player_t::start()
{
  player.start();
  renderer_t::start();
}

void TASCAR::scene_player_t::stop()
{
  player.stop();
  renderer_t::stop();
}

void TASCAR::scene_player_t::run(bool &b_quit)
{
  start();
  while( !b_quit ){
    usleep( 50000 );
    getchar();
    if( feof( stdin ) )
      b_quit = true;
  }
  stop();
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
