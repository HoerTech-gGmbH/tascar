#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "tascar.h"
#include "async_file.h"
#include "osc_scene.h"

namespace TASCAR {

  class scene_container_t {
  public:
    scene_container_t(const std::string& xmlfile);
    scene_container_t(TASCAR::Scene::scene_t* scenesrc);
    ~scene_container_t();
  protected:
    xmlpp::Document* xmldoc;
    TASCAR::Scene::scene_t* scene;
    bool own_pointer;
  };    
  
  class audioplayer_t : public scene_container_t, public jackc_transport_t  {
  public:
    audioplayer_t(const std::string& xmlfile="");
    audioplayer_t(TASCAR::Scene::scene_t*);
    virtual ~audioplayer_t();
    void run(bool &b_quit);
    void start();
    void stop();
  private:
    int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_running);
    void open_files();
    std::vector<TASCAR::Scene::sndfile_info_t> infos;
    std::vector<TASCAR::async_sndfile_t> files;
    std::vector<uint32_t> jack_port_map;
  };

  //class renderer_t : public scene_container_t, public TASCAR::Scene::osc_scene_t, public jackc_transport_t  {
  class renderer_t : public TASCAR::Scene::osc_scene_t, public jackc_transport_t  {
  public:
    renderer_t(xmlpp::Element* xmlsrc);
    virtual ~renderer_t();
    void run(bool &b_quit);
    void start();
    void stop();
  private:
    std::vector<TASCAR::Scene::sound_t*> sounds;
    std::vector<Acousticmodel::pointsource_t*> sources;
    std::vector<Acousticmodel::diffuse_source_t*> diffusesources;
    std::vector<Acousticmodel::reflector_t*> reflectors;
    std::vector<Acousticmodel::receiver_t*> receivers;
    std::vector<Acousticmodel::mask_t*> pmasks;
    // jack callback:
    int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer, uint32_t tp_frame, bool tp_rolling);
    Acousticmodel::world_t* world;
  public:
    double time;
    uint32_t active_pointsources;
    uint32_t active_diffusesources;
    uint32_t total_pointsources;
    uint32_t total_diffusesources;
  };

  class scene_player_t : public renderer_t  {
  public:
    scene_player_t(xmlpp::Element* xmlsrc);
    void start();
    void stop();
    void run(bool &b_quit);
  private:
    audioplayer_t player;
  };

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
