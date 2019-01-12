#include <iostream>
#include <set>
#include "session.h"
#include "errorhandling.h"
#include "timedisplay_glade.h"
#include <gdkmm/event.h>
#include <gtkmm.h>
#include <gtkmm/builder.h>
#include <gtkmm/window.h>

#define GET_WIDGET(x) refBuilder->get_widget(#x,x);if( !x ) throw TASCAR::ErrMsg(std::string("No widget \"")+ #x + std::string("\" in builder."))

class timedisplay_t : public TASCAR::module_base_t {
public:
  timedisplay_t( const TASCAR::module_cfg_t& cfg );
  ~timedisplay_t();
  bool on_timeout();
private:
  std::set<double> tset;
  bool remaining;
  double fontscale;
  double threshold;
  Glib::RefPtr<Gtk::Builder> refBuilder;
  Gtk::Window* timedisplaywindow;
  Gtk::Label* label;
  sigc::connection connection_timeout;
  Gdk::RGBA col_threshold;
  Gdk::RGBA col_normal;
};

timedisplay_t::timedisplay_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    remaining( false ),
    fontscale( 1 ),
    threshold( 0 )
{
  col_threshold.set_rgba(0.8,0.1,0.1,1);
  col_normal.set_rgba(0,0,0,1);
  std::vector<double> times;
  GET_ATTRIBUTE(times);
  GET_ATTRIBUTE_BOOL(remaining);
  GET_ATTRIBUTE( fontscale );
  GET_ATTRIBUTE( threshold );
  for(uint32_t k=0;k<times.size();++k)
    tset.insert( times[k] );
  //activate();
  refBuilder = Gtk::Builder::create_from_string(ui_timedisplay);
  GET_WIDGET(timedisplaywindow);
  GET_WIDGET(label);
  timedisplaywindow->show();
  connection_timeout = Glib::signal_timeout().connect( sigc::mem_fun(*this, &timedisplay_t::on_timeout), 100 );
  Pango::AttrList attrlist;
  Pango::Attribute fscale(Pango::Attribute::create_attr_scale( fontscale ));
  attrlist.insert(fscale);
  label->set_attributes( attrlist );
  int x(0);
  int y(0);
  int w(1);
  int h(1);
  timedisplaywindow->get_position(x,y);
  timedisplaywindow->get_size(w,h);
  GET_ATTRIBUTE(x);
  GET_ATTRIBUTE(y);
  GET_ATTRIBUTE(w);
  GET_ATTRIBUTE(h);
  timedisplaywindow->move(x,y);
  timedisplaywindow->resize(w,h);
}

timedisplay_t::~timedisplay_t()
{
  connection_timeout.disconnect();
  delete timedisplaywindow;
  delete label;
}

bool timedisplay_t::on_timeout()
{
  char ctmp[1024];
  double ltime(session->tp_get_time());
  if( tset.empty() ){
    if( remaining )
      ltime = session->duration - ltime;
  }else{
    std::set<double>::iterator b(tset.lower_bound( ltime ));
    if( remaining ){
      if( b == tset.end() )
	--b;
      ltime = *b - ltime;
    }else{
      if( b != tset.begin() )
	--b;
      ltime = ltime - *b;
    }
  }
  sprintf(ctmp,"%1.1f s",ltime);
  if( ltime < threshold )
    label->override_color( col_threshold );
  else
    label->override_color( col_normal );
  label->set_text(ctmp);
  return true;
}

REGISTER_MODULE(timedisplay_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */

