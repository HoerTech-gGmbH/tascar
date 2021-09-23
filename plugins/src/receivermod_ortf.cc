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

#include "delayline.h"
#include "receivermod.h"
#include <random>

const std::complex<double> i(0.0, 1.0);

class ortf_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(double srate, uint32_t chunksize, double maxdist, double c,
           uint32_t sincorder, uint32_t sincsampling);
    double fs;
    double dt;
    TASCAR::varidelay_t dline_l;
    TASCAR::varidelay_t dline_r;
    double wl = 0.0;
    double wr = 0.0;
    double itd;
    double state_l;
    double state_r;
  };
  ortf_t(tsccfg::node_t xmlsrc);
  ~ortf_t();
  void add_pointsource(const TASCAR::pos_t& prel, double width,
                       const TASCAR::wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                               std::vector<TASCAR::wave_t>& output,
                               receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,
                                                uint32_t fragsize) const;
  virtual void configure();
  virtual void release();
  virtual void postproc(std::vector<TASCAR::wave_t>& output);
  virtual void add_variables(TASCAR::osc_server_t* srv);

private:
  double distance;
  double angle;
  bool broadband = false;
  double f6db;
  double fmin;
  double scale;
  uint32_t sincorder;
  uint32_t sincsampling = 64;
  double c;
  TASCAR::pos_t dir_l;
  TASCAR::pos_t dir_r;
  TASCAR::pos_t dir_itd;
  double wpow;
  double wmin;
  double decorr_length;
  bool decorr;
  std::vector<TASCAR::overlap_save_t*> decorrflt;
  std::vector<TASCAR::wave_t*> diffuse_render_buffer;
};

ortf_t::~ortf_t() {}

ortf_t::data_t::data_t(double srate, uint32_t chunksize, double maxdist,
                       double c, uint32_t sincorder, uint32_t sincsampling)
    : fs(srate), dt(1.0 / std::max(1.0, (double)chunksize)),
      dline_l(2 * maxdist * srate / c + 2 + sincorder, srate, c, sincorder,
              sincsampling),
      dline_r(2 * maxdist * srate / c + 2 + sincorder, srate, c, sincorder,
              sincsampling),
      wl(0), wr(0), itd(0), state_l(0), state_r(0)
{
}

void ortf_t::configure()
{
  TASCAR::receivermod_base_t::configure();
  wpow = log(exp(-M_PI * f6db / f_sample)) / log(0.5);
  // wmin = pow(exp(-M_PI*fmin/srate),wpow);
  wmin = exp(-M_PI * fmin / f_sample);
  n_channels = 2;
  // initialize decorrelation filter:
  decorrflt.clear();
  diffuse_render_buffer.clear();
  uint32_t irslen(decorr_length * f_sample);
  uint32_t paddedirslen((1 << (int)(ceil(log2(irslen + n_fragment - 1)))) -
                        n_fragment + 1);
  for(uint32_t k = 0; k < 2; ++k)
    decorrflt.push_back(new TASCAR::overlap_save_t(paddedirslen, n_fragment));
  TASCAR::fft_t fft_filter(irslen);
  std::mt19937 gen(1);
  std::uniform_real_distribution<double> dis(0.0, 2 * M_PI);
  for(uint32_t k = 0; k < 2; ++k) {
    for(uint32_t b = 0; b < fft_filter.s.n_; ++b)
      fft_filter.s[b] = std::exp(i * dis(gen));
    fft_filter.ifft();
    for(uint32_t t = 0; t < fft_filter.w.n; ++t)
      fft_filter.w[t] *= (0.5 - 0.5 * cos(t * PI2 / fft_filter.w.n));
    decorrflt[k]->set_irs(fft_filter.w, false);
    diffuse_render_buffer.push_back(new TASCAR::wave_t(n_fragment));
  }
  labels.clear();
  labels.push_back("_l");
  labels.push_back("_r");
  // end of decorrelation filter.
}

void ortf_t::release()
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

void ortf_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  TASCAR::receivermod_base_t::postproc(output);
  for(uint32_t ch = 0; ch < 2; ++ch) {
    if(decorr)
      decorrflt[ch]->process(*(diffuse_render_buffer[ch]), output[ch], true);
    else
      output[ch] += *(diffuse_render_buffer[ch]);
    diffuse_render_buffer[ch]->clear();
  }
}

void ortf_t::add_variables(TASCAR::osc_server_t* srv)
{
  TASCAR::receivermod_base_t::add_variables(srv);
  srv->add_bool("/ortf/decorr", &decorr);
  srv->add_double("/ortf/distance", &distance);
  srv->add_double("/ortf/angle", &angle);
  srv->add_double("/ortf/scale", &scale);
}

ortf_t::ortf_t(tsccfg::node_t xmlsrc)
    : TASCAR::receivermod_base_t(xmlsrc), distance(0.17), angle(110 * DEG2RAD),
      f6db(3000), fmin(800), scale(1.0), sincorder(0), c(340), dir_l(1, 0, 0),
      dir_r(1, 0, 0), dir_itd(0, 1, 0), wpow(1), wmin(EPS), decorr_length(0.05),
      decorr(false)
{
  GET_ATTRIBUTE(distance, "m", "Microphone distance");
  GET_ATTRIBUTE_DEG(angle, "Angular distance between microphone axes");
  GET_ATTRIBUTE(f6db, "Hz", "6 dB cutoff frequency for 90 degrees");
  GET_ATTRIBUTE(fmin, "Hz", "Cutoff frequency for 180 degrees sounds");
  GET_ATTRIBUTE(scale, "", "Scaling factor for cosine attenuation function");
  GET_ATTRIBUTE(sincorder, "", "Sinc interpolation order of ITD delay line");
  GET_ATTRIBUTE(sincsampling, "",
                "Sinc table sampling of ITD delay line, or 0 for no table.");
  GET_ATTRIBUTE(c, "m/s", "Speed of sound");
  GET_ATTRIBUTE(decorr_length, "s", "Decorrelation length");
  GET_ATTRIBUTE_BOOL(decorr, "Flag to use decorrelatin of diffuse sounds");
  GET_ATTRIBUTE_BOOL(broadband, "Use broadband cardioid characteristics");
  dir_l.rot_z(0.5 * angle);
  dir_r.rot_z(-0.5 * angle);
}

void ortf_t::add_pointsource(const TASCAR::pos_t& prel, double width,
                             const TASCAR::wave_t& chunk,
                             std::vector<TASCAR::wave_t>& output,
                             receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  TASCAR::pos_t prel_norm(prel.normal());
  if(broadband) {
    // calculate panning parameters (as incremental values; target_XX is
    // the value reached at end of block):
    double target_wl(0.5 * dot_prod(prel_norm, dir_l) + 0.5);
    double target_wr(0.5 * dot_prod(prel_norm, dir_r) + 0.5);
    double dwl((target_wl - d->wl) * d->dt);
    double dwr((target_wr - d->wr) * d->dt);
    // itd (measured in meter!) is dist*1/2*(cos(az)+1), az is relative to y
    // axis for frontal directions: az=pi/2 -> cos(az)=0 -> itd=0.5*dist
    double target_itd(distance * (0.5 * dot_prod(prel_norm, dir_itd) + 0.5));
    double ditd((target_itd - d->itd) * d->dt);
    // apply panning:
    uint32_t N(chunk.size());
    for(uint32_t k = 0; k < N; ++k) {
      float v(chunk[k]);
      output[0][k] += d->dline_l.get_dist_push(distance - d->itd, v) * d->wl;
      output[1][k] += d->dline_r.get_dist_push(d->itd, v) * d->wr;
      d->wl += dwl;
      d->wr += dwr;
      d->itd += ditd;
    }
    // explicitely apply final values, to avoid rounding errors:
    d->wl = target_wl;
    d->wr = target_wr;
    d->itd = target_itd;
  } else {
    // calculate panning parameters (as incremental values; target_XX is
    // the value reached at end of block):
    double target_wl(pow(
        std::max(0.0, 0.5 - 0.5 * scale * dot_prod(prel_norm, dir_l)), wpow));
    double target_wr(pow(
        std::max(0.0, 0.5 - 0.5 * scale * dot_prod(prel_norm, dir_r)), wpow));
    if(target_wl > wmin)
      target_wl = wmin;
    if(target_wr > wmin)
      target_wr = wmin;
    if(!(target_wl > EPS))
      target_wl = EPS;
    if(!(target_wr > EPS))
      target_wr = EPS;
    // low pass filters for frequency-dependent directionality:
    double dwl((target_wl - d->wl) * d->dt);
    double dwr((target_wr - d->wr) * d->dt);
    // itd (measured in meter!) is dist*1/2*(cos(az)+1), az is relative to y
    // axis for frontal directions: az=pi/2 -> cos(az)=0 -> itd=0.5*dist
    double target_itd(distance * (0.5 * dot_prod(prel_norm, dir_itd) + 0.5));
    double ditd((target_itd - d->itd) * d->dt);
    // apply panning:
    uint32_t N(chunk.size());
    for(uint32_t k = 0; k < N; ++k) {
      float v(chunk[k]);
      output[0][k] +=
          (d->state_l = (d->dline_l.get_dist_push(distance - d->itd, v) *
                             (1.0f - d->wl) +
                         d->state_l * d->wl));
      output[1][k] +=
          (d->state_r = (d->dline_r.get_dist_push(d->itd, v) * (1.0f - d->wr) +
                         d->state_r * d->wr));
      d->wl += dwl;
      d->wr += dwr;
      d->itd += ditd;
    }
    // explicitely apply final values, to avoid rounding errors:
    d->wl = target_wl;
    d->wr = target_wr;
    d->itd = target_itd;
  }
}

void ortf_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                                     std::vector<TASCAR::wave_t>& output,
                                     receivermod_base_t::data_t*)
{
  float* o_l(diffuse_render_buffer[0]->d);
  float* o_r(diffuse_render_buffer[1]->d);
  const float* i_w(chunk.w().d);
  const float* i_x(chunk.x().d);
  const float* i_y(chunk.y().d);
  // decode diffuse sound field in microphone directions:
  for(uint32_t k = 0; k < chunk.size(); ++k) {
    *o_l += *i_w + dir_l.x * (*i_x) + dir_l.y * (*i_y);
    *o_r += *i_w + dir_r.x * (*i_x) + dir_r.y * (*i_y);
    ++o_l;
    ++o_r;
    ++i_w;
    ++i_x;
    ++i_y;
  }
}

TASCAR::receivermod_base_t::data_t*
ortf_t::create_state_data(double srate, uint32_t fragsize) const
{
  return new data_t(srate, fragsize, distance, c, sincorder, sincsampling);
}

REGISTER_RECEIVERMOD(ortf_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
