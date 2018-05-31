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
  private:
    dynobject_t(const dynobject_t&);
    c6dof_t c6dof_;
    c6dof_t c6dof_prev;
    xmlpp::Element* xml_location;
    xmlpp::Element* xml_orientation;
    navmesh_t* navmesh;
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
