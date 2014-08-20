#ifndef GUI_ELEMENTS_H
#define GUI_ELEMENTS_H

#include <gtkmm.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/drawingarea.h>
#include <cairomm/context.h>
#include <lo/lo.h>
#include "viewport.h"
#include "tascar.h"

class source_ctl_t : public Gtk::Frame {
public:
  source_ctl_t(lo_address client_addr, TASCAR::Scene::scene_t* s, TASCAR::Scene::route_t* r);
  void on_mute();
  void on_solo();
  Gtk::EventBox ebox;
  Gtk::HBox box;
  Gtk::Label tlabel;
  Gtk::Label label;
  Gtk::ToggleButton mute;
  Gtk::ToggleButton solo;
  lo_address client_addr_;
  std::string name_;
  TASCAR::Scene::scene_t* scene_;
  TASCAR::Scene::route_t* route_;
};

class source_panel_t : public Gtk::ScrolledWindow {
public:
  source_panel_t(lo_address client_addr);
  void set_scene(TASCAR::Scene::scene_t*);
  std::vector<source_ctl_t*> vbuttons;
  Gtk::VBox box;
  lo_address client_addr_;
};

class scene_draw_t {
public:
  scene_draw_t();
  virtual ~scene_draw_t();
  void set_scene(TASCAR::Scene::scene_t* scene);
  void select_object(TASCAR::Scene::object_t* o);
  virtual void draw(Cairo::RefPtr<Cairo::Context> cr);
protected:
  virtual void draw_face_normal(const TASCAR::face_t& f, Cairo::RefPtr<Cairo::Context> cr, double normalsize=0.0);
  virtual void draw_cube(TASCAR::pos_t pos, TASCAR::zyx_euler_t orient, TASCAR::pos_t size,Cairo::RefPtr<Cairo::Context> cr);
  virtual void draw_track(const TASCAR::Scene::object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  virtual void draw_src(const TASCAR::Scene::src_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  virtual void draw_sink_object(const TASCAR::Scene::sink_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  virtual void draw_door_src(const TASCAR::Scene::src_door_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  virtual void draw_room_src(const TASCAR::Scene::src_diffuse_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  virtual void draw_face(const TASCAR::Scene::face_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  virtual void draw_mask(TASCAR::Scene::mask_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  TASCAR::Scene::scene_t* scene_;
  viewport_t view;
  double time;
  TASCAR::Scene::object_t* selection;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
