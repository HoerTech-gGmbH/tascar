/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2022 Giso Grimm
 */
/**
 * @file calibsession.cc
 * @brief Speaker calibration class
 * @ingroup libtascar
 * @author Giso Grimm
 * @date 2022
 *
 * @section license License (GPL)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "calibsession.h"
#include "fft.h"
#include "jackiowav.h"
#include <unistd.h>

using namespace TASCAR;

std::string get_calibfor(const std::string& fname)
{
  TASCAR::xml_doc_t doc(fname, TASCAR::xml_doc_t::LOAD_FILE);
  return doc.root.get_attribute("calibfor");
}

std::vector<std::string> string_token(std::string s,
                                      const std::string& delimiter)
{
  std::vector<std::string> rv;
  size_t pos = 0;
  std::string token;
  while((pos = s.find(delimiter)) != std::string::npos) {
    token = s.substr(0, pos);
    rv.push_back(token);
    s.erase(0, pos + delimiter.length());
  }
  rv.push_back(s);
  return rv;
}

calibsession_t::calibsession_t(const std::string& fname, double reflevel,
                               const std::vector<std::string>& refport,
                               double duration_, double fmin, double fmax,
                               double subduration_, double subfmin,
                               double subfmax)
    : session_t("<?xml version=\"1.0\"?><session srv_port=\"none\"/>",
                LOAD_STRING, ""),
      gainmodified(false), levelsrecorded(false), calibrated(false),
      calibrated_diff(false), startlevel(0), startdiffgain(0), delta(0),
      delta_diff(0), spkname(fname), spkarray(NULL), refport_(refport),
      duration(duration_), subduration(subduration_), lmin(0), lmax(0),
      lmean(0), fmin_((float)fmin), fmax_((float)fmax),
      subfmin_((float)subfmin), subfmax_((float)subfmax),
      calibfor(get_calibfor(fname)), jackrec(refport_.size() + 1, "spkcalibrec")
{
  for(size_t ich = 0; ich < refport_.size() + 1; ++ich) {
    bbrecbuf.push_back((uint32_t)(jackrec.get_srate() * duration));
    subrecbuf.push_back((uint32_t)(jackrec.get_srate() * subduration));
  }
  if(calibfor.empty())
    calibfor = "type:nsp";
  // create a new session, no OSC port:
  root.set_attribute("srv_port", "none");
  // add the calibration scene:
  xml_element_t e_scene(root.add_child("scene"));
  e_scene.set_attribute("name", "calib");
  // add a point source for broadband stimulus, muted for now:
  xml_element_t e_src(e_scene.add_child("source"));
  e_src.set_attribute("mute", "true");
  xml_element_t e_snd(e_src.add_child("sound"));
  xml_element_t e_plugs(e_snd.add_child("plugins"));
  // add pink noise generator:
  xml_element_t e_pink(e_plugs.add_child("pink"));
  e_pink.set_attribute("level", TASCAR::to_string(reflevel));
  e_pink.set_attribute("period", TASCAR::to_string(duration));
  e_pink.set_attribute("fmin", TASCAR::to_string(fmin));
  e_pink.set_attribute("fmax", TASCAR::to_string(fmax));
  // add a point source for subwoofer stimulus, muted for now:
  xml_element_t e_subsrc(e_scene.add_child("source"));
  e_subsrc.set_attribute("name", "srcsub");
  e_subsrc.set_attribute("mute", "true");
  xml_element_t e_subsnd(e_subsrc.add_child("sound"));
  xml_element_t e_subplugs(e_subsnd.add_child("plugins"));
  // add pink noise generator:
  xml_element_t e_subpink(e_subplugs.add_child("pink"));
  e_subpink.set_attribute("level", TASCAR::to_string(reflevel));
  e_subpink.set_attribute("period", TASCAR::to_string(subduration));
  e_subpink.set_attribute("fmin", TASCAR::to_string(subfmin));
  e_subpink.set_attribute("fmax", TASCAR::to_string(subfmax));
  // receiver 1 is always nsp, for speaker level differences:
  xml_element_t e_rcvr(e_scene.add_child("receiver"));
  e_rcvr.set_attribute("type", "nsp");
  e_rcvr.set_attribute("layout", fname);
  // receiver 2 is specific to the layout, for overall calibration:
  xml_element_t e_rcvr2(e_scene.add_child("receiver"));
  e_rcvr2.set_attribute("name", "out2");
  e_rcvr2.set_attribute("mute", "true");
  e_rcvr2.set_attribute("layout", fname);
  // receiver 3 is omni, for reference signal:
  xml_element_t e_rcvr3(e_scene.add_child("receiver"));
  e_rcvr3.set_attribute("type", "omni");
  e_rcvr3.set_attribute("name", "ref");
  std::vector<std::string> receivertypeattr(string_token(calibfor, ","));
  for(auto typeattr : receivertypeattr) {
    std::vector<std::string> pair(string_token(typeattr, ":"));
    if(pair.size() != 2)
      throw TASCAR::ErrMsg(
          "Invalid format of 'calibfor' attribute '" + calibfor +
          "': Expected comma separated list of name:value pairs.");
    e_rcvr2.set_attribute(pair[0], pair[1]);
  }
  // add diffuse source for diffuse gain calibration:
  xml_element_t e_diff(e_scene.add_child("diffuse"));
  e_diff.set_attribute("mute", "true");
  xml_element_t e_plugs_diff(e_diff.add_child("plugins"));
  xml_element_t e_pink_diff(e_plugs_diff.add_child("pink"));
  e_pink_diff.set_attribute("level", TASCAR::to_string(reflevel));
  e_pink_diff.set_attribute("period", TASCAR::to_string(duration));
  e_pink_diff.set_attribute("fmin", TASCAR::to_string(fmin));
  e_pink_diff.set_attribute("fmax", TASCAR::to_string(fmax));
  // extra routes:
  xml_element_t e_mods(root.add_child("modules"));
  xml_element_t e_route_pink(e_mods.add_child("route"));
  e_route_pink.set_attribute("name", "pink");
  e_route_pink.set_attribute("channels", "1");
  xml_element_t e_route_pink_plugs(e_route_pink.add_child("plugins"));
  xml_element_t e_route_pink_pink(e_route_pink_plugs.add_child("pink"));
  e_route_pink_pink.set_attribute("level", "50");
  e_route_pink_pink.set_attribute("period", TASCAR::to_string(duration));
  e_route_pink_pink.set_attribute("fmin", TASCAR::to_string(fmin));
  e_route_pink_pink.set_attribute("fmax", TASCAR::to_string(fmax));
  xml_element_t e_route_sub(e_mods.add_child("route"));
  e_route_sub.set_attribute("name", "sub");
  e_route_sub.set_attribute("channels", "1");
  xml_element_t e_route_sub_plugs(e_route_sub.add_child("plugins"));
  xml_element_t e_route_sub_pink(e_route_sub_plugs.add_child("pink"));
  e_route_sub_pink.set_attribute("level", "50");
  e_route_sub_pink.set_attribute("period", TASCAR::to_string(subduration));
  e_route_sub_pink.set_attribute("fmin", TASCAR::to_string(subfmin));
  e_route_sub_pink.set_attribute("fmax", TASCAR::to_string(subfmax));
  // end of scene creation.
  // doc->write_to_file_formatted("temp.cfg");
  add_scene(e_scene.e);
  add_module(e_route_pink.e);
  add_module(e_route_sub.e);
  startlevel = get_caliblevel();
  startdiffgain = get_diffusegain();
  spkarray = new spk_array_diff_render_t(e_rcvr.e, false);
  levels = std::vector<float>(spkarray->size(), 0.0);
  sublevels = std::vector<float>(spkarray->subs.size(), 0.0);
  levelsfrg = std::vector<float>(spkarray->size(), 0.0);
  sublevelsfrg = std::vector<float>(spkarray->subs.size(), 0.0);
  // validate scene:
  if(scenes.empty())
    throw TASCAR::ErrMsg("Programming error: no scene");
  if(scenes[0]->source_objects.size() != 2)
    throw TASCAR::ErrMsg("Programming error: not exactly two sources.");
  if(scenes[0]->receivermod_objects.size() != 3)
    throw TASCAR::ErrMsg("Programming error: not exactly three receivers.");
  scenes.back()->source_objects[0]->dlocation = pos_t(1, 0, 0);
}

calibsession_t::~calibsession_t()
{
  delete spkarray;
}

void calibsession_t::reset_levels()
{
  levelsrecorded = false;
  for(auto recobj : scenes.back()->receivermod_objects) {
    TASCAR::receivermod_base_speaker_t* recspk(
        dynamic_cast<TASCAR::receivermod_base_speaker_t*>(recobj->libdata));
    if(recspk) {
      for(uint32_t k = 0; k < levels.size(); ++k)
        recspk->spkpos[k].gain = 1.0;
      for(uint32_t k = 0; k < sublevels.size(); ++k)
        recspk->spkpos.subs[k].gain = 1.0;
    }
  }
}

void get_levels_(spk_array_t& spks, TASCAR::Scene::src_object_t& src,
                 double prewait, jackrec2wave_t& jackrec,
                 const std::vector<TASCAR::wave_t>& recbuf,
                 const std::vector<std::string>& ports, float duration,
                 levelmeter::weight_t weight, float fmin, float fmax,
                 std::vector<float>& levels, std::vector<float>& levelrange,
                 const std::string& tag)
{
  std::cout << "--------\nRecommended " << tag << " frequency settings:\n";
  levels.clear();
  levelrange.clear();
  std::vector<float> vF;
  std::vector<float> vL;
  size_t spkno = 0u;
  // measure levels of all broadband speakers:
  for(auto& spk : spks) {
    // move source to speaker position:
    src.dlocation = spk.unitvector;
    usleep((unsigned int)(1e6 * prewait));
    // record measurement signal:
    jackrec.rec(recbuf, ports);
    //
    TASCAR::levelmeter_t levelmeter((float)jackrec.get_srate(), duration,
                                    weight);
    // calc average across input channels:
    float lev_sqr = 0.0f;
    std::vector<float> vLmean;
    std::vector<float> vLref;
    for(size_t ch = 0u; ch < ports.size() - 1u; ++ch) {
      auto& wav = recbuf[ch];
      levelmeter.update(wav);
      lev_sqr += levelmeter.ms();
      TASCAR::get_bandlevels(wav, fmin, fmax, (float)jackrec.get_srate(), 3.0f,
                             vF, vL);
      for(auto& l : vL)
        l = powf(10.0f, 0.1f * l);
      if(vLmean.empty())
        vLmean = vL;
      else {
        for(size_t k = 0; k < vL.size(); ++k)
          vLmean[k] += vL[k];
      }
    }
    for(auto& l : vLmean) {
      l /= (float)recbuf.size();
      l = 10.0f * log10f(l);
    }
    auto vl_min = vLmean.back();
    auto vl_max = vLmean.back();
    for(const auto& l : vLmean) {
      vl_min = std::min(vl_min, l);
      vl_max = std::max(vl_max, l);
    }
    levelrange.push_back(vl_max - vl_min);
    levelmeter.update(recbuf.back());
    TASCAR::get_bandlevels(recbuf.back(), fmin, fmax,
                           (float)jackrec.get_srate(), 3.0f, vF, vLref);
    float lev_ref = 10.0f * log10f(levelmeter.ms());
    lev_sqr /= (float)recbuf.size();
    lev_sqr = 10.0f * log10f(lev_sqr);
    levels.push_back(lev_sqr);
    for(auto& l : vLmean)
      l -= lev_sqr;
    for(auto& l : vLref)
      l -= lev_ref;
    for(size_t ch = 0; ch < std::min(vLmean.size(), vLref.size()); ++ch)
      vLmean[ch] = vLref[ch] - vLmean[ch];
    if(spk.eqfreq == vF)
      for(size_t ch = 0; ch < std::min(vLmean.size(), spk.eqgain.size()); ++ch)
        vLmean[ch] += spk.eqgain[ch];
    std::cout << tag << spkno << " " << spk.label << ": eqstages=\""
              << (vLmean.size() - 1u) / 3u << "\" eqfreq=\""
              << TASCAR::to_string(vF) << "\" eqgain=\""
              << TASCAR::to_string(vLmean) << "\"" << std::endl;
    ++spkno;
  }
}

void calibsession_t::get_levels(double prewait)
{
  //
  // first broadband speakers:
  //
  // unmute broadband source:
  scenes.back()->source_objects[0]->set_mute(false);
  // mute subwoofer source:
  scenes.back()->source_objects[1]->set_mute(true);
  // unmute the first receiver:
  scenes.back()->receivermod_objects[1]->set_mute(true);
  scenes.back()->receivermod_objects[0]->set_mute(false);
  auto allports = refport_;
  allports.push_back("render.calib:ref.0");
  // measure levels of all broadband speakers:
  get_levels_(*spkarray, *(scenes.back()->source_objects[0]), prewait, jackrec,
              bbrecbuf, allports, (float)duration, TASCAR::levelmeter::C,
              (float)fmin_, (float)fmax_, levels, levelsfrg, "speaker");
  //
  // subwoofer:
  //
  // mute broadband source:
  scenes.back()->source_objects[0]->set_mute(true);
  // unmute subwoofer source:
  scenes.back()->source_objects[1]->set_mute(false);
  get_levels_(spkarray->subs, *(scenes.back()->source_objects[1]), prewait,
              jackrec, subrecbuf, allports, (float)subduration,
              TASCAR::levelmeter::Z, (float)subfmin_, (float)subfmax_,
              sublevels, sublevelsfrg, "sub");
  // convert levels into gains:
  lmin = levels[0];
  lmax = levels[0];
  lmean = 0;
  for(auto l : levels) {
    lmean += l;
    lmin = std::min(l, lmin);
    lmax = std::max(l, lmax);
  }
  lmean /= (float)levels.size();
  // update gains of all receiver objects:
  for(auto recobj : scenes.back()->receivermod_objects) {
    // convert into speaker-based receiver:
    TASCAR::receivermod_base_speaker_t* recspk(
        dynamic_cast<TASCAR::receivermod_base_speaker_t*>(recobj->libdata));
    if(recspk) {
      // first modify gains:
      for(uint32_t k = 0; k < levels.size(); ++k)
        recspk->spkpos[k].gain *= pow(10.0, 0.05 * (lmin - levels[k]));
      for(uint32_t k = 0; k < sublevels.size(); ++k)
        recspk->spkpos.subs[k].gain *= pow(10.0, 0.05 * (lmin - sublevels[k]));
      // set max gain of broadband speakers to zero:
      double lmax(0);
      for(uint32_t k = 0; k < levels.size(); ++k)
        lmax = std::max(lmax, recspk->spkpos[k].gain);
      for(uint32_t k = 0; k < levels.size(); ++k)
        recspk->spkpos[k].gain /= lmax;
      for(uint32_t k = 0; k < sublevels.size(); ++k)
        recspk->spkpos.subs[k].gain /= lmax;
    }
  }
  for(auto src : scenes.back()->source_objects) {
    // mute source and reset position:
    src->set_mute(true);
    src->dlocation = pos_t(1, 0, 0);
  }
  levelsrecorded = true;
}

void calibsession_t::saveas(const std::string& fname)
{
  // convert levels into gains:
  std::vector<double> gains;
  float lmin(levels[0]);
  for(auto l : levels)
    lmin = std::min(l, lmin);
  for(uint32_t k = 0; k < levels.size(); ++k)
    gains.push_back(20 * log10((*spkarray)[k].gain) + lmin - levels[k]);
  // rewrite file:
  TASCAR::xml_doc_t doc(spkname, TASCAR::xml_doc_t::LOAD_FILE);
  if(doc.root.get_element_name() != "layout")
    throw TASCAR::ErrMsg(
        "Invalid file type, expected root node type \"layout\", got \"" +
        doc.root.get_element_name() + "\".");
  TASCAR::xml_element_t elem(doc.root);
  elem.set_attribute("caliblevel", TASCAR::to_string(get_caliblevel()));
  elem.set_attribute("diffusegain", TASCAR::to_string(get_diffusegain()));
  // update gains:
  if(!scenes.back()->receivermod_objects.empty()) {
    TASCAR::receivermod_base_speaker_t* recspk(
        dynamic_cast<TASCAR::receivermod_base_speaker_t*>(
            scenes.back()->receivermod_objects[1]->libdata));
    if(recspk) {
      TASCAR::spk_array_diff_render_t array(doc.root(), true);
      size_t k = 0;
      for(auto spk : doc.root.get_children("speaker")) {
        xml_element_t espk(spk);
        espk.set_attribute(
            "gain",
            TASCAR::to_string(
                20 *
                log10(recspk->spkpos[std::min(k, recspk->spkpos.size() - 1)]
                          .gain)));
        ++k;
      }
      k = 0;
      for(auto spk : doc.root.get_children("sub")) {
        xml_element_t espk(spk);
        espk.set_attribute(
            "gain",
            TASCAR::to_string(
                20 *
                log10(recspk->spkpos
                          .subs[std::min(k, recspk->spkpos.subs.size() - 1)]
                          .gain)));
        ++k;
      }
    }
  }
  size_t checksum = get_spklayout_checksum(elem);
  elem.set_attribute("checksum", (uint64_t)checksum);
  char ctmp[1024];
  memset(ctmp, 0, 1024);
  std::time_t t(std::time(nullptr));
  std::strftime(ctmp, 1023, "%Y-%m-%d %H:%M:%S", std::localtime(&t));
  doc.root.set_attribute("calibdate", ctmp);
  doc.root.set_attribute("calibfor", calibfor);
  doc.save(fname);
  gainmodified = false;
  levelsrecorded = false;
  calibrated = false;
  calibrated_diff = false;
}

void calibsession_t::save()
{
  saveas(spkname);
}

void calibsession_t::set_active(bool b)
{
  // activate broadband source and type-specific receiver.
  scenes.back()->source_objects[1]->set_mute(true);
  if(!b) {
    // inactive broadband, so enable nsp receiver:
    scenes.back()->receivermod_objects[0]->set_mute(false);
    scenes.back()->receivermod_objects[1]->set_mute(true);
  }
  if(b)
    // active, so mute diffuse sound:
    set_active_diff(false);
  scenes.back()->source_objects[0]->dlocation = pos_t(1, 0, 0);
  // activate broadband source if needed:
  scenes.back()->source_objects[0]->set_mute(!b);
  if(b) {
    // enable saving of file:
    calibrated = true;
    // active, so activate type-specific receiver:
    scenes.back()->receivermod_objects[0]->set_mute(true);
    scenes.back()->receivermod_objects[1]->set_mute(false);
  }
}

void calibsession_t::set_active_diff(bool b)
{
  // control diffuse source:
  scenes.back()->source_objects[1]->set_mute(true);
  if(!b) {
    scenes.back()->receivermod_objects[0]->set_mute(false);
    scenes.back()->receivermod_objects[1]->set_mute(true);
  }
  if(b)
    set_active(false);
  scenes.back()->diff_snd_field_objects.back()->set_mute(!b);
  if(b) {
    calibrated_diff = true;
    scenes.back()->receivermod_objects[0]->set_mute(true);
    scenes.back()->receivermod_objects[1]->set_mute(false);
  }
}

double calibsession_t::get_caliblevel() const
{
  if(!scenes.empty())
    if(!scenes.back()->receivermod_objects.empty()) {
      return 20.0 *
             log10(scenes.back()->receivermod_objects[1]->caliblevel * 5e4);
    }
  return 20.0 * log10(5e4);
}

double calibsession_t::get_diffusegain() const
{
  if(!scenes.empty())
    if(!scenes.back()->receivermod_objects.empty())
      return 20.0 * log10(scenes.back()->receivermod_objects[1]->diffusegain);
  return 0;
}

void calibsession_t::inc_caliblevel(double dl)
{
  gainmodified = true;
  delta += dl;
  double newlevel_pa(2e-5 * pow(10.0, 0.05 * (startlevel + delta)));
  if(!scenes.empty())
    for(auto recobj : scenes.back()->receivermod_objects) {
      TASCAR::receivermod_base_speaker_t* recspk(
          dynamic_cast<TASCAR::receivermod_base_speaker_t*>(recobj->libdata));
      if(recspk)
        recobj->caliblevel = (float)newlevel_pa;
    }
}

void calibsession_t::inc_diffusegain(double dl)
{
  gainmodified = true;
  delta_diff += dl;
  double gain(pow(10.0, 0.05 * (startdiffgain + delta_diff)));
  if(!scenes.empty())
    for(auto recobj : scenes.back()->receivermod_objects) {
      TASCAR::receivermod_base_speaker_t* recspk(
          dynamic_cast<TASCAR::receivermod_base_speaker_t*>(recobj->libdata));
      if(recspk)
        recobj->diffusegain = (float)gain;
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
