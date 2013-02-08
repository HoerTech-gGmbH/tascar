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

using namespace TASCAR;

class tascar_draw_gtkmm_t : public Gtk::DrawingArea, public scene_t
{
public:
  tascar_draw_gtkmm_t(const std::string& name);
  void set_time( double t ){time = t;};
  void set_scale(double s){scale = s;};
  void draw_track(const object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  void draw_src(const src_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  void draw_listener(const listener_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
protected:
  virtual bool on_expose_event(GdkEventExpose* event);
  bool on_timeout();
  double scale;
  double time;
};

void tascar_draw_gtkmm_t::draw_track(const object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  cr->save();
  cr->set_source_rgb(obj.color.r, obj.color.g, obj.color.b );
  cr->set_line_width( 0.1*msize );
  for( TASCAR::track_t::const_iterator it=obj.location.begin();it!=obj.location.end();++it){
    if( it==obj.location.begin() )
      cr->move_to( it->second.x, it->second.y );
    else
      cr->line_to( it->second.x, it->second.y );
  }
  cr->stroke();
  cr->restore();
}

void tascar_draw_gtkmm_t::draw_src(const src_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  pos_t p(obj.location.interp(time-obj.starttime));
  cr->save();
  cr->set_source_rgba(obj.color.r, obj.color.g, obj.color.b, 0.6);
  cr->arc(p.x, p.y, msize, 0, PI2 );
  cr->fill();
  for(unsigned int k=0;k<obj.sound.size();k++){
    pos_t ps(obj.sound[k].get_pos_global(time));
    cr->arc(ps.x, ps.y, 0.5*msize, 0, PI2 );
    cr->fill();
  }
  cr->set_source_rgb(0, 0, 0 );
  cr->move_to( p.x + 1.1*msize, p.y );
  cr->show_text( obj.name.c_str() );
  cr->set_line_width( 0.1*msize );
  cr->set_source_rgba(obj.color.r, obj.color.g, obj.color.b, 0.6);
  for(unsigned int k=0;k<obj.sound.size();k++){
    cr->move_to( p.x, p.y );
    pos_t ps(obj.sound[k].get_pos_global(time));
    cr->line_to( ps.x, ps.y );
  }
  cr->stroke();
  cr->restore();
}

void tascar_draw_gtkmm_t::draw_listener(const listener_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  pos_t p(obj.location.interp(time-obj.starttime));
  zyx_euler_t o(obj.orientation.interp(time-obj.starttime));
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
  cr->arc(p.x, p.y, 2*msize, 0, PI2 );
  cr->move_to( p1.x, p1.y );
  cr->line_to( p2.x, p2.y );
  cr->line_to( p3.x, p3.y );
  cr->move_to( p4.x, p4.y );
  cr->line_to( p5.x, p5.y );
  cr->line_to( p6.x, p6.y );
  cr->move_to( p7.x, p7.y );
  cr->line_to( p8.x, p8.y );
  cr->line_to( p9.x, p9.y );
  //cr->fill();
  cr->stroke();
  cr->set_source_rgb(0, 0, 0 );
  cr->move_to( p.x + 3.1*msize, p.y );
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

tascar_draw_gtkmm_t::tascar_draw_gtkmm_t(const std::string& name)
  : scale(200)
{
  Glib::signal_timeout().connect( sigc::mem_fun(*this, &tascar_draw_gtkmm_t::on_timeout), 60 );
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
  //Connect the signal handler if it isn't already a virtual method override:
  signal_expose_event().connect(sigc::mem_fun(*this, &tascar_draw_gtkmm_t::on_expose_event), false);
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

bool tascar_draw_gtkmm_t::on_expose_event(GdkEventExpose* event)
{
  Glib::RefPtr<Gdk::Window> window = get_window();
  if(window){
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();
    Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context();
    if(event){
      // clip to the area indicated by the expose event so that we only
      // redraw the portion of the window that needs to be redrawn
      cr->rectangle(event->area.x, event->area.y,
		    event->area.width, event->area.height);
      cr->clip();
    }

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
    draw_track( listener, cr, markersize );
    for(unsigned int k=0;k<src.size();k++){
      draw_track(src[k], cr, markersize );
    }
    draw_listener( listener, cr, markersize );
    cr->set_source_rgba(0.2, 0.2, 0.2, 0.8);
    cr->move_to(-markersize, 0 );
    cr->line_to( markersize, 0 );
    cr->move_to( 0, -markersize );
    cr->line_to( 0,  markersize );
    cr->stroke();
    for(unsigned int k=0;k<src.size();k++){
      draw_src(src[k], cr, markersize );
    }
  }
  return true;
}

bool tascar_draw_gtkmm_t::on_timeout()
{
  Glib::RefPtr<Gdk::Window> win = get_window();
  if (win){
    Gdk::Rectangle r(0, 0, 
		     get_allocation().get_width(),
		     get_allocation().get_height() );
    win->invalidate_rect(r, false);
  }
  return true;
}

class tascar_draw_t : public tascar_draw_gtkmm_t, public jackc_transport_t
{
public:
  tascar_draw_t(const std::string& fname);
  virtual int process(jack_nframes_t n,const std::vector<float*>& input,
                      const std::vector<float*>& output, 
                      uint32_t tp_frame, bool tp_rolling);
};

tascar_draw_t::tascar_draw_t(const std::string& fname)
  : tascar_draw_gtkmm_t(fname),
    jackc_transport_t(fname)
{
  read_xml(fname);
  linearize();
}

int tascar_draw_t::process(jack_nframes_t nframes,
                           const std::vector<float*>& inBuffer,
                           const std::vector<float*>& outBuffer, 
                           uint32_t tp_frame, bool tp_rolling)
{
  set_time((double)tp_frame/get_srate());
  return 0;
}

int main(int argc, char** argv)
{
  Gtk::Main kit(argc, argv);
  Gtk::Window win;
  std::string fname("tascar_draw.xml");
  if( argc > 1 )
    fname = argv[1];
  win.set_title("tascar - "+fname);
  tascar_draw_t c(fname);
  win.add(c);
  win.set_default_size(1024,768);
  c.set_scale(c.guiscale);
  //win.fullscreen();
  win.show_all();
  c.jackc_t::activate();
  Gtk::Main::run(win);
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
