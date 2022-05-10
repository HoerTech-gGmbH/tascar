/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
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

#include "spkcalib_glade.h"

#include "calibsession.h"
#include "gui_elements.h"
#include <ctime>
#include <gtkmm.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <iostream>
#include <stdint.h>
#include <thread>

using namespace TASCAR;
using namespace TASCAR::Scene;

class spkcalib_t : public Gtk::Assistant {
public:
  spkcalib_t(BaseObjectType* cobject,
             const Glib::RefPtr<Gtk::Builder>& refGlade);
  virtual ~spkcalib_t();
  void load(const std::string& fname);
  void cleanup();
  void levelinc(float d);
  void inc_diffusegain(float d);
  void update_display();
  void configure_meters();
  void clear_meters();

private:
  void manage_act_grp_save();

protected:
  Glib::RefPtr<Gtk::Builder> m_refBuilder;
  // Signal handlers:
  void on_reclevels();
  void on_resetlevels();
  void on_play();
  void on_stop();
  void on_dec_10();
  void on_dec_2();
  void on_dec_05();
  void on_inc_05();
  void on_inc_2();
  void on_inc_10();
  void on_play_diff();
  void on_stop_diff();
  void on_dec_diff_10();
  void on_dec_diff_2();
  void on_dec_diff_05();
  void on_inc_diff_05();
  void on_inc_diff_2();
  void on_inc_diff_10();
  void on_open();
  void on_close();
  void on_save();
  void on_saveas();
  void on_quit();
  void on_assistant_next(Gtk::Widget* page);
  void on_assistant_back();
  void on_level_entered();
  void on_level_diff_entered();
  bool on_timeout();
  void update_gtkentry_from_value(const std::string& name, float val);
  void update_gtkcheckbox_from_value(const std::string& name, bool val);
  void update_gtkentry_from_value(const std::string& name,
                                  const std::vector<std::string>& val);
  void update_gtkentry_from_value_dbspl(const std::string& name,
                                        const std::vector<float>& val);
  void update_value_from_gtkentry(const std::string& name, float& val);
  void update_value_dbspl_from_gtkentry(const std::string& name,
                                        std::vector<float>& val);
  void update_value_from_gtkentry(const std::string& name,
                                  std::vector<std::string>& val);
  void update_value_from_gtkentry(const std::string& name, uint32_t& val);
  void update_value_from_gtkcheckbox(const std::string& name, bool& val);
  sigc::connection con_timeout;
  Glib::RefPtr<Gio::SimpleActionGroup> refActionGroupMain;
  Glib::RefPtr<Gio::SimpleActionGroup> refActionGroupSave;
  Glib::RefPtr<Gio::SimpleActionGroup> refActionGroupClose;
  Glib::RefPtr<Gio::SimpleActionGroup> refActionGroupCalib;
  Gtk::Box* step1_select_layout;
  Gtk::Box* step2_calib_params;
  Gtk::Box* step3_initcalib;
  Gtk::Box* box_step4_validate;
  Gtk::Label* label_filename1;
  Gtk::Label* label_spklist;
  Gtk::Label* label_levels;
  Gtk::Label* label_gains;
  Gtk::Label* text_levelresults;
  Gtk::Label* text_instruction;
  Gtk::Label* text_instruction_diff;
  Gtk::Label* text_caliblevel;
  Gtk::Label* lab_step3_caliblevel;
  Gtk::Label* lab_step3_info;
  Gtk::Entry* levelentry;
  Gtk::Entry* levelentry_diff;
  Gtk::ProgressBar* rec_progress;
  // Gtk::Box* meterbox;
  // Gtk::CheckButton* chk_f_bb;
  // Gtk::CheckButton* chk_f_sub;
  // Gtk::SpinButton* flt_order_bb;
  // Gtk::SpinButton* flt_order_sub;
  // calibsession_t* session;
  // spk_eq_param_t par_speaker;
  // spk_eq_param_t par_sub;
  // std::vector<std::string> refport;
  // std::vector<TASCAR::levelmeter_t*> rmsmeter;
  // std::vector<TASCAR::wave_t*> inwave;
  std::mutex guimeter_mutex;
  std::vector<TSCGUI::splmeter_t*> guimeter;
  // double miccalibdb;
  // double miccalib;
  spkcalibrator_t spkcalib;
  int prev_page = 0;
};

void spkcalib_t::update_gtkentry_from_value(const std::string& name, float val)
{
  Gtk::Entry* entry = NULL;
  m_refBuilder->get_widget(name, entry);
  if(entry)
    entry->set_text(TASCAR::to_string(val));
}

void spkcalib_t::update_gtkcheckbox_from_value(const std::string& name,
                                               bool val)
{
  Gtk::CheckButton* entry = NULL;
  m_refBuilder->get_widget(name, entry);
  if(entry)
    entry->set_active(val);
}

void spkcalib_t::update_gtkentry_from_value(const std::string& name,
                                            const std::vector<std::string>& val)
{
  Gtk::Entry* entry = NULL;
  m_refBuilder->get_widget(name, entry);
  if(entry)
    entry->set_text(TASCAR::vecstr2str(val));
}

void spkcalib_t::update_gtkentry_from_value_dbspl(const std::string& name,
                                                  const std::vector<float>& val)
{
  Gtk::Entry* entry = NULL;
  m_refBuilder->get_widget(name, entry);
  if(entry)
    entry->set_text(TASCAR::to_string_dbspl(val));
}

void spkcalib_t::update_value_from_gtkentry(const std::string& name, float& val)
{
  Gtk::Entry* entry = NULL;
  m_refBuilder->get_widget(name, entry);
  if(entry) {
    std::string attv = entry->get_text();
    char* c;
    float tmpv(strtof(attv.c_str(), &c));
    if(c != attv.c_str())
      val = tmpv;
    entry->set_text(TASCAR::to_string(val));
  }
}

void spkcalib_t::update_value_dbspl_from_gtkentry(const std::string& name,
                                                  std::vector<float>& val)
{
  Gtk::Entry* entry = NULL;
  m_refBuilder->get_widget(name, entry);
  if(entry) {
    std::vector<float> nval;
    auto sval = TASCAR::str2vecstr(entry->get_text());
    for(const auto& attv : sval) {
      char* c;
      float tmpv(strtof(attv.c_str(), &c));
      if(c != attv.c_str())
        nval.push_back(TASCAR::dbspl2lin(tmpv));
    }
    val = nval;
    entry->set_text(TASCAR::to_string_dbspl(val));
  }
}

void spkcalib_t::update_value_from_gtkentry(const std::string& name,
                                            std::vector<std::string>& val)
{
  Gtk::Entry* entry = NULL;
  m_refBuilder->get_widget(name, entry);
  if(entry) {
    val = TASCAR::str2vecstr(entry->get_text());
    entry->set_text(TASCAR::vecstr2str(val));
  }
}

void spkcalib_t::update_value_from_gtkentry(const std::string& name,
                                            uint32_t& val)
{
  Gtk::Entry* entry = NULL;
  m_refBuilder->get_widget(name, entry);
  if(entry) {
    std::string attv = entry->get_text();
    char* c;
    float tmpv(strtof(attv.c_str(), &c));
    if(c != attv.c_str())
      val = tmpv;
    entry->set_text(std::to_string(val));
  }
}

void spkcalib_t::update_value_from_gtkcheckbox(const std::string& name,
                                               bool& val)
{
  Gtk::CheckButton* entry = NULL;
  m_refBuilder->get_widget(name, entry);
  if(entry)
    val = entry->get_active();
}

bool spkcalib_t::on_timeout()
{
  std::lock_guard<std::mutex> lock(guimeter_mutex);
  for(uint32_t k = 0; k < guimeter.size(); ++k) {
    guimeter[k]->update_levelmeter(spkcalib.get_meter(k),
                                   spkcalib.cfg.par_speaker.reflevel);
    guimeter[k]->invalidate_win();
  }
  return true;
}

#define GET_WIDGET(x)                                                          \
  x = NULL;                                                                    \
  m_refBuilder->get_widget(#x, x);                                             \
  if(!x)                                                                       \
  throw TASCAR::ErrMsg(std::string("No widget \"") + #x +                      \
                       std::string("\" in builder."))

void error_message(const std::string& msg)
{
  std::cerr << "Error: " << msg << std::endl;
  Gtk::MessageDialog dialog("Error", false, Gtk::MESSAGE_ERROR);
  dialog.set_secondary_text(msg);
  dialog.run();
}

void spkcalib_t::clear_meters()
{
  std::lock_guard<std::mutex> lock(guimeter_mutex);
  auto page = get_nth_page(get_current_page());
  Gtk::Box* meterbox = dynamic_cast<Gtk::Box*>(page);
  for(auto& pmeter : guimeter) {
    if(meterbox)
      meterbox->remove(*pmeter);
    delete pmeter;
  }
  guimeter.clear();
}

void spkcalib_t::configure_meters()
{
  std::lock_guard<std::mutex> lock(guimeter_mutex);
  auto page = get_nth_page(get_current_page());
  Gtk::Box* meterbox = dynamic_cast<Gtk::Box*>(page);
  for(auto& pmeter : guimeter) {
    if(meterbox)
      meterbox->remove(*pmeter);
    delete pmeter;
  }
  guimeter.clear();
  for(uint32_t k = 0; k < spkcalib.cfg.refport.size(); ++k) {
    auto pmeter = new TSCGUI::splmeter_t();
    guimeter.push_back(pmeter);
    pmeter->set_mode(TSCGUI::dameter_t::rmspeak);
    pmeter->set_min_and_range(spkcalib.cfg.par_speaker.reflevel - 30.0f, 40.0f);
    if(meterbox) {
      meterbox->add(*pmeter);
      meterbox->show_all();
    }
  }
}

spkcalib_t::spkcalib_t(BaseObjectType* cobject,
                       const Glib::RefPtr<Gtk::Builder>& refGlade)
    : Gtk::Assistant(cobject), m_refBuilder(refGlade), text_instruction(NULL),
      text_instruction_diff(NULL), text_caliblevel(NULL)
{
  // for(uint32_t k = 0; k < refport.size(); ++k) {
  //  add_input_port(std::string("in.") + TASCAR::to_string(k + 1));
  //  rmsmeter.push_back(new TASCAR::levelmeter_t((float)get_srate(),
  //                                              (float)par_speaker.duration,
  //                                              TASCAR::levelmeter::C));
  //  inwave.push_back(new TASCAR::wave_t(get_fragsize()));
  //  guimeter.push_back(new TSCGUI::splmeter_t());
  //  guimeter.back()->set_mode(TSCGUI::dameter_t::rmspeak);
  //  guimeter.back()->set_min_and_range((float)miccalibdb - 40.0f, 40.0f);
  //}
  // jackc_t::activate();
  // for(uint32_t k = 0; k < refport.size(); ++k)
  //  connect_in(k, refport[k], true);
  // Create actions for menus and toolbars:
  refActionGroupMain = Gio::SimpleActionGroup::create();
  refActionGroupSave = Gio::SimpleActionGroup::create();
  refActionGroupClose = Gio::SimpleActionGroup::create();
  refActionGroupCalib = Gio::SimpleActionGroup::create();
  refActionGroupMain->add_action("open",
                                 sigc::mem_fun(*this, &spkcalib_t::on_open));
  refActionGroupSave->add_action("save",
                                 sigc::mem_fun(*this, &spkcalib_t::on_save));
  refActionGroupSave->add_action("saveas",
                                 sigc::mem_fun(*this, &spkcalib_t::on_saveas));
  refActionGroupClose->add_action("close",
                                  sigc::mem_fun(*this, &spkcalib_t::on_close));
  refActionGroupMain->add_action("quit",
                                 sigc::mem_fun(*this, &spkcalib_t::on_quit));
  insert_action_group("main", refActionGroupMain);
  refActionGroupCalib->add_action(
      "reclevels", sigc::mem_fun(*this, &spkcalib_t::on_reclevels));
  refActionGroupCalib->add_action(
      "resetlevels", sigc::mem_fun(*this, &spkcalib_t::on_resetlevels));
  refActionGroupCalib->add_action("play",
                                  sigc::mem_fun(*this, &spkcalib_t::on_play));
  refActionGroupCalib->add_action("stop",
                                  sigc::mem_fun(*this, &spkcalib_t::on_stop));
  refActionGroupCalib->add_action("inc_10",
                                  sigc::mem_fun(*this, &spkcalib_t::on_inc_10));
  refActionGroupCalib->add_action("inc_2",
                                  sigc::mem_fun(*this, &spkcalib_t::on_inc_2));
  refActionGroupCalib->add_action("inc_05",
                                  sigc::mem_fun(*this, &spkcalib_t::on_inc_05));
  refActionGroupCalib->add_action("dec_10",
                                  sigc::mem_fun(*this, &spkcalib_t::on_dec_10));
  refActionGroupCalib->add_action("dec_2",
                                  sigc::mem_fun(*this, &spkcalib_t::on_dec_2));
  refActionGroupCalib->add_action("dec_05",
                                  sigc::mem_fun(*this, &spkcalib_t::on_dec_05));
  refActionGroupCalib->add_action(
      "play_diff", sigc::mem_fun(*this, &spkcalib_t::on_play_diff));
  refActionGroupCalib->add_action(
      "stop_diff", sigc::mem_fun(*this, &spkcalib_t::on_stop_diff));
  refActionGroupCalib->add_action(
      "inc_diff_10", sigc::mem_fun(*this, &spkcalib_t::on_inc_diff_10));
  refActionGroupCalib->add_action(
      "inc_diff_2", sigc::mem_fun(*this, &spkcalib_t::on_inc_diff_2));
  refActionGroupCalib->add_action(
      "inc_diff_05", sigc::mem_fun(*this, &spkcalib_t::on_inc_diff_05));
  refActionGroupCalib->add_action(
      "dec_diff_10", sigc::mem_fun(*this, &spkcalib_t::on_dec_diff_10));
  refActionGroupCalib->add_action(
      "dec_diff_2", sigc::mem_fun(*this, &spkcalib_t::on_dec_diff_2));
  refActionGroupCalib->add_action(
      "dec_diff_05", sigc::mem_fun(*this, &spkcalib_t::on_dec_diff_05));
  // insert_action_group("calib",refActionGroupCalib);
  GET_WIDGET(step1_select_layout);
  GET_WIDGET(step2_calib_params);
  GET_WIDGET(step3_initcalib);
  GET_WIDGET(box_step4_validate);
  GET_WIDGET(label_filename1);
  GET_WIDGET(label_spklist);
  GET_WIDGET(label_levels);
  GET_WIDGET(label_gains);
  GET_WIDGET(text_levelresults);
  GET_WIDGET(text_instruction_diff);
  GET_WIDGET(text_instruction);
  GET_WIDGET(text_caliblevel);
  GET_WIDGET(lab_step3_caliblevel);
  GET_WIDGET(lab_step3_info);
  GET_WIDGET(levelentry);
  GET_WIDGET(levelentry_diff);
  GET_WIDGET(rec_progress);
  // GET_WIDGET(meterbox);
  signal_cancel().connect(sigc::mem_fun(*this, &spkcalib_t::on_quit));
  signal_prepare().connect(
      sigc::mem_fun(*this, &spkcalib_t::on_assistant_next));
  levelentry_diff->signal_activate().connect(
      sigc::mem_fun(*this, &spkcalib_t::on_level_diff_entered));
  con_timeout = Glib::signal_timeout().connect(
      sigc::mem_fun(*this, &spkcalib_t::on_timeout), 250);
  update_display();
  show_all();
}

void spkcalib_t::on_level_entered()
{
  try {
    float newlevel(spkcalib.cfg.par_speaker.reflevel);
    std::string slevel(levelentry->get_text());
    newlevel = atof(slevel.c_str());
    levelinc(spkcalib.cfg.par_speaker.reflevel - newlevel);
  }
  catch(const std::exception& e) {
    error_message(e.what());
  }
}

void spkcalib_t::on_level_diff_entered()
{
  try {
    float newlevel(spkcalib.cfg.par_speaker.reflevel);
    std::string slevel(levelentry_diff->get_text());
    newlevel = atof(slevel.c_str());
    inc_diffusegain(spkcalib.cfg.par_speaker.reflevel - newlevel);
  }
  catch(const std::exception& e) {
    error_message(e.what());
  }
}

void spkcalib_t::manage_act_grp_save()
{
  //  if(session && session->complete())
  //    insert_action_group("save", refActionGroupSave);
  //  else
  //    remove_action_group("save");
  //  if(session) {
  //    std::string gainstr;
  //    if(!session->scenes.back()->receivermod_objects.empty()) {
  //      TASCAR::receivermod_base_speaker_t* recspk(
  //          dynamic_cast<TASCAR::receivermod_base_speaker_t*>(
  //              session->scenes.back()->receivermod_objects[1]->libdata));
  //      if(recspk) {
  //        for(uint32_t k = 0; k < recspk->spkpos.size(); ++k) {
  //          char lc[1024];
  //          sprintf(lc, "%1.1f(%1.1f) ", 20 * log10(recspk->spkpos[k].gain),
  //                  session->levelsfrg[k]);
  //          gainstr += lc;
  //        }
  //        if(!recspk->spkpos.subs.empty())
  //          gainstr += "subs: ";
  //        for(uint32_t k = 0; k < recspk->spkpos.subs.size(); ++k) {
  //          char lc[1024];
  //          sprintf(lc, "%1.1f(%1.1f) ", 20 *
  //          log10(recspk->spkpos.subs[k].gain),
  //                  session->sublevelsfrg[k]);
  //          gainstr += lc;
  //        }
  //      }
  //    }
  //    label_gains->set_text(gainstr);
  //  } else {
  //    label_gains->set_text("");
  //  }
  //  if(session && session->levels_complete()) {
  //    char ctmp[1024];
  //    sprintf(ctmp, "Mean level: %1.1f dB FS (range: %1.1f dB)",
  //            session->get_lmean(), session->get_lmax() -
  //            session->get_lmin());
  //    text_levelresults->set_text(ctmp);
  //  } else {
  //    text_levelresults->set_text("");
  //  }
  //  if(session) {
  //    std::string smodified("");
  //    if(session->modified())
  //      smodified = " (modified)";
  //    set_title(std::string("TASCAR speaker calibration [") +
  //              Glib::filename_display_basename(session->name()) +
  //              std::string("]") + smodified);
  //  } else
  //    set_title("TASCAR speaker calibration");
}

void guiupdate(Gtk::ProgressBar* rec_progress, double sleepsec, bool* pbquit)
{
  TASCAR::tictoc_t tic;
  double t = 0.0;
  while((!(*pbquit)) && (t<=1.0)) {
    t = tic.toc()/sleepsec;
    rec_progress->set_fraction(std::min(1.0, t));
    while(Gtk::Main::events_pending())
      Gtk::Main::iteration(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void getlevels(TASCAR::calibsession_t* session, bool* pbquit)
{
  session->get_levels();
  *pbquit = true;
}

void spkcalib_t::on_reclevels()
{
  try {
    // if(session) {
    remove_action_group("calib");
    //  remove_action_group("close");
    //  levelentry->set_sensitive(false);
    //  levelentry_diff->set_sensitive(false);
    bool bquitthread = false;
    //std::thread guiupdater(getlevels, session, &bquitthread);
    guiupdate(rec_progress, spkcalib.get_measurement_duration(),
              &bquitthread);
    //
    //  if(guiupdater.joinable())
    //    guiupdater.join();
    insert_action_group("calib", refActionGroupCalib);
    //  insert_action_group("close", refActionGroupClose);
    //  levelentry->set_sensitive(true);
    //  levelentry_diff->set_sensitive(true);
    //}
    manage_act_grp_save();
  }
  catch(const std::exception& e) {
    error_message(e.what());
  }
}

void spkcalib_t::on_resetlevels()
{
  try {
    // if(session)
    //  session->reset_levels();
    manage_act_grp_save();
  }
  catch(const std::exception& e) {
    error_message(e.what());
  }
}

void spkcalib_t::on_play()
{
  spkcalib.set_active_pointsource(true);
  manage_act_grp_save();
  set_page_complete(*step3_initcalib, true);
}

void spkcalib_t::on_stop()
{
  spkcalib.set_active_pointsource(false);
}

void spkcalib_t::on_dec_10()
{
  levelinc(-10);
}

void spkcalib_t::on_dec_2()
{
  levelinc(-2);
}

void spkcalib_t::on_dec_05()
{
  levelinc(-0.5);
}

void spkcalib_t::on_inc_05()
{
  levelinc(0.5);
}

void spkcalib_t::on_inc_2()
{
  levelinc(2);
}

void spkcalib_t::on_inc_10()
{
  levelinc(10);
}

void spkcalib_t::on_play_diff()
{
  // if(session)
  //  session->set_active_diff(true);
  spkcalib.set_active_diffuse(true);
  manage_act_grp_save();
}

void spkcalib_t::on_stop_diff()
{
  // if(session)
  //  session->set_active_diff(false);
  spkcalib.set_active_diffuse(false);
}

void spkcalib_t::on_dec_diff_10()
{
  inc_diffusegain(-10);
}

void spkcalib_t::on_dec_diff_2()
{
  inc_diffusegain(-2);
}

void spkcalib_t::on_dec_diff_05()
{
  inc_diffusegain(-0.5);
}

void spkcalib_t::on_inc_diff_05()
{
  inc_diffusegain(0.5);
}

void spkcalib_t::on_inc_diff_2()
{
  inc_diffusegain(2);
}

void spkcalib_t::on_inc_diff_10()
{
  inc_diffusegain(10);
}

#define UPDATE_GTKENTRY_FROM_VALUE(spkset, var)                                \
  update_gtkentry_from_value(#spkset "_" #var, spkcalib.cfg.spkset.var)
#define UPDATE_VALUE_FROM_GTKENTRY(spkset, var)                                \
  update_value_from_gtkentry(#spkset "_" #var, spkcalib.cfg.spkset.var)

void spkcalib_t::on_assistant_next(Gtk::Widget*)
{
  auto current_page = get_current_page();
  switch(current_page) {
  case 0:
    spkcalib.go_back();
    remove_action_group("calib");
    clear_meters();
    break;
  case 1:
    spkcalib.step1_file_selected();
    remove_action_group("calib");
    // now update all fields:
    UPDATE_GTKENTRY_FROM_VALUE(par_speaker, fmin);
    UPDATE_GTKENTRY_FROM_VALUE(par_speaker, fmax);
    UPDATE_GTKENTRY_FROM_VALUE(par_speaker, duration);
    UPDATE_GTKENTRY_FROM_VALUE(par_speaker, prewait);
    UPDATE_GTKENTRY_FROM_VALUE(par_speaker, reflevel);
    UPDATE_GTKENTRY_FROM_VALUE(par_speaker, bandsperoctave);
    UPDATE_GTKENTRY_FROM_VALUE(par_speaker, bandoverlap);
    UPDATE_GTKENTRY_FROM_VALUE(par_speaker, max_eqstages);
    UPDATE_GTKENTRY_FROM_VALUE(par_sub, fmin);
    UPDATE_GTKENTRY_FROM_VALUE(par_sub, fmax);
    UPDATE_GTKENTRY_FROM_VALUE(par_sub, duration);
    UPDATE_GTKENTRY_FROM_VALUE(par_sub, prewait);
    UPDATE_GTKENTRY_FROM_VALUE(par_sub, reflevel);
    UPDATE_GTKENTRY_FROM_VALUE(par_sub, bandsperoctave);
    UPDATE_GTKENTRY_FROM_VALUE(par_sub, bandoverlap);
    UPDATE_GTKENTRY_FROM_VALUE(par_sub, max_eqstages);
    update_gtkentry_from_value("entry_refport", spkcalib.cfg.refport);
    update_gtkentry_from_value_dbspl("entry_miccalibdb", spkcalib.cfg.miccalib);
    update_gtkcheckbox_from_value("checkbox_initcal", spkcalib.cfg.initcal);
    clear_meters();
    break;
  case 2:
    UPDATE_VALUE_FROM_GTKENTRY(par_speaker, fmin);
    UPDATE_VALUE_FROM_GTKENTRY(par_speaker, fmax);
    UPDATE_VALUE_FROM_GTKENTRY(par_speaker, duration);
    UPDATE_VALUE_FROM_GTKENTRY(par_speaker, prewait);
    UPDATE_VALUE_FROM_GTKENTRY(par_speaker, reflevel);
    UPDATE_VALUE_FROM_GTKENTRY(par_speaker, bandsperoctave);
    UPDATE_VALUE_FROM_GTKENTRY(par_speaker, bandoverlap);
    UPDATE_VALUE_FROM_GTKENTRY(par_speaker, max_eqstages);
    UPDATE_VALUE_FROM_GTKENTRY(par_sub, fmin);
    UPDATE_VALUE_FROM_GTKENTRY(par_sub, fmax);
    UPDATE_VALUE_FROM_GTKENTRY(par_sub, duration);
    UPDATE_VALUE_FROM_GTKENTRY(par_sub, prewait);
    UPDATE_VALUE_FROM_GTKENTRY(par_sub, reflevel);
    UPDATE_VALUE_FROM_GTKENTRY(par_sub, bandsperoctave);
    UPDATE_VALUE_FROM_GTKENTRY(par_sub, bandoverlap);
    UPDATE_VALUE_FROM_GTKENTRY(par_sub, max_eqstages);
    update_value_from_gtkentry("entry_refport", spkcalib.cfg.refport);
    update_value_dbspl_from_gtkentry("entry_miccalibdb", spkcalib.cfg.miccalib);
    update_value_from_gtkcheckbox("checkbox_initcal", spkcalib.cfg.initcal);
    spkcalib.step2_config_revised();
    insert_action_group("calib", refActionGroupCalib);
    spkcalib.set_active_pointsource(false);
    spkcalib.set_active_diffuse(false);
    if(!spkcalib.cfg.initcal) {
      // skip initcal page
      if(prev_page < current_page) {
        next_page();
        spkcalib.step3_calib_initialized();
      } else {
        previous_page();
        spkcalib.step1_file_selected();
      }
    } else {
      configure_meters();
      if(prev_page < current_page)
        spkcalib.set_caliblevel(140);
      update_display();
    }
    break;
  case 3:
    spkcalib.step3_calib_initialized();
    spkcalib.set_active_pointsource(false);
    spkcalib.set_active_diffuse(false);
    break;
  case 4:
    spkcalib.step4_speaker_equalized();
    configure_meters();
    break;
  case 5:
    spkcalib.step5_levels_adjusted();
    break;
  }
  prev_page = current_page;
}

void spkcalib_t::on_assistant_back()
{
  DEBUG("back");
}

void spkcalib_t::on_quit()
{
  hide(); // Closes the main window to stop the app->run().
}

void spkcalib_t::on_open()
{
  Gtk::FileChooserDialog dialog("Please choose a file",
                                Gtk::FILE_CHOOSER_ACTION_OPEN);
  dialog.set_transient_for(*this);
  // Add response buttons the the dialog:
  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
  // Add filters, so that only certain file types can be selected:
  Glib::RefPtr<Gtk::FileFilter> filter_tascar = Gtk::FileFilter::create();
  filter_tascar->set_name("speaker layout files");
  filter_tascar->add_pattern("*.spk");
  dialog.add_filter(filter_tascar);
  Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();
  filter_any->set_name("Any files");
  filter_any->add_pattern("*");
  dialog.add_filter(filter_any);
  // Show the dialog and wait for a user response:
  int result = dialog.run();
  // Handle the response:
  if(result == Gtk::RESPONSE_OK) {
    // Notice that this is a std::string, not a Glib::ustring.
    std::string filename = dialog.get_filename();
    if(!filename.empty()) {
      try {
        get_warnings().clear();
        load(filename);
      }
      catch(const std::exception& e) {
        error_message(e.what());
      }
    }
  }
}

void spkcalib_t::on_close()
{
  cleanup();
  update_display();
}

void spkcalib_t::levelinc(float d)
{
  spkcalib.inc_caliblevel(-d);
  update_display();
}

void spkcalib_t::inc_diffusegain(float d)
{
  // if(session)
  spkcalib.inc_diffusegain(d);
  update_display();
}

void spkcalib_t::on_save()
{
  try {
    // if(session)
    //  session->save();
    update_display();
  }
  catch(const std::exception& e) {
    error_message(e.what());
  }
}

void spkcalib_t::on_saveas()
{
  // if(session) {
  //  Gtk::FileChooserDialog dialog("Please choose a file",
  //                                Gtk::FILE_CHOOSER_ACTION_SAVE);
  //  dialog.set_transient_for(*this);
  //  // Add response buttons the the dialog:
  //  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  //  dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
  //  // Add filters, so that only certain file types can be selected:
  //  Glib::RefPtr<Gtk::FileFilter> filter_tascar = Gtk::FileFilter::create();
  //  filter_tascar->set_name("speaker layout files");
  //  filter_tascar->add_pattern("*.spk");
  //  dialog.add_filter(filter_tascar);
  //  Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();
  //  filter_any->set_name("Any files");
  //  filter_any->add_pattern("*");
  //  dialog.add_filter(filter_any);
  //  // Show the dialog and wait for a user response:
  //  int result = dialog.run();
  //  // Handle the response:
  //  if(result == Gtk::RESPONSE_OK) {
  //    // Notice that this is a std::string, not a Glib::ustring.
  //    std::string filename = dialog.get_filename();
  //    try {
  //      get_warnings().clear();
  //      if(session)
  //        session->saveas(filename);
  //      load(filename);
  //      update_display();
  //    }
  //    catch(const std::exception& e) {
  //      error_message(e.what());
  //    }
  //  }
  //}
}

spkcalib_t::~spkcalib_t()
{
  con_timeout.disconnect();
  cleanup();
  // for(auto it = rmsmeter.begin(); it != rmsmeter.end(); ++it)
  //  delete *it;
  // for(auto it = inwave.begin(); it != inwave.end(); ++it)
  //  delete *it;
  for(auto& pmeter : guimeter)
    delete pmeter;
}

void spkcalib_t::update_display()
{
  rec_progress->set_fraction(0);
  label_filename1->set_text(spkcalib.get_filename());
  label_spklist->set_text(spkcalib.get_speaker_desc());
  // if(session) {
  char ctmp[1024];
  sprintf(ctmp, "caliblevel: %1.1f dB diffusegain: %1.1f dB",
          spkcalib.get_caliblevel(), spkcalib.get_diffusegain());
  lab_step3_caliblevel->set_text(ctmp);
  lab_step3_info->set_text(
      std::string("Adjust the signal level with the buttons below\n"
                  "until it reaches approximately ") +
      TASCAR::to_string(spkcalib.cfg.par_speaker.reflevel) +
      std::string(" dB SPL (C-weighted)."));
  //  // chk_f_bb->set_active(session->max_fcomp_bb > 0u);
  //  // chk_f_sub->set_active(session->max_fcomp_sub > 0u);
  //  // flt_order_bb->set_text(std::to_string(session->max_fcomp_bb));
  //  // flt_order_sub->set_text(std::to_string(session->max_fcomp_sub));
  //} else {
  //  text_caliblevel->set_text("no layout file loaded.");
  //  chk_f_bb->set_active(false);
  //  chk_f_sub->set_active(false);
  //  flt_order_bb->set_text("");
  //  flt_order_sub->set_text("");
  //}
  std::string portlist;
  // for(auto it = refport.begin(); it != refport.end(); ++it)
  //  portlist += *it + " ";
  // if(portlist.size())
  //  portlist.erase(portlist.size() - 1, 1);
  // char ctmp[1024];
  // sprintf(
  //    ctmp,
  //    "1. Relative loudspeaker gains:\nPlace a measurement microphone at the "
  //    "listening position and connect to port%s \"%s\". A pink noise will be "
  //    "played from the loudspeaker positions. Press record to start.",
  //    (refport.size() > 1) ? "s" : "", portlist.c_str());
  // label_levels->set_text(ctmp);
  // sprintf(ctmp,
  //        "2. Adjust the playback level to %1.1f dB using the inc/dec buttons.
  //        " "Alternatively, enter the measured level in the field below. Use Z
  //        " "or C weighting.\n a) Point source:", par_speaker.reflevel);
  // text_instruction->set_text(ctmp);
  // text_instruction_diff->set_text(" b) Diffuse sound field:");
  // if(get_warnings().size()) {
  //  Gtk::MessageDialog dialog(*this, "Warning", false, Gtk::MESSAGE_WARNING);
  //  std::string msg;
  //  for(auto warn : get_warnings())
  //    msg += warn + "\n";
  //  dialog.set_secondary_text(msg);
  //  dialog.run();
  //  get_warnings().clear();
  //}
  // manage_act_grp_save();
  // if(session) {
  //  insert_action_group("calib", refActionGroupCalib);
  //  insert_action_group("close", refActionGroupClose);
  //  levelentry->set_sensitive(true);
  //  levelentry_diff->set_sensitive(true);
  //} else {
  //  remove_action_group("calib");
  //  remove_action_group("close");
  //  levelentry->set_text("");
  //  levelentry_diff->set_text("");
  //  levelentry->set_sensitive(false);
  //  levelentry_diff->set_sensitive(false);
  //}
}

void spkcalib_t::load(const std::string& fname)
{
  cleanup();
  if(fname.empty())
    throw TASCAR::ErrMsg("Empty file name.");
  spkcalib.set_filename(fname);
  set_page_complete(*step1_select_layout, true);
  update_display();
}

void spkcalib_t::cleanup()
{
  update_display();
}

int main(int argc, char** argv)
{
  setlocale(LC_ALL, "C");
  TASCAR::config_forceoverwrite("tascar.spkcalib.maxage", "3650");
  TASCAR::config_forceoverwrite("tascar.spkcalib.checktypeid", "0");
  int nargv(1);
  Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(
      nargv, argv, "org.tascar.tascarspkcalib", Gio::APPLICATION_NON_UNIQUE);
  Glib::RefPtr<Gtk::Builder> refBuilder;
  spkcalib_t* win(NULL);
  refBuilder = Gtk::Builder::create();
  refBuilder->add_from_string(ui_spkcalib);
  refBuilder->get_widget_derived("CalibAssistant", win);
  if(!win)
    throw TASCAR::ErrMsg("No main window");
  win->show_all();
  setlocale(LC_ALL, "C");
  if(argc > 1) {
    try {
      win->load(argv[1]);
    }
    catch(const std::exception& e) {
      std::string msg("While loading file \"");
      msg += argv[1];
      msg += "\": ";
      msg += e.what();
      std::cerr << "Error: " << msg << std::endl;
      error_message(msg);
    }
  }
  int rv(app->run(*win));
  delete win;
  return rv;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
