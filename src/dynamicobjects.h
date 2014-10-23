#ifndef DYNAMICOBJECTS_H
#define DYNAMICOBJECTS_H

#include "coordinates.h"
#include "xmlconfig.h"

namespace TASCAR {

  class dynobject_t : public xml_element_t {
  public:
    dynobject_t(xmlpp::Element*);
    void write_xml();
    void geometry_update(double t);
    pos_t get_location();
    zyx_euler_t get_orientation();
    void get_6dof(pos_t&,zyx_euler_t&);
    double starttime;
    track_t location;
    euler_track_t orientation;
    pos_t dlocation;
    zyx_euler_t dorientation;
  private:
    double localtime_;
    pos_t cached_pos;
    zyx_euler_t cached_rot;
    xmlpp::Element* xml_location;
    xmlpp::Element* xml_orientation;
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
