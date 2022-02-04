/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
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

#include <iostream>
#include "session.h"
#include "errorhandling.h"
//#include <gdkmm/event.h>
#include <gtkmm.h>
#include <gtkmm/builder.h>
#include <gtkmm/window.h>

class tracegui_t : public TASCAR::actor_module_t {
public:
  tracegui_t( const TASCAR::module_cfg_t& cfg );
  virtual ~tracegui_t();
  void update( uint32_t frame, bool running );
  virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
  bool on_timeout();
private:
  bool ontop;
  double tracelen;
  double fps;
  double guiscale;
  double linescale;
  bool unitcircle;
  bool origin;
  double orientation;
  uint32_t itracelen;
  Gtk::Window win;
  Gtk::DrawingArea map;
  sigc::connection connection_timeout;
  std::vector<std::vector<TASCAR::pos_t> > trace;
  uint32_t pos;
  std::vector<TASCAR::Scene::rgb_color_t> vcol;
  pthread_mutex_t drawlock;
};

tracegui_t::tracegui_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t(cfg,true),
    ontop(false),
    tracelen(4),
    fps(10),
    guiscale(10),
    linescale(1),
    unitcircle(true),
    origin(true),
    orientation(0.0),
    itracelen(40),
    pos(0)
{
  pthread_mutex_init(&drawlock,NULL);
  GET_ATTRIBUTE_(tracelen);
  GET_ATTRIBUTE_(fps);
  GET_ATTRIBUTE_(guiscale);
  GET_ATTRIBUTE_(linescale);
  GET_ATTRIBUTE_DEG(orientation,"canvas orientation");
  GET_ATTRIBUTE_BOOL_(unitcircle);
  GET_ATTRIBUTE_BOOL_(origin);
  GET_ATTRIBUTE_BOOL_(ontop);
  session->add_double("/tracegui/guiscale",&guiscale);
  session->add_double("/tracegui/linescale",&linescale);
  //session->add_bool("/tracegui/unitcircle",&unitcircle);
  //session->add_bool("/tracegui/origin",&origin);
  itracelen = fps*tracelen;
  win.set_title("tascar trace");
  win.set_keep_above(ontop);
  win.add(map);
  win.show();
  map.show();
  trace.resize(obj.size());
  for(uint32_t k=0;k<obj.size();++k){
    trace[k].resize(itracelen);
    vcol.push_back(obj[k].obj->color);
  }
  map.signal_draw().connect( sigc::mem_fun(*this,&tracegui_t::on_draw) );
  connection_timeout = Glib::signal_timeout().connect( sigc::mem_fun(*this, &tracegui_t::on_timeout), 1000.0/fps );
  int x(0);
  int y(0);
  int w(1);
  int h(1);
  win.get_position(x,y);
  win.get_size(w,h);
  GET_ATTRIBUTE_(x);
  GET_ATTRIBUTE_(y);
  GET_ATTRIBUTE_(w);
  GET_ATTRIBUTE_(h);
  win.move(x,y);
  win.resize(w,h);
}

bool tracegui_t::on_timeout()
{
  Glib::RefPtr<Gdk::Window> lwin(map.get_window());
  if(lwin){
    Gdk::Rectangle r(0, 0,
                     map.get_allocation().get_width(),
                     map.get_allocation().get_height());
    lwin->invalidate_rect(r, false);
  }
  return true;
}

void tracegui_t::update(uint32_t frame, bool running)
{
  if( pthread_mutex_trylock(&drawlock) == 0 ){
    for(uint32_t k=0;k<obj.size();++k){
      trace[k][pos] = obj[k].obj->get_location();
    }
    pthread_mutex_unlock(&drawlock);
  }
}

bool tracegui_t::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  if( pthread_mutex_trylock(&drawlock) == 0 ){
    Gtk::Allocation allocation(map.get_allocation());
    const int width(allocation.get_width());
    const int height(allocation.get_height());
    cr->save();
    cr->set_source_rgb( 1.0, 1.0, 1.0 );
    cr->paint();
    cr->translate(0.5*width, 0.5*height);
    double wscale(0.5*std::max(height,width));
    wscale /= guiscale;
    double mw(0.1*guiscale*linescale);
    cr->scale( wscale, wscale );
    if( orientation != 0 )
      cr->rotate(orientation);
    cr->set_source_rgb( 0.7, 0.7, 0.7 );
    if( origin ){
      cr->set_line_width( 0.075*mw );
      cr->move_to( -0.3*mw, 0 );
      cr->line_to( 0.3*mw, 0 );
      cr->move_to( 0, 0.3*mw );
      cr->line_to( 0, -0.3*mw );
      cr->stroke();
    }
    if( unitcircle ){
      cr->set_line_width( 0.075*mw );
      cr->move_to( 1, 0 );
      cr->arc(0, 0, 1, 0, TASCAR_2PI );
      cr->stroke();
    }
    // draw traces
    for(uint32_t k=0;k<trace.size();++k){
      TASCAR::Scene::rgb_color_t col(vcol[k]);
      for(uint32_t l=1;l<itracelen-1;++l){
	TASCAR::pos_t p1(trace[k][(pos+l) % itracelen]);
	TASCAR::pos_t p2(trace[k][(pos+l+1) % itracelen]);
	cr->set_source_rgba( col.r, col.g, col.b, (double)l/(double)itracelen );
	cr->move_to( p1.x, -p1.y );
	cr->line_to( p2.x, -p2.y );
	cr->stroke();
      }
      cr->set_source_rgb( col.r, col.g, col.b );
      TASCAR::pos_t p(trace[k][pos]);
      cr->arc(p.x, -p.y, 0.3*mw, 0, TASCAR_2PI );
      cr->fill();
    }
    cr->restore();
    pos = (pos+1) % itracelen;
    pthread_mutex_unlock(&drawlock);
  }
  return true;
}


tracegui_t::~tracegui_t()
{
  connection_timeout.disconnect();
  pthread_mutex_destroy(&drawlock);
}

REGISTER_MODULE(tracegui_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */

