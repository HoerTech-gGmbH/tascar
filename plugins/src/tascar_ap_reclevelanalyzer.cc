/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2025 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include "audioplugin.h"
#include "errorhandling.h"
#include "ringbuffer.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

class reclevelanalyzer_t : public TASCAR::audioplugin_base_t {
public:
  reclevelanalyzer_t(const TASCAR::audioplugin_cfg_t& cfg);
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t&,
                  const TASCAR::zyx_euler_t&, const TASCAR::transport_t&);
  void add_variables(TASCAR::osc_server_t* srv);
  ~reclevelanalyzer_t();
  void configure();
  void release();

private:
  void updatethread();
  float tau_segment = 0.125;
  float tau_analysis = 30.0;
  float update_interval = 5.0;

  TASCAR::ringbuffer_t* rb = NULL;
  // threads and synchronization:
  std::thread thread;
  std::atomic_bool run_thread = true;
  void sendthread();
  std::mutex mtx;
  std::condition_variable cond;
};

reclevelanalyzer_t::reclevelanalyzer_t(const TASCAR::audioplugin_cfg_t& cfg)
    : audioplugin_base_t(cfg)
{
}

void reclevelanalyzer_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv->set_variable_owner(
      TASCAR::strrep(TASCAR::tscbasename(__FILE__), ".cc", ""));
  // srv->add_float_db("/gain", &gain);
  // srv->add_float("/lingain", &gain);
  // srv->add_method("/fade", "ff", osc_set_fade, this);
  srv->unset_variable_owner();
}

void reclevelanalyzer_t::configure() {}
void reclevelanalyzer_t::release()
{
  if(rb)
    delete rb;
  rb = NULL;
}

reclevelanalyzer_t::~reclevelanalyzer_t() {}

void reclevelanalyzer_t::ap_process(std::vector<TASCAR::wave_t>& chunk,
                                    const TASCAR::pos_t&,
                                    const TASCAR::zyx_euler_t&,
                                    const TASCAR::transport_t&)
{
  if(!rb)
    return;
  if(!chunk.empty()) {
    // uint32_t nch(chunk.size());
    // uint32_t N(chunk[0].n);
    // std::sort(chunk[0].begin(), chunk[0].end(), std::greater<float>());

    // write to ringbuffer
  }
}

void reclevelanalyzer_t::updatethread()
{
  uint32_t segment_len = (uint32_t)(std::max(1.0,tau_segment * f_sample));
  uint32_t num_segments = (uint32_t)(std::max(1.0f,tau_analysis/tau_segment));
  //float update_interval = 5.0;
  TASCAR::wave_t segment(segment_len*n_channels);
  std::unique_lock<std::mutex> lk(mtx);
  while(run_thread) {
    cond.wait_for(lk, 100ms);
    while(rb->read_space() >= segment_len) {
      rb->read( segment.begin(), segment_len );
      float peak = segment.maxabs();
    }
  }
}

// void updatethread()
// {
//   while( runthread && (rb->read_size() > segment) ){
//     rb->read( segment )
//     peak = segment->maxabs();
//     aweight.filter(segment);
//     append_peak_and_rms;
//     update_cnt++;
//     if( update_cnt > num_segments_per_update_interval ){
//        update_cnt = 0;
//        copy_peak_and_rms_to_sortbuffer_then_sort;
//        calc_percentiles;
//        classify_results;
//     }
//   }
// }

REGISTER_AUDIOPLUGIN(reclevelanalyzer_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
