/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
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

/**
 * @file tuner.cpp
 * @brief Audio pitch detection module with OSC output for TASCAR.
 *
 * This module analyzes audio input to detect pitch, calculates tuning
 * corrections, and sends results via OSC. Uses JACK audio processing and OLA
 * (Overlap-Add) for spectral analysis.
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

/**
 * @class tuner_vars_t
 * @brief Configuration variables for the tuner module.
 *
 * Stores configuration parameters and tuning correction data.
 */
class tuner_vars_t : public TASCAR::module_base_t {
public:
  /**
   * @brief Constructor.
   * @param cfg Module configuration object.
   *
   * Initializes configuration parameters from XML attributes.
   */
  tuner_vars_t(const TASCAR::module_cfg_t& cfg);
  ~tuner_vars_t(){};

protected:
  std::string id = "tuner"; ///< Module ID for JACK and OSC paths
  std::string prefix = "/"; ///< OSC path prefix
  std::string url;          ///< OSC target URL
  uint32_t wlen = 2048;     ///< Window length for spectral analysis
  float f0 = 440.0f;        ///< Reference frequency for pitch 0 (A4)
  bool isactive = false;    ///< Enable analysis.
  bool oscactive = true;    ///< Enable OSC output
  std::vector<float> pitchcorr = {
      0.0f};                   ///< Pitch correction in cents per semitone
  std::string path = "/tuner"; ///< OSC message path
  std::string path_strobe = "/tuner/strobe"; ///< OSC message path
  float tau = 0.05f;                ///< Time constant for low-pass filters
  std::vector<std::string> connect; ///< Port names of input connection
  float fmin = 40.0f;               ///< Minimal frequency for analysis in Hz
  float fmax = 4000.0f;             ///< maximal frequency for analysis in Hz
  std::string tuning = "equal";
  float strobeperiods = 4.0f;    ///< Number of periods to display in strobe
  uint32_t strobebufferlen = 50; ///< Length of strobe buffer
  uint32_t keepnote = 5; ///< Number of samples before a new note is accepted
};

/**
 * @class tuner_t
 * @brief Audio processing module for pitch detection and OSC output.
 *
 * Implements JACK audio processing, spectral analysis using OLA, and OSC
 * communication. Runs a separate thread for OSC message sending to avoid audio
 * thread blocking.
 */
class tuner_t : public tuner_vars_t, public jackc_db_t {
public:
  /**
   * @brief Constructor.
   * @param cfg Module configuration object.
   *
   * Initializes audio ports, OSC variables, and processing buffers.
   * Starts the OSC sending thread.
   */
  tuner_t(const TASCAR::module_cfg_t& cfg);
  ~tuner_t();

  /**
   * @brief Audio processing callback.
   * @param nframes Number of frames to process.
   * @param inBuffer Input audio buffers.
   * @param outBuffer Output audio buffers.
   * @return 0 on success.
   *
   * Zeros output buffers and calls inner_process for actual processing.
   */
  virtual int process(jack_nframes_t nframes,
                      const std::vector<float*>& inBuffer,
                      const std::vector<float*>& outBuffer);

  /**
   * @brief Core audio processing logic.
   * @param nframes Number of frames to process.
   * @param inBuffer Input audio buffers.
   * @param outBuffer Output audio buffers (not used).
   * @return 0 on success.
   *
   * Performs spectral analysis, calculates pitch, and updates OSC variables.
   */
  virtual int inner_process(jack_nframes_t nframes,
                            const std::vector<float*>& inBuffer,
                            const std::vector<float*>& outBuffer);

  inline float freq2pitch(float f)
  {
    return std::min(255.0f, std::max(0.0f, 12.0f * log2f(f / f0) + 69.0f));
  };

  inline int freq2pitch_int(float f) { return (int)(freq2pitch(f) + 0.5); };

protected:
  TASCAR::ola_t ola1;           ///< First OLA processor for input signal
  TASCAR::ola_t ola2;           ///< Second OLA processor for delayed input
  float delaystate;             ///< Delay line state
  TASCAR::wave_t in_delayed;    ///< Delayed input buffer
  lo_address target = NULL;     ///< OSC target address
  float* p_freq = NULL;         ///< Pointer to frequency value in OSC message
  float* p_note = NULL;         ///< Pointer to MIDI note value in OSC message
  float* p_octave = NULL;       ///< Pointer to octave value in OSC message
  float* p_delta = NULL;        ///< Pointer to cents deviation in OSC message
  float* p_confidence = NULL;   ///< Pointer to confidence value in OSC message
  lo_message msg;               ///< OSC message buffer
  lo_message msg_strobe;        ///< OSC message buffer
  std::mutex mtx;               ///< Mutex for thread-safe data access
  std::condition_variable cond; ///< Condition variable for thread signaling
  std::atomic_bool has_data = false;  ///< Flag indicating new data is available
  std::thread thread;                 ///< OSC sending thread
  std::atomic_bool run_thread = true; ///< Thread run control flag
  void sendthread();                  ///< OSC sending thread function
  float v_freq = 0.0f;                ///< Current frequency estimate
  float v_note = 0.0f;                ///< Current MIDI note estimate
  float v_octave = 0.0f;              ///< Current octave estimate
  float v_delta = 0.0f;               ///< Current cents deviation
  float v_confidence = 0.0f;          ///< Current confidence value
  int i_note_prev = 0u;
  uint32_t note_accept_counter = 0u;
  TASCAR::o1flt_lowpass_t mean_lp; ///< Low-pass filter for frequency mean
  TASCAR::o1flt_lowpass_t
      std_lp; ///< Low-pass filter for frequency standard deviation
  std::vector<float> cum_pitches;
  std::vector<float> cum_weights;
  std::vector<float> cum_vectorstrengths;
  uint32_t bin_min = 0;
  uint32_t bin_max = 0;
  std::vector<float> pitch_ratios = {4.0f, 3.0f, 2.0f};
  std::vector<std::complex<float>> c_dphase_state;
  // variables for strobe display:
  float strobe_currentperiod = 1.0f;
  float strobe_buffertime = 0.0f;
  uint32_t strobe_bincount = 0u;
  float strobe_avg_amplitude = 0.0f;
  uint32_t strobebuffer_index_prev = 0u;
  TASCAR::wave_t strobebuffer;
  TASCAR::wave_t strobebuffersend;
};

tuner_vars_t::tuner_vars_t(const TASCAR::module_cfg_t& cfg)
    : TASCAR::module_base_t(cfg)
{
  std::map<std::string, std::vector<float>> tuningtable;
  // http://www.instrument-tuner.com/temperaments_de.html
  tuningtable["equal"] = {0.0f};
  tuningtable["werkmeister3"] = {0.0f, -3.91f, 3.91f,  0.0f, -3.91f, 3.91f,
                                 0.0f, 1.955f, -7.82f, 0.0f, 1.955f, -1.955f};
  tuningtable["werckmeister3"] = {0.0f, -3.91f, 3.91f,  0.0f, -3.91f, 3.91f,
                                  0.0f, 1.955f, -7.82f, 0.0f, 1.955f, -1.955f};
  tuningtable["meantone4"] = {10.265f,  -13.686f, 3.422f,   20.53f,
                              -3.421f,  13.686f,  -10.265f, 6.843f,
                              -17.108f, 0.0f,     17.108f,  -6.843f};
  tuningtable["meantone6"] = {6.0f,  -2.0f, 2.0f, 0.0f, -2.0f, 8.0f,
                              -6.0f, 4.0f,  2.0f, 0.0f, 4.0f,  -4.0f};
  tuningtable["valotti"] = {5.865f,  0.0f,  1.955f, 3.91f, -1.955f, 7.82f,
                            -1.955f, 3.91f, 1.955f, 0.0f,  5.865f,  -3.91f};
  tuningtable["vallotti"] = {5.865f,  0.0f,  1.955f, 3.91f, -1.955f, 7.82f,
                             -1.955f, 3.91f, 1.955f, 0.0f,  5.865f,  -3.91f};
  tuningtable["Bach-Kellner1977"] = {8.211f,  -1.564f, 2.737f,  2.346f,
                                     -2.737f, 6.256f,  -3.519f, 5.474f,
                                     0.391f,  0.000f,  4.301f,  -0.782f};
  tuningtable["Neidhardt-1724-GrosseStadt"] = {5.865f, 1.955f, 1.955f, 3.910f,
                                               0.000f, 3.910f, 1.955f, 1.955f,
                                               1.955f, 0.000f, 3.910f, 1.955f};
  std::vector<std::string> tuningnames;
  for(const auto tun : tuningtable)
    tuningnames.push_back(tun.first);
  GET_ATTRIBUTE(id, "", "ID used in jack name and OSC path");
  GET_ATTRIBUTE(prefix, "", "prefix used in OSC path");
  GET_ATTRIBUTE(wlen, "samples", "window length");
  GET_ATTRIBUTE(f0, "Hz", "frequency of pitch 0");
  GET_ATTRIBUTE(url, "", "Tuner display URL");
  GET_ATTRIBUTE(path, "", "Tuner note/detune display path");
  GET_ATTRIBUTE(path_strobe, "", "Tuner strobe display path");
  GET_ATTRIBUTE_BOOL(oscactive, "Activate OSC sending on start");
  GET_ATTRIBUTE_BOOL(isactive, "Activate analysis on start");
  GET_ATTRIBUTE(connect, "", "Port names of input connection");
  GET_ATTRIBUTE(tau, "s", "Frequency estimation time constant");
  GET_ATTRIBUTE(fmin, "Hz", "Minimal frequency for analysis");
  GET_ATTRIBUTE(fmax, "Hz", "Maximal frequency for analysis");
  GET_ATTRIBUTE(tuning, TASCAR::vecstr2str(tuningnames, " | "),
                "Tuning temperature");
  GET_ATTRIBUTE(strobeperiods, "",
                "Number of periods to display in strobe display");
  GET_ATTRIBUTE(strobebufferlen, "", "Number of samples in strobe buffer");
  GET_ATTRIBUTE(keepnote, "",
                "Number of samples before a new note is accepted");
  if(tuningtable.find(tuning) == tuningtable.end())
    throw TASCAR::ErrMsg(
        "Unsupported tuning: \"" + tuning +
        "\". Valid tunings are: " + TASCAR::vecstr2str(tuningnames, ", "));
  pitchcorr = tuningtable[tuning];
  jackc_portless_t jc(id + "_");
  auto fragsize = jc.get_fragsize();
  auto wlen_new = wlen;
  if((int)wlen > fragsize)
    wlen_new = ceil((float)wlen / (float)fragsize) * fragsize;
  else
    wlen_new =
        fragsize * powf(2.0f, ceil(log2f((float)wlen / (float)fragsize)));
  if(wlen_new != wlen)
    TASCAR::add_warning("Window length adjusted from " +
                        TASCAR::to_string(wlen) + " to " +
                        TASCAR::to_string(wlen_new) + ".");
  wlen = wlen_new;
}

tuner_t::tuner_t(const TASCAR::module_cfg_t& cfg)
    : tuner_vars_t(cfg), jackc_db_t(id, wlen),
      ola1(4 * wlen, 4 * wlen, wlen, TASCAR::stft_t::WND_HANNING,
           TASCAR::stft_t::WND_RECT, 0.5),
      ola2(4 * wlen, 4 * wlen, wlen, TASCAR::stft_t::WND_HANNING,
           TASCAR::stft_t::WND_RECT, 0.5),
      delaystate(0.0f), in_delayed(wlen), mean_lp({tau}, srate / wlen),
      std_lp({tau}, srate / wlen), strobebuffer(strobebufferlen),
      strobebuffersend(strobebufferlen)
{
  // create jack ports:
  add_input_port("in");
  // register OSC variables:
  session->set_variable_owner(
      TASCAR::strrep(TASCAR::tscbasename(__FILE__), ".cc", ""));
  session->add_bool(prefix + id + "/oscactive", &oscactive,
                    "control sending of OSC messages");
  session->add_bool(prefix + id + "/isactive", &isactive, "control analysis");
  session->add_float(prefix + id + "/f0", &f0, "[100,1000]", "pitch a in Hz");
  session->add_float(prefix + id + "/tau", &tau, "[0,10]",
                     "pitch smoothing TC in s");
  session->add_string(prefix + id + "/tuning", &tuning, "tuning temperament");
  session->unset_variable_owner();
  if(url.size())
    target = lo_address_new_from_url(url.c_str());
  // construct pitch message:
  msg = lo_message_new();
  lo_message_add_float(msg, 0.0f);
  lo_message_add_float(msg, 0.0f);
  lo_message_add_float(msg, 0.0f);
  lo_message_add_float(msg, 0.0f);
  lo_message_add_float(msg, 0.0f);
  auto oscmsgargv = lo_message_get_argv(msg);
  p_freq = &(oscmsgargv[0]->f);
  p_note = &(oscmsgargv[1]->f);
  p_octave = &(oscmsgargv[2]->f);
  p_delta = &(oscmsgargv[3]->f);
  p_confidence = &(oscmsgargv[4]->f);
  // construct strobe message:
  msg_strobe = lo_message_new();
  for(uint32_t k = 0; k < strobebufferlen; ++k)
    lo_message_add_float(msg_strobe, 0.0f);
  // create sender thread:
  thread = std::thread(&tuner_t::sendthread, this);
  cum_pitches.resize(256);
  cum_weights.resize(256);
  cum_vectorstrengths.resize(256);
  bin_min = (uint32_t)(fmin / srate * wlen);
  bin_max = std::min((uint32_t)(wlen / 2), (uint32_t)(fmax / srate * wlen));
  c_dphase_state.resize(ola1.s.n_);
  for(auto& c : c_dphase_state)
    c = 0.0f;
  // activate service:
  activate();
  for(const auto& c : tuner_vars_t::connect)
    connect_in(0, c, true);
}

int tuner_t::process(jack_nframes_t n, const std::vector<float*>& vIn,
                     const std::vector<float*>& vOut)
{
  if(!isactive)
    return 0;
  jackc_db_t::process(n, vIn, vOut);
  return 0;
}

int tuner_t::inner_process(jack_nframes_t n, const std::vector<float*>& vIn,
                           const std::vector<float*>&)
{
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
  // clear pitches and intensities:
  for(auto& p : cum_pitches)
    p = 0.0f;
  for(auto& p : cum_weights)
    p = 0.0f;
  for(auto& p : cum_vectorstrengths)
    p = 0.0f;
  // update filters and store filter coefficient:
  mean_lp.set_tau(tau);
  std_lp.set_tau(tau);
  float c1 = mean_lp.get_c1(0);
  float c2 = 1.0f - c1;
  // spectral analysis and instantaneous frequency:
  ola1.process(w_in);
  ola2.process(in_delayed);
  // use frequency with highest intensity:
  for(uint32_t k = bin_min; k < std::min(ola1.s.n_, bin_max); ++k) {
    // calculate phase increment and intensity:
    std::complex<float> c_dphase(ola1.s.b[k] * std::conj(ola2.s.b[k]));
    float intensity = std::abs(c_dphase);
    if(intensity > 0)
      c_dphase /= intensity;
    // low-pass complex phase value to calculate average instantaneous
    // frequency and vector strength:
    c_dphase_state[k] *= c1;
    c_dphase_state[k] += c2 * c_dphase;
    float vectorstrength = std::abs(c_dphase_state[k]);
    float instfreq = std::arg(c_dphase_state[k]) * f_sample / TASCAR_2PI;
    int idx_pitch = freq2pitch_int(instfreq);
    float weight = intensity * vectorstrength;
    cum_pitches[idx_pitch] += weight * instfreq;
    cum_weights[idx_pitch] += weight;
    cum_vectorstrengths[idx_pitch] += weight * vectorstrength;
  }
  for(size_t k = 0; k < cum_pitches.size(); ++k) {
    if(cum_weights[k] > 0.0f) {
      cum_pitches[k] /= cum_weights[k];
      cum_vectorstrengths[k] /= cum_weights[k];
    }
  }
  for(size_t k = 0; k < cum_pitches.size(); ++k) {
    for(auto r : pitch_ratios) {
      float f_new = cum_pitches[k] / r;
      if(f_new > fmin) {
        int idx_new = freq2pitch_int(f_new);
        if(cum_weights[idx_new] > 0.01 * cum_weights[k]) {
          cum_weights[idx_new] += cum_weights[k];
          cum_weights[k] = 0.0f;
        }
      }
    }
  }
  float intensity_max(0.0);
  float ifreq_max(0.0);
  float confidence_max(0.0f);
  for(size_t k = 0; k < cum_pitches.size(); ++k) {
    if(cum_weights[k] > intensity_max) {
      ifreq_max = cum_pitches[k];
      intensity_max = cum_weights[k];
      confidence_max = cum_vectorstrengths[k];
    }
  }
  // filter extreme values:
  if((ifreq_max < fmin) || (ifreq_max > fmax)) {
    ifreq_max = std::min(fmax, std::max(fmin, ifreq_max));
    confidence_max = 0.0f;
  }
  // update strobe buffer:
  float one_over_strobe_currentperiod = 1.0f / strobe_currentperiod;
  for(uint32_t k = 0; k < n; ++k) {
    // increment strobe buffer sample time:
    strobe_buffertime += t_sample;
    while(strobe_buffertime > strobe_currentperiod)
      strobe_buffertime -= strobe_currentperiod;
    // get index into buffer:
    uint32_t strobebuffer_index =
        strobe_buffertime * one_over_strobe_currentperiod * strobebuffer.n;
    if(strobebuffer_index > strobebuffer.n - 1)
      strobebuffer_index = strobebuffer.n - 1;
    // collect average amplitude:
    strobe_avg_amplitude += w_in.d[k];
    ++strobe_bincount;
    if(strobebuffer_index_prev != strobebuffer_index) {
      strobebuffer_index_prev = strobebuffer_index;
      if(strobe_bincount)
        strobe_avg_amplitude /= (float)strobe_bincount;
      if(!std::isfinite(strobebuffer.d[strobebuffer_index]))
        strobebuffer.d[strobebuffer_index] = 0.0f;
      else
        strobebuffer.d[strobebuffer_index] *= 0.5f;
      strobebuffer.d[strobebuffer_index] += 0.5f * strobe_avg_amplitude;
      strobe_avg_amplitude = 0.0f;
      strobe_bincount = 0u;
    }
  }
  // auto ifreq_mean = mean_lp(0, freq_max);
  // auto ifreq_std = sqrtf(std::max(
  //   0.0f, std_lp(0, (ifreq_mean - freq_max) * (ifreq_mean - freq_max))));
  // send frequency and confidence:
  if(target && oscactive) {
    if(mtx.try_lock()) {
      // v_freq = ifreq_mean;
      v_freq = ifreq_max;
      v_note =
          std::min(255.0f, std::max(0.0f, 12.0f * log2f(v_freq / f0) + 69.0f));
      int i_note = (int)(v_note + 0.5f);
      if(keepnote) {
        if(i_note != i_note_prev) {
          if(note_accept_counter) {
            --note_accept_counter;
            i_note = i_note_prev;
          } else {
            note_accept_counter = keepnote;
            i_note_prev = i_note;
          }
        }
      }
      v_delta = 100.0f * (v_note - i_note);
      auto div_note = div(i_note, 12);
      v_octave = div_note.quot - 5.0f;
      i_note = div_note.rem;
      auto i_note_corr = i_note % pitchcorr.size();
      float f_ref =
          f0 * powf(2.0f, (float)(i_note + 12 * (v_octave + 5) - 69) / 12.0f +
                              pitchcorr[i_note_corr] / 1200.0f);
      f_ref = std::min(fmax, std::max(fmin, f_ref));
      strobe_currentperiod = strobeperiods / f_ref;
      v_delta -= pitchcorr[i_note_corr];
      v_confidence = confidence_max; // expf(-ifreq_std);
      v_note = i_note;
      strobebuffersend.copy(strobebuffer);
      has_data = true;
      mtx.unlock();
      cond.notify_one();
    }
  }
  return 0;
}

void tuner_t::sendthread()
{
  std::unique_lock<std::mutex> lk(mtx);
  while(run_thread) {
    cond.wait_for(lk, 100ms);
    if(has_data) {
      *p_freq = v_freq;
      *p_note = v_note;
      *p_octave = v_octave;
      *p_delta = v_delta;
      *p_confidence = v_confidence;
      if(isactive) {
        lo_send_message(target, path.c_str(), msg);
        strobebuffersend *= 1.0f / std::max(1e-6f, strobebuffersend.maxabs());
        auto oscmsgargv = lo_message_get_argv(msg_strobe);
        for(uint32_t k = 0; k < strobebufferlen; ++k)
          oscmsgargv[k]->f = strobebuffersend.d[k];
        lo_send_message(target, path_strobe.c_str(), msg_strobe);
      }
      has_data = false;
    }
  }
}

tuner_t::~tuner_t()
{
  deactivate();
  run_thread = false;
  thread.join();
  if(target)
    lo_address_free(target);
  lo_message_free(msg);
}

REGISTER_MODULE(tuner_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */
