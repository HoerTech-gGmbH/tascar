/**
 * @file   pdfexport.h
 * @author Giso Grimm
 * @ingroup GUI
 * @brief  PDF export class
 */ 
/* License (GPL)
 *
 * Copyright (C) 2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/ or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef PDFEXPORT_H
#define PDFEXPORT_H

#include "session.h"
#include "gui_elements.h"

namespace TASCAR {

  /// Export TASCAR scene to PDF
  class pdfexport_t {
  public:
    pdfexport_t(TASCAR::session_t* s,const std::string& fname, bool acmodel);
  private:
    void draw(TSCGUI::scene_draw_t::viewt_t persp);
    void draw_views(TASCAR::scene_render_rt_t* s);
    TSCGUI::scene_draw_t drawer;
    std::string filename;
    double height;
    double width;
    double lmargin;
    double rmargin;
    double tmargin;
    double bmargin;
    Cairo::RefPtr<Cairo::PdfSurface> surface;
    bool b_acmodel;
  };

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
