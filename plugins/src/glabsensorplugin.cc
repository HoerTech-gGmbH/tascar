#include "glabsensorplugin.h"

using namespace TASCAR;

sensormsg_t::sensormsg_t(const std::string& msg_ )
  : msg(msg_),
    count(1)
{
}

sensorplugin_base_t::sensorplugin_base_t( const sensorplugin_cfg_t& cfg )
  : xml_element_t(cfg.xmlsrc),
    name(cfg.modname),
    modname(cfg.modname),
    alive_(false)
{
  GET_ATTRIBUTE(name);
  set_size_request(-1,100);
}

sensorplugin_base_t::~sensorplugin_base_t()
{
}

std::vector<sensormsg_t> sensorplugin_base_t::get_critical()
{
  std::vector<sensormsg_t> msg(msg_critical);
  msg_critical.clear();
  return msg;
}

std::vector<sensormsg_t> sensorplugin_base_t::get_warnings()
{
  std::vector<sensormsg_t> msg(msg_warnings);
  msg_warnings.clear();
  return msg;
}

void sensorplugin_base_t::add_critical(const std::string& msg)
{
  if( msg_critical.empty() )
    msg_critical.push_back( sensormsg_t(msg) );
  else{
    if( msg_critical.back().msg == msg )
      msg_critical.back().count++;
    else
      msg_critical.push_back( sensormsg_t(msg) );
  }
}

void sensorplugin_base_t::add_warning(const std::string& msg)
{
  if( msg_warnings.empty() )
    msg_warnings.push_back( sensormsg_t(msg) );
  else{
    if( msg_warnings.back().msg == msg )
      msg_warnings.back().count++;
    else
      msg_warnings.push_back( sensormsg_t(msg) );
  }
}

sensorplugin_drawing_t::sensorplugin_drawing_t( const sensorplugin_cfg_t& cfg )
  : sensorplugin_base_t(cfg)
{
  sensorplugin_base_t::add(da);
  connection_timeout = Glib::signal_timeout().connect( sigc::mem_fun(*this, &sensorplugin_drawing_t::invalidatewin), 100 );
  da.signal_draw().connect( sigc::mem_fun(*this,&sensorplugin_drawing_t::da_draw) );
}

sensorplugin_drawing_t::~sensorplugin_drawing_t()
{
  connection_timeout.disconnect();
}

bool sensorplugin_drawing_t::invalidatewin()
{
  Glib::RefPtr<Gdk::Window> win(da.get_window());  
  if( win ){
    Gdk::Rectangle r(0, 0, 
                     da.get_allocation().get_width(),
                     da.get_allocation().get_height());
    //win->invalidate_rect(da.get_allocation(),true);
    win->invalidate_rect(r,true);
  }
  return true;
}

bool sensorplugin_drawing_t::da_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  Glib::RefPtr<Gdk::Window> window(da.get_window());
  if(window){
    Gtk::Allocation allocation(da.get_allocation());
    const int width(allocation.get_width());
    const int height(allocation.get_height());
    cr->save();
    cr->rectangle(0,0,width,height);
    cr->clip();
    cr->set_source_rgb( 1.0, 1.0, 1.0 );
    cr->paint();
    cr->restore();
    cr->save();
    cr->set_source_rgb( 0, 0, 0 );
    draw(cr, width, height );
    cr->restore();
  }
  return true;
}

void TASCAR::ctext_at( const Cairo::RefPtr<Cairo::Context>& cr, double x, double y, const std::string& t )
{
  Cairo::TextExtents extents;
  cr->get_text_extents( t, extents );
  cr->move_to( x - (0.5*extents.width + extents.x_bearing), 
               y - (0.5*extents.height + extents.y_bearing) );
  cr->show_text( t );
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

