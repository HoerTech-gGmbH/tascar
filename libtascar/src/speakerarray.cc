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

#include "speakerarray.h"
#include "errorhandling.h"
#include "tascar_os.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <random>
#include <string.h>

using namespace TASCAR;

const std::complex<double> i(0.0, 1.0);

spk_array_cfg_t::spk_array_cfg_t(tsccfg::node_t xmlsrc, bool use_parent_xml)
    : xml_element_t(xmlsrc), doc(NULL), e_layout(NULL)
{
  if(use_parent_xml) {
    e_layout = xmlsrc;
  } else {
    GET_ATTRIBUTE(layout, "", "name of speaker layout file");
    if(layout.empty()) {
      // try to find layout element:
      for(auto& sne : tsccfg::node_get_children(xmlsrc, "layout"))
        e_layout = sne;
      if(e_layout == NULL)
        throw TASCAR::ErrMsg(
            "No layout file provided and no inline layout xml element.");
    } else {
      doc = new TASCAR::xml_doc_t(TASCAR::env_expand(layout),
                                  TASCAR::xml_doc_t::LOAD_FILE);
      try {
        e_layout = doc->root();
        if(!e_layout)
          throw TASCAR::ErrMsg("No root node found in document \"" + layout +
                               "\".");
        if(tsccfg::node_get_name(e_layout) != "layout")
          throw TASCAR::ErrMsg(
              "Invalid root node name. Expected \"layout\", got " +
              tsccfg::node_get_name(e_layout) + ".");
      }
      catch(...) {
        delete doc;
        throw;
      }
    }
  }
}

// speaker array:
spk_array_t::spk_array_t(tsccfg::node_t e, bool use_parent_xml,
                         const std::string& elementname_, bool allow_empty)
    : spk_array_cfg_t(e, use_parent_xml), elayout(e_layout), rmax(0), rmin(0),
      xyzgain(1.0), elementname(elementname_), mean_rotation(0)
{
  clear();
  for(auto& sn : tsccfg::node_get_children(e_layout, elementname))
    emplace_back(sn);
  elayout.GET_ATTRIBUTE(xyzgain, "", "XYZ-gain for FOA decoding");
  elayout.GET_ATTRIBUTE(name, "", "Name of layout, for documentation only");
  elayout.GET_ATTRIBUTE(onload, "",
                        "system command to be executed when layout is loaded");
  elayout.GET_ATTRIBUTE(
      onunload, "", "system command to be executed when layout is unloaded");
  //
  if(empty() && (!allow_empty))
    throw TASCAR::ErrMsg("Invalid " + elementname_ + " array (no " +
                         elementname_ + " elements defined).");
  // update derived parameters:
  if(!empty()) {
    rmax = operator[](0).norm();
    rmin = rmax;
    for(uint32_t k = 1; k < size(); k++) {
      double r(operator[](k).norm());
      if(rmax < r)
        rmax = r;
      if(rmin > r)
        rmin = r;
    }
  }
  std::complex<double> p_xy(0);
  for(uint32_t k = 0; k < size(); k++) {
    operator[](k).spkgain *= operator[](k).norm() / rmax;
    operator[](k).dr = rmax - operator[](k).norm();
    p_xy += std::exp(-(double)k * TASCAR_2PI * i / (double)(size())) *
            (operator[](k).unitvector.x + i * operator[](k).unitvector.y);
  }
  mean_rotation = std::arg(p_xy);
  didx.resize(size());
  for(uint32_t k = 0; k < size(); ++k) {
    operator[](k).update_foa_decoder(1.0f / size(), xyzgain);
    connections.push_back(operator[](k).connect);
    float w(1);
    for(uint32_t l = 0; l < size(); ++l) {
      if(l != k) {
        float d(dot_prod(operator[](k).unitvector, operator[](l).unitvector));
        if(d > 0)
          w += d;
      }
    }
    operator[](k).densityweight = size() / w;
  }
  if(!empty()) {
    float dwmean(0);
    for(uint32_t k = 0; k < size(); ++k)
      dwmean += operator[](k).densityweight;
    dwmean /= size();
    for(uint32_t k = 0; k < size(); ++k)
      operator[](k).densityweight /= dwmean;
  }
  if(!onload.empty()) {
    int err(::system(onload.c_str()));
    if(err != 0)
      std::cerr << "subprocess \"" << onload << "\" returned " << err
                << std::endl;
  }
}

spk_array_t::~spk_array_t()
{
  if(!onunload.empty()) {
    int err(::system(onunload.c_str()));
    if(err != 0)
      std::cerr << "subprocess \"" << onunload << "\" returned " << err
                << std::endl;
  }
}

double spk_descriptor_t::get_rel_azim(double az_src) const
{
  return std::arg(std::exp(i * (az_src - az)));
}

double spk_descriptor_t::get_cos_adist(pos_t src_unit) const
{
  return dot_prod(src_unit, unitvector);
}

bool sort_didx(const spk_array_t::didx_t& a, const spk_array_t::didx_t& b)
{
  return (a.d > b.d);
}

const std::vector<spk_array_t::didx_t>&
spk_array_t::sort_distance(const pos_t& psrc)
{
  for(uint32_t k = 0; k < size(); ++k) {
    didx[k].idx = k;
    didx[k].d = dot_prod(psrc, operator[](k).unitvector);
  }
  std::sort(didx.begin(), didx.end(), sort_didx);
  return didx;
}

void spk_array_diff_render_t::render_diffuse(
    std::vector<TASCAR::wave_t>& output)
{
  uint32_t channels(size());
  if(output.size() < channels)
    throw TASCAR::ErrMsg("Invalid size of speaker array");
  if(!diffuse_field_accumulator)
    throw TASCAR::ErrMsg("No diffuse field accumulator allocated.");
  if(!diffuse_render_buffer)
    throw TASCAR::ErrMsg("No diffuse field render buffer allocated.");
  uint32_t N(diffuse_field_accumulator->size());
  for(uint32_t ch = 0; ch < channels; ++ch) {
    for(uint32_t t = 0; t < N; ++t) {
      (*diffuse_render_buffer)[t] = operator[](ch).d_w *
                                    diffuse_field_accumulator->w()[t];
      (*diffuse_render_buffer)[t] += operator[](ch).d_x *
                                     diffuse_field_accumulator->x()[t];
      (*diffuse_render_buffer)[t] += operator[](ch).d_y *
                                     diffuse_field_accumulator->y()[t];
      (*diffuse_render_buffer)[t] += operator[](ch).d_z *
                                     diffuse_field_accumulator->z()[t];
    }
    if(densitycorr)
      *diffuse_render_buffer *= operator[](ch).densityweight;
    if(decorr && decorrflt.size())
      decorrflt[ch].process(*diffuse_render_buffer, output[ch], true);
    else
      output[ch] += *diffuse_render_buffer;
  }
  diffuse_field_accumulator->clear();
}

spk_descriptor_t::spk_descriptor_t(tsccfg::node_t xmlsrc)
    : xml_element_t(xmlsrc), az(0.0), el(0.0), r(1.0), delay(0.0), gain(1.0),
      spkgain(1.0), dr(0.0), d_w(0.0f), d_x(0.0f), d_y(0.0f), d_z(0.0f),
      densityweight(1.0), comp(NULL)
{
  GET_ATTRIBUTE_DEG(az, "Azimuth");
  GET_ATTRIBUTE_DEG(el, "Elevation");
  GET_ATTRIBUTE(r, "m", "Distance");
  GET_ATTRIBUTE(delay, "s", "Static delay");
  GET_ATTRIBUTE(label, "", "Additional port label");
  GET_ATTRIBUTE(connect, "", "Connection to jack port");
  GET_ATTRIBUTE(compB, "", "FIR filter coefficients for speaker calibration");
  GET_ATTRIBUTE_DB(gain, "Broadband gain correction");
  set_sphere(r, az, el);
  unitvector = normal();
  update_foa_decoder(1.0f, 1.0);
}

spk_descriptor_t::spk_descriptor_t(const spk_descriptor_t& src)
    : xml_element_t(src), pos_t(src), az(src.az), el(src.el), r(src.r),
      delay(src.delay), label(src.label), connect(src.connect),
      compB(src.compB), gain(src.gain), unitvector(src.unitvector),
      spkgain(src.spkgain), dr(src.dr), d_w(src.d_w), d_x(src.d_x),
      d_y(src.d_y), d_z(src.d_z), densityweight(src.densityweight), comp(NULL)
{
}

spk_descriptor_t::~spk_descriptor_t()
{
  if(comp)
    delete comp;
}

void spk_descriptor_t::update_foa_decoder(float gain, double xyzgain)
{
  // update of FOA decoder matrix:
  d_w = 1.4142135623730951455f * gain;
  gain *= 2.0f * xyzgain;
  d_x = unitvector.x * gain;
  d_y = unitvector.y * gain;
  d_z = unitvector.z * gain;
}

void spk_array_t::validate_attributes(std::string& msg) const
{
  spk_array_cfg_t::validate_attributes(msg);
  elayout.validate_attributes(msg);
  for(auto it = begin(); it != end(); ++it)
    it->validate_attributes(msg);
}

void spk_array_t::configure()
{
  n_channels = size();
  delaycomp.clear();
  for(uint32_t k = 0; k < size(); ++k)
    delaycomp.emplace_back(f_sample *
                           ((operator[](k).dr / 340.0) + operator[](k).delay));
  // speaker correction filter:
  for(uint32_t k = 0; k < size(); ++k) {
    spk_descriptor_t& spk(operator[](k));
    if(spk.compB.size() > 0) {
      spk.comp = new TASCAR::overlap_save_t(n_fragment + 1, n_fragment);
      spk.comp->set_irs(spk.compB, false);
    }
  }
}

void spk_array_diff_render_t::configure()
{
  spk_array_t::configure();
  n_channels = size() + subs.size();
  // initialize decorrelation filter:
  decorrflt.clear();
  uint32_t irslen(decorr_length * f_sample);
  uint32_t paddedirslen((1 << (int)(ceil(log2(irslen + n_fragment - 1)))) -
                        n_fragment + 1);
  if(irslen > 0) {
    for(uint32_t k = 0; k < size(); ++k)
      decorrflt.push_back(TASCAR::overlap_save_t(paddedirslen, n_fragment));
    TASCAR::fft_t fft_filter(irslen);
    std::mt19937 gen(1);
    std::uniform_real_distribution<double> dis(0.0, TASCAR_2PI);
    // std::exponential_distribution<double> dis(1.0);
    for(uint32_t k = 0; k < size(); ++k) {
      for(uint32_t b = 0u; b < fft_filter.s.n_; ++b)
        fft_filter.s[b] = std::exp(i * dis(gen));
      fft_filter.ifft();
      for(uint32_t t = 0; t < fft_filter.w.n; ++t)
        fft_filter.w[t] *=
            (0.5f - 0.5f * cosf(t * TASCAR_2PIf / fft_filter.w.n));
      decorrflt[k].set_irs(fft_filter.w, false);
    }
  }
  // end of decorrelation filter.
  if(diffuse_field_accumulator)
    delete diffuse_field_accumulator;
  diffuse_field_accumulator = NULL;
  diffuse_field_accumulator = new TASCAR::amb1wave_t(n_fragment);
  if(diffuse_render_buffer)
    delete diffuse_render_buffer;
  diffuse_render_buffer = NULL;
  diffuse_render_buffer = new TASCAR::wave_t(n_fragment);
  if(use_subs) {
    const double fscale(sqrt(0.5));
    flt_hp.resize(size());
    flt_allp.resize(size());
    flt_lowp.resize(subs.size());
    // configure high pass filter:
    for(auto& flt : flt_hp)
      flt.set_highpass(fscale * fcsub, f_sample);
    // configure low pass filter:
    for(auto& flt : flt_lowp)
      flt.set_lowpass(fscale * fcsub, f_sample, true);
    // configure all pass filter:
    const double f0(0.125 * fscale * fcsub / f_sample * TASCAR_2PI);
    biquad_t fallp;
    const double r(1.01);
    fallp.set_gzp(1.0, 1, -f0, 1.0 / r, f0);
    const double g(std::abs(fallp.response(TASCAR_PI)));
    for(auto& flt : flt_allp)
      flt.set_gzp(1.0 / g, 1, -f0, 1.0 / r, f0);
  }
  use_conv = false;
  for(const auto& conv : convolution_ir)
    use_conv |= !conv.empty();
  for(auto& vp_convolver : vvp_convolver)
    for(auto& p_convolver : vp_convolver)
      delete p_convolver;
  vvp_convolver.clear();
  if(use_conv) {
    size_t k(0);
    for(const auto& conv : convolution_ir) {
      if(conv.empty())
        throw TASCAR::ErrMsg(
            "No convolution IR specified for speaker channel " +
            std::to_string(k) + " (" + operator[](k).label + ").");
      ++k;
    }
    conv_channels = 0u;
    k = 0;
    for(const auto& conv : convolution_ir) {
      TASCAR::sndfile_handle_t sfinfo(conv);
      if(conv_channels == 0u)
        conv_channels = sfinfo.get_channels();
      if(sfinfo.get_channels() != conv_channels)
        throw TASCAR::ErrMsg(
            "Invalid number of channels in convolution IR for channel " +
            std::to_string(k) + ": got " +
            std::to_string(sfinfo.get_channels()) + ", expected " +
            std::to_string(conv_channels) + ".");
      if(sfinfo.get_srate() != f_sample) {
        std::ostringstream msg;
        msg << "Warning: The sample rate of convolution IR file \"" << conv
            << "\" (" << sfinfo.get_srate()
            << ") differs from system sample rate (" << f_sample << ").";
        TASCAR::add_warning(msg.str(), operator[](k).e);
      }
      ++k;
    }
    k = 0;
    for(const auto& conv : convolution_ir) {
      std::vector<TASCAR::partitioned_conv_t*> vp_convolver;
      for(size_t ch = 0u; ch < conv_channels; ++ch) {
        TASCAR::sndfile_t sndf(conv, ch);
        TASCAR::partitioned_conv_t* pcnv(
            new TASCAR::partitioned_conv_t(sndf.get_frames(), n_fragment));
        pcnv->set_irs(sndf);
        vp_convolver.push_back(pcnv);
      }
      vvp_convolver.push_back(vp_convolver);
      ++k;
    }
  }
  n_channels += conv_channels;
}

void spk_array_diff_render_t::add_diffuse_sound_field(
    const TASCAR::amb1wave_t& diff)
{
  if(!diffuse_field_accumulator)
    throw TASCAR::ErrMsg("No diffuse field accumulator allocated.");
  *diffuse_field_accumulator += diff;
  has_diffuse = true;
}

spk_array_diff_render_t::~spk_array_diff_render_t()
{
  if(diffuse_field_accumulator)
    delete diffuse_field_accumulator;
  if(diffuse_render_buffer)
    delete diffuse_render_buffer;
  for(auto& vp_convolver : vvp_convolver)
    for(auto& p_convolver : vp_convolver)
      delete p_convolver;
}

uint32_t TASCAR::get_spklayout_checksum(const xml_element_t& e)
{
  std::vector<std::string> attributes;
  attributes.push_back("decorr_length");
  attributes.push_back("decorr");
  attributes.push_back("densitycorr");
  attributes.push_back("caliblevel");
  attributes.push_back("diffusegain");
  attributes.push_back("gain");
  attributes.push_back("az");
  attributes.push_back("el");
  attributes.push_back("r");
  attributes.push_back("calibfor");
  attributes.push_back("fcsub");
  attributes.push_back("delay");
  attributes.push_back("compB");
  attributes.push_back("connect");
  return e.hash(attributes, true);
}

spk_array_diff_render_t::spk_array_diff_render_t(
    tsccfg::node_t e, bool use_parent_xml, const std::string& elementname_)
    : spk_array_t(e, use_parent_xml, elementname_),
      subs(e, use_parent_xml, "sub", true), diffuse_field_accumulator(NULL),
      diffuse_render_buffer(NULL), decorr_length(0.05), decorr(true),
      densitycorr(true), fcsub(80), caliblevel(1), diffusegain(1.0),
      calibage(0), use_subs(false)
{
  // get speaker convolution file names:
  convolution_ir.clear();
  for(auto& spk : *this) {
    convolution_ir.push_back("");
    spk.get_attribute("conv", convolution_ir.back(), "",
                      "Name of impulse response for convolution");
  }
  // validation of calibration:
  uint64_t checksum(0);
  elayout.GET_ATTRIBUTE(checksum, "",
                        "autogenerated value for validation of calibration");
  if((checksum != 0) && (layout.size() > 0)) {
    uint64_t current_checksum(get_spklayout_checksum(elayout));
    if(checksum != current_checksum)
      TASCAR::add_warning("The layout file \"" + layout +
                          "\" was modified since last calibration. "
                          "Re-calibration is recommended.");
  }
  elayout.GET_ATTRIBUTE(decorr_length, "s", "Length of decorrelation filter");
  elayout.GET_ATTRIBUTE_BOOL(
      decorr, "Decorrelate speaker signals in diffuse sound field rendering");
  elayout.GET_ATTRIBUTE_BOOL(
      densitycorr,
      "In diffuse rendering, correct gains locally for loudspeaker density");
  elayout.GET_ATTRIBUTE(
      fcsub, "Hz", "Cross-over frequency, used only if subwoofers are defined");
  has_caliblevel = elayout.has_attribute("caliblevel");
  elayout.GET_ATTRIBUTE_DBSPL(caliblevel, "Calibration level");
  has_diffusegain = elayout.has_attribute("diffusegain");
  elayout.GET_ATTRIBUTE_DB(diffusegain,
                           "Calibration gain of diffuse sound fields");
  has_calibdate = elayout.has_attribute("calibdate");
  elayout.GET_ATTRIBUTE(calibdate, "", "Calibration date in format YYYY-MM-DD");
  if(!calibdate.empty()) {
    std::time_t now(
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    std::tm tcalib;
    memset(&tcalib, 0, sizeof(tcalib));
    const char* msg(
        TASCAR::strptime(calibdate.c_str(), "%Y-%m-%d %H:%M:%S", &tcalib));
    if(!msg)
      msg = TASCAR::strptime(calibdate.c_str(), "%Y-%m-%d", &tcalib);
    if(msg) {
      std::time_t ctcalib(mktime(&tcalib));
      calibage = difftime(now, ctcalib) / (24 * 3600);
    } else {
      TASCAR::add_warning("Invalid date/time format: " + calibdate);
    }
  }
  has_calibfor = elayout.has_attribute("calibfor");
  elayout.GET_ATTRIBUTE(calibfor, "", "Summary of receiver parameters");
  elayout.GET_ATTRIBUTE(
      convlabels, "",
      "Space-separated list of labels of convolution output channels");
  elayout.GET_ATTRIBUTE_BOOL(
      convprecalib,
      "Apply convolution before calibration (true) or after (false).");
  if(!subs.empty()) {
    use_subs = true;
  }
  subweight.resize(subs.size());
  for(auto& sub : subweight)
    sub.resize(size());
  for(size_t kspk = 0; kspk < size(); ++kspk) {
    double wsum(0);
    for(size_t ksub = 0; ksub < subs.size(); ++ksub) {
      subweight[ksub][kspk] =
          0.01 / (0.01 + pow(distance(operator[](kspk).unitvector,
                                      subs[ksub].unitvector),
                             2.0));
      wsum += subweight[ksub][kspk];
    }
    for(size_t ksub = 0; ksub < subs.size(); ++ksub) {
      subweight[ksub][kspk] /= wsum;
    }
  }
  for(auto sub : subs)
    connections.push_back(sub.connect);
}

std::vector<TASCAR::pos_t> spk_array_t::get_positions() const
{
  std::vector<TASCAR::pos_t> pos;
  for(auto it = begin(); it != end(); ++it)
    pos.push_back(*it);
  return pos;
}

void spk_array_diff_render_t::clear_states()
{
  // reset all subwoofer filters:
  for(auto& flt : flt_lowp)
    flt.clear();
  for(auto& flt : flt_hp)
    flt.clear();
  for(auto& flt : flt_allp)
    flt.clear();
  for(auto& flt : decorrflt)
    flt.clear();
  has_diffuse = false;
}

void spk_array_diff_render_t::postproc(std::vector<wave_t>& output)
{
  // update diffuse signals:
  if(has_diffuse) {
    render_diffuse(output);
    has_diffuse = false;
  }
  if(output.size() != num_output_channels())
    throw TASCAR::ErrMsg(
        "Programming error: output.size()==" + std::to_string(output.size()) +
        ", spkpos.size()==" + std::to_string(size()) +
        ", subs.size()==" + std::to_string(subs.size()) +
        ", conv_channels==" + std::to_string(conv_channels));
  // subwoofer post processing:
  if(use_subs) {
    // first create raw subwoofer signals:
    for(size_t ksub = 0; ksub < subs.size(); ++ksub) {
      // clear sub signals:
      output[ksub + size()].clear();
      for(size_t kbroadband = 0; kbroadband < size(); ++kbroadband)
        output[ksub + size()].add(output[kbroadband],
                                  subweight[ksub][kbroadband]);
    }
    // now apply lp-filters to subs:
    for(size_t k = 0; k < subs.size(); ++k) {
      flt_lowp[k].filter(output[k + size()]);
    }
    // then apply hp and allp filters to broad band speakers:
    for(size_t k = 0; k < size(); ++k) {
      flt_hp[k].filter(output[k]);
      flt_allp[k].filter(output[k]);
    }
  }
  // convolution
  if(use_conv && convprecalib) {
    size_t choffset(size() + subs.size());
    // clear outputs:
    for(size_t ch = 0; ch < conv_channels; ++ch)
      output[choffset + ch].clear();
    for(size_t inchannel = 0; inchannel < vvp_convolver.size(); ++inchannel)
      for(size_t outchannel = 0; outchannel < conv_channels; ++outchannel) {
        vvp_convolver[inchannel][outchannel]->process(
            output[inchannel], output[choffset + outchannel], true);
      }
  }
  // apply calibration:
  if(delaycomp.size() != size())
    throw TASCAR::ErrMsg("Invalid delay compensation array");
  for(uint32_t k = 0; k < size(); ++k) {
    float sgain(operator[](k).spkgain * operator[](k).gain);
    for(uint32_t f = 0; f < output[k].n; ++f) {
      output[k].d[f] = sgain * delaycomp[k](output[k].d[f]);
    }
    if(operator[](k).comp)
      operator[](k).comp->process(output[k], output[k], false);
  }
  // calibration of subs:
  for(uint32_t k = 0; k < subs.size(); ++k) {
    float sgain(subs[k].spkgain * subs[k].gain);
    output[k + size()] *= sgain;
    if(subs[k].comp)
      subs[k].comp->process(output[k + size()], output[k + size()], false);
  }
  // convolution
  if(use_conv && (!convprecalib)) {
    size_t choffset(size() + subs.size());
    // clear outputs:
    for(size_t ch = 0; ch < conv_channels; ++ch)
      output[choffset + ch].clear();
    for(size_t inchannel = 0; inchannel < vvp_convolver.size(); ++inchannel)
      for(size_t outchannel = 0; outchannel < conv_channels; ++outchannel) {
        vvp_convolver[inchannel][outchannel]->process(
            output[inchannel], output[choffset + outchannel], true);
      }
  }
}

std::string spk_array_diff_render_t::get_label(size_t ch) const
{
  if(ch < size())
    return operator[](ch).label;
  if(ch < size() + subs.size())
    return subs[ch - size()].label;
  if(ch < size() + subs.size() + convlabels.size())
    return convlabels[ch - (size() + subs.size())];
  return "";
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
