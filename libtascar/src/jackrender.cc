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

TASCAR::scene_render_rt_t::scene_render_rt_t(xmlpp::Element* xmlsrc)
  : render_core_t(xmlsrc),
    osc_scene_t(xmlsrc,this),
    jackc_transport_t(jacknamer(name,"render."))
{
}

TASCAR::scene_render_rt_t::~scene_render_rt_t()
{
  if( jackc_t::active )
    jackc_t::deactivate();
}

/**
   \ingroup callgraph
 */
int TASCAR::scene_render_rt_t::process(jack_nframes_t nframes,
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

void TASCAR::scene_render_rt_t::start()
{
  chunk_cfg_t cf( get_srate(), get_fragsize() );
  // first prepare all nodes for audio processing:
  prepare( cf );
  try{
  // create all ports:
  for(std::vector<std::string>::iterator iip=input_ports.begin();iip!=input_ports.end();++iip)
    add_input_port(*iip);
  for(std::vector<std::string>::iterator iop=output_ports.begin();iop!=output_ports.end();++iop)
    add_output_port(*iop);
  add_input_port("sync_in");
  jackc_t::activate();
  //osc_server_t::activate();
  // connect jack ports of point sources:
  for(unsigned int k=0;k<sounds.size();k++){
    std::vector<std::string> cn(sounds[k]->get_connect());
    for( auto it=cn.begin();it!=cn.end();++it)
      *it = strrep(*it, "@", "player."+name+":"+sounds[k]->get_parent_name());
    if( cn.size() ){
      std::vector<std::string> cno(get_port_names_regexp( cn ));
      if( !cno.size() )
        throw TASCAR::ErrMsg("Port list \""+TASCAR::vecstr2str(cn)+"\" did not match any port.");
      for( auto it=cno.begin();it!=cno.end();++it)
        if( it->size() )
          connect_in(sounds[k]->get_port_index(),*it,true);
    }
    // connect diffuse ports:
    for(std::vector<TASCAR::Scene::diff_snd_field_obj_t*>::iterator idiff=diff_snd_field_objects.begin();idiff!=diff_snd_field_objects.end();++idiff){
      TASCAR::Scene::diff_snd_field_obj_t* pdiff(*idiff);
      std::vector<std::string> cn(pdiff->get_connect());
      for( auto it=cn.begin();it!=cn.end();++it)
        *it = strrep(*it, "@", "player."+name+":"+pdiff->get_name());
      //cn = get_port_names_regexp( cn );
      uint32_t pi(pdiff->get_port_index());
      for( auto it=cn.begin();it!=cn.end();++it)
        if( it->size() ){
          for(uint32_t k=0;k<4;++k){
            char ctmp[1024];
            sprintf(ctmp,"%s.%d",it->c_str(),k);
            connect_in(pi+k,ctmp,true);
          }
        }
    }
  }
  // connect receiver ports:
  for(unsigned int k=0;k<receivermod_objects.size();k++){
    std::vector<std::string> cn(receivermod_objects[k]->get_connect());
    for( auto it=cn.begin();it!=cn.end();++it)
      if( it->size() ){
        for(uint32_t ch=0;ch<receivermod_objects[k]->n_channels;ch++)
          connect_out(receivermod_objects[k]->get_port_index()+ch,
                      *it+receivermod_objects[k]->labels[ch],true);
      }
    std::vector<std::string> cns(receivermod_objects[k]->get_connections());
    for(uint32_t kc=0;kc<std::min((uint32_t)(cns.size()),
                                  receivermod_objects[k]->n_channels);kc++){
      if( cns[kc].size() )
        connect_out(receivermod_objects[k]->get_port_index()+kc,
                    cns[kc],true);
    }
  }
  }
  catch( ... ){
    release();
    throw;
  }
}

void TASCAR::scene_render_rt_t::stop()
{
  jackc_t::deactivate();
  release();
}

void TASCAR::scene_render_rt_t::run(bool& b_quit)
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
