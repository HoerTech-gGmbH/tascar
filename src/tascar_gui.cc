/**
   \file tascar_draw.cc
   \ingroup apptascar
   \brief Draw coordinates delivered via jack
   \author Giso Grimm
   \date 2012

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
#include "osc_helper.h"
#include <iostream>
#include <getopt.h>

using namespace TASCAR;

class g_scene_t : public scene_t {
public:
  g_scene_t(const std::string& n);
  ~g_scene_t();
private:
  FILE* h_pipe;
};

g_scene_t::g_scene_t(const std::string& n)
 : h_pipe(NULL)
{
  read_xml(n);
  char ctmp[1024];
  sprintf(ctmp,"tascar_renderer -c %s",n.c_str());
  h_pipe = popen( ctmp, "w" );
  if( !h_pipe )
    throw ErrMsg("Unable to open renderer pipe (tascar_renderer -c <filename>).");
  linearize();
}

g_scene_t::~g_scene_t()
{
  pclose( h_pipe );
}


//class tascar_gui_t : public Gtk::DrawingArea, public scene_t, public osc_server_t
class tascar_gui_t : public osc_server_t, public jackc_transport_t
{
public:
  tascar_gui_t(const std::string& name,const std::string& oscport);
  ~tascar_gui_t();
  void open_scene(const std::string& name);
  void set_time( double t ){time = t;};
  void set_scale(double s){scale = s;};
  void draw_track(const object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  void draw_src(const src_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  void draw_listener(const listener_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  static int set_head(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void set_head(double rot){headrot = rot;};
  virtual int process(jack_nframes_t n,const std::vector<float*>& input,
                      const std::vector<float*>& output, 
                      uint32_t tp_frame, bool tp_rolling);
protected:
  virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
#ifdef GTKMM24
  virtual bool on_expose_event(GdkEventExpose* event);
#endif
  bool on_timeout();
  void on_time_changed();
  void on_tp_start(){tp_start();};
  void on_tp_stop(){tp_stop();};
  void on_tp_rewind(){tp_locate(0.0);};
  void on_tp_time_p(){tp_locate(guitime+0.1);};
  void on_tp_time_pp(){tp_locate(guitime+1.0);};
  void on_tp_time_m(){tp_locate(guitime-0.1);};
  void on_tp_time_mm(){tp_locate(guitime-1.0);};
  void on_reload();
  void on_view_p();
  void on_view_m();
  double scale;
  double time;
  double guitime;
  double headrot;
  //Gtk::HBox hbox;
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
public:
  Gtk::DrawingArea da;
  Gtk::VBox vbox;
  Gtk::HBox tp_box;
  Gtk::HBox ctl_box;
#ifdef GTKMM30
  Gtk::Scale timescale;
#else
  Gtk::HScale timescale;
#endif
private:
  pthread_mutex_t mtx_scene;
  g_scene_t* scene;
  std::string filename;
};

void tascar_gui_t::on_time_changed()
{
  double ltime(timescale.get_value());
  if( ltime != guitime ){
    tp_locate(ltime);
  }
}

void tascar_gui_t::draw_track(const object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  cr->save();
  cr->set_source_rgb(obj.color.r, obj.color.g, obj.color.b );
  cr->set_line_width( 0.1*msize );
  for( TASCAR::track_t::const_iterator it=obj.location.begin();it!=obj.location.end();++it){
    if( it==obj.location.begin() )
      cr->move_to( it->second.x, -it->second.y );
    else
      cr->line_to( it->second.x, -it->second.y );
  }
  cr->stroke();
  cr->restore();
}

void tascar_gui_t::draw_src(const src_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  bool active(obj.isactive(time));
  if( !active )
    msize *= 0.4;
  pos_t p(obj.location.interp(time-obj.starttime));
  cr->save();
  cr->set_source_rgba(obj.color.r, obj.color.g, obj.color.b, 0.6);
  cr->arc(p.x, -p.y, msize, 0, PI2 );
  cr->fill();
  for(unsigned int k=0;k<obj.sound.size();k++){
    pos_t ps(obj.sound[k].get_pos_global(time));
    cr->arc(ps.x, -ps.y, 0.5*msize, 0, PI2 );
    cr->fill();
  }
  if( !active )
    cr->set_source_rgb(0.5, 0.5, 0.5 );
  else
    cr->set_source_rgb(0, 0, 0 );
  cr->move_to( p.x + 1.1*msize, -p.y );
  cr->show_text( obj.name.c_str() );
  if( active ){
    cr->set_line_width( 0.1*msize );
    cr->set_source_rgba(obj.color.r, obj.color.g, obj.color.b, 0.6);
    for(unsigned int k=0;k<obj.sound.size();k++){
      cr->move_to( p.x, -p.y );
      pos_t ps(obj.sound[k].get_pos_global(time));
      cr->line_to( ps.x, -ps.y );
    }
    cr->stroke();
  }
  cr->restore();
}

void tascar_gui_t::open_scene(const std::string& name)
{
  pthread_mutex_lock( &mtx_scene );
  if( scene )
    delete scene;
  scene = NULL;
  if( name.size() ){
    scene = new g_scene_t(name);
    timescale.set_range(0,scene->duration);
    set_scale(scene->guiscale);
  }
  pthread_mutex_unlock( &mtx_scene );
}

void tascar_gui_t::on_reload()
{
  open_scene( filename );
}

void tascar_gui_t::on_view_p()
{
  pthread_mutex_lock( &mtx_scene );
  if( scene ){
    scene->guiscale /= 1.2;
    set_scale(scene->guiscale);
  }
  pthread_mutex_unlock( &mtx_scene );
}

void tascar_gui_t::on_view_m()
{
  pthread_mutex_lock( &mtx_scene );
  if( scene ){
    scene->guiscale *= 1.2;
    set_scale(scene->guiscale);
  }
  pthread_mutex_unlock( &mtx_scene );
}

void tascar_gui_t::draw_listener(const listener_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  pos_t p(obj.location.interp(time-obj.starttime));
  zyx_euler_t o(obj.orientation.interp(time-obj.starttime));
  o.z += headrot;
  pos_t p1(1.8*msize,-0.6*msize,0);
  pos_t p2(2.9*msize,0,0);
  pos_t p3(1.8*msize,0.6*msize,0);
  pos_t p4(-0.5*msize,2.3*msize,0);
  pos_t p5(0,1.7*msize,0);
  pos_t p6(0.5*msize,2.3*msize,0);
  pos_t p7(-0.5*msize,-2.3*msize,0);
  pos_t p8(0,-1.7*msize,0);
  pos_t p9(0.5*msize,-2.3*msize,0);
  p1 *= o;
  p2 *= o;
  p3 *= o;
  p4 *= o;
  p5 *= o;
  p6 *= o;
  p7 *= o;
  p8 *= o;
  p9 *= o;
  p1+=p;
  p2+=p;
  p3+=p;
  p4+=p;
  p5+=p;
  p6+=p;
  p7+=p;
  p8+=p;
  p9+=p;
  cr->save();
  cr->set_line_width( 0.1*msize );
  cr->set_source_rgba(obj.color.r, obj.color.g, obj.color.b, 0.6);
  cr->arc(p.x, -p.y, 2*msize, 0, PI2 );
  cr->move_to( p1.x, -p1.y );
  cr->line_to( p2.x, -p2.y );
  cr->line_to( p3.x, -p3.y );
  cr->move_to( p4.x, -p4.y );
  cr->line_to( p5.x, -p5.y );
  cr->line_to( p6.x, -p6.y );
  cr->move_to( p7.x, -p7.y );
  cr->line_to( p8.x, -p8.y );
  cr->line_to( p9.x, -p9.y );
  //cr->fill();
  cr->stroke();
  cr->set_source_rgb(0, 0, 0 );
  cr->move_to( p.x + 3.1*msize, -p.y );
  cr->show_text( obj.name.c_str() );
  //cr->set_line_width( 0.1*msize );
  //cr->set_source_rgba(obj.color.r, obj.color.g, obj.color.b, 0.6);
  //for(unsigned int k=0;k<obj.sound.size();k++){
  //  cr->move_to( p.x, p.y );
  //  pos_t ps(obj.sound[k].get_pos_global(time));
  //  cr->line_to( ps.x, ps.y );
  //}
  //cr->stroke();
  cr->restore();
}

int tascar_gui_t::set_head(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  tascar_gui_t* h((tascar_gui_t*)user_data);
  if( h && (argc == 1) && (types[0] == 'f') ){
    h->set_head(DEG2RAD*(argv[0]->f));
    return 0;
  }
  return 1;
}

#define CON_BUTTON(b) button_ ## b.signal_clicked().connect(sigc::mem_fun(*this,&tascar_gui_t::on_ ## b))

tascar_gui_t::tascar_gui_t(const std::string& name, const std::string& oscport)
  : osc_server_t("",oscport,true),
    jackc_transport_t(name),
    scale(200),
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
#ifdef GTKMM30
    timescale(Gtk::ORIENTATION_HORIZONTAL),
#endif
    scene(NULL),
    filename(name)
{
  Glib::signal_timeout().connect( sigc::mem_fun(*this, &tascar_gui_t::on_timeout), 60 );
#ifdef GTKMM30
  da.signal_draw().connect(sigc::mem_fun(*this, &tascar_gui_t::on_draw), false);
#else
  da.signal_expose_event().connect(sigc::mem_fun(*this, &tascar_gui_t::on_expose_event), false);
#endif
  osc_server_t::add_method("/headrot","f",set_head,this);
  ctl_box.pack_start( button_reload, Gtk::PACK_SHRINK );
  ctl_box.pack_start( button_view_p, Gtk::PACK_SHRINK );
  ctl_box.pack_start( button_view_m, Gtk::PACK_SHRINK );
  vbox.pack_start( ctl_box, Gtk::PACK_SHRINK);
  vbox.pack_start( da );
  timescale.set_range(0,100);
  timescale.set_value(50);
  tp_box.pack_start( button_tp_rewind, Gtk::PACK_SHRINK );
  tp_box.pack_start( button_tp_stop, Gtk::PACK_SHRINK );
  tp_box.pack_start( button_tp_start, Gtk::PACK_SHRINK );
  tp_box.pack_start( button_tp_time_mm, Gtk::PACK_SHRINK );
  tp_box.pack_start( button_tp_time_m, Gtk::PACK_SHRINK );
  tp_box.pack_start( timescale );
  tp_box.pack_start( button_tp_time_p, Gtk::PACK_SHRINK );
  tp_box.pack_start( button_tp_time_pp, Gtk::PACK_SHRINK );
  vbox.pack_start( tp_box, Gtk::PACK_SHRINK );
  timescale.signal_value_changed().connect(sigc::mem_fun(*this,
                                                         &tascar_gui_t::on_time_changed));
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
  pthread_mutex_init( &mtx_scene, NULL );
  open_scene(name);
}

tascar_gui_t::~tascar_gui_t()
{
  pthread_mutex_trylock( &mtx_scene );
  pthread_mutex_unlock(  &mtx_scene);
  pthread_mutex_destroy( &mtx_scene );
}

#ifdef GTKMM24
bool tascar_gui_t::on_expose_event(GdkEventExpose* event)
{
  if( event ){
    // This is where we draw on the window
    Glib::RefPtr<Gdk::Window> window = da.get_window();
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
    Glib::RefPtr<Gdk::Window> window = da.get_window();
    if(window && scene){
      Gtk::Allocation allocation = da.get_allocation();
      const int width = allocation.get_width();
      const int height = allocation.get_height();
      cr->rectangle(0,0,width,height);
      cr->clip();
      cr->translate(0.5*width, 0.5*height);
      double wscale(0.5*std::min(height,width)/scale);
      double markersize(0.02*scale);
      cr->scale( wscale, wscale );
      cr->set_line_width( 0.3*markersize );
      cr->set_font_size( 2*markersize );

      cr->save();
      cr->set_source_rgb( 1, 1, 1 );
      cr->paint();
      cr->restore();
      draw_track( scene->listener, cr, markersize );
      for(unsigned int k=0;k<scene->src.size();k++){
        draw_track(scene->src[k], cr, markersize );
      }
      draw_listener( scene->listener, cr, markersize );
      cr->set_source_rgba(0.2, 0.2, 0.2, 0.8);
      cr->move_to(-markersize, 0 );
      cr->line_to( markersize, 0 );
      cr->move_to( 0, -markersize );
      cr->line_to( 0,  markersize );
      cr->stroke();
      for(unsigned int k=0;k<scene->src.size();k++){
        draw_src(scene->src[k], cr, markersize );
      }
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
  Glib::RefPtr<Gdk::Window> win = da.get_window();
  //DEBUG(win);
  if (win){
    Gdk::Rectangle r(da.get_allocation().get_x(), da.get_allocation().get_y(), 
		     da.get_allocation().get_width(),
		     da.get_allocation().get_height() );
    win->invalidate_rect(r, true);
  }
  return true;
}

int tascar_gui_t::process(jack_nframes_t nframes,
                          const std::vector<float*>& inBuffer,
                          const std::vector<float*>& outBuffer, 
                          uint32_t tp_frame, bool tp_rolling)
{
  set_time((double)tp_frame/get_srate());
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
  std::string cfgfile("");
  std::string oscport("9876");
  const char *options = "c:hp:";
  struct option long_options[] = { 
    { "config",   1, 0, 'c' },
    { "help",     0, 0, 'h' },
    { "oscport",  1, 0, 'p' },
    { 0, 0, 0, 0 }
  };
  int opt(0);
  int option_index(0);
  while( (opt = getopt_long(argc, argv, options,
                            long_options, &option_index)) != -1){
    switch(opt){
    case 'c':
      cfgfile = optarg;
      break;
    case 'h':
      usage(long_options);
      return -1;
    case 'n':
     oscport = optarg;
     break;
    }
  }
  if( cfgfile.size() == 0 ){
    usage(long_options);
    return -1;
  }
  win.set_title("tascar - "+cfgfile);
  tascar_gui_t c(cfgfile,oscport);
  win.add(c.vbox);
  win.set_default_size(1024,768);
  //c.set_scale(200);
  //c.set_scale(c.guiscale);
  //win.fullscreen();
  win.show_all();
  c.jackc_t::activate();
  c.osc_server_t::activate();
  Gtk::Main::run(win);
  c.osc_server_t::deactivate();
  c.jackc_t::deactivate();
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
