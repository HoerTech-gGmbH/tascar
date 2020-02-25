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

namespace App {

  class pdf_export_t : public TASCAR::tsc_reader_t {
  public:
    pdf_export_t(const std::string& session_filename,const std::string& pdfname);
    void draw_acousticmodel() { drawer.set_show_acoustic_model(true); };
    ~pdf_export_t();
    void render_time(const std::vector<double>& time);
    void render_time(TASCAR::render_core_t* scene,const std::vector<double>& time);
    void set_ism_order_range(uint32_t ism_min,uint32_t ism_max);
  protected:
    virtual void add_scene(xmlpp::Element *e);
  private:
    void draw(TSCGUI::scene_draw_t::viewt_t persp);
    double time;
    std::string filename;
    double height;
    double width;
    double lmargin;
    double rmargin;
    double tmargin;
    double bmargin;
    Cairo::RefPtr<Cairo::PdfSurface> surface;
    TSCGUI::scene_draw_t drawer;
    std::vector<TASCAR::render_core_t*> scenes;
  };

}

App::pdf_export_t::pdf_export_t(const std::string& session_filename,const std::string& pdfname)
  : tsc_reader_t(session_filename,LOAD_FILE,session_filename),
    time(0),
    filename(session_filename),
    height(72*210/25.4),
    width(72*297/25.4),
    lmargin(72*12/25.4),
    rmargin(72*12/25.4),
    tmargin(72*18/25.4),
    bmargin(72*12/25.4),
    surface(Cairo::PdfSurface::create(pdfname, width, height ))
{
  read_xml();
}

App::pdf_export_t::~pdf_export_t()
{
  for(std::vector<TASCAR::render_core_t*>::iterator sit=scenes.begin();sit!=scenes.end();++sit)
    delete (*sit);
}

void App::pdf_export_t::set_ism_order_range(uint32_t ism_min,uint32_t ism_max)
{
  for(std::vector<TASCAR::render_core_t*>::iterator ipl=scenes.begin();ipl!=scenes.end();++ipl)
    (*ipl)->set_ism_order_range(ism_min,ism_max);
}

void App::pdf_export_t::add_scene(xmlpp::Element* sne)
{
  scenes.push_back(new render_core_t(sne));
}

void App::pdf_export_t::render_time(const std::vector<double>& t)
{
  for(std::vector<TASCAR::render_core_t*>::iterator it=scenes.begin();it!=scenes.end();++it)
    render_time(*it,t);
}

void App::pdf_export_t::render_time(TASCAR::render_core_t* s, const std::vector<double>& t)
{
  chunk_cfg_t cf(44100);
  s->prepare( cf );
  uint32_t nch_in(s->num_input_ports());
  uint32_t nch_out(s->num_output_ports());
  float v(0);
  std::vector<float*> dIn(nch_in,&v);
  std::vector<float*> dOut(nch_out,&v);
  drawer.set_scene(s);
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
  TASCAR::transport_t tp;
  for(uint32_t k=0;k<t.size();k++){
    time = t[k];
    tp.session_time_seconds = time;
    tp.session_time_samples = time;
    tp.object_time_seconds = time;
    tp.object_time_samples = time;
    // s->geometry_update(time);
    s->process(1,tp,dIn,dOut);
    s->process(1,tp,dIn,dOut);
    draw(TSCGUI::scene_draw_t::xy);
  }
  for(uint32_t k=0;k<t.size();k++){
    time = t[k];
    tp.session_time_seconds = time;
    tp.session_time_samples = time;
    tp.object_time_seconds = time;
    tp.object_time_samples = time;
    //s->geometry_update(time);
    s->process(1,tp,dIn,dOut);
    s->process(1,tp,dIn,dOut);
    draw(TSCGUI::scene_draw_t::xz);
  }
  for(uint32_t k=0;k<t.size();k++){
    time = t[k];
    //s->geometry_update(time);
    s->process(1,tp,dIn,dOut);
    s->process(1,tp,dIn,dOut);
    draw(TSCGUI::scene_draw_t::yz);
  }
  //for(uint32_t k=0;k<t.size();k++){
  //  time = t[k];
  //  //s->geometry_update(time);
  //  s->process(time,1,dIn,dOut);
  //  s->process(time,1,dIn,dOut);
  //  draw(scene_draw_t::p);
  //}
  s->release();
}

void App::pdf_export_t::draw(TSCGUI::scene_draw_t::viewt_t persp)
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
  cr->stroke();
  cr->move_to( bx, by+22 );
  cr->line_to( bx+bw,by+22);
  cr->stroke();
  cr->set_font_size( 10 );
  char ctmp[1024];
  switch( persp ){
  case TSCGUI::scene_draw_t::xy :
    sprintf(ctmp,"top ortho");
    break;
  case TSCGUI::scene_draw_t::xz :
    sprintf(ctmp,"front ortho");
    break;
  case TSCGUI::scene_draw_t::yz :
    sprintf(ctmp,"left ortho");
    break;
  case TSCGUI::scene_draw_t::p :
    sprintf(ctmp,"perspective");
    break;
  case TSCGUI::scene_draw_t::xyz :
    sprintf(ctmp,"xyz");
    break;
  }
  cr->move_to( bx+12, by+36 );
  cr->show_text( ctmp );
  cr->stroke();
  if( persp == TSCGUI::scene_draw_t::p )
    sprintf(ctmp,"fov %g°",drawer.view.get_fov());
  else
    sprintf(ctmp,"scale 1:%g",drawer.view.get_scale()/(wscale/72*0.0254));
  cr->move_to( bx+12, by+48 );
  cr->show_text( ctmp );
  cr->stroke();
  sprintf(ctmp,"time %g s",time);
  cr->move_to( bx+12, by+60 );
  cr->show_text( ctmp );
  cr->stroke();
  surface->show_page();
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_pdf -c sessionfile [options]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

int main(int argc, char** argv)
{
  std::string tscfile("");
  std::string pdffile("");
  const char *options = "o:ht:a40:1:";
  struct option long_options[] = { 
    { "output",   1, 0, 'o' },
    { "help",     0, 0, 'h' },
    { "time",     1, 0, 't' },
    { "acousticmodel", 0, 0, 'a' },
    { "ismmin", 1, 0, '0' },
    { "ismmax", 1, 0, '1' },
    { 0, 0, 0, 0 }
  };
  int opt(0);
  bool b_am(false);
  uint32_t ism_min(0);
  uint32_t ism_max(3);
  int option_index(0);
  std::vector<double> time;
  while( (opt = getopt_long(argc, argv, options,
                            long_options, &option_index)) != -1){
    switch(opt){
    case 'o':
      pdffile = optarg;
      break;
    case 't':
      time.push_back(atof(optarg));
      break;
    case 'h':
      usage(long_options);
      return -1;
    case 'a':
      b_am = true;
      break;
    case '0':
      ism_min = atoi(optarg);
      break;
    case '1':
      ism_max = atoi(optarg);
      break;
    }
  }
  if( optind < argc )
    tscfile = argv[optind++];
  if( tscfile.size() == 0 ){
    usage(long_options);
    return -1;
  }
  if( time.empty() )
    time.push_back(0.0);
  if( pdffile.size() == 0 ){
    pdffile = tscfile+".pdf";
  }
  App::pdf_export_t c(tscfile,pdffile);
  if( b_am )
    c.draw_acousticmodel();
  c.set_ism_order_range(ism_min,ism_max);
  c.render_time(time);
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
