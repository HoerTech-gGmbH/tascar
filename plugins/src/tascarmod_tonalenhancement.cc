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

#include "ola.h"
#include "session.h"
#include <stdlib.h>

class tonalenhance_vars_t : public TASCAR::module_base_t {
public:
  tonalenhance_vars_t(const TASCAR::module_cfg_t& cfg);
  ~tonalenhance_vars_t(){};

protected:
  std::string id = "tonalenhance";
  std::string oscprefix = "";
  float tau_envelope = 4.0f;
  float tau_std = 0.4f;
  float wet = 1.0f;
  uint32_t wlen = 256;
  double gain = 1.0;
  bool delayenvelope = true;
  float sigma0 = 4.0f;
  bool inverse = true;
};

tonalenhance_vars_t::tonalenhance_vars_t(const TASCAR::module_cfg_t& cfg)
    : TASCAR::module_base_t(cfg)
{
  GET_ATTRIBUTE(id, "", "ID used for jack and OSC");
  GET_ATTRIBUTE(oscprefix, "", "Prefix used in OSC");
  GET_ATTRIBUTE(tau_envelope, "s", "Envelope tracking time constant");
  GET_ATTRIBUTE(tau_std, "s", "Stability time constant");
  GET_ATTRIBUTE(wet, "", "Wet-dry ratio");
  GET_ATTRIBUTE(wlen, "samples", "Window length");
  GET_ATTRIBUTE_DB(gain, "Gain");
  GET_ATTRIBUTE_BOOL(delayenvelope, "Delay envelope to match processed signal");
  GET_ATTRIBUTE(sigma0, "Hz", "standard deviation for -6 dB gain");
  GET_ATTRIBUTE_BOOL(inverse, "Inverse gain, attenuate instationary");
}

class tonalenhance_t : public tonalenhance_vars_t, public jackc_db_t {
public:
  tonalenhance_t(const TASCAR::module_cfg_t& cfg);
  ~tonalenhance_t();
  virtual int inner_process(jack_nframes_t nframes,
                            const std::vector<float*>& inBuffer,
                            const std::vector<float*>& outBuffer);
  virtual int process(jack_nframes_t, const std::vector<float*>&,
                      const std::vector<float*>&);
  static int osc_apply(const char* path, const char* types, lo_arg** argv,
                       int argc, lo_message msg, void* user_data);
  void set_apply(float t);

protected:
  TASCAR::ola_t ola;
  TASCAR::spec_t prev;
  double Lin = 0.0;
  double Lout = 0.0;
  uint32_t t_apply = 0;
  float deltaw = 0.0;
  float currentw = 0.0;
  float fsppi = 1.0;
  TASCAR::o1flt_lowpass_t mean_lp;
  TASCAR::o1flt_lowpass_t std_lp;
};

int tonalenhance_t::osc_apply(const char*, const char*, lo_arg** argv, int,
                              lo_message, void* user_data)
{
  ((tonalenhance_t*)user_data)->set_apply(argv[0]->f);
  return 0;
}

void tonalenhance_t::set_apply(float t)
{
  deltaw = 0;
  t_apply = 0;
  if(t >= 0) {
    uint32_t tau(std::max(1, (int32_t)(srate * t)));
    deltaw = (wet - currentw) / (float)tau;
    t_apply = tau;
  } else {
    currentw = wet;
  }
}

int tonalenhance_t::process(jack_nframes_t n, const std::vector<float*>& vIn,
                            const std::vector<float*>& vOut)
{
  jackc_db_t::process(n, vIn, vOut);
  TASCAR::wave_t w_in(n, vIn[0]);
  TASCAR::wave_t w_out(n, vOut[0]);
  if(!delayenvelope) {
    float env_c1(0);
    if(tau_envelope > 0) {
      env_c1 = exp(-1.0 / (tau_envelope * (double)srate));
    }
    double env_c2(1.0 - env_c1);
    // envelope reconstruction:
    for(uint32_t k = 0; k < w_in.n; ++k) {
      Lin *= env_c1;
      Lin += env_c2 * w_in[k] * w_in[k];
      Lout *= env_c1;
      Lout += env_c2 * w_out[k] * w_out[k];
      if(TASCAR::is_denormal(Lin)) {
        Lin = 0.0;
      }
      if(TASCAR::is_denormal(Lout)) {
        Lout = 0.0;
      }
      if(Lout > 0.0)
        w_out[k] *= sqrt(Lin / Lout);
      if(TASCAR::is_denormal(w_out[k]))
        w_out[k] = 0.0f;
    }
  }
  for(uint32_t k = 0; k < w_in.n; ++k) {
    if(t_apply) {
      t_apply--;
      currentw += deltaw;
      if(!t_apply)
        currentw = wet;
    }
    w_out[k] *= gain * currentw;
    w_out[k] += (1.0f - std::max(0.0f, currentw)) * w_in[k];
  }
  return 0;
}

/**
 * @brief Actual filter algorithm
 */
int tonalenhance_t::inner_process(jack_nframes_t n,
                                  const std::vector<float*>& vIn,
                                  const std::vector<float*>& vOut)
{
  std_lp.set_tau(tau_std);
  auto sigma0_corr = logf(2.0f) / sigma0;
  TASCAR::wave_t w_in(n, vIn[0]);
  TASCAR::wave_t w_out(n, vOut[0]);
  // process short-term spectrum:
  ola.process(w_in);
  //
  for(uint32_t k = ola.s.size(); k--;) {
    auto a = ola.s[k];
    auto b = prev[k];
    prev[k] = a;
    auto ifreq = atan2f(a.imag() * b.real() - a.real() * b.imag(),
                        a.real() * b.real() + a.imag() * b.imag());
    if(ifreq < 0)
      ifreq += TASCAR_2PIf;
    ifreq *= fsppi;
    auto ifreq_mean = mean_lp(k, ifreq);
    auto ifreq_std = sqrtf(
        std::max(0.0f, std_lp(k, (ifreq_mean - ifreq) * (ifreq_mean - ifreq))));
    auto lgain = 1.0f - expf(-ifreq_std * sigma0_corr);
    if(lgain < 1e-4f)
      lgain = 1e-4f;
    if(inverse)
      lgain = 1.0f - lgain;
    ola.s[k] *= lgain;
  }
  ola.ifft(w_out);
  if(delayenvelope) {
    TASCAR::wave_t w_in(n, vIn[0]);
    TASCAR::wave_t w_out(n, vOut[0]);
    float env_c1(0);
    if(tau_envelope > 0)
      env_c1 = exp(-1.0 / (tau_envelope * (double)srate));
    float env_c2(1.0f - env_c1);
    // envelope reconstruction:
    for(uint32_t k = 0; k < w_in.size(); ++k) {
      Lin *= env_c1;
      Lin += env_c2 * w_in[k] * w_in[k];
      Lout *= env_c1;
      Lout += env_c2 * w_out[k] * w_out[k];
      if(Lout > 0)
        w_out[k] *= sqrt(Lin / Lout);
    }
  }
  return 0;
}

tonalenhance_t::tonalenhance_t(const TASCAR::module_cfg_t& cfg)
    : tonalenhance_vars_t(cfg), jackc_db_t(id, wlen),
      ola(4 * wlen, 2 * wlen, wlen, TASCAR::stft_t::WND_HANNING,
          TASCAR::stft_t::WND_HANNING, 0.5, TASCAR::stft_t::WND_RECT),
      prev(ola.s.size()), Lin(0), Lout(0), t_apply(0), deltaw(0), currentw(0),
      mean_lp(std::vector<float>(ola.s.size(), tau_std), srate / wlen),
      std_lp(std::vector<float>(ola.s.size(), tau_std), srate / wlen)
{
  for(uint32_t k = 0; k < prev.size(); ++k)
    prev[k] = 0.0f;
  add_input_port("in");
  add_output_port("out");
  session->add_float("/" + oscprefix + id + "/tau_env", &tau_envelope);
  session->add_float("/" + oscprefix + id + "/tau_std", &tau_std);
  session->add_double_db("/" + oscprefix + id + "/gain", &gain);
  session->add_float("/" + oscprefix + id + "/wet", &wet);
  session->add_float("/" + oscprefix + id + "/sigma0", &sigma0, "[0,22000]",
                     "Frequency variance in Hz");
  session->add_bool("/" + oscprefix + id + "/inverse", &inverse);
  session->add_method("/" + oscprefix + id + "/wetapply", "f",
                      &tonalenhance_t::osc_apply, this);
  session->add_bool("/" + oscprefix + id + "/tonalenhance/delayenvelope",
                    &delayenvelope);
  fsppi = srate / (TASCAR_2PIf * wlen);
  set_apply(0);
  activate();
}

tonalenhance_t::~tonalenhance_t()
{
  deactivate();
}

REGISTER_MODULE(tonalenhance_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */
