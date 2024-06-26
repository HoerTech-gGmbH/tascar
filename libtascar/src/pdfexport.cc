/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
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

#include "pdfexport.h"

using namespace TSCGUI;

TASCAR::pdfexport_t::pdfexport_t(TASCAR::session_t* s, const std::string& fname,
                                 bool ac_model)
    : filename(fname), height(72 * 210 / 25.4), width(72 * 297 / 25.4),
      lmargin(72 * 12 / 25.4), rmargin(72 * 12 / 25.4), tmargin(72 * 18 / 25.4),
      bmargin(72 * 12 / 25.4),
      surface(Cairo::PdfSurface::create(filename, width, height))
{
  if(ac_model) {
    drawer.set_print_labels(false);
    drawer.set_show_acoustic_model(true);
  }
  for(std::vector<TASCAR::scene_render_rt_t*>::iterator it = s->scenes.begin();
      it != s->scenes.end(); ++it)
    draw_views(*it);
}

void TASCAR::pdfexport_t::draw_views(TASCAR::scene_render_rt_t* s)
{
  drawer.set_scene(s);
  double wscale(0.5 * std::max(height, width));
  double res(wscale / 72 * 0.0254);
  double nscale(s->guiscale / res);
  nscale = pow(10.0, ceil(log10(nscale)));
  std::vector<double> div;
  div.push_back(1);
  div.push_back(2);
  div.push_back(2.5);
  div.push_back(10.0 / 3.0);
  div.push_back(4);
  div.push_back(5);
  div.push_back(8);
  uint32_t k(0);
  while((k < div.size()) && (nscale / div[k] >= s->guiscale / res))
    k++;
  if(k > 0)
    nscale /= div[k - 1];
  drawer.view.set_scale(nscale * res);
  draw(scene_draw_t::xy);
  draw(scene_draw_t::xz);
  draw(scene_draw_t::yz);
  // draw(scene_draw_t::p);
}

void TASCAR::pdfexport_t::draw(scene_draw_t::viewt_t persp)
{
  drawer.set_viewport(persp);
  Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
  // view.set_ref(listener.get_location(time));
  cr->rectangle(lmargin, tmargin, width - lmargin - rmargin,
                height - tmargin - bmargin);
  cr->clip();
  cr->save();
  cr->translate(0.5 * width, 0.5 * height);
  double wscale(0.5 * std::max(height, width));
  double markersize(0.007);
  cr->scale(wscale, wscale);
  cr->set_line_width(0.3 * markersize);
  cr->set_font_size(3 * markersize);
  cr->save();
  cr->set_source_rgb(1, 1, 1);
  cr->paint();
  cr->restore();
  drawer.set_markersize(markersize);
  drawer.draw(cr);
  cr->set_source_rgba(0.2, 0.2, 0.2, 0.8);
  cr->move_to(-markersize, 0);
  cr->line_to(markersize, 0);
  cr->move_to(0, -markersize);
  cr->line_to(0, markersize);
  cr->stroke();
  cr->restore();
  double bw(50 * 72 / 25.4);
  double bh(24 * 72 / 25.4);
  double bx(width - rmargin - bw);
  double by(height - bmargin - bh);
  cr->set_source_rgb(0, 0, 0);
  cr->set_font_size(12);
  cr->set_line_width(2);
  cr->rectangle(lmargin, tmargin, width - lmargin - rmargin,
                height - tmargin - bmargin);
  cr->stroke();
  cr->set_source_rgb(1, 1, 1);
  cr->rectangle(bx, by + 22, bw, bh - 22);
  cr->fill();
  // cr->set_line_width( 2 );
  cr->set_source_rgb(0, 0, 0);
  cr->rectangle(bx, by + 22, bw, bh - 22);
  cr->stroke();
  // cr->move_to( bx+12, by+15 );
  // cr->show_text( filename.c_str() );
  // cr->move_to( bx, by+22 );
  // cr->line_to( bx+bw,by+22);
  // cr->stroke();
  cr->set_font_size(10);
  char ctmp[1024];
  ctmp[1023] = 0;
  switch(persp) {
  case scene_draw_t::xy:
    snprintf(ctmp, 1023, "top ortho");
    break;
  case scene_draw_t::xz:
    snprintf(ctmp, 1023, "front ortho");
    break;
  case scene_draw_t::yz:
    snprintf(ctmp, 1023, "left ortho");
    break;
  case scene_draw_t::xyz:
    snprintf(ctmp, 1023, "xyz");
    break;
  case scene_draw_t::p:
    snprintf(ctmp, 1023, "perspective");
    break;
  }
  cr->move_to(bx + 12, by + 36);
  cr->show_text(ctmp);
  if(persp == scene_draw_t::p)
    snprintf(ctmp, 1023, "fov %g°", drawer.view.get_fov());
  else
    snprintf(ctmp, 1023, "scale 1:%g",
             drawer.view.get_scale() / (wscale / 72 * 0.0254));
  cr->move_to(bx + 12, by + 48);
  cr->show_text(ctmp);
  surface->show_page();
}

class wrfun_t {
public:
  std::stringstream s;
  Cairo::ErrorStatus wr(const unsigned char* data, unsigned int len)
  {
    s.write(reinterpret_cast<const char*>(data), len);
    return CAIRO_STATUS_SUCCESS;
  };
};

std::string TASCAR::export_svg(TASCAR::render_core_t* scene,
                               double width_in_points, double height_in_points,
                               TSCGUI::scene_draw_t::viewt_t persp)
{
  double lmargin = 0;
  double rmargin = 0;
  double tmargin = 0;
  double bmargin = 0;
  wrfun_t svg;
  {
    auto surface = Cairo::SvgSurface::create_for_stream(
        sigc::mem_fun(svg, &wrfun_t::wr), width_in_points, height_in_points);
    TSCGUI::scene_draw_t drawer;
    drawer.set_scene(scene);
    double wscale(0.5 * std::max(height_in_points, width_in_points));
    double res(wscale / 72 * 0.0254);
    double nscale(scene->guiscale / res);
    nscale = pow(10.0, ceil(log10(nscale)));
    std::vector<double> div;
    div.push_back(1);
    div.push_back(2);
    div.push_back(2.5);
    div.push_back(10.0 / 3.0);
    div.push_back(4);
    div.push_back(5);
    div.push_back(8);
    uint32_t k(0);
    while((k < div.size()) && (nscale / div[k] >= scene->guiscale / res))
      k++;
    if(k > 0)
      nscale /= div[k - 1];
    drawer.view.set_scale(nscale * res);
    drawer.set_viewport(persp);
    Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
    // view.set_ref(listener.get_location(time));
    cr->rectangle(lmargin, tmargin, width_in_points - lmargin - rmargin,
                  height_in_points - tmargin - bmargin);
    cr->clip();
    cr->save();
    cr->translate(0.5 * width_in_points, 0.5 * height_in_points);
    double markersize(0.02);
    cr->scale(wscale, wscale);
    cr->set_line_width(0.3 * markersize);
    cr->set_font_size(3 * markersize);
    cr->save();
    cr->set_source_rgb(1, 1, 1);
    cr->paint();
    cr->restore();
    drawer.set_markersize(markersize);
    drawer.draw(cr);
    cr->set_source_rgba(0.2, 0.2, 0.2, 0.8);
    cr->move_to(-markersize, 0);
    cr->line_to(markersize, 0);
    cr->move_to(0, -markersize);
    cr->line_to(0, markersize);
    cr->stroke();
    cr->restore();
    surface->show_page();
  }
  return svg.s.str();
}

std::string TASCAR::export_svg(TASCAR::session_t* s, double width_in_points,
                               double height_in_points,
                               TSCGUI::scene_draw_t::viewt_t persp)
{
  double lmargin = 0;
  double rmargin = 0;
  double tmargin = 0;
  double bmargin = 0;
  wrfun_t svg;
  {
    auto surface = Cairo::SvgSurface::create_for_stream(
        sigc::mem_fun(svg, &wrfun_t::wr), width_in_points, height_in_points);
    for(auto scene : s->scenes) {
      DEBUG("scene");
      TSCGUI::scene_draw_t drawer;
      drawer.set_scene(scene);
      double wscale(0.5 * std::max(height_in_points, width_in_points));
      double res(wscale / 72 * 0.0254);
      double nscale(scene->guiscale / res);
      nscale = pow(10.0, ceil(log10(nscale)));
      std::vector<double> div;
      div.push_back(1);
      div.push_back(2);
      div.push_back(2.5);
      div.push_back(10.0 / 3.0);
      div.push_back(4);
      div.push_back(5);
      div.push_back(8);
      uint32_t k(0);
      while((k < div.size()) && (nscale / div[k] >= scene->guiscale / res))
        k++;
      if(k > 0)
        nscale /= div[k - 1];
      drawer.view.set_scale(nscale * res);
      drawer.set_viewport(persp);
      Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
      // view.set_ref(listener.get_location(time));
      cr->rectangle(lmargin, tmargin, width_in_points - lmargin - rmargin,
                    height_in_points - tmargin - bmargin);
      cr->clip();
      cr->save();
      cr->translate(0.5 * width_in_points, 0.5 * height_in_points);
      double markersize(0.02);
      cr->scale(wscale, wscale);
      cr->set_line_width(0.3 * markersize);
      cr->set_font_size(3 * markersize);
      cr->save();
      cr->set_source_rgb(1, 1, 1);
      cr->paint();
      cr->restore();
      drawer.set_markersize(markersize);
      drawer.draw(cr);
      cr->set_source_rgba(0.2, 0.2, 0.2, 0.8);
      cr->move_to(-markersize, 0);
      cr->line_to(markersize, 0);
      cr->move_to(0, -markersize);
      cr->line_to(0, markersize);
      cr->stroke();
      cr->restore();
      surface->show_page();
    }
  }
  return svg.s.str();
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
