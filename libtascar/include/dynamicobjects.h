/**
 * @file   dynamicobjects.h
 * @author Giso Grimm
 * 
 * @brief  Base class for objects with trajectories
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
#ifndef DYNAMICOBJECTS_H
#define DYNAMICOBJECTS_H

#include "xmlconfig.h"

namespace TASCAR {

  class navmesh_t : public xml_element_t {
  public:
    navmesh_t(xmlpp::Element*);
    ~navmesh_t();
    void update_pos(TASCAR::pos_t& p);
  private:
    std::vector<TASCAR::ngon_t*> mesh;
    double maxstep;
    double zshift;
  };

  class dynobject_t : public xml_element_t {
  public:
    dynobject_t(xmlpp::Element*);
    ~dynobject_t();
    virtual void geometry_update(double t);
    pos_t get_location() const;
    zyx_euler_t get_orientation() const;
    void get_6dof(pos_t&,zyx_euler_t&) const;
    void get_6dof_prev(pos_t&,zyx_euler_t&) const;
    double starttime;
    double sampledorientation;
    track_t location;
    euler_track_t orientation;
    pos_t dlocation;
    zyx_euler_t dorientation;
    const c6dof_t& c6dof;
    const c6dof_t& c6dof_nodelta;
  private:
    dynobject_t(const dynobject_t&);
    c6dof_t c6dof_nodelta_;
    c6dof_t c6dof_;
    c6dof_t c6dof_prev;
    xmlpp::Element* xml_location;
    xmlpp::Element* xml_orientation;
    navmesh_t* navmesh;
    pos_t localpos;
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
