/**
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

#include "session.h"
#include <getopt.h>
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
  //DEBUG(p1.print_cart());
  //DEBUG(p2.print_cart());
  if( !(has_infinity(p1) || has_infinity(p2)) ){
    cr->move_to(p1.x,-p1.y);
    cr->line_to(p2.x,-p2.y);
  }
}

class pdf_export_t : public TASCAR::session_t {
public:
  pdf_export_t(const std::string& scenename,const std::string& pdfname);
  ~pdf_export_t();
  void render_time(const std::vector<double>& time);
  void render_time(TASCAR::renderer_t* scene,const std::vector<double>& time);
private:
  void draw(scene_draw_t::viewt_t persp);
  double time;
  std::string filename;
  double height;
  double width;
  double lmargin;
  double rmargin;
  double tmargin;
  double bmargin;
  Cairo::RefPtr<Cairo::PdfSurface> surface;
  scene_draw_t drawer;
};

pdf_export_t::pdf_export_t(const std::string& scenename,const std::string& pdfname)
  : session_t(scenename,LOAD_FILE,scenename),
    time(0),
    filename(scenename),
    height(72*210/25.4),
    width(72*297/25.4),
    lmargin(72*12/25.4),
    rmargin(72*12/25.4),
    tmargin(72*18/25.4),
    bmargin(72*12/25.4),
    surface(Cairo::PdfSurface::create(pdfname, width, height ))
{
}

pdf_export_t::~pdf_export_t()
{
}

void pdf_export_t::render_time(const std::vector<double>& t)
{
  for(std::vector<TASCAR::scene_player_t*>::iterator it=player.begin();it!=player.end();++it)
    render_time(*it,t);
}

void pdf_export_t::render_time(TASCAR::renderer_t* s, const std::vector<double>& t)
{
  drawer.set_scene(s);
  //read_xml(scenename);
  //linearize_sounds();
  //prepare(44100,1024);
  double wscale(0.5*std::max(height,width));
  double res(wscale/72*0.0254);
  double nscale(s->guiscale/res);
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
  while( (k < div.size()) && (nscale/div[k] >= s->guiscale/res) )
    k++;
  if( k > 0 )
    nscale /= div[k-1];
  drawer.view.set_scale(nscale*res);
  for(uint32_t k=0;k<t.size();k++){
    time = t[k];
    s->geometry_update(time);
    draw(scene_draw_t::xy);
  }
  for(uint32_t k=0;k<t.size();k++){
    time = t[k];
    s->geometry_update(time);
    draw(scene_draw_t::xz);
  }
  for(uint32_t k=0;k<t.size();k++){
    time = t[k];
    s->geometry_update(time);
    draw(scene_draw_t::yz);
  }
  for(uint32_t k=0;k<t.size();k++){
    time = t[k];
    s->geometry_update(time);
    draw(scene_draw_t::p);
  }
}

void pdf_export_t::draw(scene_draw_t::viewt_t persp)
{
  drawer.set_viewport(persp);
  Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
  //view.set_ref(listener.get_location(time));
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
  drawer.set_markersize(markersize);
  drawer.draw(cr);
  cr->set_source_rgba(0.2, 0.2, 0.2, 0.8);
  cr->move_to(-markersize, 0 );
  cr->line_to( markersize, 0 );
  cr->move_to( 0, -markersize );
  cr->line_to( 0,  markersize );
  cr->stroke();
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
  case scene_draw_t::xy :
    sprintf(ctmp,"top ortho");
    break;
  case scene_draw_t::xz :
    sprintf(ctmp,"front ortho");
    break;
  case scene_draw_t::yz :
    sprintf(ctmp,"left ortho");
    break;
  case scene_draw_t::p :
    sprintf(ctmp,"perspective");
    break;
  }
  cr->move_to( bx+12, by+36 );
  cr->show_text( ctmp );
  if( persp == scene_draw_t::p )
    sprintf(ctmp,"fov %g°",drawer.view.get_fov());
  else
    sprintf(ctmp,"scale 1:%g",drawer.view.get_scale()/(wscale/72*0.0254));
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
  const char *options = "c:o:ht:";
  struct option long_options[] = { 
    { "config",   1, 0, 'c' },
    { "output",   1, 0, 'o' },
    { "help",     0, 0, 'h' },
    { "time",     1, 0, 't' },
    { 0, 0, 0, 0 }
  };
  int opt(0);
  int option_index(0);
  std::vector<double> time;
  while( (opt = getopt_long(argc, argv, options,
                            long_options, &option_index)) != -1){
    switch(opt){
    case 'c':
      cfgfile = optarg;
      break;
    case 'o':
      pdffile = optarg;
      break;
    case 't':
      time.push_back(atof(optarg));
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
  if( time.empty() )
    time.push_back(0.0);
  if( pdffile.size() == 0 ){
    pdffile = cfgfile+".pdf";
  }
  pdf_export_t c(cfgfile,pdffile);
  c.start();
  c.render_time(time);
  c.stop();
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
