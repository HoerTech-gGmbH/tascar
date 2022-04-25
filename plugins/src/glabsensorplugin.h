/* License (GPL)
 *
 * Copyright (C) 2014,2015,2016,2017,2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/or
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
#ifndef GLABSENSORS_H
#define GLABSENSORS_H

#include "osc_helper.h"
#include "tascarplugin.h"
#include "tscconfig.h"
#include <gtkmm.h>

namespace TASCAR {

  class sensormsg_t {
  public:
    sensormsg_t(const std::string& msg_ = "");
    std::string msg;
    uint32_t count;
  };

  class sensorplugin_cfg_t {
  public:
    sensorplugin_cfg_t(tsccfg::node_t xmlsrc) : xmlsrc(xmlsrc){};
    tsccfg::node_t xmlsrc;
    std::string modname;
    std::string hostname;
  };

  /**
     \brief Base class of sensor plugins
  */
  class sensorplugin_base_t : public TASCAR::xml_element_t, public Gtk::Frame {
  public:
    sensorplugin_base_t(const sensorplugin_cfg_t& cfg);
    virtual ~sensorplugin_base_t();
    const std::string& get_name() const { return name; };
    const std::string& get_modname() const { return modname; };
    virtual Gtk::Widget& get_gtkframe() { return *((Gtk::Widget*)this); };
    virtual void add_variables(TASCAR::osc_server_t*){};
    virtual std::vector<sensormsg_t> get_critical();
    virtual std::vector<sensormsg_t> get_warnings();
    virtual bool get_alive()
    {
      bool r(alive_);
      alive_ = false;
      return r;
    };
    virtual void prepare(){};
    virtual void release(){};

  protected:
    void add_critical(const std::string& msg);
    void add_warning(const std::string& msg);
    void alive() { alive_ = true; };
    bool get_alive_const() const { return alive_; };

  protected:
    std::string name;

  protected:
    std::string modname;
    double alivetimeout;

  private:
    std::vector<sensormsg_t> msg_critical;
    std::vector<sensormsg_t> msg_warnings;
    bool alive_;
  };

  class sensorplugin_drawing_t : public sensorplugin_base_t {
  public:
    sensorplugin_drawing_t(const sensorplugin_cfg_t& cfg);
    ~sensorplugin_drawing_t();

  protected:
    virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr, double width,
                      double height) = 0;
    Gtk::DrawingArea da;

  private:
    bool da_draw(const Cairo::RefPtr<Cairo::Context>& cr);
    bool invalidatewin();
    sigc::connection connection_timeout;
  };

  void ctext_at(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y,
                const std::string& t);

} // namespace TASCAR

#define REGISTER_SENSORPLUGIN(x)                                               \
  TASCAR_PLUGIN(sensorplugin_base_t, const sensorplugin_cfg_t&, x)

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
