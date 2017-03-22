#ifndef JACKRENDER_H
#define JACKRENDER_H

#include "render.h"
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
  
  class audioplayer_t : public TASCAR::scene_container_t, public jackc_transport_t  {
  public:
    audioplayer_t(const std::string& xmlfile="");
    audioplayer_t(TASCAR::Scene::scene_t*);
    virtual ~audioplayer_t();
    void run(bool &b_quit);
    void start();
    void stop();
  private:
    int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_rolling);
    void open_files();
    std::vector<TASCAR::Scene::sndfile_info_t> infos;
    std::vector<TASCAR::async_sndfile_t> files;
    std::vector<uint32_t> jack_port_map;
  };

  class render_rt_t : public TASCAR::render_core_t, public TASCAR::Scene::osc_scene_t, public jackc_transport_t  {
  public:
    render_rt_t(xmlpp::Element* xmlsrc);
    virtual ~render_rt_t();
    virtual void write_xml();
    void run(bool &b_quit);
    void start();
    void stop();
  private:
    // jack callback:
    int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer, uint32_t tp_frame, bool tp_rolling);
  };

  class scene_player_t : public render_rt_t  {
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
