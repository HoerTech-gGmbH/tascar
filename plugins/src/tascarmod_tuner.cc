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
      0.0f};                        ///< Pitch correction in cents per semitone
  std::string path = "/tuner";      ///< OSC message path
  float tau = 0.05f;                ///< Time constant for low-pass filters
  std::vector<std::string> connect; ///< Port names of input connection
  float fmin = 40.0f;               ///< Minimal frequency for analysis in Hz
  float fmax = 4000.0f;             ///< maximal frequency for analysis in Hz
  std::string tuning = "equal";
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
  float* p_good = NULL;         ///< Pointer to confidence value in OSC message
  lo_message msg;               ///< OSC message buffer
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
  float v_good = 0.0f;                ///< Current confidence value
  TASCAR::o1flt_lowpass_t mean_lp;    ///< Low-pass filter for frequency mean
  TASCAR::o1flt_lowpass_t
      std_lp; ///< Low-pass filter for frequency standard deviation
  std::vector<float> cum_pitches;
  std::vector<float> cum_intensities;
  uint32_t bin_min = 0;
  uint32_t bin_max = 0;
  std::vector<float> pitch_ratios = {4.0f, 3.0f, 2.0f};
};

tuner_vars_t::tuner_vars_t(const TASCAR::module_cfg_t& cfg)
    : TASCAR::module_base_t(cfg)
{
  GET_ATTRIBUTE(id, "", "ID used in jack name and OSC path");
  GET_ATTRIBUTE(prefix, "", "prefix used in OSC path");
  GET_ATTRIBUTE(wlen, "samples", "window length");
  GET_ATTRIBUTE(f0, "Hz", "frequency of pitch 0");
  GET_ATTRIBUTE(url, "", "Tuner display URL");
  GET_ATTRIBUTE(path, "", "Tuner display path");
  GET_ATTRIBUTE_BOOL(oscactive, "Activate OSC sending on start");
  GET_ATTRIBUTE_BOOL(isactive, "Activate analysis on start");
  GET_ATTRIBUTE(connect, "", "Port names of input connection");
  GET_ATTRIBUTE(fmin, "Hz", "Minimal frequency for analysis");
  GET_ATTRIBUTE(fmax, "Hz", "Maximal frequency for analysis");
  GET_ATTRIBUTE(tuning, "equal|werkmeister3|meantone4|meantone6|valotti",
                "Tuning");
  // http://www.instrument-tuner.com/temperaments_de.html
  if(tuning == "equal")
    pitchcorr = {0.0f};
  else if(tuning == "werkmeister3")
    pitchcorr = {0.0f, -3.91f, 3.91f,  0.0f, -3.91f, 3.91f,
                 0.0f, 1.955f, -7.82f, 0.0f, 1.955f, -1.955f};
  else if(tuning == "meantone4")
    pitchcorr = {10.265f,  -13.686f, 3.422f,   20.53f, -3.421f, 13.686f,
                 -10.265f, 6.843f,   -17.108f, 0.0f,   17.108f, -6.843f};
  else if(tuning == "meantone6")
    pitchcorr = {6.0f,  -2.0f, 2.0f, 0.0f, -2.0f, 8.0f,
                 -6.0f, 4.0f,  2.0f, 0.0f, 4.0f,  -4.0f};
  else if(tuning == "valotti")
    pitchcorr = {5.865f,  0.0f,  1.955f, 3.91f, -1.955f, 7.82f,
                 -1.955f, 3.91f, 1.955f, 0.0f,  5.865f,  -3.91f};
  else
    throw TASCAR::ErrMsg("Unsupported tuning: \"" + tuning + "\".");
}

tuner_t::tuner_t(const TASCAR::module_cfg_t& cfg)
    : tuner_vars_t(cfg), jackc_db_t(id, wlen),
      // ola_t(uint32_t fftlen, uint32_t wndlen, uint32_t chunksize,
      // windowtype_t wnd, windowtype_t zerownd,double wndpos,windowtype_t
      // postwnd=WND_RECT);
      ola1(4 * wlen, 4 * wlen, wlen, TASCAR::stft_t::WND_HANNING,
           TASCAR::stft_t::WND_RECT, 0.5),
      ola2(4 * wlen, 4 * wlen, wlen, TASCAR::stft_t::WND_HANNING,
           TASCAR::stft_t::WND_RECT, 0.5),
      delaystate(0.0f), in_delayed(wlen), mean_lp({tau}, srate / wlen),
      std_lp({tau}, srate / wlen)

{
  std::set<double> vf;
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
  p_good = &(oscmsgargv[4]->f);
  thread = std::thread(&tuner_t::sendthread, this);
  cum_pitches.resize(256);
  cum_intensities.resize(256);
  bin_min = (uint32_t)(fmin / srate * wlen);
  bin_max = std::min((uint32_t)(wlen / 2), (uint32_t)(fmax / srate * wlen));
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
  for(auto& p : cum_intensities)
    p = 0.0f;
  // spectral analysis and instantaneous frequency:
  ola1.process(w_in);
  ola2.process(in_delayed);
  // use frequency with highest intensity:
  for(uint32_t k = bin_min; k < std::min(ola1.s.n_, bin_max); ++k) {
    std::complex<float> p(ola1.s.b[k] * std::conj(ola2.s.b[k]));
    float instfreq = std::arg(p) * f_sample / TASCAR_2PI;
    float intensity = std::abs(p);
    int idx_pitch = freq2pitch_int(instfreq);
    cum_pitches[idx_pitch] += intensity * instfreq;
    cum_intensities[idx_pitch] += intensity;
  }
  for(size_t k = 0; k < cum_pitches.size(); ++k) {
    if(cum_intensities[k] > 0.0f)
      cum_pitches[k] /= cum_intensities[k];
  }
  for(size_t k = 0; k < cum_pitches.size(); ++k) {
    for(auto r : pitch_ratios) {
      float f_new = cum_pitches[k] / r;
      if(f_new > fmin) {
        int idx_new = freq2pitch_int(f_new);
        if(cum_intensities[idx_new] > 0.01 * cum_intensities[k]) {
          cum_intensities[idx_new] += cum_intensities[k];
          cum_intensities[k] = 0.0f;
        }
      }
    }
  }
  double intensity_max(0.0);
  double freq_max(0.0);
  for(size_t k = 0; k < cum_pitches.size(); ++k) {
    if(cum_intensities[k] > intensity_max) {
      freq_max = cum_pitches[k];
      intensity_max = cum_intensities[k];
    }
  }
  // freq_max *= f_sample / TASCAR_2PI;
  mean_lp.set_tau(tau);
  std_lp.set_tau(tau);
  auto ifreq_mean = mean_lp(0, freq_max);
  auto ifreq_std = sqrtf(std::max(
      0.0f, std_lp(0, (ifreq_mean - freq_max) * (ifreq_mean - freq_max))));
  // phase modification, e.g., convert to minimum phase stimulus:
  // send fill state:
  if(target && oscactive) {
    if(mtx.try_lock()) {
      v_freq = ifreq_mean;
      v_note = std::min(255.0f,
                        std::max(0.0f, 12.0f * log2f(freq_max / f0) + 69.0f));
      int i_note = (int)(v_note + 0.5f);
      v_delta = 100.0f * (v_note - i_note);
      auto div_note = div(i_note, 12);
      v_octave = div_note.quot - 5.0f;
      i_note = div_note.rem;
      auto i_note_corr = i_note % pitchcorr.size();
      v_delta -= pitchcorr[i_note_corr];
      v_good = expf(-ifreq_std);
      v_note = i_note;
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
      *p_good = v_good;
      if(isactive)
        lo_send_message(target, path.c_str(), msg);
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
