/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
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

#include "amb33defs.h"
#include "errorhandling.h"
#include "receivermod.h"

//#include "fft.h"
#include "fdn.h"
#include "optim.h"
#include <limits>

class simplefdn_vars_t : public TASCAR::receivermod_base_t {
public:
  simplefdn_vars_t(tsccfg::node_t xmlsrc);
  ~simplefdn_vars_t();
  // protected:
  uint32_t fdnorder = 5;
  float w = 0.0;
  float dw = 60.0;
  float t60 = 0.0;
  float damping = 0.3f;
  bool prefilt = true;
  uint32_t forwardstages = 0;
  bool logdelays = true;
  float absorption = 0.6f;
  float c = 340.0f;
  bool fixcirculantmat = false;
  TASCAR::pos_t volumetric;
  TASCAR::fdn_t::gainmethod_t gm;
  std::vector<float> vcf;
  std::vector<float> vt60;
  uint32_t numiter = 100;
  float lowcut = 0;
  TASCAR::biquadf_t lowcut_w;
  TASCAR::biquadf_t lowcut_x;
  TASCAR::biquadf_t lowcut_y;
  TASCAR::biquadf_t lowcut_z;
  bool use_lowcut = false;
  bool truncate_forward = false;
};

simplefdn_vars_t::simplefdn_vars_t(tsccfg::node_t xmlsrc)
    : receivermod_base_t(xmlsrc)
{
  GET_ATTRIBUTE(fdnorder, "", "Order of FDN (number of recursive paths)");
  GET_ATTRIBUTE(dw, "rounds/s", "Spatial spread of rotation");
  GET_ATTRIBUTE(t60, "s", "T60, or zero to use Sabine's equation");
  GET_ATTRIBUTE(damping, "",
                "Damping (first order lowpass) coefficient to control spectral "
                "tilt of T60");
  GET_ATTRIBUTE_BOOL(prefilt,
                     "Apply additional filter before inserting audio into FDN");
  GET_ATTRIBUTE(forwardstages, "", "Number of feed forward stages");
  GET_ATTRIBUTE(absorption, "", "Absorption used in Sabine's equation");
  GET_ATTRIBUTE(c, "m/s", "Speed of sound");
  GET_ATTRIBUTE(volumetric, "m", "Dimension of room x y z");
  GET_ATTRIBUTE_BOOL(
      fixcirculantmat,
      "Apply fix to correctly initialize circulant feedback matrix");
  GET_ATTRIBUTE(
      vcf, "Hz",
      "Center frequencies for T60 optimization, or empty for no optimization");
  GET_ATTRIBUTE(vt60, "s", "T60 at specified center frequencies");
  if(vcf.size() != vt60.size())
    throw TASCAR::ErrMsg("Mismatching number of entries in vcf and vt60.");
  GET_ATTRIBUTE(numiter, "", "Number of iterations in T60 optimization");
  std::string gainmethod("original");
  GET_ATTRIBUTE(gainmethod, "original mean schroeder",
                "Gain calculation method");
  if(gainmethod == "original")
    gm = TASCAR::fdn_t::original;
  else if(gainmethod == "mean")
    gm = TASCAR::fdn_t::mean;
  else if(gainmethod == "schroeder")
    gm = TASCAR::fdn_t::schroeder;
  else
    throw TASCAR::ErrMsg(
        "Invalid gain method \"" + gainmethod +
        "\". Possible values are original, mean or schroeder.");
  GET_ATTRIBUTE(lowcut, "Hz", "low cut off frequency, or zero for no low cut");
  GET_ATTRIBUTE_BOOL(truncate_forward, "Truncate delays of feed forward path");
}

simplefdn_vars_t::~simplefdn_vars_t() {}

class simplefdn_t : public simplefdn_vars_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    // ambisonic weights:
    float _w[AMB11ACN::idx::channels];
    float w_current[AMB11ACN::idx::channels];
    float dw[AMB11ACN::idx::channels];
    float dt;
  };
  simplefdn_t(tsccfg::node_t xmlsrc);
  ~simplefdn_t();
  inline void fdnfilter(TASCAR::foa_sample_t& x);
  void postproc(std::vector<TASCAR::wave_t>& output);
  void add_pointsource(const TASCAR::pos_t& prel, double width,
                       const TASCAR::wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t&,
                               std::vector<TASCAR::wave_t>&,
                               receivermod_base_t::data_t*){};
  void update_par();
  void setlogdelays(bool ld);
  void configure();
  // void optim_param();
  void release();
  void get_ir(TASCAR::wave_t& w);
  receivermod_base_t::data_t* create_state_data(double srate,
                                                uint32_t fragsize) const;
  virtual void add_variables(TASCAR::osc_server_t* srv);
  static int osc_set_dim_damp_absorption(const char* path, const char* types,
                                         lo_arg** argv, int argc,
                                         lo_message msg, void* user_data);
  static int osc_fixcirculantmat(const char* path, const char* types,
                                 lo_arg** argv, int argc, lo_message msg,
                                 void* user_data);
  /**
     @brief Get T60 at octave bands around provided center frequencies
     @param cf list of center frequencies in Hz
     @retval t60 T60 in seconds
  */
  void get_t60(const std::vector<float>& cf, std::vector<float>& t60);
  float t60err(const std::vector<float>& param);
  float slopeerr(const std::vector<float>& param);

private:
  TASCAR::fdn_t* feedback_delay_network = NULL;
  std::vector<TASCAR::fdnpath_t> srcpath;
  std::vector<TASCAR::fdn_t*> feedforward_delay_network;
  TASCAR::amb1wave_t* foa_out = NULL;
  pthread_mutex_t mtx;
  float wgain = MIN3DB;
  float distcorr = 1.0f;
  TASCAR::wave_t* ir_bb = NULL;
  TASCAR::wave_t* ir_band = NULL;
};

int simplefdn_t::osc_fixcirculantmat(const char*, const char* types,
                                     lo_arg** argv, int argc, lo_message,
                                     void* user_data)
{
  if((1 == argc) && (types[0] == 'i')) {
    ((simplefdn_t*)user_data)->fixcirculantmat = (argv[0]->i > 0);
    ((simplefdn_t*)user_data)->update_par();
  }
  return 0;
}

int simplefdn_t::osc_set_dim_damp_absorption(const char*, const char* types,
                                             lo_arg** argv, int argc,
                                             lo_message, void* user_data)
{
  if((5 == argc) && (types[0] == 'f') && (types[1] == 'f') &&
     (types[2] == 'f') && (types[3] == 'f') && (types[4] == 'f')) {
    ((simplefdn_t*)user_data)->volumetric.x = argv[0]->f;
    ((simplefdn_t*)user_data)->volumetric.y = argv[1]->f;
    ((simplefdn_t*)user_data)->volumetric.z = argv[2]->f;
    ((simplefdn_t*)user_data)->damping = argv[3]->f;
    ((simplefdn_t*)user_data)->absorption = argv[4]->f;
    ((simplefdn_t*)user_data)->t60 = 0.0;
    ((simplefdn_t*)user_data)->update_par();
  }
  return 0;
}

simplefdn_t::simplefdn_t(tsccfg::node_t cfg) : simplefdn_vars_t(cfg)
{
  if(t60 <= 0.0)
    t60 =
        0.161f * volumetric.boxvolumef() / (absorption * volumetric.boxareaf());
  pthread_mutex_init(&mtx, NULL);
}

static float t60err_(const std::vector<float>& param, void* data)
{
  return ((simplefdn_t*)data)->t60err(param);
}

static float slopeerr_(const std::vector<float>& param, void* data)
{
  return ((simplefdn_t*)data)->slopeerr(param);
}

float simplefdn_t::t60err(const std::vector<float>& param)
{
  if(param.empty())
    throw TASCAR::ErrMsg("Invalid (empty) parameter space");
  absorption = std::max(0.0f, std::min(1.0f, param[0]));
  t60 = 0.0f;
  update_par();
  std::vector<float> xt60;
  get_t60(vcf, xt60);
  float t60max = 0.0f;
  float t60max_ref = 0.0f;
  for(size_t k = 0; k < std::min(xt60.size(), vt60.size()); ++k) {
    t60max = std::max(t60max, xt60[k]);
    t60max_ref = std::max(t60max_ref, vt60[k]);
  }
  float le = fabs(t60max / t60max_ref - 1.0f);
  le *= le;
  return le;
}

float simplefdn_t::slopeerr(const std::vector<float>& param)
{
  if(param.empty())
    throw TASCAR::ErrMsg("Invalid (empty) parameter space");
  damping = std::max(0.0f, std::min(0.999f, param[0]));
  update_par();
  std::vector<float> xt60;
  get_t60(vcf, xt60);
  float slope = 0.0f;
  for(size_t k = 1; k < std::min(xt60.size(), vt60.size()); ++k)
    slope += (xt60[k] - xt60[0]) / (logf(vcf[k]) - logf(vcf[0]));
  float slope_ref = 0.0f;
  for(size_t k = 1; k < std::min(xt60.size(), vt60.size()); ++k)
    slope_ref += (vt60[k] - vt60[0]) / (logf(vcf[k]) - logf(vcf[0]));
  float le = fabs(slope / slope_ref - 1.0f);
  le *= le;
  return le;
}

void simplefdn_t::configure()
{
  receivermod_base_t::configure();
  n_channels = AMB11ACN::idx::channels;
  if(lowcut > 0) {
    use_lowcut = true;
    lowcut_w.set_butterworth(lowcut, f_sample, true);
    lowcut_x.set_butterworth(lowcut, f_sample, true);
    lowcut_y.set_butterworth(lowcut, f_sample, true);
    lowcut_z.set_butterworth(lowcut, f_sample, true);
  } else {
    use_lowcut = false;
  }
  if(feedback_delay_network)
    delete feedback_delay_network;
  srcpath.resize(fdnorder);
  feedback_delay_network =
      new TASCAR::fdn_t(fdnorder, (uint32_t)f_sample, logdelays, gm, true);
  for(auto& pff : feedforward_delay_network)
    delete pff;
  feedforward_delay_network.clear();
  for(uint32_t k = 0; k < forwardstages; ++k)
    feedforward_delay_network.push_back(
        new TASCAR::fdn_t(fdnorder, (uint32_t)f_sample, logdelays, gm, false));
  if(foa_out)
    delete foa_out;
  foa_out = new TASCAR::amb1wave_t(n_fragment);
  labels.clear();
  for(uint32_t ch = 0; ch < n_channels; ++ch) {
    char ctmp[32];
    ctmp[31] = 0;
    snprintf(ctmp, 31, ".%d%c", (ch > 0), AMB11ACN::channelorder[ch]);
    labels.push_back(ctmp);
  }
  // prepare impulse response container for T60 measurements:
  size_t irlen((size_t)f_sample);
  float maxdim((float)volumetric.x);
  maxdim = std::max(maxdim, (float)volumetric.y);
  maxdim = std::max(maxdim, (float)volumetric.z);
  size_t irlendim((size_t)(10.0f * maxdim / c * (float)f_sample));
  irlen = std::max(irlen, irlendim);
  ir_bb = new TASCAR::wave_t((uint32_t)irlen);
  ir_band = new TASCAR::wave_t((uint32_t)irlen);
  if(vcf.size() > 0) {
    // optimize damping and absorption to match given T60
    // first optimize absorption
    t60 = 0.0f;
    damping = 0.2f;
    absorption =
        0.161f * volumetric.boxvolumef() / (vt60[0] * volumetric.boxareaf());
    float eps = 1.0f;
    float lasterr = 10000.0f;
    std::vector<float> param = {absorption};
    for(size_t it = 0; it < numiter; ++it) {
      float err = downhill_iterate(eps, param, t60err_, this, {1e-3f});
      param[0] = fabs(param[0]);
      if((err < 0.00005) || (fabsf(lasterr / err - 1.0f) < 1e-9f))
        it = numiter;
      lasterr = err;
    }
    absorption = fabsf(param[0]);
    param = {(float)damping};
    lasterr = 10000.0f;
    for(size_t it = 0; it < numiter; ++it) {
      float err = downhill_iterate(eps, param, slopeerr_, this, {2e-3f});
      param[0] = std::min(0.99f, fabs(param[0]));
      if(err > 4.0f * lasterr)
        eps *= 0.5f;
      if((err < 0.00005f) || (fabsf(lasterr / err - 1.0f) < 1e-9f))
        it = numiter;
      lasterr = err;
    }
    damping = param[0];
    t60 = 0.0f;
    update_par();
    std::vector<float> xt60;
    get_t60(vcf, xt60);
    std::cout << "Optimization of T60:\n  absorption=\"" << absorption
              << "\" damping=\"" << damping << "\" t60=\"0\"\n";
    for(size_t k = 0; k < vcf.size(); ++k) {
      std::cout << "  " << vcf[k] << " Hz: " << xt60[k] << " s\n";
    }
    std::cout << "  (T60_sab = " << t60 << "s)\n";
    t60 = 0.0f;
  }
  update_par();
}

void simplefdn_t::release()
{
  TASCAR::receivermod_base_t::release();
  delete ir_bb;
  delete ir_band;
}

void simplefdn_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv->set_variable_owner(
      TASCAR::strrep(TASCAR::tscbasename(__FILE__), ".cc", ""));
  srv->add_method("/fixcirculantmat", "i", &osc_fixcirculantmat, this, true,
                  false, "bool",
                  "Fix a neglegible bug in the feedback matrix design");
  srv->add_method(
      "/dim_damp_absorption", "fffff", &osc_set_dim_damp_absorption, this, true,
      false, "",
      "Set dimension (x,y,z in m), damping and absorption coefficient");
  srv->unset_variable_owner();
}

simplefdn_t::~simplefdn_t()
{
  if(feedback_delay_network)
    delete feedback_delay_network;
  for(auto& pff : feedforward_delay_network)
    delete pff;
  if(foa_out)
    delete foa_out;
  pthread_mutex_destroy(&mtx);
}

void simplefdn_t::update_par()
{
  if(pthread_mutex_lock(&mtx) == 0) {
    distcorr = 1.0f / (0.5f * powf((float)volumetric.x * (float)volumetric.y *
                                       (float)volumetric.z,
                                   0.33333f));
    float tmin(std::min((float)volumetric.x,
                        std::min((float)volumetric.y, (float)volumetric.z)) /
               c);
    float tmax(std::max((float)volumetric.x,
                        std::max((float)volumetric.y, (float)volumetric.z)) /
               c);
    if(t60 <= 0.0)
      t60 = 0.161f * volumetric.boxvolumef() /
            (absorption * volumetric.boxareaf());
    if(feedback_delay_network) {
      float wscale(TASCAR_2PIf * tmin);
      feedback_delay_network->setpar_t60(
          wscale * w, wscale * dw, (float)f_sample * tmin,
          (float)f_sample * tmax, (float)f_sample * t60,
          std::max(0.0f, std::min(0.999f, damping)), fixcirculantmat,
          truncate_forward);
      for(auto& pff : feedforward_delay_network)
        pff->setpar_t60(wscale * w, wscale * dw, (float)f_sample * tmin,
                        (float)f_sample * tmax, (float)f_sample * t60,
                        std::max(0.0f, std::min(0.999f, damping)),
                        fixcirculantmat, truncate_forward);
    }
    pthread_mutex_unlock(&mtx);
  }
}

void simplefdn_t::setlogdelays(bool ld)
{
  if(pthread_mutex_lock(&mtx) == 0) {
    if(feedback_delay_network) {
      feedback_delay_network->set_logdelays(ld);
      for(auto& pff : feedforward_delay_network)
        pff->set_logdelays(ld);
      float tmin(std::min((float)volumetric.x,
                          std::min((float)volumetric.y, (float)volumetric.z)) /
                 c);
      float tmax(std::max((float)volumetric.x,
                          std::max((float)volumetric.y, (float)volumetric.z)) /
                 c);
      if(t60 <= 0.0f)
        t60 = 0.161f * volumetric.boxvolumef() /
              (absorption * volumetric.boxareaf());
      float wscale(TASCAR_2PIf * tmin);
      feedback_delay_network->setpar_t60(
          wscale * w, wscale * dw, (float)f_sample * tmin,
          (float)f_sample * tmax, (float)f_sample * t60,
          std::max(0.0f, std::min(0.999f, damping)), fixcirculantmat,
          truncate_forward);
      for(auto& pff : feedforward_delay_network)
        pff->setpar_t60(wscale * w, wscale * dw, (float)f_sample * tmin,
                        (float)f_sample * tmax, (float)f_sample * t60,
                        std::max(0.0f, std::min(0.999f, damping)),
                        fixcirculantmat, truncate_forward);
    }
    pthread_mutex_unlock(&mtx);
  }
}

void simplefdn_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  // correct for volumetric gain:
  if(pthread_mutex_trylock(&mtx) == 0) {
    *foa_out *= distcorr;
    if(feedback_delay_network) {
      if(use_lowcut) {
        lowcut_w.filter(foa_out->w());
        lowcut_x.filter(foa_out->x());
        lowcut_y.filter(foa_out->y());
        lowcut_z.filter(foa_out->z());
      }
      for(uint32_t t = 0; t < n_fragment; ++t) {
        TASCAR::foa_sample_t x(foa_out->w()[t], foa_out->x()[t],
                               foa_out->y()[t], foa_out->z()[t]);
        fdnfilter(x);

        output[AMB11ACN::idx::w][t] += feedback_delay_network->outval.w;
        output[AMB11ACN::idx::x][t] += feedback_delay_network->outval.x;
        output[AMB11ACN::idx::y][t] += feedback_delay_network->outval.y;
        output[AMB11ACN::idx::z][t] += feedback_delay_network->outval.z;
      }
    }
    foa_out->clear();
    pthread_mutex_unlock(&mtx);
  }
}

void simplefdn_t::fdnfilter(TASCAR::foa_sample_t& x)
{
  if(prefilt) {
    feedback_delay_network->prefilt0.filter(x);
    feedback_delay_network->prefilt1.filter(x);
  }
  auto psrc = &srcpath;
  for(auto& path : srcpath)
    path.dlout = x;
  for(auto& pff : feedforward_delay_network) {
    pff->process(*psrc);
    psrc = &(pff->fdnpath);
  }
  feedback_delay_network->process(*psrc);
  x = feedback_delay_network->outval;
}

float ir_get_t60(TASCAR::wave_t& ir, float fs)
{
  if(ir.n < 2)
    return -1;
  // max threshold is -0.2 dB below max:
  float thmax = powf(10.0f, -0.1f * 10.2f);
  float thmin = powf(10.0f, -0.1f * 30.2f);
  float cs = 0.0f;
  for(size_t k = ir.n - 1; k != 0; --k) {
    cs += ir.d[k] * ir.d[k];
    ir.d[k] = cs;
  }
  thmax *= cs;
  thmin *= cs;
  size_t idx_max = 0;
  size_t idx_min = 0;
  for(size_t k = 0; k < ir.n; ++k) {
    if(ir.d[k] > thmax)
      idx_max = k;
    if(ir.d[k] > thmin)
      idx_min = k;
  }
  if(idx_min > idx_max) {
    cs = (-60.0f / (10.0f * log10f(ir.d[idx_min] / ir.d[idx_max]) * fs)) *
         (float)(idx_min - idx_max);
    return cs;
  }
  return -1.0f;
}

void simplefdn_t::get_t60(const std::vector<float>& cf, std::vector<float>& t60)
{
  if(pthread_mutex_trylock(&mtx) == 0) {
    if(feedback_delay_network) {
      t60.clear();
      get_ir(*ir_bb);
      TASCAR::bandpass_t bp(176.78f, 353.55f, f_sample);
      for(auto f : cf) {
        bp.set_range(f * sqrtf(0.5f), f * sqrtf(2.0f));
        ir_band->copy(*ir_bb);
        bp.filter(*ir_band);
        bp.clear();
        bp.filter(*ir_band);
        bp.clear();
        bp.filter(*ir_band);
        bp.clear();
        bp.filter(*ir_band);
        t60.push_back(ir_get_t60(*ir_band, (float)f_sample));
      }
    }
    pthread_mutex_unlock(&mtx);
  }
}

void simplefdn_t::get_ir(TASCAR::wave_t& ir)
{
  if(feedback_delay_network) {
    for(auto& pff : feedforward_delay_network)
      pff->set_zero();
    feedback_delay_network->set_zero();
    TASCAR::foa_sample_t inval;
    inval.w = 1.0f;
    inval.x = 1.0f;
    inval.y = 0.0f;
    inval.z = 0.0f;
    for(size_t k = 0u; k < ir.n; ++k) {
      fdnfilter(inval);
      ir.d[k] = inval.w;
      inval.set_zero();
    }
    for(auto& pff : feedforward_delay_network)
      pff->set_zero();
    feedback_delay_network->set_zero();
  }
}

TASCAR::receivermod_base_t::data_t*
simplefdn_t::create_state_data(double, uint32_t fragsize) const
{
  return new data_t(fragsize);
}

/**
 * @brief add input signal to ambisonics signal container
 *
 * Use panning, using source direction
 */
void simplefdn_t::add_pointsource(const TASCAR::pos_t& prel, double,
                                  const TASCAR::wave_t& chunk,
                                  std::vector<TASCAR::wave_t>&,
                                  receivermod_base_t::data_t* sd)
{
  TASCAR::pos_t pnorm(prel.normal());
  data_t* d((data_t*)sd);
  // use ACN everywhere:
  d->_w[AMB11ACN::idx::w] = wgain;
  d->_w[AMB11ACN::idx::x] = (float)pnorm.x;
  d->_w[AMB11ACN::idx::y] = (float)pnorm.y;
  d->_w[AMB11ACN::idx::z] = (float)pnorm.z;
  for(uint32_t acn = 0; acn < AMB11ACN::idx::channels; ++acn)
    d->dw[acn] = (d->_w[acn] - d->w_current[acn]) * d->dt;
  for(uint32_t acn = 0; acn < AMB11ACN::idx::channels; ++acn)
    for(uint32_t i = 0; i < chunk.size(); i++)
      (*foa_out)[acn][i] += (d->w_current[acn] += d->dw[acn]) * chunk[i];
  for(uint32_t acn = 0; acn < AMB11ACN::idx::channels; ++acn)
    d->w_current[acn] = d->_w[acn];
}

simplefdn_t::data_t::data_t(uint32_t chunksize)
{
  for(uint32_t k = 0; k < AMB11ACN::idx::channels; k++)
    _w[k] = w_current[k] = dw[k] = 0;
  dt = 1.0f / std::max(1.0f, (float)chunksize);
}

REGISTER_RECEIVERMOD(simplefdn_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
