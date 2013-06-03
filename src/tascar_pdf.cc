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
#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>
#include "tascar.h"
#include <iostream>
#include <getopt.h>
#include "viewport.h"

using namespace TASCAR;

class pdf_export_t : public scene_t {
public:
  enum view_t {
    xy, xz, yz, p
  };
  pdf_export_t(const std::string& scenename,const std::string& pdfname);
  ~pdf_export_t();
  void render_time(double time);
private:
  void draw(pdf_export_t::view_t persp);
  void draw_track(const object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  void draw_src(const src_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  void draw_listener(const listener_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  void draw_room(const diffuse_reverb_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  void draw_face(const face_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize);
  double time;
  viewport_t view;
  std::string filename;
  double height;
  double width;
  double lmargin;
  double rmargin;
  double tmargin;
  double bmargin;
  Cairo::RefPtr<Cairo::PdfSurface> surface;
};

void pdf_export_t::draw_track(const object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  cr->save();
  cr->set_source_rgb(obj.color.r, obj.color.g, obj.color.b );
  cr->set_line_width( 0.1*msize );
  for( TASCAR::track_t::const_iterator it=obj.location.begin();it!=obj.location.end();++it){
    pos_t p(view(it->second));
    if( it==obj.location.begin() )
      cr->move_to( p.x, -p.y );
    else
      cr->line_to( p.x, -p.y );
  }
  cr->stroke();
  cr->restore();
}

void pdf_export_t::draw_src(const src_object_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  bool active(obj.isactive(time));
  double plot_time(time);
  if( !active ){
    msize *= 0.4;
    plot_time = std::min(std::max(plot_time,obj.starttime),obj.endtime);
  }
  //pos_t p(view(obj.location.interp(plot_time-obj.starttime)));
  pos_t p(obj.location.interp(plot_time-obj.starttime));
  //DEBUG(p.print_cart());
  p = view(p);
  //DEBUG(p.print_cart());
  cr->save();
  cr->set_source_rgba(obj.color.r, obj.color.g, obj.color.b, 0.6);
  cr->arc(p.x, -p.y, msize, 0, PI2 );
  cr->fill();
  for(unsigned int k=0;k<obj.sound.size();k++){
    pos_t ps(view(obj.sound[k].get_pos_global(plot_time)));
    //pos_t ps(obj.sound[k].get_pos(time));
    cr->arc(ps.x, -ps.y, 0.5*msize, 0, PI2 );
    cr->fill();
  }
  if( !active )
    cr->set_source_rgb(0.5, 0.5, 0.5 );
  else
    cr->set_source_rgb(0, 0, 0 );
  cr->move_to( p.x + 1.1*msize, -p.y );
  cr->show_text( obj.get_name().c_str() );
  if( active ){
    cr->set_line_width( 0.1*msize );
    cr->set_source_rgba(obj.color.r, obj.color.g, obj.color.b, 0.6);
    //cr->move_to( p.x, -p.y );
    if( obj.sound.size()){
      pos_t pso(view(obj.sound[0].get_pos_global(plot_time)));
      for(unsigned int k=1;k<obj.sound.size();k++){
        pos_t ps(view(obj.sound[k].get_pos_global(plot_time)));
        //pos_t ps(obj.sound[k].get_pos(time));
        bool view_x((fabs(ps.x)<1)||
                    (fabs(pso.x)<1));
        bool view_y((fabs(ps.y)<1)||
                    (fabs(pso.y)<1));
        if( view_x && view_y ){
          cr->move_to( pso.x, -pso.y );
          cr->line_to( ps.x, -ps.y );
          cr->stroke();
        }
        pso = ps;
      }
    }
  }
  cr->restore();
}

void pdf_export_t::draw_listener(const listener_t& obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  if( view.get_perspective() )
    return;
  msize *= 1.5;
  pos_t p(obj.location.interp(time-obj.starttime));
  p += obj.dlocation;
  zyx_euler_t o(obj.orientation.interp(time-obj.starttime));
  o += obj.dorientation;
  //DEBUG(o.print());
  //o.z += headrot;
  double scale(0.5*view.get_scale());
  pos_t p1(1.8*msize*scale,-0.6*msize*scale,0);
  pos_t p2(2.9*msize*scale,0,0);
  pos_t p3(1.8*msize*scale,0.6*msize*scale,0);
  pos_t p4(-0.5*msize*scale,2.3*msize*scale,0);
  pos_t p5(0,1.7*msize*scale,0);
  pos_t p6(0.5*msize*scale,2.3*msize*scale,0);
  pos_t p7(-0.5*msize*scale,-2.3*msize*scale,0);
  pos_t p8(0,-1.7*msize*scale,0);
  pos_t p9(0.5*msize*scale,-2.3*msize*scale,0);
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
  p = view(p);
  p1 = view(p1);
  p2 = view(p2);
  p3 = view(p3);
  p4 = view(p4);
  p5 = view(p5);
  p6 = view(p6);
  p7 = view(p7);
  p8 = view(p8);
  p9 = view(p9);
  cr->save();
  cr->set_line_width( 0.2*msize );
  cr->set_source_rgba(obj.color.r, obj.color.g, obj.color.b, 0.6);
  cr->move_to( p.x, -p.y );
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
  cr->show_text( obj.get_name().c_str() );
  cr->restore();
}

void pdf_export_t::draw_room(const TASCAR::diffuse_reverb_t& reverb,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  std::vector<pos_t> roomnodes(8,reverb.center);
  roomnodes[0].x -= 0.5*reverb.size.x;
  roomnodes[1].x += 0.5*reverb.size.x;
  roomnodes[2].x += 0.5*reverb.size.x;
  roomnodes[3].x -= 0.5*reverb.size.x;
  roomnodes[4].x -= 0.5*reverb.size.x;
  roomnodes[5].x += 0.5*reverb.size.x;
  roomnodes[6].x += 0.5*reverb.size.x;
  roomnodes[7].x -= 0.5*reverb.size.x;
  roomnodes[0].y -= 0.5*reverb.size.y;
  roomnodes[1].y -= 0.5*reverb.size.y;
  roomnodes[2].y += 0.5*reverb.size.y;
  roomnodes[3].y += 0.5*reverb.size.y;
  roomnodes[4].y -= 0.5*reverb.size.y;
  roomnodes[5].y -= 0.5*reverb.size.y;
  roomnodes[6].y += 0.5*reverb.size.y;
  roomnodes[7].y += 0.5*reverb.size.y;
  roomnodes[0].z -= 0.5*reverb.size.z;
  roomnodes[1].z -= 0.5*reverb.size.z;
  roomnodes[2].z -= 0.5*reverb.size.z;
  roomnodes[3].z -= 0.5*reverb.size.z;
  roomnodes[4].z += 0.5*reverb.size.z;
  roomnodes[5].z += 0.5*reverb.size.z;
  roomnodes[6].z += 0.5*reverb.size.z;
  roomnodes[7].z += 0.5*reverb.size.z;
  for(unsigned int k=0;k<8;k++)
    roomnodes[k] *= reverb.orientation;
  cr->save();
  cr->set_line_width( 0.1*msize );
  cr->set_source_rgba(0,0,0,0.6);
  cr->move_to( roomnodes[0].x, -roomnodes[0].y );
  cr->line_to( roomnodes[1].x, -roomnodes[1].y );
  cr->line_to( roomnodes[2].x, -roomnodes[2].y );
  cr->line_to( roomnodes[3].x, -roomnodes[3].y );
  cr->line_to( roomnodes[0].x, -roomnodes[0].y );
  cr->move_to( roomnodes[4].x, -roomnodes[4].y );
  cr->line_to( roomnodes[5].x, -roomnodes[5].y );
  cr->line_to( roomnodes[6].x, -roomnodes[6].y );
  cr->line_to( roomnodes[7].x, -roomnodes[7].y );
  cr->line_to( roomnodes[4].x, -roomnodes[4].y );
  for(unsigned int k=0;k<4;k++){
    cr->move_to( roomnodes[k].x, -roomnodes[k].y );
    cr->line_to( roomnodes[k+4].x, -roomnodes[k+4].y );
  }
  cr->stroke();
  cr->set_source_rgb(0, 0, 0 );
  cr->move_to( roomnodes[0].x + 0.1*msize, -roomnodes[0].y );
  cr->show_text( reverb.get_name().c_str() );
  cr->restore();
}

void pdf_export_t::draw_face(const TASCAR::face_object_t& face,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  bool active(face.isactive(time));
  if( !active )
    msize*=0.5;
  std::vector<pos_t> roomnodes(18);
  // width:
  roomnodes[0].y += 0.5*face.width;
  roomnodes[1].y += 0.25*face.width;
  roomnodes[3].y -= 0.25*face.width;
  roomnodes[4].y -= 0.5*face.width;
  roomnodes[5].y -= 0.5*face.width;
  roomnodes[6].y -= 0.5*face.width;
  roomnodes[7].y -= 0.5*face.width;
  roomnodes[8].y -= 0.5*face.width;
  roomnodes[9].y -= 0.25*face.width;
  roomnodes[11].y += 0.25*face.width;
  roomnodes[12].y += 0.5*face.width;
  roomnodes[13].y += 0.5*face.width;
  roomnodes[14].y += 0.5*face.width;
  roomnodes[15].y += 0.5*face.width;
  // height:
  roomnodes[0].z += 0.5*face.height;
  roomnodes[1].z += 0.5*face.height;
  roomnodes[2].z += 0.5*face.height;
  roomnodes[3].z += 0.5*face.height;
  roomnodes[4].z += 0.5*face.height;
  roomnodes[5].z += 0.25*face.height;
  roomnodes[7].z -= 0.25*face.height;
  roomnodes[8].z -= 0.5*face.height;
  roomnodes[9].z -= 0.5*face.height;
  roomnodes[10].z -= 0.5*face.height;
  roomnodes[11].z -= 0.5*face.height;
  roomnodes[12].z -= 0.5*face.height;
  roomnodes[13].z -= 0.25*face.height;
  roomnodes[15].z += 0.25*face.height;
  // normal:
  roomnodes[17].x += 30*msize;
  pos_t loc(face.get_location(time));
  zyx_euler_t o(face.get_orientation(time));
  for(unsigned int k=0;k<roomnodes.size();k++){
    roomnodes[k] *= o;
    roomnodes[k] += loc;
    roomnodes[k] = view(roomnodes[k]);
  }
  cr->save();
  // outline:
  cr->set_line_width( 0.3*msize );
  cr->set_source_rgba(face.color.r,face.color.g,face.color.b,0.6);
  for(unsigned int k=0;k<16;k++){
    unsigned int k1((k+1)&15);
    bool view_x((fabs(roomnodes[k].x)<1)||
                (fabs(roomnodes[k1].x)<1));
    bool view_y((fabs(roomnodes[k].y)<1)||
                (fabs(roomnodes[k1].y)<1));
    if( view_x && view_y ){
      cr->move_to( roomnodes[k].x, -roomnodes[k].y );
      cr->line_to( roomnodes[k1].x, -roomnodes[k1].y );
    }
  }
  cr->stroke();
  // fill:
  if( active ){
    cr->set_source_rgba(face.color.r,face.color.g,face.color.b,0.3);
    for(unsigned int k=0;k<16;k++){
      // is at least one point in view?
      unsigned int k1((k+1)&15);
      bool view_x((fabs(roomnodes[16].x)<1)||
                  (fabs(roomnodes[k].x)<1)||
                  (fabs(roomnodes[k1].x)<1));
      bool view_y((fabs(roomnodes[16].y)<1)||
                  (fabs(roomnodes[k].y)<1)||
                  (fabs(roomnodes[k1].y)<1));
      if( view_x && view_y ){
        cr->move_to( roomnodes[16].x, -roomnodes[16].y );
        cr->line_to( roomnodes[k].x, -roomnodes[k].y );
        cr->line_to( roomnodes[k1].x, -roomnodes[k1].y );
        cr->fill();
      }
    }
  }
  if( active ){
    // normal and name:
    cr->set_source_rgba(face.color.r,face.color.g,0.5+0.5*face.color.b,0.8);
    cr->move_to( roomnodes[16].x, -roomnodes[16].y );
    cr->line_to( roomnodes[17].x, -roomnodes[17].y );
    cr->stroke();
    cr->set_source_rgba(face.color.r,face.color.g,0.5+0.5*face.color.b,0.3);
    cr->arc(roomnodes[16].x, -roomnodes[16].y, msize, 0, PI2 );
    cr->fill();
    cr->set_source_rgb(0, 0, 0 );
    cr->move_to( roomnodes[16].x + 0.1*msize, -roomnodes[16].y );
    cr->show_text( face.get_name().c_str() );
  }
  cr->restore();
}

pdf_export_t::pdf_export_t(const std::string& scenename,const std::string& pdfname)
  : time(0),
    filename(scenename),
    height(72*210/25.4),
    width(72*297/25.4),
    lmargin(72*12/25.4),
    rmargin(72*12/25.4),
    tmargin(72*18/25.4),
    bmargin(72*12/25.4),
    surface(Cairo::PdfSurface::create(pdfname, width, height ))
{
  read_xml(scenename);
  linearize_sounds();
  prepare(44100,1024);
  double wscale(0.5*std::max(height,width));
  double res(wscale/72*0.0254);
  double nscale(guiscale/res);
  nscale = pow(10.0,ceil(log10(nscale)));
  std::vector<double> div;
  div.push_back(1);
  div.push_back(2);
  div.push_back(2.5);
  div.push_back(10.0/3.0);
  div.push_back(4);
  div.push_back(5);
  div.push_back(8);
  uint32_t k(0);
  while( (k < div.size()) && (nscale/div[k] >= guiscale/res) )
    k++;
  if( k > 0 )
    nscale /= div[k-1];
  view.set_scale(nscale*res);
}

pdf_export_t::~pdf_export_t()
{
}

void pdf_export_t::render_time(double t)
{
  time = t;
  draw(pdf_export_t::xy);
  draw(pdf_export_t::xz);
  draw(pdf_export_t::yz);
  draw(pdf_export_t::p);
}

void pdf_export_t::draw(view_t persp)
{
  Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
  view.set_ref(listener.get_location(time));
  switch( persp ){
  case p : 
    view.set_perspective(true);
    view.set_euler(listener.get_orientation(time));
    break;
  case xy :
    view.set_perspective(false);
    view.set_euler(zyx_euler_t(0,0,0));
    break;
  case xz :
    view.set_perspective(false);
    view.set_euler(zyx_euler_t(0,0,0.5*M_PI));
    break;
  case yz :
    view.set_perspective(false);
    view.set_euler(zyx_euler_t(0,0.5*M_PI,0.5*M_PI));
    break;
  }
  cr->rectangle(lmargin,tmargin,width-lmargin-rmargin,height-tmargin-bmargin);
  cr->clip();
  cr->save();
  cr->translate(0.5*width, 0.5*height);
  double wscale(0.5*std::max(height,width));
  double markersize(0.007);
  cr->scale( wscale, wscale );
  cr->set_line_width( 0.3*markersize );
  cr->set_font_size( 3*markersize );
  cr->save();
  cr->set_source_rgb( 1, 1, 1 );
  cr->paint();
  cr->restore();
  draw_track( listener, cr, markersize );
  for(unsigned int k=0;k<srcobjects.size();k++){
    draw_track(srcobjects[k], cr, markersize );
  }
  for(unsigned int k=0;k<reverbs.size();k++){
    draw_room(reverbs[k], cr, markersize );
  }
  for(unsigned int k=0;k<faces.size();k++){
    draw_face(faces[k], cr, markersize );
  }
  draw_listener( listener, cr, markersize );
  cr->set_source_rgba(0.2, 0.2, 0.2, 0.8);
  cr->move_to(-markersize, 0 );
  cr->line_to( markersize, 0 );
  cr->move_to( 0, -markersize );
  cr->line_to( 0,  markersize );
  cr->stroke();
  for(unsigned int k=0;k<srcobjects.size();k++){
    draw_src(srcobjects[k], cr, markersize );
  }
  cr->restore();
  double bw(50*72/25.4);
  double bh(24*72/25.4);
  double bx(width-rmargin-bw);
  double by(height-bmargin-bh);
  cr->set_source_rgb(0,0,0);
  cr->set_font_size( 12 );
  cr->set_line_width( 2 );
  cr->rectangle(lmargin,tmargin,width-lmargin-rmargin,height-tmargin-bmargin);
  cr->stroke();
  cr->set_source_rgb(1,1,1);
  cr->rectangle(bx,by,bw,bh);
  cr->fill();
  //cr->set_line_width( 2 );
  cr->set_source_rgb(0,0,0);
  cr->rectangle(bx,by,bw,bh);
  cr->stroke();
  cr->move_to( bx+12, by+15 );
  cr->show_text( filename.c_str() );
  cr->move_to( bx, by+22 );
  cr->line_to( bx+bw,by+22);
  cr->stroke();
  cr->set_font_size( 10 );
  char ctmp[1024];
  switch( persp ){
  case xy :
    sprintf(ctmp,"top ortho");
    break;
  case xz :
    sprintf(ctmp,"front ortho");
    break;
  case yz :
    sprintf(ctmp,"left ortho");
    break;
  case p :
    sprintf(ctmp,"perspective");
    break;
  }
  cr->move_to( bx+12, by+36 );
  cr->show_text( ctmp );
  if( persp == p )
    sprintf(ctmp,"fov %g°",view.get_fov());
  else
    sprintf(ctmp,"scale 1:%g",view.get_scale()/(wscale/72*0.0254));
  cr->move_to( bx+12, by+48 );
  cr->show_text( ctmp );
  sprintf(ctmp,"time %g s",time);
  cr->move_to( bx+12, by+60 );
  cr->show_text( ctmp );
  surface->show_page();
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
  std::string cfgfile("");
  std::string pdffile("");
  const char *options = "c:o:h:";
  struct option long_options[] = { 
    { "config",   1, 0, 'c' },
    { "output",   1, 0, 'o' },
    { "help",     0, 0, 'h' },
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
    case 'o':
      pdffile = optarg;
      break;
    case 'h':
      usage(long_options);
      return -1;
    }
  }
  if( cfgfile.size() == 0 ){
    usage(long_options);
    return -1;
  }
  if( pdffile.size() == 0 ){
    pdffile = cfgfile+".pdf";
  }
  pdf_export_t c(cfgfile,pdffile);
  c.render_time(0);
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
