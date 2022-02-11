/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
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

class loopmachine_t : public TASCAR::audioplugin_base_t {
public:
  loopmachine_t(const TASCAR::audioplugin_cfg_t& cfg);
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos,
                  const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void add_variables(TASCAR::osc_server_t* srv);
  void configure();
  void release();
  ~loopmachine_t();

private:
  double bpm;
  double durationbeats;
  double ramplen;
  bool bypass;
  bool clear;
  bool record;
  float gain;
  TASCAR::looped_wave_t* loop;
  TASCAR::wave_t* ramp;
  size_t rec_counter;
  size_t ramp_counter;
  size_t t_rec_counter;
  size_t t_ramp_counter;
};

loopmachine_t::loopmachine_t(const TASCAR::audioplugin_cfg_t& cfg)
    : audioplugin_base_t(cfg), bpm(120), durationbeats(4), ramplen(0.01),
      bypass(false), clear(false), record(false), gain(1.0f), loop(NULL),
      ramp(NULL), rec_counter(0), ramp_counter(0), t_rec_counter(0),
      t_ramp_counter(0)
{
  GET_ATTRIBUTE(bpm, "", "Beats per minute");
  GET_ATTRIBUTE(durationbeats, "beats", "Record duration");
  GET_ATTRIBUTE(ramplen, "s", "Ramp length");
  GET_ATTRIBUTE_DB(gain, "Playback gain");
  GET_ATTRIBUTE_BOOL(bypass, "Start in bypass mode");
}

void loopmachine_t::configure()
{
  audioplugin_base_t::configure();
  loop = new TASCAR::looped_wave_t(f_sample * durationbeats / bpm * 60.0);
  loop->set_loop(0);
  ramp = new TASCAR::wave_t(f_sample * ramplen);
  for(size_t k = 0; k < ramp->n; ++k)
    ramp->d[k] = 0.5f + 0.5f * cosf(k * t_sample * TASCAR_PI / ramplen);
}

void loopmachine_t::release()
{
  audioplugin_base_t::release();
  delete loop;
  delete ramp;
}

void loopmachine_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv->add_bool_true("/clear", &clear);
  srv->add_bool_true("/record", &record);
  srv->add_bool("/bypass", &bypass);
  srv->add_float("/gain", &gain);
  srv->add_float_db("/gaindb", &gain);
}

loopmachine_t::~loopmachine_t() {}

void loopmachine_t::ap_process(std::vector<TASCAR::wave_t>& chunk,
                               const TASCAR::pos_t& pos,
                               const TASCAR::zyx_euler_t&,
                               const TASCAR::transport_t& tp)
{
  if(chunk.size() == 0)
    return;
  if(record) {
    record = false;
    rec_counter = loop->n;
    ramp_counter = ramp->n;
    t_rec_counter = 0;
    t_ramp_counter = 0;
  }
  if(clear) {
    clear = false;
    loop->clear();
  }
  for(size_t k = 0; k < n_fragment; ++k) {
    if(rec_counter) {
      loop->d[t_rec_counter] = chunk[0].d[k];
      if(t_rec_counter < ramp->n)
        loop->d[t_rec_counter] *= 1.0f - ramp->d[t_rec_counter];
      --rec_counter;
      ++t_rec_counter;
    } else if(ramp_counter) {
      loop->d[t_ramp_counter] += (ramp->d[t_ramp_counter] * chunk[0].d[k]);
      --ramp_counter;
      ++t_ramp_counter;
    }
  }
  if(bypass) {
    loop->add_chunk_looped(0.0f, chunk[0]);
  } else {
    loop->add_chunk_looped(gain, chunk[0]);
  }
}

REGISTER_AUDIOPLUGIN(loopmachine_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
