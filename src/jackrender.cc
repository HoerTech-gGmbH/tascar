#include "jackrender.h"
#include <string.h>
#include <unistd.h>

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

TASCAR::scene_container_t::scene_container_t(const std::string& xmlfile)
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

TASCAR::scene_container_t::scene_container_t(TASCAR::Scene::scene_t* scenesrc)
  : xmldoc(NULL),scene(scenesrc),own_pointer(false)
{
}

TASCAR::scene_container_t::~scene_container_t()
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

int TASCAR::audioplayer_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_rolling)
{
  for(uint32_t ch=0;ch<outBuffer.size();ch++)
    memset(outBuffer[ch],0,nframes*sizeof(float));
  for(uint32_t k=0;k<files.size();k++){
    uint32_t numchannels(infos[k].channels);
    float* dp[numchannels];
    for(uint32_t ch=0;ch<numchannels;ch++)
      dp[ch] = outBuffer[jack_port_map[k]+ch];
    files[k].request_data(tp_frame,nframes*tp_rolling,numchannels,dp);
  }
  return 0;
}

void TASCAR::audioplayer_t::open_files()
{
  if( !scene )
    throw TASCAR::ErrMsg("Invalid scene pointer");
  for(std::vector<TASCAR::Scene::src_object_t*>::iterator it=scene->object_sources.begin();it!=scene->object_sources.end();++it){
    infos.insert(infos.end(),(*it)->sndfiles.begin(),(*it)->sndfiles.end());
  }
  for(std::vector<TASCAR::Scene::src_diffuse_t*>::iterator it=scene->diffuse_sources.begin();it!=scene->diffuse_sources.end();++it){
    infos.insert(infos.end(),(*it)->sndfiles.begin(),(*it)->sndfiles.end());
  }
  for(std::vector<TASCAR::Scene::sndfile_info_t>::iterator it=infos.begin();it!=infos.end();++it){
    files.push_back(TASCAR::async_sndfile_t(it->channels,1<<18,get_fragsize()));
  }
  jack_port_map.clear();
  for(uint32_t k=0;k<files.size();k++){
    files[k].open(infos[k].fname,infos[k].firstchannel,infos[k].starttime*get_srate(),
                  infos[k].gain,infos[k].loopcnt);
    jack_port_map.push_back(get_num_output_ports());
    if( infos[k].channels != 1 ){
      for(uint32_t ch=0;ch<infos[k].channels;ch++){
        char pname[1024];
        sprintf(pname,"%s.%d.%d",infos[k].parentname.c_str(),infos[k].objectchannel,ch);
        add_output_port(pname);
      }
    }else{
      char pname[1024];
      sprintf(pname,"%s.%d",infos[k].parentname.c_str(),infos[k].objectchannel);
      add_output_port(pname);
    }
  }
}

void TASCAR::audioplayer_t::start()
{
  if( !scene )
    throw TASCAR::ErrMsg("Invalid (NULL) scene pointer.");
  // first prepare all nodes for audio processing:
  scene->prepare(get_srate(), get_fragsize());
  open_files();
  for(uint32_t k=0;k<files.size();k++)
    files[k].start_service();
  jackc_t::activate();
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
TASCAR::render_rt_t::render_rt_t(xmlpp::Element* xmlsrc)
  : render_core_t(xmlsrc),
    osc_scene_t(xmlsrc,this),
    jackc_transport_t(jacknamer(name,"render."))
{
}

TASCAR::render_rt_t::~render_rt_t()
{
  if( active )
    jackc_t::deactivate();
}

void TASCAR::render_rt_t::write_xml()
{
  TASCAR::render_core_t::write_xml();
}

/**
   \ingroup callgraph
 */
int TASCAR::render_rt_t::process(jack_nframes_t nframes,
                                const std::vector<float*>& inBuffer,
                                const std::vector<float*>& outBuffer, 
                                uint32_t tp_frame, bool tp_rolling)
{
  TASCAR::transport_t tp;
  tp.rolling = tp_rolling;
  tp.session_time_samples = tp_frame;
  tp.session_time_seconds = (double)tp_frame/(double)srate;
  tp.object_time_samples = tp_frame;
  tp.object_time_seconds = (double)tp_frame/(double)srate;
  render_core_t::process(nframes,tp,inBuffer,outBuffer);
  return 0;
}

void TASCAR::render_rt_t::start()
{
  // first prepare all nodes for audio processing:
  prepare(get_srate(), get_fragsize());
  // create all ports:
  for(std::vector<std::string>::iterator iip=input_ports.begin();iip!=input_ports.end();++iip)
    add_input_port(*iip);
  for(std::vector<std::string>::iterator iop=output_ports.begin();iop!=output_ports.end();++iop)
    add_output_port(*iop);
  // activate repositioning services for each object:
  add_child_methods(this);
  add_input_port("sync_in");
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
    std::string cn(door_sources[k]->get_connect());
    if( cn.size() ){
      cn = strrep(cn,"@","player."+name+":"+door_sources[k]->get_name());
      connect_in(door_sources[k]->get_port_index(),cn,true);
    }
  }
  // todo: connect diffuse ports.
  for(std::vector<TASCAR::Scene::src_diffuse_t*>::iterator idiff=diffuse_sources.begin();idiff!=diffuse_sources.end();++idiff){
    TASCAR::Scene::src_diffuse_t* pdiff(*idiff);
    std::string cn(pdiff->get_connect());
    cn = strrep(cn,"@","player."+name+":"+pdiff->get_name());
    uint32_t pi(pdiff->get_port_index());
    if( cn.size() ){
      for(uint32_t k=0;k<4;++k){
        char ctmp[1024];
        sprintf(ctmp,"%s.%d",cn.c_str(),k);
        connect_in(pi+k,ctmp,true);
      }
    }
  }
  // connect receiver ports:
  for(unsigned int k=0;k<receivermod_objects.size();k++){
    std::string cn(receivermod_objects[k]->get_connect());
    if( cn.size() ){
      cn = strrep(cn,"@","player."+name+":"+receivermod_objects[k]->get_name());
      for(uint32_t ch=0;ch<receivermod_objects[k]->get_num_channels();ch++)
        connect_out(receivermod_objects[k]->get_port_index()+ch,cn+receivermod_objects[k]->get_channel_postfix(ch),true);
    }
    std::vector<std::string> cns(receivermod_objects[k]->get_connections());
    for(uint32_t kc=0;kc<std::min((uint32_t)(cns.size()),
                                  receivermod_objects[k]->get_num_channels());kc++){
      if( cns[kc].size() )
        connect_out(receivermod_objects[k]->get_port_index()+kc,
                    cns[kc],true);
    }
  }
}

void TASCAR::render_rt_t::stop()
{
  osc_server_t::deactivate();
  jackc_t::deactivate();
  release();
}

void TASCAR::render_rt_t::run(bool& b_quit)
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
  : render_rt_t(xmlsrc),
    player(dynamic_cast<TASCAR::Scene::scene_t*>(this))
{
}

void TASCAR::scene_player_t::start()
{
  player.start();
  render_rt_t::start();
}

void TASCAR::scene_player_t::stop()
{
  player.stop();
  render_rt_t::stop();
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
