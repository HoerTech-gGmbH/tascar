#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "tascar.h"
#include "scene.h"
#include "async_file.h"
#include "osc_scene.h"

namespace TASCAR {

  class audioplayer_t : public TASCAR::Scene::scene_t, public jackc_transport_t  {
  public:
    audioplayer_t(const std::string& jackname,const std::string& xmlfile="");
    virtual ~audioplayer_t();
    void run(bool &b_quit);
    void start();
    void stop();
  private:
    int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_running);
    void open_files();
    std::vector<TASCAR::Scene::sndfile_info_t> infos;
    std::vector<TASCAR::async_sndfile_t> files;
    std::vector<uint32_t> portno;
  };

  class renderer_t : public TASCAR::Scene::osc_scene_t, public jackc_transport_t  {
  public:
    renderer_t(const std::string& srv_addr, 
               const std::string& srv_port, 
               const std::string& jack_name, 
               const std::string& cfg_file);
    virtual ~renderer_t();
    void run(bool &b_quit);
    void start();
    void stop();
  private:
    std::vector<TASCAR::Scene::sound_t*> sounds;
    std::vector<Acousticmodel::pointsource_t*> sources;
    std::vector<Acousticmodel::diffuse_source_t*> diffusesources;
    std::vector<Acousticmodel::reflector_t*> reflectors;
    std::vector<Acousticmodel::sink_t*> sinks;
    std::vector<Acousticmodel::mask_t*> pmasks;
    // jack callback:
    int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer, uint32_t tp_frame, bool tp_rolling);
    Acousticmodel::world_t* world;
  public:
    double time;
  };

  class scene_player_t : private audioplayer_t, public renderer_t {
  public:
    scene_player_t(const std::string& srv_addr, 
                   const std::string& srv_port, 
                   const std::string& jack_name, 
                   const std::string& cfg_file);
    void start();
    void stop();
    void run(bool &b_quit);
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
