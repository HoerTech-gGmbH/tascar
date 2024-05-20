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
#include "errorhandling.h"
#include "levelmeter.h"
#include "ola.h"
#include "session.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <lo/lo.h>
#include <mutex>
#include <stdlib.h>
#include <thread>

using namespace std::chrono_literals;

class granularsynth_vars_t : public TASCAR::module_base_t {
public:
  granularsynth_vars_t(const TASCAR::module_cfg_t& cfg);
  ~granularsynth_vars_t(){};

protected:
  std::string id = "granularsynth";
  std::string prefix = "/c/";
  std::string url;
  std::string path = "/grainstorefill";
  float wet = 1.0;
  uint32_t wlen = 8192;
  double f0 = 415;
  uint32_t numgrains = 100;
  double t0 = 0;
  double bpm = 120;
  double loop = 64;
  double gain = 1.0;
  double ponset = 1.0;
  double psustain = 0.0;
  bool active_ = true;
  bool bypass_ = false;
  uint32_t fillthreshold = 5;
  bool oscactive = false;
  std::vector<double> pitches;
  std::vector<double> durations;
  float hue = 0;
  float saturation = 1;
};

granularsynth_vars_t::granularsynth_vars_t(const TASCAR::module_cfg_t& cfg)
    : TASCAR::module_base_t(cfg)
{
  GET_ATTRIBUTE(id, "", "ID used in jack name and OSC path");
  GET_ATTRIBUTE(prefix, "", "prefix used in OSC path");
  GET_ATTRIBUTE(wet, "", "Mixing gain");
  GET_ATTRIBUTE(wlen, "samples", "window length");
  GET_ATTRIBUTE(f0, "Hz", "frequency of pitch 0");
  GET_ATTRIBUTE(pitches, "semitones", "Pitch numbers");
  GET_ATTRIBUTE(durations, "beats", "Durations");
  GET_ATTRIBUTE(numgrains, "", "Number of grains to keep");
  GET_ATTRIBUTE(t0, "s", "Melody start time");
  GET_ATTRIBUTE(bpm, "bpm", "Tempo");
  GET_ATTRIBUTE(loop, "beats", "Time when to loop");
  GET_ATTRIBUTE(ponset, "", "Onset playback probabbility");
  GET_ATTRIBUTE(psustain, "", "Sustained sound probability");
  GET_ATTRIBUTE_DB(gain, "Gain");
  get_attribute_bool("active", active_, "", "active");
  get_attribute_bool("bypass", bypass_, "", "bypass");
  GET_ATTRIBUTE(url, "", "Grainstore fill URL");
  GET_ATTRIBUTE(path, "", "Grainstore fill path");
  GET_ATTRIBUTE(fillthreshold, "",
                "Minimum number of grains per frequency in fill counter");
  GET_ATTRIBUTE_BOOL(oscactive, "Activate OSC sending on start");
  GET_ATTRIBUTE(hue, "degree", "Hue component (0-360)");
  GET_ATTRIBUTE(saturation, "", "Saturation component (0-1)");
}

class grainloop_t : public std::vector<TASCAR::spec_t*> {
public:
  grainloop_t(double f0, uint32_t nbins, uint32_t ngrains);
  grainloop_t(const grainloop_t& src);
  ~grainloop_t();
  size_t get_num_filled() const { return num_filled; };
  void add(const TASCAR::spec_t& s);
  void play(TASCAR::spec_t& s, double gain);
  void reset()
  {
    num_filled = 0;
    fill_next = 0;
    num_next = 0;
  };

private:
  size_t num_filled;
  size_t num_next;
  size_t fill_next;
  uint32_t nbins_;
  uint32_t ngrains_;

public:
  double f;
};

grainloop_t::grainloop_t(const grainloop_t& src)
    : std::vector<TASCAR::spec_t*>(), num_filled(0), num_next(0), fill_next(0),
      nbins_(src.nbins_), ngrains_(src.ngrains_), f(src.f)
{
  for(uint32_t k = 0; k < ngrains_; ++k)
    push_back(new TASCAR::spec_t(nbins_));
}

grainloop_t::grainloop_t(double f0, uint32_t nbins, uint32_t ngrains)
    : num_filled(0), num_next(0), fill_next(0), nbins_(nbins),
      ngrains_(ngrains), f(f0)
{
  for(uint32_t k = 0; k < ngrains; ++k) {
    push_back(new TASCAR::spec_t(nbins));
  }
}

grainloop_t::~grainloop_t()
{
  for(auto it = rbegin(); it != rend(); ++it) {
    delete *it;
  }
}

void grainloop_t::add(const TASCAR::spec_t& s)
{
  if(fill_next < size()) {
    operator[](fill_next)->copy(s);
    ++fill_next;
    if(num_filled < size())
      ++num_filled;
  }
  if(fill_next >= size())
    fill_next = 0;
}

void grainloop_t::play(TASCAR::spec_t& s, double gain)
{
  if(num_next < std::min(size(), num_filled)) {
    s.add_scaled(*(operator[](num_next)), gain);
    ++num_next;
  }
  if(num_next >= std::min(size(), num_filled))
    num_next = 0;
}

class granularsynth_t : public granularsynth_vars_t, public jackc_db_t {
public:
  granularsynth_t(const TASCAR::module_cfg_t& cfg);
  ~granularsynth_t();
  virtual int inner_process(jack_nframes_t nframes,
                            const std::vector<float*>& inBuffer,
                            const std::vector<float*>& outBuffer);
  virtual int process(jack_nframes_t nframes,
                      const std::vector<float*>& inBuffer,
                      const std::vector<float*>& outBuffer);
  static int osc_apply(const char* path, const char* types, lo_arg** argv,
                       int argc, lo_message msg, void* user_data);
  void set_apply(float t);

protected:
  std::vector<grainloop_t> grains;
  TASCAR::ola_t ola1;
  TASCAR::ola_t ola2;
  TASCAR::minphase_t minphase;
  // TASCAR::wave_t phase;
  float delaystate;
  TASCAR::wave_t in_delayed;
  uint32_t t_apply;
  float deltaw;
  float currentw;
  std::vector<uint64_t> startingtimes;
  std::vector<uint64_t> idurations;
  std::vector<double> vfreqs;
  uint64_t tprev;
  bool reset;
  lo_address target = NULL;
  float* p_value = NULL;
  float* p_sat = NULL;
  float* p_hue = NULL;
  lo_message msg;
  std::mutex mtx;
  std::condition_variable cond;
  std::atomic_bool has_data = false;
  std::thread thread;
  std::atomic_bool run_thread = true;
  void sendthread();
  float fill = 0.0f;
};

granularsynth_t::granularsynth_t(const TASCAR::module_cfg_t& cfg)
    : granularsynth_vars_t(cfg), jackc_db_t(id, wlen),
      // ola_t(uint32_t fftlen, uint32_t wndlen, uint32_t chunksize,
      // windowtype_t wnd, windowtype_t zerownd,double wndpos,windowtype_t
      // postwnd=WND_RECT);
      ola1(4 * wlen, 4 * wlen, wlen, TASCAR::stft_t::WND_HANNING,
           TASCAR::stft_t::WND_RECT, 0.5),
      ola2(4 * wlen, 4 * wlen, wlen, TASCAR::stft_t::WND_HANNING,
           TASCAR::stft_t::WND_RECT, 0.5),
      minphase(4 * wlen), delaystate(0.0f), in_delayed(wlen), t_apply(0),
      deltaw(0), currentw(0), tprev(0), reset(false)
{
  std::set<double> vf;
  // convert semitone pitches to frequencies in Hz, and store unique
  // list of frequencies:
  for(auto pitch : pitches) {
    vfreqs.push_back(f0 * pow(2.0, pitch / 12.0));
    vf.insert(vfreqs.back());
  }
  // for each frequency, allocate a grainloop:
  for(auto f : vf)
    grains.push_back(grainloop_t(f, ola1.s.n_, numgrains));
  // calculate start times from durations:
  uint64_t t(0);
  for(auto dur : durations) {
    idurations.push_back(srate * dur / (bpm / 60.0));
    startingtimes.push_back(t);
    t += idurations.back();
  }
  // create jack ports:
  add_input_port("in");
  add_output_port("out");
  // register OSC variables:
  session->set_variable_owner(
      TASCAR::strrep(TASCAR::tscbasename(__FILE__), ".cc", ""));
  session->add_float(prefix + id + "/wet", &wet);
  session->add_method(prefix + id + "/wetapply", "f",
                      &granularsynth_t::osc_apply, this);
  session->add_double(prefix + id + "/t0", &t0);
  session->add_double(prefix + id + "/ponset", &ponset);
  session->add_double(prefix + id + "/psustain", &psustain);
  session->add_double_db(prefix + id + "/gain", &gain);
  session->add_bool_true(prefix + id + "/reset", &reset);
  session->add_bool(prefix + id + "/active", &active_);
  session->add_bool(prefix + id + "/bypass", &bypass_);
  session->add_bool(prefix + id + "/oscactive", &oscactive);
  session->unset_variable_owner();
  if(url.size())
    target = lo_address_new_from_url(url.c_str());
  msg = lo_message_new();
  lo_message_add_float(msg, hue);
  lo_message_add_float(msg, saturation);
  lo_message_add_float(msg, 0);
  lo_message_add_float(msg, 0.01);
  auto oscmsgargv = lo_message_get_argv(msg);
  p_value = &(oscmsgargv[2]->f);
  p_hue = &(oscmsgargv[0]->f);
  p_sat = &(oscmsgargv[1]->f);
  thread = std::thread(&granularsynth_t::sendthread, this);
  // activate service:
  set_apply(0.01f);
  activate();
}

int granularsynth_t::osc_apply(const char*, const char*, lo_arg** argv, int,
                               lo_message, void* user_data)
{
  ((granularsynth_t*)user_data)->set_apply(argv[0]->f);
  return 0;
}

void granularsynth_t::set_apply(float t)
{
  deltaw = 0;
  t_apply = 0;
  if(t >= 0) {
    uint32_t tau(std::max(1, (int32_t)(srate * t)));
    deltaw = (wet - currentw) / (float)tau;
    t_apply = tau;
  }
}

int granularsynth_t::process(jack_nframes_t n, const std::vector<float*>& vIn,
                             const std::vector<float*>& vOut)
{
  jackc_db_t::process(n, vIn, vOut);
  if(active_) {
    // apply dry/wet mixing:
    TASCAR::wave_t w_in(n, vIn[0]);
    TASCAR::wave_t w_out(n, vOut[0]);
    for(uint32_t k = 0; k < w_in.n; ++k) {
      if(t_apply) {
        t_apply--;
        currentw += deltaw;
      }
      if(currentw < 0.0f)
        currentw = 0.0f;
      if(currentw > 1.0f)
        currentw = 1.0f;
      w_out[k] *= gain * currentw;
      w_out[k] += (1.0f - currentw) * w_in[k];
    }
  } else {
    if(bypass_) {
      // copy input to output:
      for(size_t ch = 0; ch < std::min(vIn.size(), vOut.size()); ++ch)
        memcpy(vOut[ch], vIn[ch], n * sizeof(float));
      for(size_t ch = vIn.size(); ch < vOut.size(); ++ch)
        memset(vOut[ch], 0, n * sizeof(float));
    } else {
      for(size_t ch = 0; ch < vOut.size(); ++ch)
        memset(vOut[ch], 0, n * sizeof(float));
    }
  }
  return 0;
}

int granularsynth_t::inner_process(jack_nframes_t n,
                                   const std::vector<float*>& vIn,
                                   const std::vector<float*>& vOut)
{
  // calculate time base:
  uint64_t t0i(t0 * srate);
  uint64_t t(jack_get_current_transport_frame(jc));
  t = t - std::min(t, t0i);
  uint64_t loopi(loop * srate / (bpm / 60.0));
  ldiv_t tmp(ldiv(t, loopi));
  t = tmp.rem;
  // if inactive, do only time update, and return zeros:
  if(!active_) {
    tprev = t;
    for(auto oBuf : vOut)
      memset(oBuf, 0, sizeof(float) * n);
    return 0;
  }
  // optionally, reset grains:
  if(reset) {
    for(auto& grain : grains)
      grain.reset();
    reset = false;
  }
  // create delayed input signal:
  if(in_delayed.n != n) {
    DEBUG(in_delayed.n);
    DEBUG(n);
    throw TASCAR::ErrMsg("Programming error");
  }
  TASCAR::wave_t w_in(n, vIn[0]);
  for(uint32_t k = 0; k < n; ++k) {
    in_delayed.d[k] = delaystate;
    delaystate = w_in.d[k];
  }
  // spectral analysis and instantaneous frequency:
  ola1.process(w_in);
  ola2.process(in_delayed);
  // use frequency with highest intensity:
  double intensity_max(0.0);
  double freq_max(0.0);
  for(uint32_t k = 0; k < ola1.s.n_; ++k) {
    std::complex<float> p(ola1.s.b[k] * std::conj(ola2.s.b[k]));
    float f(std::arg(p));
    float intensity(std::abs(p));
    if(intensity > intensity_max) {
      freq_max = f;
      intensity_max = intensity;
    }
  }
  // if instantaneous frequency matches, then add grain:
  freq_max *= f_sample / TASCAR_2PI;
  // phase modification, e.g., convert to minimum phase stimulus:
  minphase(ola1.s);
  // add grains to grain store if features are matching:
  for(auto& grain : grains)
    if(fabs(12.0 * log2(freq_max / grain.f)) < 0.5) {
      grain.add(ola1.s);
    }
  // prepare for output:
  ola1.s.clear();
  TASCAR::wave_t w_out(n, vOut[0]);
  // play melody from grain collection:
  const double vrandom(TASCAR::drand());
  for(uint32_t k = 0; k < std::min(startingtimes.size(), vfreqs.size()); ++k) {
    uint64_t tstart(startingtimes[k]);
    if(((t >= tstart) && ((tprev < tstart) || (tprev > t))) &&
       (vrandom < ponset))
      for(auto& grain : grains)
        if(vfreqs[k] == grain.f) {
          grain.play(ola1.s, gain);
        }
    if((t >= tstart) && (t < tstart + idurations[k]) && (vrandom < psustain))
      for(auto& grain : grains)
        if(vfreqs[k] == grain.f) {
          grain.play(ola1.s, gain);
        }
  }
  // convert back to time domain:
  ola1.ifft(w_out);
  // w_out.clear();
  tprev = t;
  // send fill state:
  if(target && oscactive) {
    if(mtx.try_lock()) {
      fill = 0.0f;
      for(auto& grain : grains)
        fill += (grain.get_num_filled() > fillthreshold);
      fill *= 1.0 / grains.size();
      has_data = true;
      mtx.unlock();
      cond.notify_one();
    }
  }
  return 0;
}

void granularsynth_t::sendthread()
{
  std::unique_lock<std::mutex> lk(mtx);
  while(run_thread) {
    cond.wait_for(lk, 100ms);
    if(has_data) {
      *p_value = fill;
      *p_hue = hue;
      *p_sat = saturation;
      if(active)
        lo_send_message(target, path.c_str(), msg);
      has_data = false;
    }
  }
}

granularsynth_t::~granularsynth_t()
{
  deactivate();
  run_thread = false;
  thread.join();
  if(target)
    lo_address_free(target);
  lo_message_free(msg);
}

REGISTER_MODULE(granularsynth_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */
