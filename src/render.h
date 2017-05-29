#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "tascar.h"
#include "async_file.h"

namespace TASCAR {

  class render_core_t : public TASCAR::Scene::scene_t {
  public:
    render_core_t(xmlpp::Element* xmlsrc);
    virtual ~render_core_t();
    void prepare(double fs, uint32_t fragsize);
    void release();
    void set_ism_order_range( uint32_t ism_min, uint32_t ism_max, bool b_0_14=false );
    void set_v014();
    void process(uint32_t nframes,
                 const TASCAR::transport_t& tp,
                 const std::vector<float*>& inBuffer,
                 const std::vector<float*>& outBuffer);
    uint32_t num_input_ports() const { return input_ports.size();};
    uint32_t num_output_ports() const { return output_ports.size();};
    //void configure_levelmeter( float fs, float tc, TASCAR::levelmeter_t::weight_t w );
  protected:
    std::vector<TASCAR::Scene::sound_t*> sounds;
    std::vector<Acousticmodel::pointsource_t*> sources;
    std::vector<Acousticmodel::diffuse_source_t*> diffusesources;
    std::vector<Acousticmodel::reflector_t*> reflectors;
    std::vector<Acousticmodel::obstacle_t*> obstacles;
    std::vector<Acousticmodel::receiver_t*> receivers;
    std::vector<Acousticmodel::mask_t*> pmasks;
    std::vector<std::string> input_ports;
    std::vector<std::string> output_ports;
    std::vector<TASCAR::Scene::audio_port_t*> audioports;
    std::vector<TASCAR::Scene::audio_port_t*> audioports_in;
    std::vector<TASCAR::Scene::audio_port_t*> audioports_out;
    pthread_mutex_t mtx_world;
  public:
    Acousticmodel::world_t* world;
  public:
    //double time;
    uint32_t active_pointsources;
    uint32_t active_diffusesources;
    uint32_t total_pointsources;
    uint32_t total_diffusesources;
  private:
    bool is_prepared;
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
