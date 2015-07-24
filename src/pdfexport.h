#ifndef PDFEXPORT_H
#define PDFEXPORT_H

#include "session.h"
#include "gui_elements.h"

namespace TASCAR {

  class pdfexport_t {
  public:
    pdfexport_t(TASCAR::session_t* s,const std::string& fname, bool acmodel);
  private:
    void draw(scene_draw_t::viewt_t persp);
    void draw_views(TASCAR::renderer_t* s);
    scene_draw_t drawer;
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
