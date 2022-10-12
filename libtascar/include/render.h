/* License (GPL)
 *
 * Copyright (C) 2014,2015,2016,2017,2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "async_file.h"
#include "tascar.h"

namespace TASCAR {

  /**
     \brief Data container for load measurements and profiling
   */
  class render_profiler_t {
  public:
    render_profiler_t();
    void normalize(double t_total);
    void update(const render_profiler_t& src);
    void set_tau(double t, double fs);
    double t_init;
    double t_geo;
    double t_preproc;
    double t_acoustics;
    double t_postproc;
    double t_copy;

  private:
    double B0, A1;
  };

  /**
     \brief Container class for components of virtual acoustic environments
   */
  class render_core_t : public TASCAR::Scene::scene_t {
  public:
    render_core_t(tsccfg::node_t xmlsrc);
    virtual ~render_core_t();
    void configure();
    void release();
    void set_ism_order_range(uint32_t ism_min, uint32_t ism_max);
    /**
       \callgraph
       \callergraph
     */
    void process(uint32_t nframes, const TASCAR::transport_t& tp,
                 const std::vector<float*>& inBuffer,
                 const std::vector<float*>& outBuffer);
    uint32_t num_input_ports() const { return (uint32_t)input_ports.size(); };
    uint32_t num_output_ports() const { return (uint32_t)output_ports.size(); };
    // protected:
    std::vector<Acousticmodel::source_t*> sources;
    std::vector<Acousticmodel::diffuse_t*> diffuse_sound_fields;
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
    render_profiler_t loadaverage;
    Acousticmodel::world_t* world;

  public:
    uint32_t active_pointsources;
    uint32_t active_diffuse_sound_fields;
    uint32_t total_pointsources;
    uint32_t total_diffuse_sound_fields;

  private:
    bool is_prepared;
    TASCAR::amb1wave_t* ambbuf;
    render_profiler_t load_cycle;
  };

} // namespace TASCAR

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
