#include "dynamicobjects.h"

TASCAR::dynobject_t::dynobject_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc),
    starttime(0),
    c6dof(c6dof_),
    xml_location(NULL),
    xml_orientation(NULL)
{
  get_attribute("start",starttime);
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "position")){
      xml_location = sne;
      location.read_xml(sne);
    }
    if( sne && ( sne->get_name() == "orientation")){
      xml_orientation = sne;
      orientation.read_xml(sne);
    }
    if( sne && (sne->get_name() == "creator")){
      xmlpp::Node::NodeList subnodes = sne->get_children();
      location.edit(subnodes);
      TASCAR::track_t::iterator it_old=location.end();
      double old_azim(0);
      double new_azim(0);
      for(TASCAR::track_t::iterator it=location.begin();it!=location.end();++it){
        if( it_old != location.end() ){
          pos_t p=it->second;
          p -= it_old->second;
          new_azim = p.azim();
          while( new_azim - old_azim > M_PI )
            new_azim -= 2*M_PI;
          while( new_azim - old_azim < -M_PI )
            new_azim += 2*M_PI;
          orientation[it_old->first] = zyx_euler_t(new_azim,0,0);
          old_azim = new_azim;
        }
        if( TASCAR::distance(it->second,it_old->second) > 0 )
          it_old = it;
      }
    }
  }
}

void TASCAR::dynobject_t::write_xml()
{
  set_attribute("start",starttime);
  if( location.size() ){
    if( !xml_location )
      xml_location = e->add_child("position");
    location.write_xml( xml_location );
  }
  if( orientation.size() ){
    if( !xml_orientation )
      xml_orientation = e->add_child("orientation");
    orientation.write_xml( xml_orientation );
  }
}

void TASCAR::dynobject_t::geometry_update(double time)
{
  double ltime(time-starttime);
  c6dof_.p = location.interp(ltime);
  c6dof_.p += dlocation;
  c6dof_.o = orientation.interp(ltime);
  c6dof_.o += dorientation;
}

TASCAR::pos_t TASCAR::dynobject_t::get_location()
{
  return c6dof_.p;
}

TASCAR::zyx_euler_t TASCAR::dynobject_t::get_orientation()
{
  return c6dof_.o;
}

void TASCAR::dynobject_t::get_6dof(pos_t& p,zyx_euler_t& o)
{
  p = c6dof_.p;
  o = c6dof_.o;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
