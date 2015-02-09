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
  source_ctl_t(TASCAR::Scene::scene_t* s, TASCAR::Scene::route_t* r);
  void setup();
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
  bool use_osc;
};

class source_panel_t : public Gtk::ScrolledWindow {
public:
  source_panel_t(lo_address client_addr);
  source_panel_t(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
  void set_scene(TASCAR::Scene::scene_t*);
  std::vector<source_ctl_t*> vbuttons;
  Gtk::VBox box;
  lo_address client_addr_;
  bool use_osc;
};

class scene_draw_t {
public:
  enum viewt_t {
    xy, xz, yz, p
  };
  scene_draw_t();
  virtual ~scene_draw_t();
  void set_scene(TASCAR::Scene::scene_t* scene);
  void select_object(TASCAR::Scene::object_t* o);
  void set_viewport(const scene_draw_t::viewt_t& v);
  virtual void draw(Cairo::RefPtr<Cairo::Context> cr);
  void set_markersize(double msize);
  void set_blink(bool blink);
  void set_time(double t);
  double get_time() const {return time;};
protected:
  void draw_edge(Cairo::RefPtr<Cairo::Context> cr, pos_t p1, pos_t p2);
  void draw_object(TASCAR::Scene::object_t* obj,Cairo::RefPtr<Cairo::Context> cr);
  virtual void ngon_draw_normal(TASCAR::ngon_t* f, Cairo::RefPtr<Cairo::Context> cr, double normalsize);
  virtual void ngon_draw(TASCAR::ngon_t* f, Cairo::RefPtr<Cairo::Context> cr,bool fill = false, bool area = false);
  virtual void draw_cube(TASCAR::pos_t pos, TASCAR::zyx_euler_t orient, TASCAR::pos_t size,Cairo::RefPtr<Cairo::Context> cr);
  // object draw functions:
  virtual void draw_track(TASCAR::Scene::object_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  virtual void draw_src(TASCAR::Scene::src_object_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  virtual void draw_receiver_object(TASCAR::Scene::receivermod_object_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  virtual void draw_door_src(TASCAR::Scene::src_door_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  virtual void draw_room_src(TASCAR::Scene::src_diffuse_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  virtual void draw_face(TASCAR::Scene::face_object_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  virtual void draw_facegroup(TASCAR::Scene::face_group_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  virtual void draw_mask(TASCAR::Scene::mask_object_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  TASCAR::Scene::scene_t* scene_;
public:
  viewport_t view;
protected:
  double time;
  TASCAR::Scene::object_t* selection;
  double markersize;
  bool blink;
private:
  pthread_mutex_t mtx;
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
