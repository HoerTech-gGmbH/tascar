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

class g_scene_t : public osc_scene_t {
public:
  g_scene_t(const std::string& cfg_file, const std::string& flags,bool nobackend, 
            const std::string& srv_addr, const std::string& srv_port);
  ~g_scene_t();
private:
  FILE* h_pipe;
};

g_scene_t::g_scene_t(const std::string& cfg_file, const std::string& flags,bool nobackend, 
                     const std::string& srv_addr, const std::string& srv_port)
  : osc_scene_t(srv_addr,srv_port,cfg_file),
    h_pipe(NULL)
{
  if( !nobackend ){
    char ctmp[1024];
    sprintf(ctmp,"tascar_renderer %s -c %s 2>&1",flags.c_str(),cfg_file.c_str());
    h_pipe = popen( ctmp, "w" );
    if( !h_pipe )
      throw ErrMsg("Unable to open renderer pipe (tascar_renderer -c <filename>).");
  }
  linearize_sounds();
  for( std::vector<src_object_t>::iterator i=object_sources.begin();i!=object_sources.end();++i)
    i->location.fill_gaps(0.25);
  for( std::vector<sink_object_t>::iterator i=sink_objects.begin();i!=sink_objects.end();++i)
    i->location.fill_gaps(0.25);
  add_child_methods();
  activate();
}

g_scene_t::~g_scene_t()
{
  deactivate();
  if( h_pipe )
    pclose( h_pipe );
}

class tascar_gui_t : public jackc_transport_t
{
public:
  enum viewt_t {
    top,
    front,
    right,
    perspective
  };
  tascar_gui_t(const std::string& srv_addr, 
               const std::string& srv_port,
               const std::string& cfg_file,
               const std::string& backend_flags,bool nobackend);
  ~tascar_gui_t();
  void open_scene();
  void set_time( double t ){time = t;};
  void set_scale(double s){draw.view.set_scale( s );};
  //void draw_face_normal(const face_t& f, Cairo::RefPtr<Cairo::Context> cr, double normalsize=0.0);
  //void draw_cube(pos_t pos, zyx_euler_t orient, pos_t size,Cairo::RefPtr<Cairo::Context> cr);
  //void draw_track(const object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  //void draw_src(const src_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  //void draw_sink_object(const sink_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  //void draw_door_src(const src_door_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  //void draw_room_src(const src_diffuse_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  //void draw_face(const face_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  //void draw_mask(mask_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
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
    if( scene && (guitime >= scene->duration) )
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
  g_scene_t* scene;
  std::string srv_addr_;
  std::string srv_port_;
  std::string cfg_file_;
  std::string backend_flags_;
  std::vector<TASCAR::pos_t> roomnodes;
  bool blink;
  int32_t selected_range;
  bool nobackend_;
  viewt_t viewt;
};

void tascar_gui_t::on_tp_rewind()
{
  if( scene && (selected_range >= 0) && (selected_range<(int32_t)(scene->ranges.size())) ){
    tp_locate(scene->ranges[selected_range].start);
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
      if( scene->ranges[k].name == rg ){
        nr = k;
        tp_locate(scene->ranges[k].start);
      }
  }
  selected_range = nr;
}

void tascar_gui_t::on_view_selected()
{
  std::string rg(viewselector.get_active_text());
  if( rg == "top" )
    viewt = top;
  if( rg == "right" )
    viewt = right;
  if( rg == "front" )
    viewt = front;
  if( rg == "perspective" )
    viewt = perspective;
}

//void tascar_gui_t::draw_track(const object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
//{
//  bool solo(obj.get_solo());
//  pos_t p0;
//  cr->save();
//  if( solo && blink ){
//    cr->set_source_rgba(1,0,0,0.5);
//    cr->set_line_width( 1.2*msize );
//    for( TASCAR::track_t::const_iterator it=obj.location.begin();it!=obj.location.end();++it){
//      pos_t p(view(it->second));
//      if( it != obj.location.begin() )
//        draw_edge( cr, p0,p );
//      p0 = p;
//    }
//    cr->stroke();
//  }
//  cr->set_source_rgb(obj.color.r, obj.color.g, obj.color.b );
//  if( solo && blink )
//    cr->set_line_width( 0.3*msize );
//  else
//    cr->set_line_width( 0.1*msize );
//  for( TASCAR::track_t::const_iterator it=obj.location.begin();it!=obj.location.end();++it){
//    pos_t p(view(it->second));
//    if( it != obj.location.begin() )
//      draw_edge( cr, p0,p );
//    p0 = p;
//  }
//  cr->stroke();
//  cr->restore();
//}
//
//void tascar_gui_t::draw_src(const src_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
//{
//  bool active(obj.isactive(time));
//  bool solo(obj.get_solo());
//  double plot_time(time);
//  if( !active ){
//    msize *= 0.4;
//    plot_time = std::min(std::max(plot_time,obj.starttime),obj.endtime);
//  }
//  pos_t p(obj.get_location(plot_time));
//  cr->save();
//  p = view(p);
//  if( p.z != std::numeric_limits<double>::infinity()){
//    if( solo && blink ){
//      cr->set_source_rgba(1, 0, 0, 0.5);
//      cr->arc(p.x, -p.y, 3*msize, 0, PI2 );
//      cr->fill();
//    }
//    cr->set_source_rgba(obj.color.r, obj.color.g, obj.color.b, 0.6);
//    cr->arc(p.x, -p.y, msize, 0, PI2 );
//    cr->fill();
//  }
//  for(unsigned int k=0;k<obj.sound.size();k++){
//    pos_t ps(view(obj.sound[k].get_pos_global(plot_time)));
//    if( ps.z != std::numeric_limits<double>::infinity()){
//      cr->arc(ps.x, -ps.y, 0.5*msize, 0, PI2 );
//      cr->fill();
//    }
//  }
//  if( !active )
//    cr->set_source_rgb(0.5, 0.5, 0.5 );
//  else
//    cr->set_source_rgb(0, 0, 0 );
//  if( p.z != std::numeric_limits<double>::infinity()){
//    cr->move_to( p.x + 1.1*msize, -p.y );
//    cr->show_text( obj.get_name().c_str() );
//  }
//  if( active ){
//    cr->set_line_width( 0.1*msize );
//    cr->set_source_rgba(obj.color.r, obj.color.g, obj.color.b, 0.6);
//    if( obj.sound.size()){
//      pos_t pso(view(obj.sound[0].get_pos_global(plot_time)));
//      if( pso.z != std::numeric_limits<double>::infinity()){
//        for(unsigned int k=1;k<obj.sound.size();k++){
//          pos_t ps(view(obj.sound[k].get_pos_global(plot_time)));
//          bool view_x((fabs(ps.x)<1)||
//                      (fabs(pso.x)<1));
//          bool view_y((fabs(ps.y)<1)||
//                      (fabs(pso.y)<1));
//          if( view_x && view_y ){
//            cr->move_to( pso.x, -pso.y );
//            cr->line_to( ps.x, -ps.y );
//            cr->stroke();
//          }
//          pso = ps;
//        }
//      }
//    }
//  }
//  cr->restore();
//}

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
    scene = new g_scene_t(cfg_file_, backend_flags_,nobackend_,srv_addr_,srv_port_);
    timescale.set_range(0,scene->duration);
    for(unsigned int k=0;k<scene->ranges.size();k++){
      timescale.add_mark(scene->ranges[k].start,Gtk::POS_BOTTOM,"");
      timescale.add_mark(scene->ranges[k].end,Gtk::POS_BOTTOM,"");
#ifdef GTKMM24
      rangeselector.append_text(scene->ranges[k].name);
#else
      rangeselector.append(scene->ranges[k].name);
#endif
    }
    set_scale(scene->guiscale);
    button_loop.set_active(scene->loop);
  }
  wdg_source.set_scene( scene );
  draw.set_scene(scene);
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

//void tascar_gui_t::draw_sink_object(const sink_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
//{
//  if( draw.view.get_perspective() )
//    return;
//  msize *= 1.5;
//  cr->save();
//  cr->set_line_width( 0.2*msize );
//  cr->set_source_rgba(obj.color.r, obj.color.g, obj.color.b, 0.6);
//  pos_t p(obj.get_location(time));
//  zyx_euler_t o(obj.get_orientation(time));
//  //DEBUG(o.print());
//  o.z += headrot;
//  if( (obj.size.x!=0)&&(obj.size.y!=0)&&(obj.size.z!=0) ){
//    draw_cube(p,o,obj.size,cr);
//    if( obj.falloff > 0 ){
//      std::vector<double> dash(2);
//      dash[0] = msize;
//      dash[1] = msize;
//      cr->set_dash(dash,0);
//      draw_cube(p,o,obj.size+pos_t(2*obj.falloff,2*obj.falloff,2*obj.falloff),cr);
//      dash[0] = 1.0;
//      dash[1] = 0.0;
//      cr->set_dash(dash,0);
//    }
//  }
//  double scale(0.5*draw.view.get_scale());
//  pos_t p1(1.8*msize*scale,-0.6*msize*scale,0);
//  pos_t p2(2.9*msize*scale,0,0);
//  pos_t p3(1.8*msize*scale,0.6*msize*scale,0);
//  pos_t p4(-0.5*msize*scale,2.3*msize*scale,0);
//  pos_t p5(0,1.7*msize*scale,0);
//  pos_t p6(0.5*msize*scale,2.3*msize*scale,0);
//  pos_t p7(-0.5*msize*scale,-2.3*msize*scale,0);
//  pos_t p8(0,-1.7*msize*scale,0);
//  pos_t p9(0.5*msize*scale,-2.3*msize*scale,0);
//  p1 *= o;
//  p2 *= o;
//  p3 *= o;
//  p4 *= o;
//  p5 *= o;
//  p6 *= o;
//  p7 *= o;
//  p8 *= o;
//  p9 *= o;
//  p1+=p;
//  p2+=p;
//  p3+=p;
//  p4+=p;
//  p5+=p;
//  p6+=p;
//  p7+=p;
//  p8+=p;
//  p9+=p;
//  p = view(p);
//  p1 = view(p1);
//  p2 = view(p2);
//  p3 = view(p3);
//  p4 = view(p4);
//  p5 = view(p5);
//  p6 = view(p6);
//  p7 = view(p7);
//  p8 = view(p8);
//  p9 = view(p9);
//  cr->move_to( p.x, -p.y );
//  cr->arc(p.x, -p.y, 2*msize, 0, PI2 );
//  cr->move_to( p1.x, -p1.y );
//  cr->line_to( p2.x, -p2.y );
//  cr->line_to( p3.x, -p3.y );
//  cr->move_to( p4.x, -p4.y );
//  cr->line_to( p5.x, -p5.y );
//  cr->line_to( p6.x, -p6.y );
//  cr->move_to( p7.x, -p7.y );
//  cr->line_to( p8.x, -p8.y );
//  cr->line_to( p9.x, -p9.y );
//  //cr->fill();
//  cr->stroke();
//  if( obj.use_mask ){
//    cr->set_line_width( 0.1*msize );
//    pos_t p(obj.mask.get_location(time));
//    zyx_euler_t o(obj.mask.get_orientation(time));
//    draw_cube(p,o,obj.mask.size,cr);
//    if( obj.mask.falloff > 0 ){
//      std::vector<double> dash(2);
//      dash[0] = msize;
//      dash[1] = msize;
//      cr->set_dash(dash,0);
//      draw_cube(p,o,obj.mask.size+pos_t(2*obj.mask.falloff,2*obj.mask.falloff,2*obj.mask.falloff),cr);
//      dash[0] = 1.0;
//      dash[1] = 0.0;
//      cr->set_dash(dash,0);
//    }
//  }
//  cr->set_source_rgb(0, 0, 0 );
//  cr->move_to( p.x + 3.1*msize, -p.y );
//  cr->show_text( obj.get_name().c_str() );
//  cr->restore();
//}
//
//
//void tascar_gui_t::draw_mask(mask_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
//{
//  if( draw.view.get_perspective() )
//    return;
//  msize *= 1.5;
//  cr->save();
//  cr->set_line_width( 0.2*msize );
//  cr->set_source_rgba(obj.color.r, obj.color.g, obj.color.b, 0.6);
//  obj.geometry_update(time);
//  if( obj.mask_inner ){
//    draw_cube(obj.center,obj.shoebox_t::orientation,obj.xmlsize+pos_t(2*obj.xmlfalloff,2*obj.xmlfalloff,2*obj.xmlfalloff),cr);
//    std::vector<double> dash(2);
//    dash[0] = msize;
//    dash[1] = msize;
//    cr->set_dash(dash,0);
//    draw_cube(obj.center,obj.shoebox_t::orientation,obj.xmlsize,cr);
//    dash[0] = 1.0;
//    dash[1] = 0.0;
//    cr->set_dash(dash,0);
//  }else{
//    draw_cube(obj.center,obj.shoebox_t::orientation,obj.xmlsize,cr);
//    std::vector<double> dash(2);
//    dash[0] = msize;
//    dash[1] = msize;
//    cr->set_dash(dash,0);
//    draw_cube(obj.center,obj.shoebox_t::orientation,obj.xmlsize+pos_t(2*obj.xmlfalloff,2*obj.xmlfalloff,2*obj.xmlfalloff),cr);
//    dash[0] = 1.0;
//    dash[1] = 0.0;
//    cr->set_dash(dash,0);
//  }
//  cr->restore();
//}
//
//void tascar_gui_t::draw_cube(pos_t pos, zyx_euler_t orient, pos_t size,Cairo::RefPtr<Cairo::Context> cr)
//{
//  std::vector<pos_t> roomnodes(8,pos_t());
//  roomnodes[0].x -= 0.5*size.x;
//  roomnodes[1].x += 0.5*size.x;
//  roomnodes[2].x += 0.5*size.x;
//  roomnodes[3].x -= 0.5*size.x;
//  roomnodes[4].x -= 0.5*size.x;
//  roomnodes[5].x += 0.5*size.x;
//  roomnodes[6].x += 0.5*size.x;
//  roomnodes[7].x -= 0.5*size.x;
//  roomnodes[0].y -= 0.5*size.y;
//  roomnodes[1].y -= 0.5*size.y;
//  roomnodes[2].y += 0.5*size.y;
//  roomnodes[3].y += 0.5*size.y;
//  roomnodes[4].y -= 0.5*size.y;
//  roomnodes[5].y -= 0.5*size.y;
//  roomnodes[6].y += 0.5*size.y;
//  roomnodes[7].y += 0.5*size.y;
//  roomnodes[0].z -= 0.5*size.z;
//  roomnodes[1].z -= 0.5*size.z;
//  roomnodes[2].z -= 0.5*size.z;
//  roomnodes[3].z -= 0.5*size.z;
//  roomnodes[4].z += 0.5*size.z;
//  roomnodes[5].z += 0.5*size.z;
//  roomnodes[6].z += 0.5*size.z;
//  roomnodes[7].z += 0.5*size.z;
//  for(unsigned int k=0;k<8;k++)
//    roomnodes[k] *= orient;
//  for(unsigned int k=0;k<8;k++)
//    roomnodes[k] += pos;
//  for(unsigned int k=0;k<8;k++)
//    roomnodes[k] = view(roomnodes[k]);
//  cr->save();
//  draw_edge(cr,roomnodes[0],roomnodes[1]);
//  draw_edge(cr,roomnodes[1],roomnodes[2]);
//  draw_edge(cr,roomnodes[2],roomnodes[3]);
//  draw_edge(cr,roomnodes[3],roomnodes[0]);
//  draw_edge(cr,roomnodes[4],roomnodes[5]);
//  draw_edge(cr,roomnodes[5],roomnodes[6]);
//  draw_edge(cr,roomnodes[6],roomnodes[7]);
//  draw_edge(cr,roomnodes[7],roomnodes[4]);
//  for(unsigned int k=0;k<4;k++){
//    draw_edge(cr,roomnodes[k],roomnodes[k+4]);
//  }
//  cr->stroke();
//  cr->restore();
//}
//
//void tascar_gui_t::draw_face_normal(const face_t& f,Cairo::RefPtr<Cairo::Context> cr,double normalsize)
//{
//  std::vector<pos_t> roomnodes(4,pos_t());
//  roomnodes[0] = f.get_anchor();
//  roomnodes[1] = f.get_anchor();
//  roomnodes[1] += f.get_e1();
//  roomnodes[2] = f.get_anchor();
//  roomnodes[2] += f.get_e1();
//  roomnodes[2] += f.get_e2();
//  roomnodes[3] = f.get_anchor();
//  roomnodes[3] += f.get_e2();
//  for(unsigned int k=0;k<roomnodes.size();k++)
//    roomnodes[k] = view(roomnodes[k]);
//  cr->save();
//  draw_edge(cr,roomnodes[0],roomnodes[1]);
//  draw_edge(cr,roomnodes[1],roomnodes[2]);
//  draw_edge(cr,roomnodes[2],roomnodes[3]);
//  draw_edge(cr,roomnodes[3],roomnodes[0]);
//  if( normalsize >= 0 ){
//    pos_t pn(f.get_normal());
//    pn *= normalsize;
//    pn += f.get_anchor();
//    pn = view(pn);
//    draw_edge(cr,roomnodes[0],pn);
//  }
//  cr->stroke();
//  cr->restore();
//}
//
//void tascar_gui_t::draw_room_src(const TASCAR::Scene::src_diffuse_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
//{
//  bool solo(obj.get_solo());
//  pos_t p(obj.get_location(time));
//  zyx_euler_t o(obj.get_orientation(time));
//  cr->save();
//  if( solo && blink )
//    cr->set_line_width( 0.6*msize );
//  else
//    cr->set_line_width( 0.1*msize );
//  cr->set_source_rgb(obj.color.r, obj.color.g, obj.color.b );
//  draw_cube(p,o,obj.size,cr);
//  std::vector<double> dash(2);
//  dash[0] = msize;
//  dash[1] = msize;
//  cr->set_dash(dash,0);
//  pos_t falloff(obj.falloff,obj.falloff,obj.falloff);
//  falloff *= 2.0;
//  falloff += obj.size;
//  draw_cube(p,o,falloff,cr);
//  p = view(p);
//  cr->set_source_rgb(0, 0, 0 );
//  if( p.z != std::numeric_limits<double>::infinity()){
//    cr->move_to( p.x, -p.y );
//    cr->show_text( obj.get_name().c_str() );
//  }
//  cr->restore();
//}
//
//void tascar_gui_t::draw_door_src(const TASCAR::Scene::src_door_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
//{
//  bool solo(obj.get_solo());
//  pos_t p(obj.get_location(time));
//  zyx_euler_t o(obj.get_orientation(time));
//  o += obj.dorientation;
//  cr->save();
//  if( solo && blink )
//    cr->set_line_width( 1.2*msize );
//  else
//    cr->set_line_width( 0.4*msize );
//  cr->set_source_rgb(obj.color.r, obj.color.g, obj.color.b );
//  face_t f;
//  f.set(p,o,obj.width,obj.height);
//  draw_face_normal(f,cr);
//  std::vector<double> dash(2);
//  dash[0] = msize;
//  dash[1] = msize;
//  cr->set_dash(dash,0);
//  cr->set_source_rgba(obj.color.r, obj.color.g, obj.color.b, 0.6 );
//  f += obj.falloff;
//  draw_face_normal(f,cr);
//  p = view(p);
//  cr->set_source_rgb(0, 0, 0 );
//  if( p.z != std::numeric_limits<double>::infinity()){
//    cr->move_to( p.x, -p.y );
//    cr->show_text( obj.get_name().c_str() );
//  }
//  cr->restore();
//}
//
//void tascar_gui_t::draw_face(const TASCAR::Scene::face_object_t& face,Cairo::RefPtr<Cairo::Context> cr, double msize)
//{
//  bool active(face.isactive(time));
//  bool solo(face.get_solo());
//  if( !active )
//    msize*=0.5;
//  pos_t loc(view(face.get_location(time)));
//  cr->save();
//  if( solo && blink ){
//    // solo indicating:
//    cr->set_line_width( 1.2*msize );
//    cr->set_source_rgba(1,0,0,0.5);
//    draw_face_normal(face,cr);
//  }
//  // outline:
//  cr->set_line_width( 0.2*msize );
//  cr->set_source_rgba(face.color.r,face.color.g,face.color.b,0.6);
//  draw_face_normal(face,cr,true);
//  // fill:
//  if( active ){
//    // normal and name:
//    cr->set_source_rgba(face.color.r,face.color.g,0.5+0.5*face.color.b,0.3);
//    if( loc.z != std::numeric_limits<double>::infinity()){
//      cr->arc(loc.x, -loc.y, msize, 0, PI2 );
//      cr->fill();
//      cr->set_line_width( 0.4*msize );
//      cr->set_source_rgb(face.color.r,face.color.g,face.color.b);
//      draw_face_normal(face,cr);
//      cr->set_source_rgb(0, 0, 0 );
//      cr->move_to( loc.x + 0.1*msize, -loc.y );
//      cr->show_text( face.get_name().c_str() );
//    }
//  }
//  cr->restore();
//}

#define CON_BUTTON(b) button_ ## b.signal_clicked().connect(sigc::mem_fun(*this,&tascar_gui_t::on_ ## b))

tascar_gui_t::tascar_gui_t(const std::string& srv_addr, 
                           const std::string& srv_port,
                           const std::string& cfg_file,
                           const std::string& backend_flags,bool nobackend)
  : jackc_transport_t(cfg_file),
    //scale(200),
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
    viewt(top)
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
      switch( viewt ){
      case top :
        draw.view.set_perspective(false);
        draw.view.set_ref(scene->guicenter);
        draw.view.set_euler(zyx_euler_t());
        break;
      case front :
        draw.view.set_perspective(false);
        draw.view.set_ref(scene->guicenter);
        draw.view.set_euler(zyx_euler_t(0,-0.5*M_PI,0.5*M_PI));
        break;
      case right :
        draw.view.set_perspective(false);
        draw.view.set_ref(scene->guicenter);
        draw.view.set_euler(zyx_euler_t(0,0,0.5*M_PI));
        break;
      case perspective :
        draw.view.set_perspective(true);
        if( scene->sink_objects.size() ){
          draw.view.set_ref(scene->sink_objects[0].get_location(time));
          draw.view.set_euler(scene->sink_objects[0].get_orientation(time));
        }
        break;
      }
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
      draw.set_time(time);
      draw.draw(cr);
      //for(unsigned int k=0;k<scene->faces.size();k++){
      //  scene->faces[k].geometry_update(time);
      //  draw_face(scene->faces[k], cr, markersize );
      //}
      cr->set_source_rgba(0.2, 0.2, 0.2, 0.8);
      cr->move_to(-markersize, 0 );
      cr->line_to( markersize, 0 );
      cr->move_to( 0, -markersize );
      cr->line_to( 0,  markersize );
      cr->stroke();
      //// draw tracks:
      //for(unsigned int k=0;k<scene->object_sources.size();k++){
      //  draw_track(scene->object_sources[k], cr, markersize );
      //}
      //for(unsigned int k=0;k<scene->sink_objects.size();k++){
      //  draw_track(scene->sink_objects[k], cr, markersize );
      //}
      //// draw other objects:
      //for(unsigned int k=0;k<scene->diffuse_sources.size();k++){
      //  draw_room_src(scene->diffuse_sources[k], cr, markersize );
      //}
      //for(unsigned int k=0;k<scene->door_sources.size();k++){
      //  draw_door_src(scene->door_sources[k], cr, markersize );
      //}
      //for(unsigned int k=0;k<scene->object_sources.size();k++){
      //  draw_src(scene->object_sources[k], cr, markersize );
      //}
      //for(unsigned int k=0;k<scene->sink_objects.size();k++){
      //  draw_sink_object( scene->sink_objects[k], cr, markersize );
      //}
      //for(unsigned int k=0;k<scene->masks.size();k++){
      //  draw_mask( scene->masks[k], cr, markersize );
      //}
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
      if( (scene->ranges[selected_range].end <= time) && 
          (scene->ranges[selected_range].end + (double)nframes/(double)srate > time) ){
        if( scene->loop ){
          tp_locate(scene->ranges[selected_range].start);
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
