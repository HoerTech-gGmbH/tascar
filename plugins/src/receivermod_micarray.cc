/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2021 FSchwark, Giso Grimm
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
#include "errorhandling.h"
#include "receivermod.h"

class mic_processor_t;

/**
   A generator of filtercoefficients based on axis and relative position
 */
class filter_model_t : public TASCAR::xml_element_t {
public:
  // filter types, extend as needed:
  enum filtertype_t { equalizer, highshelf };
  filter_model_t(tsccfg::node_t xmlsrc);
  /**
     the model function, called once in each process cycle
     @param flt Reference to a filter to be updated
     @param rel_pos Relative position
  */
  void update_par(TASCAR::biquad_t& flt, const TASCAR::pos_t& rel_pos,
                  double& fs) const;

private:
  // Model parameters, initialized from XML file:
  TASCAR::pos_t axis;
  filtertype_t filtertype;
  // high-shelf parameters
  double theta_st;
  double beta;
  double omega;
  double alpha_st;
  double alpha_m;
  // equalizer parameters
  double theta_end;
  double gain_st;
  double gain_end;
  double omega_st;
  double omega_end;
  double Q;
};

#define CHECKNAN(x)                                                            \
  if(!(x < HUGE_VAL))                                                          \
  throw TASCAR::ErrMsg("No value for \"" #x "\" was given.")

filter_model_t::filter_model_t(tsccfg::node_t xmlsrc)
    : TASCAR::xml_element_t(xmlsrc), axis(0, 0, 0), theta_st(HUGE_VAL),
      beta(HUGE_VAL), omega(HUGE_VAL), alpha_st(HUGE_VAL), alpha_m(HUGE_VAL),
      theta_end(HUGE_VAL), gain_st(HUGE_VAL), gain_end(HUGE_VAL),
      omega_st(HUGE_VAL), omega_end(HUGE_VAL), Q(HUGE_VAL)
{
  GET_ATTRIBUTE(axis, "",
                "orientation axis for filter parameter "
                "variation relative to receiver orientation");
  axis.normalize();
  std::string type;
  GET_ATTRIBUTE(type, "", "filter model type");
  if(type == "equalizer") {
    GET_ATTRIBUTE(theta_end, "rad", "angle until which the gain is varied");
    CHECKNAN(theta_end);
    GET_ATTRIBUTE(gain_st, "dB", "gain applied at theta = 0 rad");
    CHECKNAN(gain_st);
    GET_ATTRIBUTE(gain_end, "dB", "gain applied for all theta >= theta_end");
    CHECKNAN(gain_end);
    GET_ATTRIBUTE(omega_st, "Hz", "center frequency at theta = 0 rad");
    CHECKNAN(omega_st);
    GET_ATTRIBUTE(omega_end, "Hz", "center frequency for theta >= theta_end");
    CHECKNAN(omega_end);
    GET_ATTRIBUTE(Q, "", "quality factor");
    CHECKNAN(Q);
    filtertype = equalizer;
  } else if(type == "highshelf") {
    GET_ATTRIBUTE(theta_st, "rad",
                  "angle at which the zero position starts to vary");
    CHECKNAN(theta_st);
    GET_ATTRIBUTE(beta, "",
                  "parameter to determine angle at which alpha = alpha_m");
    CHECKNAN(beta);
    GET_ATTRIBUTE(omega, "Hz", "cut-off frequency of high-shelf");
    CHECKNAN(omega);
    GET_ATTRIBUTE(alpha_st, "", "alpha for all theta < theta_st");
    CHECKNAN(alpha_st);
    GET_ATTRIBUTE(alpha_m, "", "alpha at theta = beta*(pi-theta_st)");
    CHECKNAN(alpha_m);
    filtertype = highshelf;
  } else
    throw TASCAR::ErrMsg("Invalid filter type \"" + type +
                         "\", must be \"equalizer\" or \"highshelf\".");
}

void filter_model_t::update_par(TASCAR::biquad_t& flt,
                                const TASCAR::pos_t& rel_pos, double& fs) const
{
  switch(filtertype) {
  // filter design of parametric equalizer
  case equalizer: {
    double theta = acos(dot_prod(rel_pos.normal(), axis));
    // gain (dB) variation
    double gain = (cos(std::min(theta / theta_end, 1.0) * M_PI) + 1.0) * 0.5 *
                      (gain_st - gain_end) +
                  gain_end;
    // center frequency variation
    double Omega = (cos(std::min(theta / theta_end, 1.0) * M_PI) + 1.0) * 0.5 *
                       (omega_st - omega_end) +
                   omega_end;
    // bilinear transformation
    double t = 1.0 / tan(M_PI * Omega / fs);
    double t_sq = t * t;
    double Bc = t / Q;
    if(gain < 0.0) {
      double g = pow(10.0, (-gain / 20.0));
      double inv_a0 = 1.0 / (t_sq + 1.0 + g * Bc);
      flt.set_coefficients(
          2.0 * (1.0 - t_sq) * inv_a0, (t_sq + 1.0 - g * Bc) * inv_a0,
          (t_sq + 1.0 + Bc) * inv_a0, 2.0 * (1.0 - t_sq) * inv_a0,
          (t_sq + 1.0 - Bc) * inv_a0);
    } else {
      double g = pow(10.0, (gain / 20.0));
      double inv_a0 = 1.0 / (t_sq + 1.0 + Bc);
      flt.set_coefficients(
          2.0 * (1.0 - t_sq) * inv_a0, (t_sq + 1.0 - Bc) * inv_a0,
          (t_sq + 1.0 + g * Bc) * inv_a0, 2.0 * (1.0 - t_sq) * inv_a0,
          (t_sq + 1.0 - g * Bc) * inv_a0);
    }
    break;
  }
  // filterdesign of high-shelf filter
  case highshelf: {
    double theta = acos(dot_prod(rel_pos.normal(), axis));
    double inv_a0 = 1.0 / (omega + fs);
    if(theta > theta_st) {
      double alpha =
          (alpha_st + alpha_m) * 0.5 +
          (alpha_st - alpha_m) * 0.5 *
              cos((theta - theta_st) / (beta * (M_PI - theta_st)) * M_PI);
      flt.set_coefficients((omega - fs) * inv_a0, 0.0,
                           (omega + alpha * fs) * inv_a0,
                           (omega - alpha * fs) * inv_a0, 0.0);
    } else
      flt.set_coefficients((omega - fs) * inv_a0, 0.0,
                           (omega + alpha_st * fs) * inv_a0,
                           (omega - alpha_st * fs) * inv_a0, 0.0);
    break;
  }
  }
}

/**
   The main class consisting of a delayline and an array of biquad filters
 */
class mic_t : public TASCAR::xml_element_t {
public:
  // delay line model types:
  enum delayline_model_t { freefield, sphere };
  mic_t(tsccfg::node_t xmlsrc, const TASCAR::pos_t& parentposition, double c_);
  ~mic_t();
  void process(TASCAR::wave_t& snd) const;
  size_t get_num_nodes() const;
  void prepare(const chunk_cfg_t&);
  void release();
  void process(const TASCAR::wave_t& input, const TASCAR::pos_t& rel_pos,
               std::vector<mic_processor_t*>& processors,
               std::vector<TASCAR::wave_t>& output, size_t& channelindex,
               double tau_parent);
  void process_diffuse(const TASCAR::amb1wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       size_t& channelindex);
  void add_processors(std::vector<mic_processor_t*>& processors,
                      const chunk_cfg_t& cfg, double delaycorr) const;
  void validate_attributes(std::string&) const;

  TASCAR::pos_t position;
  TASCAR::pos_t position_norm;
  std::vector<filter_model_t> filtermodels;
  // delay line model parameters:
  delayline_model_t delaylinemodel;
  double c;
  double sincorder;
  uint32_t sincsampling = 64;
  double target_tau; // delay w.r.t. parent at the end of chunk
  double maxdist;    // maximal objectsize w.r.t to origin

private:
  std::vector<mic_t*> children;
  std::string name;
  TASCAR::pos_t parentposition; // position of parent microphone
  double tau_parent;            // delay of parent microphone
};

/**
   Signal processor and state class of single mic
 */
class mic_processor_t {
public:
  mic_processor_t(const mic_t* creator, const chunk_cfg_t& cfg,
                  double delaycorr, uint32_t sincsampling);
  ~mic_processor_t();
  void process(const TASCAR::wave_t& input, TASCAR::wave_t& output,
               TASCAR::pos_t rel_pos);
  TASCAR::wave_t sigbuf;
  TASCAR::varidelay_t dline;

private:
  const mic_t* configuration;
  std::vector<TASCAR::biquad_t*> filters;
  double dt;
  double fs;
  double tau;        // delay w.r.t. parent at begin of chunk
  double target_tau; // delay w.r.t. parent at the end of chunk
  double dtau;       // delay increment
};

mic_processor_t::mic_processor_t(const mic_t* creator, const chunk_cfg_t& cfg,
                                 double delaycorr, uint32_t sincsampling)
    : sigbuf(cfg.n_fragment),
      dline(2 * delaycorr * cfg.f_sample + 2 * creator->sincorder, cfg.f_sample,
            creator->c, creator->sincorder, sincsampling),
      configuration(creator), dt(1.0 / std::max(1.0, (double)cfg.n_fragment)),
      fs(cfg.f_sample)
{
  for(size_t k = 0; k < creator->filtermodels.size(); ++k)
    filters.push_back(new TASCAR::biquad_t());
}

mic_processor_t::~mic_processor_t()
{
  for(auto filter : filters)
    delete filter;
}

void mic_processor_t::process(const TASCAR::wave_t& input,
                              TASCAR::wave_t& output, TASCAR::pos_t rel_pos)
{
  // copy signal:
  sigbuf.copy(input);
  // apply biquads:
  size_t kflt(0);
  if(filters.size() != configuration->filtermodels.size()) {
    throw TASCAR::ErrMsg(std::string(__FILE__) + ":" +
                         std::to_string(__LINE__) +
                         ": Programming error: number of filters differs from "
                         "number of filter models.");
  }
  for(auto filter : filters) {
    configuration->filtermodels[kflt].update_par(*filter, rel_pos, fs);
    filter->filter(sigbuf);
    ++kflt;
  }
  // delayline:
  target_tau = configuration->target_tau;
  dtau = (target_tau - tau) * dt;
  uint32_t N(sigbuf.size());
  for(uint32_t k = 0; k < N; ++k) {
    float in(sigbuf[k]);
    output[k] += dline.get_dist_push(tau, in);
    tau += dtau;
  }
  tau = target_tau;
}

void mic_t::process(const TASCAR::wave_t& input, const TASCAR::pos_t& rel_pos,
                    std::vector<mic_processor_t*>& processors,
                    std::vector<TASCAR::wave_t>& output, size_t& channelindex,
                    double tau_parent)
{
  size_t thisindex(channelindex);
  // relative delay w.r.t. parent
  TASCAR::pos_t pos(rel_pos);
  TASCAR::pos_t axis = position - parentposition;
  pos -= parentposition;
  double cos_theta = dot_prod(pos.normal(), axis.normal());
  double theta = acos(cos_theta);
  if(delaylinemodel == sphere) {
    if(theta < M_PI * 0.5)
      target_tau = -axis.norm() * cos_theta;
    else
      target_tau = axis.norm() * (theta - M_PI * 0.5);
  } else if(delaylinemodel == freefield)
    target_tau = -axis.norm() * cos_theta;
  target_tau += tau_parent;
  // filtering and delay line
  processors[thisindex]->process(input, output[thisindex], rel_pos);
  for(auto child : children) {
    ++channelindex;
    child->process(processors[thisindex]->sigbuf, rel_pos, processors, output,
                   channelindex, target_tau);
  }
}

void mic_t::process_diffuse(const TASCAR::amb1wave_t& chunk,
                            std::vector<TASCAR::wave_t>& output,
                            size_t& channelindex)
{
  // this algoithm will put a cardioid pattern in axis of microphone relative to
  // origin:
  float* o_l(output[channelindex].d);
  const float* i_w(chunk.w().d);
  const float* i_x(chunk.x().d);
  const float* i_y(chunk.y().d);
  const float* i_z(chunk.z().d);
  for(uint32_t k = 0; k < chunk.size(); ++k) {
    *o_l += *i_w + *i_x * position_norm.x + *i_y * position_norm.y +
            *i_z * position_norm.z;
    ++o_l;
    ++i_w;
    ++i_x;
    ++i_y;
    ++i_z;
  }
  for(auto child : children) {
    ++channelindex;
    child->process_diffuse(chunk, output, channelindex);
  }
}

void mic_t::add_processors(std::vector<mic_processor_t*>& processors,
                           const chunk_cfg_t& cfg, double delaycorr) const
{
  processors.push_back(new mic_processor_t(this, cfg, delaycorr, sincsampling));
  for(auto child : children)
    child->add_processors(processors, cfg, delaycorr);
}

mic_t::mic_t(tsccfg::node_t xmlsrc, const TASCAR::pos_t& parentposition_,
             double c_)
    : TASCAR::xml_element_t(xmlsrc), c(c_), sincorder(0), target_tau(0.0),
      maxdist(0.0), parentposition(parentposition_), tau_parent(0.0)
{
  GET_ATTRIBUTE(name, "", "microphone label");
  GET_ATTRIBUTE(position, "m",
                "microphone position relative to receiver origin");
  // normalized position is needed for diffuse rendering:
  position_norm = position;
  position_norm.normalize();
  std::string delay("freefield");
  GET_ATTRIBUTE(delay, "",
                "delay line model type, \"freefield\" or \"sphere\"");
  GET_ATTRIBUTE(sincorder, "", "Sinc interpolation order of delay line");
  GET_ATTRIBUTE(sincsampling, "",
                "Sampling of sinc table, or 0 for direct calculation");
  if(delay == "freefield")
    delaylinemodel = freefield;
  else if(delay == "sphere")
    delaylinemodel = sphere;
  else
    throw TASCAR::ErrMsg("Invalid delay line model \"" + delay + "\".");
  for(auto subelem : tsccfg::node_get_children(xmlsrc)) {
    if(subelem) {
      if(tsccfg::node_get_name(subelem) == "mic") {
        children.emplace_back(new mic_t(subelem, position, c));
      } else if(tsccfg::node_get_name(subelem) == "filter") {
        filtermodels.emplace_back(filter_model_t(subelem));
      } else {
        TASCAR::add_warning("invalid element in mic definition: " +
                            tsccfg::node_get_name(subelem));
      }
    }
  }
  for(auto child : children)
    maxdist =
        std::max(std::max(maxdist, child->position.norm()), child->maxdist);
}

mic_t::~mic_t()
{
  for(auto child : children)
    delete child;
}

size_t mic_t::get_num_nodes() const
{
  if(children.empty())
    return 1;
  size_t nodes(1);
  for(auto child : children)
    nodes += child->get_num_nodes();
  return nodes;
}

class mic_vars_t {
public:
  mic_vars_t(tsccfg::node_t cfg);
  double c = 340.0;
};

mic_vars_t::mic_vars_t(tsccfg::node_t cfg)
{
  TASCAR::xml_element_t e(cfg);
  e.GET_ATTRIBUTE(c, "m/s", "speed of sound");
}

/**
  This plugin implements a receiver consisting of a hierarchical
  structure of microphones realted by relative transfer functions
  (RTFs). Each RTF consisting of a delay model and a filter model.
 */
class micarray_t : public mic_vars_t, public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(const mic_t& creator, const chunk_cfg_t& cfg, double delaycorr);
    ~data_t();
    std::vector<mic_processor_t*> processors;
  };
  micarray_t(tsccfg::node_t xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width,
                       const TASCAR::wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                               std::vector<TASCAR::wave_t>& output,
                               receivermod_base_t::data_t*);
  void configure();
  void add_variables(TASCAR::osc_server_t* srv);
  receivermod_base_t::data_t* create_state_data(double srate,
                                                uint32_t fragsize) const;
  void validate_attributes(std::string&) const;
  double get_delay_comp() const;

private:
  mic_t origin;
};

micarray_t::data_t::data_t(const mic_t& creator, const chunk_cfg_t& cfg,
                           double delaycorr)
{
  creator.add_processors(processors, cfg, delaycorr);
}

micarray_t::data_t::~data_t()
{
  for(auto proc : processors)
    delete proc;
}

micarray_t::micarray_t(tsccfg::node_t xmlsrc)
    : mic_vars_t(xmlsrc), TASCAR::receivermod_base_t(xmlsrc),
      origin(find_or_add_child("mic"), TASCAR::pos_t(), c)
{
}

double micarray_t::get_delay_comp() const
{
  return origin.maxdist * 0.5 * M_PI / c;
  // maximal possible delay due to sphere delay model
}

void micarray_t::add_variables(TASCAR::osc_server_t* srv) {}

void micarray_t::add_pointsource(const TASCAR::pos_t& prel, double width,
                                 const TASCAR::wave_t& chunk,
                                 std::vector<TASCAR::wave_t>& output,
                                 receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  // process data:
  size_t channelindex(0);
  origin.process(chunk, prel, d->processors, output, channelindex,
                 get_delay_comp() * c);
}

void micarray_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                                         std::vector<TASCAR::wave_t>& output,
                                         receivermod_base_t::data_t*)
{
  size_t channelindex(0);
  origin.process_diffuse(chunk, output, channelindex);
}

void micarray_t::configure()
{
  // number of output channels
  n_channels = origin.get_num_nodes();
}

TASCAR::receivermod_base_t::data_t*
micarray_t::create_state_data(double srate, uint32_t fragsize) const
{
  if(!is_prepared())
    throw TASCAR::ErrMsg(std::string(__FILE__) + ":" +
                         std::to_string(__LINE__) +
                         ": creating data from an unprepared configuration.");
  return new data_t(origin, cfg(), get_delay_comp());
}

void micarray_t::validate_attributes(std::string& msg) const
{
  TASCAR::xml_element_t::validate_attributes(msg);
  origin.validate_attributes(msg);
}

void mic_t::validate_attributes(std::string& msg) const
{
  TASCAR::xml_element_t::validate_attributes(msg);
  for(auto child : children)
    child->validate_attributes(msg);
  for(auto flt : filtermodels)
    flt.validate_attributes(msg);
}

REGISTER_RECEIVERMOD(micarray_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
