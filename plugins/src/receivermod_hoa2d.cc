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

#include "errorhandling.h"
#include "scene.h"
#include <fftw3.h>
#include <string.h>

const std::complex<double> i(0.0, 1.0);
const std::complex<float> i_f(0.0, 1.0);

class hoa2d_t : public TASCAR::receivermod_base_speaker_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize, uint32_t channels, double srate,
           TASCAR::fsplit_t::shape_t shape, double tau);
    virtual ~data_t();
    // point source speaker weights:
    uint32_t amb_order;
    TASCAR::spec_t enc_wp;
    TASCAR::spec_t enc_wm;
    TASCAR::spec_t enc_dwp;
    TASCAR::spec_t enc_dwm;
    TASCAR::wave_t wx_1;
    TASCAR::wave_t wx_2;
    TASCAR::wave_t wy_1;
    TASCAR::wave_t wy_2;
    TASCAR::fsplit_t fsplitdelay;
    TASCAR::varidelay_t dx;
    TASCAR::varidelay_t dy;
  };
  hoa2d_t(tsccfg::node_t xmlsrc);
  virtual ~hoa2d_t();
  void add_pointsource(const TASCAR::pos_t& prel, double width,
                       const TASCAR::wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                               std::vector<TASCAR::wave_t>& output,
                               receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,
                                                uint32_t fragsize) const;
  // initialize decoder and allocate buffers:
  virtual void configure();
  virtual void release();
  // decode HOA signals:
  void postproc(std::vector<TASCAR::wave_t>& output);
  void add_variables(TASCAR::osc_server_t* srv);

private:
  uint32_t nbins;
  uint32_t order;
  uint32_t amb_order;
  TASCAR::spec_t s_encoded;
  float* s_decoded;
  fftwf_plan dec;
  float fft_scale;
  // double wgain;
  bool maxre;
  double rotation;
  std::vector<std::complex<float>> ordergain;
  bool diffup;
  double diffup_rot;
  double diffup_delay;
  uint32_t diffup_maxorder;
  uint32_t idelay;
  uint32_t idelaypoint;
  // Zotter width control:
  // uint32_t truncofs; //< truncation offset
  // double dispersion; //< dispersion constant (to be replaced by physical
  // object size) double d0; //< distance at which the dispersion constant is
  // achieved
  double filterperiod; //< filter period time in seconds
  TASCAR::fsplit_t::shape_t shape;
  size_t n_mainchannels;
};

hoa2d_t::hoa2d_t(tsccfg::node_t xmlsrc)
    : TASCAR::receivermod_base_speaker_t(xmlsrc), nbins(spkpos.size() / 2 + 1),
      order(0), amb_order(nbins - 2), s_encoded(1), s_decoded(NULL), dec(NULL),
      fft_scale(1.0),
      // wgain(sqrt(2.0)),
      maxre(false), rotation(0.0), diffup(false), diffup_rot(45 * DEG2RAD),
      diffup_delay(0.01), diffup_maxorder(100), idelay(0), idelaypoint(0),
      filterperiod(0.005)
{
  if(spkpos.size() < 3)
    throw TASCAR::ErrMsg(
        "At least three loudspeakers are required for HOA decoding.");
  for(size_t ch = 0; ch < spkpos.size(); ++ch)
    if(spkpos[ch].z != 0.0)
      throw TASCAR::ErrMsg("The hoa2d receiver requires a flat loudspeaker "
                           "layout on the xy-plane, the z value of speaker " +
                           std::to_string(ch) + " is " +
                           TASCAR::to_string(spkpos[ch].z) + " (elevation " +
                           TASCAR::to_string(RAD2DEG * spkpos[ch].el) +
                           " degree)");
  GET_ATTRIBUTE(order, "", "Ambisonics order; 0: use maximum possible");
  rotation = -spkpos[0].az;
  for(size_t ch = 0; ch < spkpos.size(); ++ch) {
    if(distance(spkpos[ch].unitvector,
                TASCAR::pos_t(cos(-rotation + ch * TASCAR_2PI / spkpos.size()),
                              sin(-rotation + ch * TASCAR_2PI / spkpos.size()), 0.0)) >
       1e-4) {
      double az(RAD2DEG * spkpos[ch].az);
      while(az < 0)
        az += 360.0;
      throw TASCAR::ErrMsg(
          "The hoa2d receiver requires a regular loudspeaker layout. Speaker " +
          std::to_string(ch) + " is at " + TASCAR::to_string(az) +
          " degree, expected " +
          TASCAR::to_string(-rotation * RAD2DEG +
                            ch * 360.0 / spkpos.size()) +
          " degree.");
    }
  }
  GET_ATTRIBUTE_BOOL(maxre,
                     "Use $\\max r_E$ decoder (true) or basic decoder (false)");
  GET_ATTRIBUTE_BOOL(diffup,
                     "Use diffuse upsampling similar to \\citet{Zotter2014}");
  GET_ATTRIBUTE_DEG(diffup_rot, "Decorrelation rotation");
  GET_ATTRIBUTE(diffup_delay, "s", "Decorrelation delay");
  GET_ATTRIBUTE(diffup_maxorder, "", "Maximum order of diffuse sound fields");
  GET_ATTRIBUTE(filterperiod, "s", "Filter period for source width encoding");
  std::string filtershape("none");
  GET_ATTRIBUTE(filtershape, "",
                "De-correlation filter shape for source width encoding, one "
                "of ``none'', ``notch'', ``sine'', ``tria'', ``triald''");
  // GET_ATTRIBUTE_(wgain);
  if(filtershape == "none")
    shape = TASCAR::fsplit_t::none;
  else if(filtershape == "notch")
    shape = TASCAR::fsplit_t::notch;
  else if(filtershape == "sine")
    shape = TASCAR::fsplit_t::sine;
  else if(filtershape == "tria")
    shape = TASCAR::fsplit_t::tria;
  else if(filtershape == "triald")
    shape = TASCAR::fsplit_t::triald;
  else
    throw TASCAR::ErrMsg("Invalid shape: " + filtershape);
  if(order > 0)
    amb_order = std::min((uint32_t)order, amb_order);
  typeidattr.push_back("order");
  typeidattr.push_back("maxre");
  typeidattr.push_back("diffup");
  n_mainchannels = spkpos.size();
}

void hoa2d_t::add_variables(TASCAR::osc_server_t* srv)
{
  TASCAR::receivermod_base_speaker_t::add_variables(srv);
  srv->set_variable_owner("receivermod_hoa2d");
  // srv->add_double( "/wgain", &wgain );
  srv->add_bool("/diffup", &diffup);
  srv->add_double_degree("/diffup_rot", &diffup_rot);
  srv->add_double("/diffup_delay", &diffup_delay);
  srv->add_uint("/diffup_maxorder", &diffup_maxorder);
  srv->unset_variable_owner();
}

hoa2d_t::~hoa2d_t() {}

void hoa2d_t::configure()
{
  TASCAR::receivermod_base_speaker_t::configure();
  update();
  fft_scale = 1.0f / ((float)n_mainchannels);
  s_encoded.resize(n_fragment * nbins);
  s_encoded.clear();
  s_decoded = new float[n_fragment * n_mainchannels];
  int ichannels(n_mainchannels);
  dec = fftwf_plan_many_dft_c2r(
      1, &ichannels, n_fragment, (fftwf_complex*)s_encoded.b, NULL, 1, nbins,
      s_decoded, NULL, 1, n_mainchannels, FFTW_ESTIMATE);
  for(uint32_t k = 0; k < n_fragment * nbins; ++k)
    s_encoded[k] = 0.0f;
  // memset(s_encoded,0,sizeof(std::complex<float>)*n_fragment*nbins);
  memset(s_decoded, 0, sizeof(float) * n_fragment * n_mainchannels);
  ordergain.resize(nbins);
  for(uint32_t m = 0; m <= amb_order; ++m) {
    ordergain[m] = (double)fft_scale * std::exp(-(double)m * i * rotation);
    if(maxre)
      ordergain[m] *= cosf(m * TASCAR_PI2f / (amb_order + 1U));
  }
  idelay = diffup_delay * f_sample;
  idelaypoint = filterperiod * f_sample;
}

void hoa2d_t::release()
{
  TASCAR::receivermod_base_speaker_t::release();
  fftwf_destroy_plan(dec);
  delete[] s_decoded;
}

hoa2d_t::data_t::data_t(uint32_t chunksize, uint32_t channels, double srate,
                        TASCAR::fsplit_t::shape_t shape, double tau)
    : amb_order((channels - 1) / 2), enc_wp(amb_order + 1),
      enc_wm(amb_order + 1), enc_dwp(amb_order + 1), enc_dwm(amb_order + 1),
      wx_1(chunksize), wx_2(chunksize), wy_1(chunksize), wy_2(chunksize),
      fsplitdelay(srate, shape, srate * tau), dx(srate, srate, 340, 0, 0),
      dy(srate, srate, 340, 0, 0)
{
}

hoa2d_t::data_t::~data_t() {}

void hoa2d_t::add_pointsource(const TASCAR::pos_t& prel, double width,
                              const TASCAR::wave_t& chunk,
                              std::vector<TASCAR::wave_t>&,
                              receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  float az(-prel.azim());
  if(shape == TASCAR::fsplit_t::none) {
    std::complex<float> ciazp(std::exp(i_f * az));
    std::complex<float> ckiazp(ciazp);
    for(uint32_t ko = 1; ko <= amb_order; ++ko) {
      d->enc_dwp[ko] = (ordergain[ko] * ckiazp - d->enc_wp[ko]) * (float)t_inc;
      ckiazp *= ciazp;
    }
    d->enc_dwp[0] = 0.0f;
    d->enc_wp[0] = ordergain[0];
    float* vpend(chunk.d + chunk.n);
    std::complex<float>* encp(s_encoded.b);
    for(float* vp = chunk.d; vp != vpend; ++vp) {
      std::complex<float>* pwp(d->enc_wp.b);
      std::complex<float>* pdwp(d->enc_dwp.b);
      for(uint32_t ko = 0; ko <= amb_order; ++ko) {
        *encp += ((*pwp += *pdwp) * (*vp));
        ++encp;
        ++pwp;
        ++pdwp;
      }
      for(uint32_t ko = amb_order + 1; ko < nbins; ++ko)
        ++encp;
    }
  } else {
    std::complex<float> ciazp(std::exp(i * (az + width)));
    std::complex<float> ciazm(std::exp(i * (az - width)));
    std::complex<float> ckiazp(ciazp);
    std::complex<float> ckiazm(ciazm);
    for(uint32_t ko = 1; ko <= amb_order; ++ko) {
      d->enc_dwp[ko] = (ordergain[ko] * ckiazp - d->enc_wp[ko]) * (float)t_inc;
      ckiazp *= ciazp;
      d->enc_dwm[ko] = (ordergain[ko] * ckiazm - d->enc_wm[ko]) * (float)t_inc;
      ckiazm *= ciazm;
    }
    d->enc_dwp[0] = d->enc_dwm[0] = 0.0f;
    d->enc_wp[0] = d->enc_wm[0] = ordergain[0];
    for(uint32_t kt = 0; kt < n_fragment; ++kt) {
      d->fsplitdelay.push(chunk[kt]);
      float vp, vm;
      d->fsplitdelay.get(vp, vm);
      for(uint32_t ko = 0; ko <= amb_order; ++ko)
        s_encoded[kt * nbins + ko] += ((d->enc_wp[ko] += d->enc_dwp[ko]) * vp +
                                       (d->enc_wm[ko] += d->enc_dwm[ko]) * vm);
    }
  }
}

void hoa2d_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  fftwf_execute(dec);
  // copy to output:
  float* p_encode(s_decoded);
  for(uint32_t kt = 0; kt < n_fragment; ++kt) {
    for(uint32_t kch = 0; kch < n_mainchannels; ++kch)
      output[kch][kt] += p_encode[kch];
    p_encode += n_mainchannels;
  }
  //
  s_encoded.clear();
  memset(s_decoded, 0, sizeof(float) * n_fragment * n_mainchannels);
  TASCAR::receivermod_base_speaker_t::postproc(output);
}

void hoa2d_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                                      std::vector<TASCAR::wave_t>& output,
                                      receivermod_base_t::data_t* sd)
{
  if(!diffup) {
    TASCAR::receivermod_base_speaker_t::add_diffuse_sound_field(chunk, output,
                                                                sd);
  } else {
    idelay = diffup_delay * f_sample;
    data_t* d((data_t*)sd);
    // copy first order data:
    float wgain(fft_scale * sqrt(2.0));
    float xyzgain(fft_scale * 0.5);
    if(!diffup) {
      if(maxre)
        xyzgain *= sqrtf(0.5f);
    } else {
      xyzgain *= cosf(TASCAR_PI2f / (amb_order + 1U));
    }
    for(uint32_t kt = 0; kt < n_fragment; ++kt) {
      s_encoded[kt * nbins] += wgain * chunk.w()[kt];
      s_encoded[kt * nbins + 1] +=
          xyzgain * (chunk.x()[kt] + i_f * chunk.y()[kt]);
    }
    std::complex<float> rot_p(std::exp(i * diffup_rot));
    std::complex<float> rot_m(std::exp(-i * diffup_rot));
    // create filtered x and y signals:
    uint32_t n(chunk.size());
    for(uint32_t kt = 0; kt < n; ++kt) {
      // fill delayline:
      float xin(chunk.x()[kt]);
      float yin(chunk.y()[kt]);
      d->dx.push(xin);
      d->dy.push(yin);
      // get delayed value:
      float xdelayed(d->dx.get(idelay));
      float ydelayed(d->dy.get(idelay));
      // create filtered signals:
      std::complex<float> tmp_p(0.5f *
                                ((xin + xdelayed) + i_f * (yin + ydelayed)));
      std::complex<float> tmp_m(0.5f *
                                ((xin - xdelayed) + i_f * (yin - ydelayed)));
      for(uint32_t l = 2; l <= std::min(amb_order, diffup_maxorder); ++l) {
        tmp_p *= rot_p;
        tmp_m *= rot_m;
        s_encoded[kt * nbins + l] += ordergain[l] * (tmp_p + tmp_m);
      }
    }
  }
}

TASCAR::receivermod_base_t::data_t*
hoa2d_t::create_state_data(double srate, uint32_t fragsize) const
{
  return new data_t(fragsize, spkpos.size(), srate, shape, filterperiod);
}

REGISTER_RECEIVERMOD(hoa2d_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
