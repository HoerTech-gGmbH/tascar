/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
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

// Author: Giso Grimm
#include "irrender.h"
#include "errorhandling.h"

#include <string.h>
#include <unistd.h>

TASCAR::wav_render_t::wav_render_t(const std::string& tscname,
                                   const std::string& scene_, bool verbose)
    : session_core_t(tscname, LOAD_FILE, tscname), scene(scene_), pscene(NULL),
      verbose_(verbose), t0(clock()), t1(clock()), t2(clock())
{
  read_xml();
  if(!pscene)
    throw TASCAR::ErrMsg("Scene " + scene + " not found.");
}

TASCAR::wav_render_t::~wav_render_t()
{
  if(pscene)
    delete pscene;
}

void TASCAR::wav_render_t::add_scene(tsccfg::node_t sne)
{
  if((!pscene) &&
     (scene.empty() || (tsccfg::node_get_attribute_value(sne,"name") == scene))) {
    pscene = new render_core_t(sne);
  } else {
    if(pscene && (pscene->name == tsccfg::node_get_attribute_value(sne,"name")))
      throw TASCAR::ErrMsg("A scene of name \"" + pscene->name +
                           "\" already exists in the session.");
  }
}

void TASCAR::wav_render_t::set_ism_order_range(uint32_t ism_min,
                                               uint32_t ism_max)
{
  if(pscene)
    pscene->set_ism_order_range(ism_min, ism_max);
}

std::vector<size_t> get_chmap(const std::vector<size_t>& chmap, size_t& nch)
{
  std::vector<size_t> m;
  if(chmap.empty()) {
    for(size_t k = 0; k < nch; ++k)
      m.push_back(k);
  } else {
    for(auto c : chmap)
      if(c < nch)
        m.push_back(c);
      else
        TASCAR::add_warning("Ignoring channel " + std::to_string(c) +
                            " (only " + std::to_string(nch) +
                            " channels are available).");
  }
  nch = m.size();
  return m;
}

void TASCAR::wav_render_t::render(uint32_t fragsize, double srate,
                                  double duration, const std::string& ofname,
                                  double starttime, bool b_dynamic)
{
  if(!pscene)
    throw TASCAR::ErrMsg("No scene loaded");
  // open sound files:
  if(duration == 0)
    duration = session_core_t::duration;
  int64_t iduration(srate * duration);
  chunk_cfg_t cf(srate, fragsize, 1);
  uint32_t num_fragments((uint32_t)((iduration - 1) / cf.n_fragment) + 1);
  // configure maximum delayline length:
  double maxdist(duration * (pscene->c));
  for(std::vector<TASCAR::Scene::sound_t*>::iterator isnd =
          pscene->sounds.begin();
      isnd != pscene->sounds.end(); ++isnd) {
    if((*isnd)->maxdist > maxdist)
      (*isnd)->maxdist = maxdist;
  }
  // initialize scene:
  pscene->prepare(cf);
  pscene->post_prepare();
  add_licenses(this);
  pscene->add_licenses(this);
  size_t nch_in(pscene->num_input_ports());
  size_t nch_out_scene(pscene->num_output_ports());
  size_t nch_out = nch_out_scene;
  std::vector<size_t> chmap = get_chmap(ochannels, nch_out);
  sndfile_handle_t sf_out(ofname, cf.f_sample, nch_out);
  // allocate io audio buffer:
  float* sf_out_buf(new float[nch_out * cf.n_fragment]);
  // allocate render audio buffer:
  std::vector<float*> a_in;
  for(uint32_t k = 0; k < nch_in; ++k) {
    a_in.push_back(new float[cf.n_fragment]);
    memset(a_in.back(), 0, sizeof(float) * cf.n_fragment);
  }
  std::vector<float*> a_out;
  for(size_t k = 0; k < nch_out_scene; ++k) {
    a_out.push_back(new float[cf.n_fragment]);
    memset(a_out.back(), 0, sizeof(float) * cf.n_fragment);
  }
  TASCAR::transport_t tp;
  tp.rolling = true;
  tp.session_time_seconds = starttime;
  tp.session_time_samples = starttime * cf.f_sample;
  tp.object_time_seconds = starttime;
  tp.object_time_samples = starttime * cf.f_sample;
  pscene->process(cf.n_fragment, tp, a_in, a_out);
  if(verbose_)
    std::cerr << "rendering " << pscene->active_pointsources << " of "
              << pscene->total_pointsources << " point sources.\n";
  t1 = clock();
  int64_t n(0);
  for(uint32_t k = 0; k < num_fragments; ++k) {
    // load audio chunk from file
    for(uint32_t kf = 0; kf < cf.n_fragment; ++kf)
      for(uint32_t kc = 0; kc < nch_in; ++kc)
        a_in[kc][kf] = 0.0f;
    // process audio:
    pscene->process(cf.n_fragment, tp, a_in, a_out);
    // save audio:
    for(uint32_t kf = 0; kf < cf.n_fragment; ++kf)
      for(uint32_t kc = 0; kc < nch_out; ++kc)
        sf_out_buf[kc + nch_out * kf] = a_out[chmap[kc]][kf];
    sf_out.writef_float(
        sf_out_buf,
        std::max((int64_t)0, std::min(iduration - n, (int64_t)cf.n_fragment)));
    // increment time:
    if(b_dynamic) {
      tp.session_time_samples += cf.n_fragment;
      tp.session_time_seconds = ((double)tp.session_time_samples) / cf.f_sample;
      tp.object_time_samples += cf.n_fragment;
      tp.object_time_seconds = ((double)tp.object_time_samples) / cf.f_sample;
    }
    n += cf.n_fragment;
  }
  t2 = clock();
  pscene->release();
  // de-allocate render audio buffer:
  for(uint32_t k = 0; k < nch_in; ++k)
    delete[] a_in[k];
  for(uint32_t k = 0; k < nch_out_scene; ++k)
    delete[] a_out[k];
  delete[] sf_out_buf;
}

void TASCAR::wav_render_t::render(uint32_t fragsize, const std::string& ifname,
                                  const std::string& ofname, double starttime,
                                  bool b_dynamic)
{
  if(!pscene)
    throw TASCAR::ErrMsg("No scene loaded");
  // open sound files:
  sndfile_handle_t sf_in(ifname);
  chunk_cfg_t cf(sf_in.get_srate(), std::min(fragsize, sf_in.get_frames()),
                 sf_in.get_channels());
  chunk_cfg_t cffile(cf);
  uint32_t num_fragments((uint32_t)((sf_in.get_frames() - 1) / cf.n_fragment) +
                         1);
  // configure maximum delayline length: the maximum meaningful length
  // is the length of the sound file:
  double maxdist((sf_in.get_frames() + 1) / cf.f_sample * (pscene->c));
  for(std::vector<TASCAR::Scene::sound_t*>::iterator isnd =
          pscene->sounds.begin();
      isnd != pscene->sounds.end(); ++isnd) {
    if((*isnd)->maxdist > maxdist)
      (*isnd)->maxdist = maxdist;
  }
  // initialize scene:
  pscene->prepare(cf);
  pscene->post_prepare();
  add_licenses(this);
  pscene->add_licenses(this);
  uint32_t nch_in(pscene->num_input_ports());
  size_t nch_out_scene(pscene->num_output_ports());
  size_t nch_out = nch_out_scene;
  std::vector<size_t> chmap = get_chmap(ochannels, nch_out);
  sndfile_handle_t sf_out(ofname, cf.f_sample, nch_out);
  // allocate io audio buffer:
  float* sf_in_buf(new float[cffile.n_channels * cffile.n_fragment]);
  float* sf_out_buf(new float[nch_out * cf.n_fragment]);
  // allocate render audio buffer:
  std::vector<float*> a_in;
  for(uint32_t k = 0; k < nch_in; ++k) {
    a_in.push_back(new float[cffile.n_fragment]);
    memset(a_in.back(), 0, sizeof(float) * cffile.n_fragment);
  }
  std::vector<float*> a_out;
  for(uint32_t k = 0; k < nch_out_scene; ++k) {
    a_out.push_back(new float[cf.n_fragment]);
    memset(a_out.back(), 0, sizeof(float) * cf.n_fragment);
  }
  TASCAR::transport_t tp;
  tp.rolling = true;
  tp.session_time_seconds = starttime;
  tp.session_time_samples = starttime * cf.f_sample;
  tp.object_time_seconds = starttime;
  tp.object_time_samples = starttime * cf.f_sample;
  pscene->process(cf.n_fragment, tp, a_in, a_out);
  if(verbose_)
    std::cerr << "rendering " << pscene->active_pointsources << " of "
              << pscene->total_pointsources << " point sources.\n";
  t1 = clock();
  for(uint32_t k = 0; k < num_fragments; ++k) {
    // load audio chunk from file
    uint32_t n_in(sf_in.readf_float(sf_in_buf, cffile.n_fragment));
    if(n_in > 0) {
      if(n_in < cffile.n_fragment)
        memset(&(sf_in_buf[n_in * cffile.n_channels]), 0,
               (cffile.n_fragment - n_in) * cffile.n_channels * sizeof(float));
      for(uint32_t kf = 0; kf < cffile.n_fragment; ++kf)
        for(uint32_t kc = 0; kc < nch_in; ++kc)
          if(kc < cffile.n_channels)
            a_in[kc][kf] = sf_in_buf[kc + cffile.n_channels * kf];
          else
            a_in[kc][kf] = 0.0f;
      // process audio:
      pscene->process(cf.n_fragment, tp, a_in, a_out);
      // save audio:
      for(uint32_t kf = 0; kf < cf.n_fragment; ++kf)
        for(uint32_t kc = 0; kc < nch_out; ++kc)
          sf_out_buf[kc + nch_out * kf] = a_out[chmap[kc]][kf];
      sf_out.writef_float(sf_out_buf, n_in);
      // increment time:
      if(b_dynamic) {
        tp.session_time_samples += cf.n_fragment;
        tp.session_time_seconds =
            ((double)tp.session_time_samples) / cf.f_sample;
        tp.object_time_samples += cf.n_fragment;
        tp.object_time_seconds = ((double)tp.object_time_samples) / cf.f_sample;
      }
    }
  }
  t2 = clock();
  pscene->release();
  // de-allocate render audio buffer:
  for(uint32_t k = 0; k < nch_in; ++k)
    delete[] a_in[k];
  for(uint32_t k = 0; k < nch_out_scene; ++k)
    delete[] a_out[k];
  delete[] sf_in_buf;
  delete[] sf_out_buf;
}

void TASCAR::wav_render_t::render_ir(uint32_t len, double fs,
                                     const std::string& ofname,
                                     double starttime, uint32_t inputchannel)
{
  if(!pscene)
    throw TASCAR::ErrMsg("No scene loaded");
  // configure maximum delayline length:
  double maxdist((len + 1) / fs * (pscene->c));
  // std::vector<TASCAR::Scene::sound_t*> snds(pscene->linearize_sounds());
  for(std::vector<TASCAR::Scene::sound_t*>::iterator isnd =
          pscene->sounds.begin();
      isnd != pscene->sounds.end(); ++isnd) {
    (*isnd)->maxdist = maxdist;
  }
  chunk_cfg_t cf(fs, len);
  // initialize scene:
  pscene->prepare(cf);
  pscene->post_prepare();
  add_licenses(this);
  pscene->add_licenses(this);
  uint32_t nch_in(pscene->num_input_ports());
  if(inputchannel >= nch_in)
    throw TASCAR::ErrMsg("Input channel number " +
                         TASCAR::to_string(inputchannel) +
                         " is not smaller than number of input channels (" +
                         TASCAR::to_string(nch_in) + ").");
  size_t nch_out_scene(pscene->num_output_ports());
  size_t nch_out = nch_out_scene;
  std::vector<size_t> chmap = get_chmap(ochannels, nch_out);
  sndfile_handle_t sf_out(ofname, fs, nch_out);
  // allocate io audio buffer:
  float* sf_out_buf(new float[nch_out * len]);
  // allocate render audio buffer:
  std::vector<float*> a_in;
  for(uint32_t k = 0; k < nch_in; ++k) {
    a_in.push_back(new float[len]);
    memset(a_in.back(), 0, sizeof(float) * len);
  }
  std::vector<float*> a_out;
  for(uint32_t k = 0; k < nch_out_scene; ++k) {
    a_out.push_back(new float[len]);
    memset(a_out.back(), 0, sizeof(float) * len);
  }
  TASCAR::transport_t tp;
  tp.rolling = false;
  tp.session_time_seconds = starttime;
  tp.session_time_samples = starttime * fs;
  tp.object_time_seconds = starttime;
  tp.object_time_samples = starttime * fs;
  pscene->process(len, tp, a_in, a_out);
  if(verbose_)
    std::cerr << "rendering " << pscene->active_pointsources << " of "
              << pscene->total_pointsources << " point sources.\n";
  a_in[inputchannel][0] = 1.0f;
  // process audio:
  pscene->process(len, tp, a_in, a_out);
  // save audio:
  for(uint32_t kf = 0; kf < len; ++kf)
    for(uint32_t kc = 0; kc < nch_out; ++kc)
      sf_out_buf[kc + nch_out * kf] = a_out[chmap[kc]][kf];
  sf_out.writef_float(sf_out_buf, len);
  // increment time:
  // de-allocate render audio buffer:
  pscene->release();
  for(uint32_t k = 0; k < nch_in; ++k)
    delete[] a_in[k];
  for(uint32_t k = 0; k < nch_out_scene; ++k)
    delete[] a_out[k];
  delete[] sf_out_buf;
}

void TASCAR::wav_render_t::validate_attributes(std::string& msg) const
{
  root.validate_attributes(msg);
  if( pscene )
    pscene->validate_attributes(msg);
}

void TASCAR::wav_render_t::set_channelmap( const std::vector<size_t>& channels )
{
  ochannels = channels;
}

void TASCAR::wav_render_t::reset_channelmap()
{
  ochannels.clear();
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
