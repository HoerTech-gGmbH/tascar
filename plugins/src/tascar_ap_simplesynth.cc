/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
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

#include "alsamidicc.h"
#include "audioplugin.h"
#include <complex>

struct snddevname_t {
  std::string dev;
  std::string desc;
};

class tone_t {
public:
  tone_t(){};
  std::vector<float> partialweights = {1.0f, 0.5f, 0.25f};
  std::vector<float> amplitudes = {1.0f, 0.5f, 0.25f};
  std::vector<std::complex<float>> phase = {1.0f, 1.0f, 1.0f};
  size_t N = 3u;
  float rampdur_1 = 0;
  float rampt = 0;
  float dt = 0;
  float fscale = 0.0f;
  std::complex<float> dphi = 1.0f;
  std::complex<float> dphi_detune = 1.0f;
  float amplitude = 0.0f;
  int pitch = 0;
  float decay = 0.99f;
  float decaydamping = 0.99f;
  float decayoffset = 0.99f;

  void set_srate(float srate)
  {
    fscale = TASCAR_2PIf / srate;
    dt = 1.0f / srate;
  }
  void set_pitch(int p, float a, float f0, float tau, float tauoff, float onset,
                 float detune, float taupitch)
  {
    if(onset < 1e-4f)
      onset = 1e-4f;
    pitch = p;
    rampt = onset;
    rampdur_1 = 1.0f / onset;
    for(auto& ph : phase)
      ph = 1.0f;
    amplitudes = partialweights;
    float f = powf(2.0f, (float)(p - 69) / 12.0f) * f0;
    dphi = std::polar(1.0f, fscale * f);
    dphi_detune = std::polar(1.0f, fscale * detune);
    if(tau > 0)
      decay = exp(-fscale / tau);
    else
      decay = 1.0f;
    if(taupitch > 0)
      decaydamping = exp(-fscale / (taupitch * f0 / f));
    else
      decaydamping = 1.0f;
    if(tauoff > 0)
      decayoffset = exp(-fscale / tauoff);
    else
      decayoffset = 1.0f;
    amplitude = a;
  }
  void unset_pitch(int p)
  {
    if(pitch == p)
      decay = decayoffset;
    // amplitude = 0.0f;
  }
  void add(float& val)
  {
    if(amplitude > 1e-8f) {
      float oval = 0.0f;
      std::complex<float> ldphi = dphi;
      float compdecay = 1.0f;
      for(size_t k = 0; k < N; ++k) {
        phase[k] *= ldphi * dphi_detune;
        ldphi *= dphi;
        oval += amplitude * amplitudes[k] * std::real(phase[k]);
        amplitudes[k] *= compdecay;
        compdecay *= decaydamping;
      }
      amplitude *= decay;
      if(rampt > 0) {
        oval *= 0.5 + 0.5 * cosf(TASCAR_PIf * rampt * rampdur_1);
        rampt -= dt;
      }
      val += oval;
    } else
      amplitude = 0.0f;
  }
};

class simplesynth_t : public TASCAR::audioplugin_base_t,
                      public TASCAR::midi_ctl_t {
public:
  simplesynth_t(const TASCAR::audioplugin_cfg_t& cfg);
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos,
                  const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void add_variables(TASCAR::osc_server_t* srv);
  ~simplesynth_t();
  void configure();
  void release();
  virtual void emit_event_note(int channel, int pitch, int velocity);
  virtual void emit_event(int, int, int){};

private:
  int midichannel = 0;
  size_t nexttone = 0;
  float f0 = 440.0;
  uint32_t maxvoices = 8;
  std::vector<float> partialweights = {1.0f,    0.562f,  0.316f, 0.355f,
                                       0.282f,  0.355f,  0.2f,   0.0891f,
                                       0.0398f, 0.0398f, 0.0398f};
  float decay = 4.0f;
  float decayoffset = 0.5f;
  float onset = 0.02f;
  std::vector<tone_t> tones;
  float level = 0.06f;
  float detune = 1.0f;
  float decaydamping = 8.0f;
  std::string connect;
};

simplesynth_t::simplesynth_t(const TASCAR::audioplugin_cfg_t& cfg)
    : audioplugin_base_t(cfg), TASCAR::midi_ctl_t("simplesynth")
{
  bool autoconnect = false;
  GET_ATTRIBUTE(midichannel, "", "MIDI channel");
  GET_ATTRIBUTE_DBSPL(level, "Sound level");
  GET_ATTRIBUTE(maxvoices, "", "Maximum number of polyphonic voices");
  GET_ATTRIBUTE(partialweights, "", "Linear amplitudes of tone components");
  GET_ATTRIBUTE(decay, "s", "Tone decay time");
  GET_ATTRIBUTE(decaydamping, "s", "Damping tone decay time");
  GET_ATTRIBUTE(decayoffset, "s", "Tone offset decay time");
  GET_ATTRIBUTE(f0, "Hz", "Tuning frequency");
  GET_ATTRIBUTE(onset, "s", "Onset time");
  GET_ATTRIBUTE_BOOL(autoconnect, "Autoconnect to input ports");
  GET_ATTRIBUTE(connect, "", "ALSA device name to connect to");
  GET_ATTRIBUTE(detune, "Hz", "Detuning frequency in Hz");
  tones.resize(maxvoices);
  if(!connect.empty()) {
    connect_input(connect, true);
  }
  if(autoconnect) {
    int thisclient = get_client_id();
    auto cl = get_client_ids();
    for(auto c : cl) {
      if(c != thisclient) {
        auto ports = client_get_ports(c, SND_SEQ_PORT_CAP_READ);
        for(auto p : ports) {
          connect_input(c, p);
        }
      }
    }
  }
}

simplesynth_t::~simplesynth_t() {}

void simplesynth_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv->set_variable_owner(
      TASCAR::strrep(TASCAR::tscbasename(__FILE__), ".cc", ""));
  srv->add_float_dbspl("/level", &level, "[0,100]", "Sound level in dB SPL");
  srv->add_float("/decay", &decay, "]0,20]", "Decay time in s");
  srv->add_float("/decayoffset", &decayoffset, "]0,20]",
                 "Offset decay time in s");
  srv->add_float("/decaydamping", &decaydamping, "[0,10]",
                 "Damping decay in s");
  srv->add_float("/f0", &f0, "[100,1000]", "Tuning frequency in Hz");
  srv->add_float("/onset", &onset, "[0,0.2]", "Onset duration in s");
  srv->add_float("/detune", &detune, "[-10,10]", "Detuning in Hz");
  srv->unset_variable_owner();
}

void simplesynth_t::ap_process(std::vector<TASCAR::wave_t>& chunk,
                               const TASCAR::pos_t&, const TASCAR::zyx_euler_t&,
                               const TASCAR::transport_t&)
{
  if(chunk.size() > 0) {
    for(uint32_t k = 0; k < chunk[0].n; ++k) {
      float v = 0.0f;
      for(auto& t : tones)
        t.add(v);
      v *= level;
      for(uint32_t ch = 0; ch < n_channels; ++ch)
        chunk[ch].d[k] += v;
    }
  }
}

void simplesynth_t::emit_event_note(int channel, int pitch, int velocity)
{
  if(midichannel == channel) {
    if(velocity > 0) {
      tones[nexttone].set_pitch(
          pitch, velocity / 127.0f, f0, decay, decayoffset,
          onset * (1.0f - (float)velocity / 127.0f), detune, decaydamping);
      ++nexttone;
      if(nexttone >= tones.size())
        nexttone = 0;
    } else {
      for(auto& t : tones)
        t.unset_pitch(pitch);
    }
  }
}

void simplesynth_t::configure()
{
  TASCAR::audioplugin_base_t::configure();
  for(auto& t : tones) {
    t.partialweights = partialweights;
    t.phase.resize(partialweights.size());
    for(auto& p : t.phase)
      p = 1.0f;
    t.amplitudes.resize(partialweights.size());
    t.set_srate(f_sample);
    t.N = partialweights.size();
  }
  start_service();
}

void simplesynth_t::release()
{
  stop_service();
  TASCAR::audioplugin_base_t::release();
}

REGISTER_AUDIOPLUGIN(simplesynth_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
