/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2020 Fenja Schwark, Giso Grimm
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

/*

  HRTF simulation.

This receiver describes the main features of measured head related
transfer functions by using a few low-order digital filters. The
parametrization is based on the Spherical Head Model (SHM) by
Brown & Duda (1998) and includes three further low-order filters.

The Duda SHM was extended by O. Buttler and S.D. Ewert in the context
of room acoustics simulator RAZR (Wendt, 2014; www.razrengine.com)
in Buttler (2018) to improve left-right, front-back, and
elevation perception.

In order to optimize the values for the filter parameters of the
original RAZR SHM-Model, the frequency response of the receiver has
been fitted to measured HRTFs of the KEMAR dummy head (Schwark, 2020)
provided by the OlHeaD-HRTF database (Denk & Kollmeier, 2020).

This implementation was written by Fenja Schwark.


C Phillip Brown and Richard O Duda. A structural model for binaural
  sound synthesis. IEEE Transition on Speech and Audio Processing,
  6(5):476–488, 1998. doi: 10.1109/89.709673.  URL
  https://doi.org/10.1109/89.709673.

Torben Wendt, Steven van de Par, and Stephan Ewert. A
  Computationally-Efficient and Perceptually-Plausible Algorithm for
  Binaural Room Impulse Response Simulation. Jour- nal of the Audio
  Engineering Society, 62(11):748–766, dec 2014. ISSN 15494950.  doi:
  10.17743/jaes.2014.0042. URL
  http://www.aes.org/e-lib/browse.cfm?elib= 17550.

Olliver Buttler. Optimierung und erweiterung einer effizienten methode
  zur synthese binau- raler raumimpulsantworten. MSc thesis, 2018.

Fenja Schwark. Data-driven optimization of parameterized head related
  transfer functions for the implementation in a real-time virtual
  acoustic rendering framework. BSc thesis, 2020.

Florian Denk and Birger Kollmeier. The hearpiece database of
  individual transfer functions of an openly available in-the-ear
  earpiece for hearing device research. https://zenodo.
  org/record/3900114, 2020.


 */

#include "delayline.h"
#include "filterclass.h"
#include "receivermod.h"
#include <random>

const std::complex<double> i(0.0, 1.0);
const std::complex<float> i_f(0.0f, 1.0f);

class hrtf_param_t : public TASCAR::xml_element_t {
public:
  hrtf_param_t(tsccfg::node_t xmlsrc);
  uint32_t sincorder;
  uint32_t sincsampling = 64;
  float c;
  TASCAR::pos_t dir_l;
  TASCAR::pos_t dir_r;
  TASCAR::pos_t dir_front;
  float radius;
  float angle;
  float thetamin; // minimum of the filter modelling the head shadow effect
  float omega;    // cut-off frequency of HSM
  float alphamin; // parameter which determines maximal depth of SHM at thetamin
  float startangle_front; // define range in which filter modelling pinna
                          // shadow operates
  float omega_front;      // cut-off frequency of pinna shadow filter
  float alphamin_front;   // parameter which determines maximal depth of pinna
                          // shadow filter
  float startangle_up;    // define range in which filter modelling torso shadow
                          // operates
  float omega_up;         // cut-off frequency of torso shadow filter
  float alphamin_up;      // parameter which determines maximal depth of torso
                          // shadow filter
  float startangle_notch; // define range in which parametric equalizer operates
  float freq_start;       // notch frequency at startangle_notch
  float freq_end;         // notch frequency at 90 degr elev
  float maxgain; // gain applied at 90 degr elev (0 dB gain at startangle_notch
                 // and linear increase)
  float Q_notch; // inverse Q factor of the parametric equalizer
  bool diffuse_hrtf;           // apply hrtf model also to diffuse rendering
  uint32_t prewarpingmode = 0; // Azimuth prewarping mode
  std::vector<float> gaincorr = { 1.0f, 1.0f }; // channel-wise gain correction
};

hrtf_param_t::hrtf_param_t(tsccfg::node_t xmlsrc)
    : TASCAR::xml_element_t(xmlsrc), sincorder(0), c(340), dir_l(1, 0, 0),
      dir_r(1, 0, 0), dir_front(1, 0, 0), radius(0.08f),
      angle(90.0f * DEG2RADf), // ears modelled at +/- 90 degree
      thetamin(160.0f * DEG2RADf), omega(3100), alphamin(0.14f),
      startangle_front(0), omega_front(11200.0f), alphamin_front(0.39f),
      startangle_up(135.0f * DEG2RADf),
      //    omega_up(c/radius/2),
      alphamin_up(0.1f), startangle_notch(102.0f * DEG2RADf),
      freq_start(1300.0f), freq_end(650.0f), maxgain(-5.4f), Q_notch(2.3f),
      diffuse_hrtf(false)
{
  GET_ATTRIBUTE(sincorder, "", "Sinc interpolation order of ITD delay line");
  GET_ATTRIBUTE(sincsampling, "",
                "Sinc table sampling of ITD delay line, or 0 for no table.");
  GET_ATTRIBUTE(c, "m/s", "Speed of sound");
  GET_ATTRIBUTE(radius, "m", "Radius of sphere modeling the head");
  GET_ATTRIBUTE_DEG(angle, "Position of the ears on the sphere");
  GET_ATTRIBUTE_DEG(
      thetamin, "angle with respect to the position of the ears at which the "
                "maximum depth of the high-shelf realizing the SHM is reached");
  GET_ATTRIBUTE(omega, "Hz",
                "cut-off frequency of the high-self realizing the SHM");
  GET_ATTRIBUTE(alphamin, "",
                "parameter which determines the depth of the high-shelf "
                "realizing the SHM");
  GET_ATTRIBUTE_DEG(startangle_front,
                    "the second high-shelf, e.g. to model pinna shadow effect, "
                    "is applied when the angle with respect to front direction "
                    "[1 0 0] is larger than \\attr{startangle_front}");
  GET_ATTRIBUTE(omega_front, "Hz", "cut-off frequency of the second high-self");
  GET_ATTRIBUTE(
      alphamin_front, "",
      "parameter which determines the depth of the second high-shelf");
  GET_ATTRIBUTE_DEG(startangle_up,
                    "the third high-shelf which models the shadow effect of "
                    "the torso is applied when the angle with respect to up "
                    "direction [0 0 1] is larger than \\attr{startangle_up}");
  omega_up = c / radius / 2;
  GET_ATTRIBUTE(omega_up, "Hz",
                "cut-off frequency of the second high-shelf in Hz");
  GET_ATTRIBUTE(
      alphamin_up, "",
      "parameter which determines the depth of the second high-shelf");
  GET_ATTRIBUTE_DEG(
      startangle_notch,
      "notch filter to model concha notch is applied if angle with respect to "
      "up direction [0 0 1] is smaller than \\attr{startangle_notch}");
  GET_ATTRIBUTE(freq_start, "Hz",
                "notch center frequency at \\attr{startangle_notch}");
  GET_ATTRIBUTE(freq_end, "Hz", "notch center frequency at [0 0 1]");
  GET_ATTRIBUTE(maxgain, "dB",
                "gain applied at [0 0 1] -- gain is 0 dB at "
                "\\attr{startangle_notch} and increases linearly");
  GET_ATTRIBUTE(Q_notch, "", "quality factor of the notch filter");
  dir_l.rot_z(angle);
  dir_r.rot_z(-angle);
  GET_ATTRIBUTE_BOOL(diffuse_hrtf,
                     "apply hrtf model also to diffuse rendering");
  GET_ATTRIBUTE(
      prewarpingmode, "",
      "Azimuth pre-warping mode, 0 = original, 1 = none, 2 = corrected");
  GET_ATTRIBUTE_DB(gaincorr, "channel-wise gain correction");
  if(gaincorr.size() != 2) {
    throw TASCAR::ErrMsg("gaincorr requires two entries");
  }
}

class hrtf_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(float srate, uint32_t chunksize, const hrtf_param_t& par_plugin);
    void filterdesign(const float theta_left, const float theta_right,
                      const float theta_front, const float elevation);
    inline void filter(const float& input)
    {
      out_l = (state_l = (dline_l.get_dist_push(tau_l, input)));
      out_r = (state_r = (dline_r.get_dist_push(tau_r, input)));
      out_l = bqelev_l.filter(bqazim_l.filter(out_l));
      out_r = bqelev_r.filter(bqazim_r.filter(out_r));
    }
    void set_param(const TASCAR::pos_t& prel_norm, uint32_t prewarpingmode);
    float fs;
    float dt;
    const hrtf_param_t& par;
    TASCAR::varidelay_t dline_l;
    TASCAR::varidelay_t dline_r;
    TASCAR::biquadf_t bqazim_l;
    TASCAR::biquadf_t bqazim_r;
    TASCAR::biquadf_t bqelev_l;
    TASCAR::biquadf_t bqelev_r;
    float out_l;
    float out_r;
    float state_l;
    float state_r;
    float target_tau_l;
    float target_tau_r;
    float tau_l;
    float tau_r;
    float inv_a0;
    float alpha_l;
    float alpha_r;
    float inv_a0_f;
    float alpha_f;
    float inv_a0_u;
    float alpha_u;
  };
  class diffuse_data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    diffuse_data_t(float srate, uint32_t chunksize,
                   const hrtf_param_t& par_plugin);
    hrtf_t::data_t xp, xm, yp, ym, zp, zm;
  };
  hrtf_t(tsccfg::node_t xmlsrc);
  ~hrtf_t();
  void add_pointsource(const TASCAR::pos_t& prel, double width,
                       const TASCAR::wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       receivermod_base_t::data_t*);
  // void tau_woodworth_schlosberg(const float theta, float& tau);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                               std::vector<TASCAR::wave_t>& output,
                               receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,
                                                uint32_t fragsize) const;
  receivermod_base_t::data_t*
  create_diffuse_state_data(double srate, uint32_t fragsize) const;
  virtual void configure();
  virtual void release();
  virtual void postproc(std::vector<TASCAR::wave_t>& output);
  virtual void add_variables(TASCAR::osc_server_t* srv);

private:
  hrtf_param_t par;
  float decorr_length;
  bool decorr;
  std::vector<TASCAR::overlap_save_t*> decorrflt;
  std::vector<TASCAR::wave_t*> diffuse_render_buffer;
};

// calculate time delay according to woodworth and schlosberg
void tau_woodworth_schlosberg(const float theta, const float radius, float& tau)
{
  if(theta < TASCAR_PI2f)
    tau = -radius * (cosf(theta) - 1.0f);
  else
    tau = radius * (theta - TASCAR_PI2f + 1.0f);
}

hrtf_t::~hrtf_t() {}

hrtf_t::data_t::data_t(float srate, uint32_t chunksize,
                       const hrtf_param_t& par_plugin)
    : fs(srate), dt(1.0f / std::max(1.0f, (float)chunksize)), par(par_plugin),
      dline_l((uint32_t)(4.0f * par.radius * srate / par.c + 2.0f +
                         (float)par.sincorder),
              srate, par.c, par.sincorder, par.sincsampling),
      dline_r((uint32_t)(4.0f * par.radius * srate / par.c + 2.0f +
                         (float)par.sincorder),
              srate, par.c, par.sincorder, par.sincsampling),
      out_l(0), out_r(0), state_l(0), state_r(0), tau_l(0), tau_r(0),
      inv_a0(1.0f / (par.omega + fs)), alpha_l(0.0f), alpha_r(0.0f),
      inv_a0_f(1.0f / (par.omega_front + fs)), alpha_f(0.0f),
      inv_a0_u(1.0f / (par.omega_up + fs)), alpha_u(0.0f)
{
}

void hrtf_t::data_t::filterdesign(const float theta_l, const float theta_r,
                                  const float theta_f, const float elevation)
{
  // bqazim_l/r
  // parameter of SHM
  alpha_l = (1.0f + 0.5f * par.alphamin +
             (1.0f - 0.5f * par.alphamin) *
                 cosf(theta_l / par.thetamin * TASCAR_PIf)) *
            fs;
  alpha_r = (1.0f + 0.5f * par.alphamin +
             (1.0f - 0.5f * par.alphamin) *
                 cosf(theta_r / par.thetamin * TASCAR_PIf)) *
            fs;

  if(theta_f > par.startangle_front) // pinna shadow effect
  {
    alpha_f = (1.0f - (1.0f - par.alphamin_front) *
                          (0.5f - cosf((theta_f - par.startangle_front) /
                                       (TASCAR_PIf - par.startangle_front) *
                                       TASCAR_PIf) *
                                      0.5f)) *
              fs;
    // alpha_f = (1.0 - ( 1.0-par.alphamin_front ) * ( 0.5-0.5*cos(theta_f) ) )
    // *fs; // if startangle_front == 0

    bqazim_l.set_coefficients(
        (par.omega - fs) * inv_a0 + (par.omega_front - fs) * inv_a0_f,
        (par.omega - fs) * inv_a0 * (par.omega_front - fs) * inv_a0_f,
        (par.omega + alpha_l) * inv_a0 * (par.omega_front + alpha_f) * inv_a0_f,
        (par.omega + alpha_l) * inv_a0 * (par.omega_front - alpha_f) *
                inv_a0_f +
            (par.omega - alpha_l) * inv_a0 * (par.omega_front + alpha_f) *
                inv_a0_f,
        (par.omega - alpha_l) * inv_a0 * (par.omega_front - alpha_f) *
            inv_a0_f);

    bqazim_r.set_coefficients(
        (par.omega - fs) * inv_a0 + (par.omega_front - fs) * inv_a0_f,
        (par.omega - fs) * inv_a0 * (par.omega_front - fs) * inv_a0_f,
        (par.omega + alpha_r) * inv_a0 * (par.omega_front + alpha_f) * inv_a0_f,
        (par.omega + alpha_r) * inv_a0 * (par.omega_front - alpha_f) *
                inv_a0_f +
            (par.omega - alpha_r) * inv_a0 * (par.omega_front + alpha_f) *
                inv_a0_f,
        (par.omega - alpha_r) * inv_a0 * (par.omega_front - alpha_f) *
            inv_a0_f);
  } else {
    bqazim_l.set_coefficients((par.omega - fs) * inv_a0, 0.0,
                              (par.omega + alpha_l) * inv_a0,
                              (par.omega - alpha_l) * inv_a0, 0.0);
    bqazim_r.set_coefficients((par.omega - fs) * inv_a0, 0.0,
                              (par.omega + alpha_r) * inv_a0,
                              (par.omega - alpha_r) * inv_a0, 0.0);
  }

  // bqelev_l/r
  if(elevation <
     par.startangle_notch) // concha notch (parametric equalizer for cut)
  {
    float p_angle = (par.startangle_notch - elevation) / par.startangle_notch;
    float transform_const =
        1.0f /
        tanf(TASCAR_PIf *
             (p_angle * (par.freq_end - par.freq_start) + par.freq_start) / fs);
    float Q_n = transform_const / par.Q_notch;
    float inv_gain_Q = powf(10.0f, (-par.maxgain * p_angle / 20.0f)) * Q_n;
    float tc_sq = transform_const * transform_const;
    float inv_a0_n = 1.0f / (tc_sq + 1.0f + inv_gain_Q);
    bqelev_l.set_coefficients(
        2.0f * (1.0f - tc_sq) * inv_a0_n,
        (tc_sq + 1.0f - inv_gain_Q) * inv_a0_n, (tc_sq + 1.0f + Q_n) * inv_a0_n,
        2.0f * (1.0f - tc_sq) * inv_a0_n, (tc_sq + 1.0f - Q_n) * inv_a0_n);
    bqelev_r.set_coefficients(
        2.0f * (1.0f - tc_sq) * inv_a0_n,
        (tc_sq + 1.0f - inv_gain_Q) * inv_a0_n, (tc_sq + 1.0f + Q_n) * inv_a0_n,
        2.0f * (1.0f - tc_sq) * inv_a0_n, (tc_sq + 1.0f - Q_n) * inv_a0_n);
  } else if(elevation > par.startangle_up) // torso shadow effect
  {
    alpha_u = (1.0f - (1.0f - par.alphamin_up) *
                          (0.5f - 0.5f * cosf((elevation - par.startangle_up) /
                                              (TASCAR_PIf - par.startangle_up) *
                                              TASCAR_PIf))) *
              fs;

    bqelev_l.set_coefficients((par.omega_up - fs) * inv_a0_u, 0.0f,
                              (par.omega_up + alpha_u) * inv_a0_u,
                              (par.omega_up - alpha_u) * inv_a0_u, 0.0f);
    bqelev_r.set_coefficients((par.omega_up - fs) * inv_a0_u, 0.0f,
                              (par.omega_up + alpha_u) * inv_a0_u,
                              (par.omega_up - alpha_u) * inv_a0_u, 0.0f);
  } else {
    bqelev_l.set_coefficients(0.0, 0.0, 1.0, 0.0, 0.0);
    bqelev_r.set_coefficients(0.0, 0.0, 1.0, 0.0, 0.0);
  }
}

hrtf_t::diffuse_data_t::diffuse_data_t(float srate, uint32_t chunksize,
                                       const hrtf_param_t& par_plugin)
    : xp(srate, chunksize, par_plugin), xm(srate, chunksize, par_plugin),
      yp(srate, chunksize, par_plugin), ym(srate, chunksize, par_plugin),
      zp(srate, chunksize, par_plugin), zm(srate, chunksize, par_plugin)
{
  xp.set_param(TASCAR::pos_t(1, 0, 0), 0);
  xm.set_param(TASCAR::pos_t(-1, 0, 0), 0);
  yp.set_param(TASCAR::pos_t(0, 1, 0), 0);
  ym.set_param(TASCAR::pos_t(0, -1, 0), 0);
  zp.set_param(TASCAR::pos_t(0, 0, 1), 0);
  zm.set_param(TASCAR::pos_t(0, 0, -1), 0);
}

void hrtf_t::configure()
{
  TASCAR::receivermod_base_t::configure();
  n_channels = 2;
  // initialize decorrelation filter:
  decorrflt.clear();
  diffuse_render_buffer.clear();
  uint32_t irslen((uint32_t)(decorr_length * (float)f_sample));
  uint32_t paddedirslen((1 << (int)(ceil(log2(irslen + n_fragment - 1)))) -
                        n_fragment + 1);
  for(uint32_t k = 0; k < 2; ++k)
    decorrflt.push_back(new TASCAR::overlap_save_t(paddedirslen, n_fragment));
  TASCAR::fft_t fft_filter(irslen);
  std::mt19937 gen(1);
  std::uniform_real_distribution<float> dis(0.0f, TASCAR_2PIf);
  for(uint32_t k = 0; k < 2; ++k) {
    for(uint32_t b = 0; b < fft_filter.s.n_; ++b)
      fft_filter.s[b] = std::exp(i_f * dis(gen));
    fft_filter.ifft();
    for(uint32_t t = 0; t < fft_filter.w.n; ++t)
      fft_filter.w[t] *= (0.5f - 0.5f * cosf((float)t * TASCAR_2PIf /
                                             (float)(fft_filter.w.n)));
    decorrflt[k]->set_irs(fft_filter.w, false);
    diffuse_render_buffer.push_back(new TASCAR::wave_t(n_fragment));
  }
  labels.clear();
  labels.push_back("_l");
  labels.push_back("_r");
  // end of decorrelation filter.
}

void hrtf_t::release()
{
  TASCAR::receivermod_base_t::release();
  for(auto it = decorrflt.begin(); it != decorrflt.end(); ++it)
    delete(*it);
  for(auto it = diffuse_render_buffer.begin();
      it != diffuse_render_buffer.end(); ++it)
    delete(*it);
  decorrflt.clear();
  diffuse_render_buffer.clear();
}

void hrtf_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  TASCAR::receivermod_base_t::postproc(output);
  for(uint32_t ch = 0; ch < 2; ++ch) {
    if(decorr)
      decorrflt[ch]->process(*(diffuse_render_buffer[ch]), output[ch], true);
    else
      output[ch] += *(diffuse_render_buffer[ch]);
    diffuse_render_buffer[ch]->clear();
    output[ch] *= par.gaincorr[ch];
  }
}

// Implementation of TASCAR::receivermod_base_t::add_variables()
void hrtf_t::add_variables(TASCAR::osc_server_t* srv)
{
  TASCAR::receivermod_base_t::add_variables(srv);
  srv->add_bool("/hrtf/decorr", &decorr);
  srv->add_float("/hrtf/angle", &par.angle);
  srv->add_float("/hrtf/radius", &par.radius);
  srv->add_float("/hrtf/thetamin", &par.thetamin);
  srv->add_float("/hrtf/omega", &par.omega);
  srv->add_float("/hrtf/alphamin", &par.alphamin);
  srv->add_float("/hrtf/startangle_front", &par.startangle_front);
  srv->add_float("/hrtf/omega_front", &par.omega_front);
  srv->add_float("/hrtf/alphamin_front", &par.alphamin_front);
  srv->add_float("/hrtf/startangle_up", &par.startangle_up);
  srv->add_float("/hrtf/omega_up", &par.omega_up);
  srv->add_float("/hrtf/alphamin_up", &par.alphamin_up);
  srv->add_float("/hrtf/startangle_notch", &par.startangle_notch);
  srv->add_float("/hrtf/freq_start", &par.freq_start);
  srv->add_float("/hrtf/freq_end", &par.freq_end);
  srv->add_float("/hrtf/maxgain", &par.maxgain);
  srv->add_float("/hrtf/Q_notch", &par.Q_notch);
  srv->add_bool("/hrtf/diffuse_hrtf", &par.diffuse_hrtf);
  srv->add_uint("/hrtf/prewarpingmode", &par.prewarpingmode, "[0,1,2]",
                "pre-warping mode, 0 = original, 1 = none, 2 = corrected");
  srv->add_vector_float_db("/hrtf/gaincorr", &par.gaincorr, "",
                           "channel-wise gain correction");
}

hrtf_t::hrtf_t(tsccfg::node_t xmlsrc)
    : TASCAR::receivermod_base_t(xmlsrc), par(xmlsrc), decorr_length(0.05f),
      decorr(false)
{
  GET_ATTRIBUTE(decorr_length, "s", "Decorrelation length");
  GET_ATTRIBUTE_BOOL(decorr, "Flag to use decorrelation of diffuse sounds");
}

void hrtf_t::data_t::set_param(const TASCAR::pos_t& prel_norm,
                               uint32_t prewarpingmode)
{
  // angle with respect to front (range [0, pi])
  float theta_front = acosf(dot_prodf(prel_norm, par.dir_front));
  // angle with respect to left ear (range [0, pi])
  float theta_l = acosf(dot_prodf(par.dir_l, prel_norm));
  // angle with respect to right ear (range [0, pi])
  float theta_r = acosf(dot_prodf(par.dir_r, prel_norm));

  switch(prewarpingmode) {
  case 0: // original
    // warping for better grip on small angles
    theta_l *= (1.0f - 0.5f * cosf(sqrtf(theta_l * TASCAR_PIf))) / 1.5f;
    theta_r *= (1.0f - 0.5f * cosf(sqrtf(theta_r * TASCAR_PIf))) / 1.5f;
    break;

  case 2: // corrected
    // warping for better grip on small angles
    theta_l /= (1.0f - 0.5f * cosf(sqrtf(theta_l * TASCAR_PIf))) / 1.5f;
    theta_r /= (1.0f - 0.5f * cosf(sqrtf(theta_r * TASCAR_PIf))) / 1.5f;
    break;
  }

  // time delay in meter (panning parameters: target_tau is reached at end of
  // the block)
  tau_woodworth_schlosberg(theta_l, par.radius, target_tau_l);
  tau_woodworth_schlosberg(theta_r, par.radius, target_tau_r);
  // calculate delta tau for each panning step

  filterdesign(theta_l, theta_r, theta_front,
               -(prel_norm.elevf() - TASCAR_PI2f));
}

void hrtf_t::add_pointsource(const TASCAR::pos_t& prel, double,
                             const TASCAR::wave_t& chunk,
                             std::vector<TASCAR::wave_t>& output,
                             receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  d->set_param(prel.normal(), par.prewarpingmode);
  // calculate delta tau for each panning step
  float dtau_l((d->target_tau_l - d->tau_l) * d->dt);
  float dtau_r((d->target_tau_r - d->tau_r) * d->dt);

  // apply panning:
  uint32_t N(chunk.size());
  for(uint32_t k = 0; k < N; ++k) {
    float v(chunk[k]);
    d->filter(v);
    output[0][k] += d->out_l;
    output[1][k] += d->out_r;
    d->tau_l += dtau_l;
    d->tau_r += dtau_r;
  }
  // explicitely apply final values, to avoid rounding errors:
  d->tau_r = d->target_tau_r;
  d->tau_l = d->target_tau_l;
}

void hrtf_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                                     std::vector<TASCAR::wave_t>&,
                                     receivermod_base_t::data_t* sd)
{
  hrtf_t::diffuse_data_t* d((hrtf_t::diffuse_data_t*)sd);
  if(par.diffuse_hrtf) {
    float* o_l(diffuse_render_buffer[0]->d);
    float* o_r(diffuse_render_buffer[1]->d);
    const float* i_w(chunk.w().d);
    const float* i_x(chunk.x().d);
    const float* i_y(chunk.y().d);
    const float* i_z(chunk.z().d);
    // decode diffuse sound field in microphone directions:
    for(uint32_t k = 0; k < chunk.size(); ++k) {
      // FOA decoding into main axes:
      d->xp.filter(0.70711f * *i_w + *i_x);
      d->xm.filter(0.70711f * *i_w - *i_x);
      d->yp.filter(0.70711f * *i_w + *i_y);
      d->ym.filter(0.70711f * *i_w - *i_y);
      d->zp.filter(0.70711f * *i_w + *i_z);
      d->zm.filter(0.70711f * *i_w - *i_z);
      *o_l += 0.25f * (d->xp.out_l + d->xm.out_l + d->yp.out_l + d->ym.out_l +
                       d->zp.out_l + d->zm.out_l);
      *o_r += 0.25f * (d->xp.out_r + d->xm.out_r + d->yp.out_r + d->ym.out_r +
                       d->zp.out_r + d->zm.out_r);
      ++o_l;
      ++o_r;
      ++i_w;
      ++i_x;
      ++i_y;
      ++i_z;
    }
  } else {
    float* o_l(diffuse_render_buffer[0]->d);
    float* o_r(diffuse_render_buffer[1]->d);
    const float* i_w(chunk.w().d);
    const float* i_x(chunk.x().d);
    const float* i_y(chunk.y().d);
    // decode diffuse sound field in microphone directions:
    for(uint32_t k = 0; k < chunk.size(); ++k) {
      *o_l += *i_w + (float)par.dir_l.x * (*i_x) + (float)par.dir_l.y * (*i_y);
      *o_r += *i_w + (float)par.dir_r.x * (*i_x) + (float)par.dir_r.y * (*i_y);
      ++o_l;
      ++o_r;
      ++i_w;
      ++i_x;
      ++i_y;
    }
  }
}

TASCAR::receivermod_base_t::data_t*
hrtf_t::create_state_data(double srate, uint32_t fragsize) const
{
  return new data_t((float)srate, fragsize, par);
}

TASCAR::receivermod_base_t::data_t*
hrtf_t::create_diffuse_state_data(double srate, uint32_t fragsize) const
{
  return new diffuse_data_t((float)srate, fragsize, par);
}

REGISTER_RECEIVERMOD(hrtf_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
