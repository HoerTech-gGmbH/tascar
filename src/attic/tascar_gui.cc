/**
   \file tascar_gui.cc
   \ingroup apptascar
   \brief Draw TASCAR scene
   \author Giso Grimm
   \date 2013

   \section license License (GPL)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; version 2 of the
   License.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

#include <libxml++/libxml++.h>
#include <gtkmm.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/drawingarea.h>
#include <cairomm/context.h>
#include "tascar.h"
#include "osc_scene.h"
#include <iostream>
#include <getopt.h>
#include "viewport.h"
#include <limits>
#include "gui_elements.h"
#include "session.h"

using namespace TASCAR;
using namespace TASCAR::Scene;

bool has_infinity(const pos_t& p)
{
  if( p.x == std::numeric_limits<double>::infinity() )
    return true;
  if( p.y == std::numeric_limits<double>::infinity() )
    return true;
  if( p.z == std::numeric_limits<double>::infinity() )
    return true;
  return false;
}

void draw_edge(Cairo::RefPtr<Cairo::Context> cr, pos_t p1, pos_t p2)
{
  if( !(has_infinity(p1) || has_infinity(p2)) ){
    cr->move_to(p1.x,-p1.y);
    cr->line_to(p2.x,-p2.y);
  }
}

class tascar_gui_t : public jackc_transport_t
{
public:
  tascar_gui_t(const std::string& srv_addr, 
               const std::string& srv_port,
               const std::string& cfg_file,
               const std::string& backend_flags,bool nobackend);
  ~tascar_gui_t();
  void open_scene();
  void set_time( double t ){time = t;};
  void set_scale(double s){draw.view.set_scale( s );};
  virtual int process(jack_nframes_t n,const std::vector<float*>& input,
                      const std::vector<float*>& output, 
                      uint32_t tp_frame, bool tp_rolling);
protected:
  virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
#ifdef GTKMM24
  virtual bool on_expose_event(GdkEventExpose* event);
#endif
  bool on_timeout();
  bool on_timeout_blink();
  void on_time_changed();
  void on_tp_start(){
    if( scene && (guitime >= scene->get_duration()) )
      tp_locate(0.0);
    tp_start();
  };
  void on_tp_stop(){tp_stop();};
  void on_tp_rewind();
  void on_tp_time_p(){tp_locate(guitime+0.1);};
  void on_tp_time_pp(){tp_locate(guitime+1.0);};
  void on_tp_time_m(){tp_locate(guitime-0.1);};
  void on_tp_time_mm(){tp_locate(guitime-1.0);};
  void on_reload();
  void on_view_p();
  void on_view_m();
  void on_range_selected();
  void on_view_selected();
  void on_loop();
  scene_draw_t draw;
  //viewport_t view;
  double time;
  double guitime;
  double headrot;
  Gtk::Button button_tp_stop;
  Gtk::Button button_tp_start;
  Gtk::Button button_tp_rewind;
  Gtk::Button button_tp_time_p;
  Gtk::Button button_tp_time_pp;
  Gtk::Button button_tp_time_m;
  Gtk::Button button_tp_time_mm;
  Gtk::Button button_reload;
  Gtk::Button button_view_p;
  Gtk::Button button_view_m;
  Gtk::ComboBoxText rangeselector;
  Gtk::ComboBoxText viewselector;
  Gtk::ToggleButton button_loop;
private:
  lo_address client_addr;
public:
  Gtk::DrawingArea wdg_scenemap;
  source_panel_t wdg_source;
  Gtk::VBox wdg_vertmain;
  Gtk::HBox wdg_source_and_map;
  Gtk::HBox wdg_transport_box;
  Gtk::HBox wdg_file_ui_box;
#ifdef GTKMM30
  Gtk::Scale timescale;
#else
  Gtk::HScale timescale;
#endif
private:
  pthread_mutex_t mtx_scene;
  session_t* scene;
  std::string srv_addr_;
  std::string srv_port_;
  std::string cfg_file_;
  std::string backend_flags_;
  std::vector<TASCAR::pos_t> roomnodes;
  bool blink;
  int32_t selected_range;
  bool nobackend_;
  scene_draw_t::viewt_t viewt;
};

void tascar_gui_t::on_tp_rewind()
{
  if( scene && (selected_range >= 0) && (selected_range<(int32_t)(scene->ranges.size())) ){
    tp_locate(scene->ranges[selected_range]->start);
  }else{
    tp_locate(0.0);
  }
}

void tascar_gui_t::on_time_changed()
{
  double ltime(timescale.get_value());
  if( ltime != guitime ){
    tp_locate(ltime);
  }
}

void tascar_gui_t::on_range_selected()
{
  int32_t nr(-1);
  if( scene ){
    std::string rg(rangeselector.get_active_text());
    
    for(unsigned int k=0;k<scene->ranges.size();k++)
      if( scene->ranges[k]->name == rg ){
        nr = k;
        tp_locate(scene->ranges[k]->start);
      }
  }
  selected_range = nr;
}

void tascar_gui_t::on_view_selected()
{
  std::string rg(viewselector.get_active_text());
  if( rg == "top" )
    viewt = scene_draw_t::xy;
  if( rg == "right" )
    viewt = scene_draw_t::xz;
  if( rg == "front" )
    viewt = scene_draw_t::yz;
  if( rg == "perspective" )
    viewt = scene_draw_t::p;
}

void tascar_gui_t::open_scene()
{
  pthread_mutex_lock( &mtx_scene );
  if( scene )
    delete scene;
  scene = NULL;
  draw.set_scene(NULL);
  timescale.clear_marks();
#ifdef GTKMM24
  rangeselector.clear();
  rangeselector.append_text("- scene -");
#else
  rangeselector.remove_all();
  rangeselector.append("- scene -");
#endif
  rangeselector.set_active_text("- scene -");
  selected_range = -1;
  if( cfg_file_.size() ){
    scene = new session_t(cfg_file_,TASCAR::xml_doc_t::LOAD_FILE,cfg_file_);
    if( scene->player.size() != 1 )
      throw TASCAR::ErrMsg("This program supports only a single scene.");
    timescale.set_range(0,scene->duration);
    for(unsigned int k=0;k<scene->ranges.size();k++){
      timescale.add_mark(scene->ranges[k]->start,Gtk::POS_BOTTOM,"");
      timescale.add_mark(scene->ranges[k]->end,Gtk::POS_BOTTOM,"");
#ifdef GTKMM24
      rangeselector.append_text(scene->ranges[k]->name);
#else
      rangeselector.append(scene->ranges[k]->name);
#endif
    }
    set_scale(scene->player[0]->guiscale);
    button_loop.set_active(scene->loop);
  }
  wdg_source.set_scene( (scene->player[0]) );
  draw.set_scene( (scene->player[0]) );
  pthread_mutex_unlock( &mtx_scene );
}

void tascar_gui_t::on_reload()
{
  open_scene( );
}

void tascar_gui_t::on_loop()
{
  if( pthread_mutex_trylock( &mtx_scene ) == 0 ){
    if( scene ){
      scene->loop = button_loop.get_active();
    }
    pthread_mutex_unlock( &mtx_scene );
  }
}

void tascar_gui_t::on_view_p()
{
  pthread_mutex_lock( &mtx_scene );
  if( scene ){
    scene->player[0]->guiscale /= 1.2;
    set_scale(scene->player[0]->guiscale);
  }
  pthread_mutex_unlock( &mtx_scene );
}

void tascar_gui_t::on_view_m()
{
  pthread_mutex_lock( &mtx_scene );
  if( scene ){
    scene->player[0]->guiscale *= 1.2;
    set_scale(scene->player[0]->guiscale);
  }
  pthread_mutex_unlock( &mtx_scene );
}

#define CON_BUTTON(b) button_ ## b.signal_clicked().connect(sigc::mem_fun(*this,&tascar_gui_t::on_ ## b))

tascar_gui_t::tascar_gui_t(const std::string& srv_addr, 
                           const std::string& srv_port,
                           const std::string& cfg_file,
                           const std::string& backend_flags,bool nobackend)
  : jackc_transport_t(cfg_file),
    time(0),
    guitime(0),
    button_tp_stop("stop"),
    button_tp_start("play"),
    button_tp_rewind("rew"),
    button_tp_time_p("+"),
  button_tp_time_pp("++"),
  button_tp_time_m("-"),
    button_tp_time_mm("--"),
    button_reload("reload"),
    button_view_p("zoom +"),
    button_view_m("zoom -"),
  button_loop("loop"),
  client_addr(lo_address_new(srv_addr.c_str(),srv_port.c_str())),
  wdg_source(client_addr),
#ifdef GTKMM30
  timescale(Gtk::ORIENTATION_HORIZONTAL),
#endif
  scene(NULL),
  srv_addr_(srv_addr),
  srv_port_(srv_port),
  cfg_file_(cfg_file),
  backend_flags_(backend_flags),
  blink(false),
  selected_range(-1),
  nobackend_(nobackend),
  viewt(scene_draw_t::xy)
{
  Glib::signal_timeout().connect( sigc::mem_fun(*this, &tascar_gui_t::on_timeout), 60 );
  Glib::signal_timeout().connect( sigc::mem_fun(*this, &tascar_gui_t::on_timeout_blink), 600 );
#ifdef GTKMM30
  wdg_scenemap.signal_draw().connect(sigc::mem_fun(*this, &tascar_gui_t::on_draw), false);
#else
  wdg_scenemap.signal_expose_event().connect(sigc::mem_fun(*this, &tascar_gui_t::on_expose_event), false);
#endif
  wdg_file_ui_box.pack_start( button_reload, Gtk::PACK_SHRINK );
  wdg_file_ui_box.pack_start( button_view_p, Gtk::PACK_SHRINK );
  wdg_file_ui_box.pack_start( button_view_m, Gtk::PACK_SHRINK );
  wdg_file_ui_box.pack_start( rangeselector, Gtk::PACK_SHRINK );
  wdg_file_ui_box.pack_start( button_loop, Gtk::PACK_SHRINK );
  wdg_file_ui_box.pack_start( viewselector, Gtk::PACK_SHRINK );
  wdg_transport_box.pack_start( button_tp_rewind, Gtk::PACK_SHRINK );
  wdg_transport_box.pack_start( button_tp_stop, Gtk::PACK_SHRINK );
  wdg_transport_box.pack_start( button_tp_start, Gtk::PACK_SHRINK );
  wdg_transport_box.pack_start( button_tp_time_mm, Gtk::PACK_SHRINK );
  wdg_transport_box.pack_start( button_tp_time_m, Gtk::PACK_SHRINK );
  wdg_transport_box.pack_start( timescale );
  wdg_transport_box.pack_start( button_tp_time_p, Gtk::PACK_SHRINK );
  wdg_transport_box.pack_start( button_tp_time_pp, Gtk::PACK_SHRINK );
  wdg_vertmain.pack_start( wdg_file_ui_box, Gtk::PACK_SHRINK);

  wdg_source_and_map.pack_start( wdg_source, Gtk::PACK_SHRINK );
  wdg_source_and_map.pack_start( wdg_scenemap );

  wdg_vertmain.pack_start( wdg_source_and_map );
  wdg_vertmain.pack_start( wdg_transport_box, Gtk::PACK_SHRINK );
  timescale.set_range(0,100);
  timescale.set_value(0);
  timescale.signal_value_changed().connect(sigc::mem_fun(*this,
                                                         &tascar_gui_t::on_time_changed));
  rangeselector.signal_changed().connect(sigc::mem_fun(*this,
                                                       &tascar_gui_t::on_range_selected));
  viewselector.signal_changed().connect(sigc::mem_fun(*this,
                                                      &tascar_gui_t::on_view_selected));
  CON_BUTTON(tp_rewind);
  CON_BUTTON(tp_start);
  CON_BUTTON(tp_stop);
  CON_BUTTON(tp_time_p);
  CON_BUTTON(tp_time_pp);
  CON_BUTTON(tp_time_m);
  CON_BUTTON(tp_time_mm);
  CON_BUTTON(reload);
  CON_BUTTON(view_p);
  CON_BUTTON(view_m);
  CON_BUTTON(loop);
#ifdef GTKMM30
  Gdk::RGBA col;
  col.set_rgba_u(27*256,249*256,163*256);
  button_loop.override_background_color(col,Gtk::STATE_FLAG_ACTIVE);
#else
  Gdk::Color col;
  col.set_rgb(27*256,249*256,163*256);
  button_loop.modify_bg(Gtk::STATE_ACTIVE,col);
  button_loop.modify_bg(Gtk::STATE_PRELIGHT,col);
  button_loop.modify_bg(Gtk::STATE_SELECTED,col);
#endif
  pthread_mutex_init( &mtx_scene, NULL );
  if( cfg_file_.size() )
    open_scene();
#ifdef GTKMM24
  viewselector.append_text("top");
  viewselector.append_text("front");
  viewselector.append_text("right");
  viewselector.append_text("perspective");
#else
  viewselector.append("top");
  viewselector.append("front");
  viewselector.append("right");
  viewselector.append("perspective");
#endif
}

tascar_gui_t::~tascar_gui_t()
{
  pthread_mutex_trylock( &mtx_scene );
  pthread_mutex_unlock(  &mtx_scene);
  pthread_mutex_destroy( &mtx_scene );
  if( scene )
    delete scene;
}

#ifdef GTKMM24
bool tascar_gui_t::on_expose_event(GdkEventExpose* event)
{
  if( event ){
    // This is where we draw on the window
    Glib::RefPtr<Gdk::Window> window = wdg_scenemap.get_window();
    if(window){
      Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context();
      return on_draw(cr);
    }
  }
  return true;
}
#endif


bool tascar_gui_t::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  pthread_mutex_lock( &mtx_scene );
  try{
    Glib::RefPtr<Gdk::Window> window = wdg_scenemap.get_window();
    int mp_x(0);
    int mp_y(0);
    if(window && scene){
      Gtk::Allocation allocation = wdg_scenemap.get_allocation();
      const int width = allocation.get_width();
      const int height = allocation.get_height();
      wdg_scenemap.get_pointer(mp_x,mp_y);
      cr->rectangle(0,0,width,height);
      cr->clip();
      cr->translate(0.5*width, 0.5*height);
      double wscale(0.5*std::max(height,width));
      double markersize(0.02);
      cr->scale( wscale, wscale );
      cr->set_line_width( 0.3*markersize );
      cr->set_font_size( 2*markersize );
      cr->save();
      cr->set_source_rgb( 1, 1, 1 );
      cr->paint();
      cr->restore();
      draw.set_time(time);
      draw.set_viewport(viewt);
      draw.draw(cr);
      if( (mp_x > 0) && (mp_x < width ) && (mp_y > 0) && (mp_y < height) ){
        pos_t mp(mp_x,mp_y,0.0);
        mp -= pos_t(0.5*width,0.5*height,0.0);
        mp *= 0.5*draw.view.get_scale()/wscale;
        mp += draw.view.get_ref();
        cr->save();
        cr->move_to( -0.9,0.9 );
        char cmp[1024];
        sprintf(cmp,"%1.1f | %1.1f",mp.x,-mp.y);
        cr->set_source_rgb( 0, 0, 0 );
        cr->show_text( cmp );
        cr->stroke();
        cr->restore();
      }
      cr->set_source_rgba(0.2, 0.2, 0.2, 0.8);
      cr->move_to(-markersize, 0 );
      cr->line_to( markersize, 0 );
      cr->move_to( 0, -markersize );
      cr->line_to( 0,  markersize );
      cr->stroke();
    }
    guitime = time;
    timescale.set_value(guitime);
    pthread_mutex_unlock( &mtx_scene );
  }
  catch(...){
    pthread_mutex_unlock( &mtx_scene );
    throw;
  }
  return true;
}

bool tascar_gui_t::on_timeout()
{
  Glib::RefPtr<Gdk::Window> win = wdg_scenemap.get_window();
  if (win){
    Gdk::Rectangle r(0,0, 
		     wdg_scenemap.get_allocation().get_width(),
		     wdg_scenemap.get_allocation().get_height() );
    win->invalidate_rect(r, true);
  }
  return true;
}

bool tascar_gui_t::on_timeout_blink()
{
  if( blink )
    blink = false;
  else
    blink = true;
  draw.set_blink(blink);
  return true;
}

int tascar_gui_t::process(jack_nframes_t nframes,
                          const std::vector<float*>& inBuffer,
                          const std::vector<float*>& outBuffer, 
                          uint32_t tp_frame, bool tp_rolling)
{
  set_time((double)tp_frame/get_srate());
  if( scene ){
    if( (selected_range >= 0) && (selected_range < (int32_t)(scene->ranges.size())) ){
      if( (scene->ranges[selected_range]->end <= time) && 
          (scene->ranges[selected_range]->end + (double)nframes/(double)srate > time) ){
        if( scene->loop ){
          tp_locate(scene->ranges[selected_range]->start);
        }else
          tp_stop();
      }
    }else{
      if( (scene->duration <= time) && (scene->duration + (double)nframes/(double)srate > time) ){
        if( scene->loop ){
          tp_locate(0.0);
        }else
          tp_stop();
      }
    }
  }
  return 0;
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_gui -c configfile [options]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

int main(int argc, char** argv)
{
  Gtk::Main kit(argc, argv);
  Gtk::Window win;
  std::string cfg_file("");
  std::string srv_addr("239.255.1.7");
  std::string srv_port("9877");
  bool nobackend(false);
#ifdef LINUXTRACK
  bool use_ltr(false);
  const char *options = "c:hj:p:a:nl";
#else
  const char *options = "c:hj:p:a:n";
#endif
  struct option long_options[] = { 
    { "config",   1, 0, 'c' },
    { "help",     0, 0, 'h' },
    { "jackname", 1, 0, 'j' },
    { "srvaddr",  1, 0, 'a' },
    { "srvport",  1, 0, 'p' },
    { "nobackend",0, 0, 'n' },
#ifdef LINUXTRACK
    { "linuxtrack", 0, 0, 'l'},
#endif
    { 0, 0, 0, 0 }
  };
  int opt(0);
  int option_index(0);
  while( (opt = getopt_long(argc, argv, options,
                            long_options, &option_index)) != -1){
    switch(opt){
    case 'c':
      cfg_file = optarg;
      break;
    case 'h':
      usage(long_options);
      return -1;
    case 'p':
      srv_port = optarg;
      break;
    case 'a':
      srv_addr = optarg;
      break;
    case 'n':
      nobackend = true;
      break;
#ifdef LINUXTRACK
    case 'l':
      use_ltr = true;
      break;
#endif
    }
  }
  if( cfg_file.size() == 0 ){
    usage(long_options);
    return -1;
  }
  win.set_title("tascar - "+cfg_file);
#ifdef LINUXTRACK
  tascar_gui_t c(srv_addr,srv_port,cfg_file,use_ltr?"-l":"",nobackend);
#else
  tascar_gui_t c(srv_addr,srv_port,cfg_file,"",nobackend);
#endif
  win.add(c.wdg_vertmain);
  win.set_default_size(1024,768);
  win.show_all();
  c.activate();
  Gtk::Main::run(win);
  c.deactivate();
  return 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
