/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm, Tobias Hegemann
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

#include "receivermod.h"
#include "errorhandling.h"
#include "tascar_os.h"
#include <dlfcn.h>

using namespace TASCAR;

TASCAR_RESOLVER(receivermod_base_t, tsccfg::node_t);

TASCAR::receivermod_t::receivermod_t(tsccfg::node_t cfg)
    : receivermod_base_t(cfg), receivertype("omni"), lib(NULL), libdata(NULL)
{
  get_attribute("type", receivertype, "", "receiver type");
  receivertype = env_expand(receivertype);
  std::string libname("tascarreceiver_");
#ifdef PLUGINPREFIX
  libname = PLUGINPREFIX + libname;
#endif
  libname += receivertype + TASCAR::dynamic_lib_extension();
  lib = dlopen(libname.c_str(), RTLD_NOW);
  if(!lib)
    throw TASCAR::ErrMsg("Unable to open receiver module \"" + receivertype +
                         "\": " + dlerror());
  try {
    receivermod_base_t_resolver(&libdata, cfg, lib, libname);
  }
  catch(...) {
    dlclose(lib);
    throw;
  }
}

void TASCAR::receivermod_t::add_pointsource(const pos_t& prel, double width,
                                            const wave_t& chunk,
                                            std::vector<wave_t>& output,
                                            receivermod_base_t::data_t* data)
{
  libdata->add_pointsource(prel, width, chunk, output, data);
}

void TASCAR::receivermod_t::add_diffuse_sound_field(
    const amb1wave_t& chunk, std::vector<wave_t>& output,
    receivermod_base_t::data_t* data)
{
  libdata->add_diffuse_sound_field(chunk, output, data);
}

void TASCAR::receivermod_t::postproc(std::vector<wave_t>& output)
{
  libdata->postproc(output);
}

void TASCAR::receivermod_t::validate_attributes(std::string& msg) const
{
  receivermod_base_t::validate_attributes(msg);
  libdata->validate_attributes(msg);
}

std::vector<std::string> TASCAR::receivermod_t::get_connections() const
{
  return libdata->get_connections();
}

double TASCAR::receivermod_t::get_delay_comp() const
{
  return libdata->get_delay_comp();
}

void TASCAR::receivermod_t::add_variables(TASCAR::osc_server_t* srv)
{
  return libdata->add_variables(srv);
}

void TASCAR::receivermod_t::configure()
{
  receivermod_base_t::configure();
  libdata->prepare(*this);
}

void TASCAR::receivermod_t::post_prepare()
{
  receivermod_base_t::post_prepare();
  libdata->post_prepare();
}

void TASCAR::receivermod_t::release()
{
  receivermod_base_t::release();
  libdata->release();
}

TASCAR::receivermod_base_t::data_t*
TASCAR::receivermod_t::create_state_data(double srate, uint32_t fragsize) const
{
  return libdata->create_state_data(srate, fragsize);
}

TASCAR::receivermod_base_t::data_t*
TASCAR::receivermod_t::create_diffuse_state_data(double srate, uint32_t fragsize) const
{
  return libdata->create_diffuse_state_data(srate, fragsize);
}

TASCAR::receivermod_t::~receivermod_t()
{
  delete libdata;
  dlclose(lib);
}

TASCAR::receivermod_base_t::receivermod_base_t(tsccfg::node_t xmlsrc)
    : xml_element_t(xmlsrc)
{
}

TASCAR::receivermod_base_t::~receivermod_base_t() {}

std::string spatial_error_t::to_string(const std::string& label,
                                       const std::string& sampling)
{
  return "% error " + sampling + ":\n" + "e." + label +
         ".abs_rV = " + TASCAR::to_string(abs_rV_error) + ";\n" + "e." + label +
         ".abs_rE = " + TASCAR::to_string(abs_rE_error) + ";\n" + "e." + label +
         ".ang_rV = " + TASCAR::to_string(RAD2DEG * angular_rV_error) + ";\n" +
         "e." + label +
         ".ang_rE = " + TASCAR::to_string(RAD2DEG * angular_rE_error) + ";\n" +
         "e." + label +
         ".az_rV = " + TASCAR::to_string(RAD2DEG * azim_rV_error) + ";\n" +
         "e." + label +
         ".az_rE = " + TASCAR::to_string(RAD2DEG * azim_rE_error) + ";\n" +
         "e." + label +
         ".el_rV = " + TASCAR::to_string(RAD2DEG * elev_rV_error) + ";\n" +
         "e." + label +
         ".el_rE = " + TASCAR::to_string(RAD2DEG * elev_rE_error) + ";\n";
}

TASCAR::receivermod_base_speaker_t::receivermod_base_speaker_t(
    tsccfg::node_t xmlsrc)
    : receivermod_base_t(xmlsrc), spkpos(xmlsrc, false), typeidattr({"type"}),
      showspatialerror(false)
{
  GET_ATTRIBUTE_BOOL(
      showspatialerror,
      "show absolute and angular error for rE and rV for 2D and 3D rendering, "
      "given the actual speaker layout and settings");
  GET_ATTRIBUTE(spatialerrorpos, "m",
                "Additional point list in Cartesian coordinates for testing "
                "spatial error");
}

std::string TASCAR::receivermod_base_speaker_t::get_spktypeid() const
{
  std::string r;
  for(auto& ta : typeidattr)
    r += ta + ":" + tsccfg::node_get_attribute_value(e, ta) + ",";
  if(r.size() && (r[r.size() - 1] == ','))
    r.erase(r.size() - 1);
  return r;
}

void TASCAR::receivermod_base_speaker_t::add_variables(
    TASCAR::osc_server_t* srv)
{
  receivermod_base_t::add_variables(srv);
  srv->add_bool("/decorr", &(spkpos.decorr));
  srv->add_bool("/densitycorr", &(spkpos.densitycorr));
}

void TASCAR::receivermod_base_speaker_t::validate_attributes(
    std::string& msg) const
{
  receivermod_base_t::validate_attributes(msg);
  spkpos.validate_attributes(msg);
  spkpos.elayout.validate_attributes(msg);
}

void TASCAR::receivermod_base_speaker_t::add_diffuse_sound_field(
    const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output,
    receivermod_base_t::data_t* sd)
{
  spkpos.add_diffuse_sound_field(chunk);
}

std::vector<std::string>
TASCAR::receivermod_base_speaker_t::get_connections() const
{
  return spkpos.connections;
}

void TASCAR::receivermod_base_speaker_t::postproc(std::vector<wave_t>& output)
{
  // update diffuse signals:
  spkpos.render_diffuse(output);
  // subwoofer post processing:
  if(spkpos.use_subs) {
    if(output.size() != spkpos.subs.size() + spkpos.size())
      throw TASCAR::ErrMsg(
          "Programming error: output.size()==" + std::to_string(output.size()) +
          ", spkpos.size()==" + std::to_string(spkpos.size()) +
          ", subs.size()==" + std::to_string(spkpos.subs.size()));
    // first create raw subwoofer signals:
    for(size_t ksub = 0; ksub < spkpos.subs.size(); ++ksub) {
      // clear sub signals:
      output[ksub + spkpos.size()].clear();
      for(size_t kbroadband = 0; kbroadband < spkpos.size(); ++kbroadband)
        output[ksub + spkpos.size()].add(output[kbroadband],
                                         spkpos.subweight[ksub][kbroadband]);
    }
    // now apply lp-filters to subs:
    for(size_t k = 0; k < spkpos.subs.size(); ++k) {
      spkpos.flt_lowp[k].filter(output[k + spkpos.size()]);
    }
    // then apply hp and allp filters to broad band speakers:
    for(size_t k = 0; k < spkpos.size(); ++k) {
      spkpos.flt_hp[k].filter(output[k]);
      spkpos.flt_allp[k].filter(output[k]);
    }
  }
  // apply calibration:
  if(spkpos.delaycomp.size() != spkpos.size())
    throw TASCAR::ErrMsg("Invalid delay compensation array");
  for(uint32_t k = 0; k < spkpos.size(); ++k) {
    float sgain(spkpos[k].spkgain * spkpos[k].gain);
    for(uint32_t f = 0; f < output[k].n; ++f) {
      output[k].d[f] = sgain * spkpos.delaycomp[k](output[k].d[f]);
    }
    if(spkpos[k].comp)
      spkpos[k].comp->process(output[k], output[k], false);
  }
  // calibration of subs:
  for(uint32_t k = 0; k < spkpos.subs.size(); ++k) {
    float sgain(spkpos.subs[k].spkgain * spkpos.subs[k].gain);
    output[k + spkpos.size()] *= sgain;
    if(spkpos.subs[k].comp)
      spkpos.subs[k].comp->process(output[k + spkpos.size()],
                                   output[k + spkpos.size()], false);
  }
}

void TASCAR::receivermod_base_speaker_t::configure()
{
  receivermod_base_t::configure();
  n_channels = spkpos.size() + spkpos.subs.size();
  spkpos.prepare(cfg());
  labels.clear();
  for(uint32_t ch = 0; ch < n_channels; ++ch) {
    if(ch < spkpos.size())
      labels.push_back("." + TASCAR::to_string(ch) + spkpos[ch].label);
    else
      labels.push_back(".S" + std::to_string(ch - spkpos.size()) +
                       spkpos.subs[ch - spkpos.size()].label);
  }
}

void TASCAR::receivermod_base_speaker_t::release()
{
  receivermod_base_t::release();
  spkpos.release();
}

spatial_error_t TASCAR::receivermod_base_speaker_t::get_spatial_error(
    const std::vector<TASCAR::pos_t>& srcpos)
{
  if(!is_prepared())
    throw TASCAR::ErrMsg("not in configured state. unable to calculate "
                         "get_rE() of an unconfigured receiver.");
  TASCAR::receivermod_base_t::data_t* sd(
      create_state_data(f_sample, n_fragment));
  TASCAR::wave_t ones(n_fragment);
  ones += 1.0f;
  std::vector<wave_t> output(spkpos.size() + spkpos.subs.size(),
                             TASCAR::wave_t(n_fragment));
  spatial_error_t err;
  for(auto& pos : srcpos) {
    for(size_t ch = 0; ch < output.size(); ++ch)
      output[ch].clear();
    add_pointsource(pos, 0.0, ones, output, sd);
    postproc(output);
    for(size_t outch(0); outch < spkpos.size(); ++outch) {
      output[outch] *= 1.0f / spkpos[outch].gain;
    }
    spkpos.clear_states();
    TASCAR::pos_t rE;
    TASCAR::pos_t rV;
    double norm_E(0.0);
    double norm_V(0.0);
    for(size_t outch(0); outch < spkpos.size(); ++outch) {
      double w(output[outch].d[output[outch].n - 1]);
      // calculate rV:
      TASCAR::pos_t pV(spkpos[outch].unitvector);
      pV *= w;
      rV += pV;
      norm_V += w;
      // calculate rE:
      w *= w;
      TASCAR::pos_t pE(spkpos[outch].unitvector);
      pE *= w;
      rE += pE;
      norm_E += w;
    }
    if(norm_E > 0.0)
      rE /= norm_E;
    if(norm_V != 0.0)
      rV /= norm_V;
    err.abs_rV_error += pow(1.0 - rV.norm(), 2.0);
    err.abs_rE_error += pow(1.0 - rE.norm(), 2.0);
    err.angular_rV_error += dot_prod(rV, pos);
    err.angular_rE_error += dot_prod(rE, pos);
    err.azim_rV_error += rV.azim() - pos.azim();
    err.azim_rE_error += rE.azim() - pos.azim();
    err.elev_rV_error += rV.elev() - pos.elev();
    err.elev_rE_error += rE.elev() - pos.elev();
  }
  err.abs_rV_error = sqrt(err.abs_rV_error / srcpos.size());
  err.abs_rE_error = sqrt(err.abs_rE_error / srcpos.size());
  err.angular_rV_error = acos(err.angular_rV_error / srcpos.size());
  err.angular_rE_error = acos(err.angular_rE_error / srcpos.size());
  err.azim_rV_error /= srcpos.size();
  err.azim_rE_error /= srcpos.size();
  err.elev_rV_error /= srcpos.size();
  err.elev_rE_error /= srcpos.size();
  return err;
}

void TASCAR::receivermod_base_speaker_t::post_prepare()
{
  receivermod_base_t::post_prepare();
  spkpos.post_prepare();
  if(showspatialerror) {
    std::vector<TASCAR::pos_t> ring;
    ring.resize(360);
    for(size_t k = 0; k < ring.size(); ++k)
      ring[k].set_sphere(1.0, k * TASCAR_2PI / ring.size(), 0.0);
    spatial_error_t err = get_spatial_error(ring);
    std::cout << "% spatial error:\n";
    std::cout << "e.layout = '" << spkpos.layout << "';\n";
    std::cout << "e.typeid = '" << get_spktypeid() << "';\n";
    std::cout << "e.numchannels = " << spkpos.size() << ";\n";
    std::cout << err.to_string("err2d", "on a ring");
    std::vector<TASCAR::pos_t> sphere(TASCAR::generate_icosahedron());
    sphere = subdivide_and_normalize_mesh(sphere, 5);
    err = get_spatial_error(sphere);
    std::cout << err.to_string("err3d", "on a sphere");
    if(spatialerrorpos.size()) {
      err = get_spatial_error(spatialerrorpos);
      std::cout << err.to_string("user",
                                 "on " + TASCAR::to_string(spatialerrorpos));
    }
  }
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
