#include "gui_elements.h"


source_ctl_t::source_ctl_t(lo_address client_addr, TASCAR::Scene::scene_t* s, TASCAR::Scene::route_t* r)
  : mute("M"),solo("S"),client_addr_(client_addr),name_(r->get_name()),scene_(s),route_(r)
{
  ebox.add( box );
  label.set_text(r->get_name());
  if( dynamic_cast<TASCAR::Scene::face_object_t*>(r))
    tlabel.set_text("mir");
  if( dynamic_cast<TASCAR::Scene::src_object_t*>(r))
    tlabel.set_text("src");
  if( dynamic_cast<TASCAR::Scene::src_diffuse_t*>(r))
    tlabel.set_text("dif");
  if( dynamic_cast<TASCAR::Scene::sink_object_t*>(r))
    tlabel.set_text("sink");
  if( dynamic_cast<TASCAR::Scene::src_door_t*>(r))
    tlabel.set_text("door");
  box.pack_start( tlabel, Gtk::PACK_SHRINK );
  box.pack_start( label, Gtk::PACK_EXPAND_PADDING );
  box.pack_start( mute, Gtk::PACK_SHRINK );
  box.pack_start( solo, Gtk::PACK_SHRINK );
  mute.set_active(r->get_mute());
  solo.set_active(r->get_solo());
#ifdef GTKMM30
  Gdk::RGBA col_yellow("f4e83a");
  //col.set_rgba(244.0/256,232.0/256,58.0/256);
  mute.override_background_color(col_yellow);
  col_yellow.set_rgba_u(219*256,18*256,18*256);
  solo.override_background_color(col_yellow,Gtk::STATE_FLAG_ACTIVE);
  Gdk::RGBA col;
  if( TASCAR::Scene::object_t* o=dynamic_cast<TASCAR::Scene::object_t*>(r) ){
    TASCAR::Scene::rgb_color_t c(o->color);
    col.set_rgba(c.r,c.g,c.b,0.3);
    ebox.override_background_color(col);
  }
#else
  Gdk::Color col;
  col.set_rgb(244*256,232*256,58*256);
  mute.modify_bg(Gtk::STATE_ACTIVE,col);
  mute.modify_bg(Gtk::STATE_PRELIGHT,col);
  mute.modify_bg(Gtk::STATE_SELECTED,col);
  col.set_rgb(244*256,30*256,30*256);
  solo.modify_bg(Gtk::STATE_ACTIVE,col);
  solo.modify_bg(Gtk::STATE_PRELIGHT,col);
  if( object_t* o=dynamic_cast<object_t*>(r) ){
    rgb_color_t c(o->color);
    col.set_rgb_p(0.5+0.3*c.r,0.5+0.3*c.g,0.5+0.3*c.b);
    ebox.modify_bg(Gtk::STATE_NORMAL,col);
  }
#endif
  add(ebox);
  mute.signal_clicked().connect(sigc::mem_fun(*this,&source_ctl_t::on_mute));
  solo.signal_clicked().connect(sigc::mem_fun(*this,&source_ctl_t::on_solo));
}

void source_ctl_t::on_mute()
{
  bool m(mute.get_active());
  std::string path("/"+scene_->name+"/"+name_+"/mute");
  lo_send(client_addr_,path.c_str(),"i",m);
}

void source_ctl_t::on_solo()
{
  bool m(solo.get_active());
  std::string path("/"+scene_->name+"/"+name_+"/solo");
  lo_send(client_addr_,path.c_str(),"i",m);
}

source_panel_t::source_panel_t(lo_address client_addr)
 : client_addr_(client_addr)
{
  set_size_request( 300, -1 );
  add(box);
}

void source_panel_t::set_scene(TASCAR::Scene::scene_t* s)
{
  for( unsigned int k=0;k<vbuttons.size();k++){
    box.remove(*(vbuttons[k]));
    delete vbuttons[k];
  }
  vbuttons.clear();
  if( s ){
    std::vector<TASCAR::Scene::object_t*> obj(s->get_objects());
    for(std::vector<TASCAR::Scene::object_t*>::iterator it=obj.begin();it!=obj.end();++it)
      vbuttons.push_back(new source_ctl_t(client_addr_,s,*it));
  }
  for( unsigned int k=0;k<vbuttons.size();k++){
    box.pack_start(*(vbuttons[k]), Gtk::PACK_SHRINK);
  }
  show_all();
}

scene_draw_t::scene_draw_t()
  : scene_(NULL),
    time(0),
    selection(NULL)
{
}

scene_draw_t::~scene_draw_t()
{
}

void scene_draw_t::set_scene(TASCAR::Scene::scene_t* scene)
{
  scene_ = scene;
}

void scene_draw_t::select_object(TASCAR::Scene::object_t* o)
{
  selection = o;
}

void scene_draw_t::draw(Cairo::RefPtr<Cairo::Context> cr)
{
}

void scene_draw_t::draw_face_normal(const TASCAR::face_t& f, Cairo::RefPtr<Cairo::Context> cr, double normalsize)
{
}

void scene_draw_t::draw_cube(TASCAR::pos_t pos, TASCAR::zyx_euler_t orient, TASCAR::pos_t size,Cairo::RefPtr<Cairo::Context> cr)
{
}

void scene_draw_t::draw_track(const TASCAR::Scene::object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
}

void scene_draw_t::draw_src(const TASCAR::Scene::src_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
}

void scene_draw_t::draw_sink_object(const TASCAR::Scene::sink_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
}

void scene_draw_t::draw_door_src(const TASCAR::Scene::src_door_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
}

void scene_draw_t::draw_room_src(const TASCAR::Scene::src_diffuse_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
}

void scene_draw_t::draw_face(const TASCAR::Scene::face_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
}

void scene_draw_t::draw_mask(TASCAR::Scene::mask_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
