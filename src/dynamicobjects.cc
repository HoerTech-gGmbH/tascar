#include "dynamicobjects.h"
#include <fstream>
#include "errorhandling.h"

TASCAR::navmesh_t::navmesh_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc),
    maxstep(0.5),
    zshift(0)
{
  std::string importraw;
  GET_ATTRIBUTE(maxstep);
  GET_ATTRIBUTE(importraw);
  GET_ATTRIBUTE(zshift);
  if( !importraw.empty() ){
    std::ifstream rawmesh( TASCAR::env_expand(importraw).c_str() );
    if( !rawmesh.good() )
      throw TASCAR::ErrMsg("Unable to open mesh file \""+TASCAR::env_expand(importraw)+"\".");
    while(!rawmesh.eof() ){
      std::string meshline;
      getline(rawmesh,meshline,'\n');
      if( !meshline.empty() ){
        TASCAR::ngon_t* p_face(new TASCAR::ngon_t());
        p_face->nonrt_set(TASCAR::str2vecpos(meshline));
        mesh.push_back(p_face);
      }
    }
  }
  std::stringstream txtmesh(TASCAR::xml_get_text(xmlsrc,"faces"));
  while(!txtmesh.eof() ){
    std::string meshline;
    getline(txtmesh,meshline,'\n');
    if( !meshline.empty() ){
      TASCAR::ngon_t* p_face(new TASCAR::ngon_t());
      p_face->nonrt_set(TASCAR::str2vecpos(meshline));
      mesh.push_back(p_face);
    }
  }
  for(std::vector<TASCAR::ngon_t*>::iterator it=mesh.begin();it!=mesh.end();++it)
    *(*it) += TASCAR::pos_t(0,0,zshift);
}

void TASCAR::navmesh_t::update_pos(TASCAR::pos_t& p)
{
  if( mesh.empty() )
    return;
  //p.z -= zshift;
  TASCAR::pos_t pnearest(mesh[0]->nearest(p));
  double dist((p.x-pnearest.x)*(p.x-pnearest.x) + (p.y-pnearest.y)*(p.y-pnearest.y) + 1e-3*(p.z-pnearest.z)*(p.z-pnearest.z));
  for(std::vector<TASCAR::ngon_t*>::iterator it=mesh.begin();it!=mesh.end();++it){
    TASCAR::pos_t pnl((*it)->nearest(p));
    double ld((p.x-pnl.x)*(p.x-pnl.x) + (p.y-pnl.y)*(p.y-pnl.y) + 1e-3*(p.z-pnl.z)*(p.z-pnl.z));
    if( (ld < dist) && (pnl.z-p.z <= maxstep) ){
      pnearest = pnl;
      dist = ld;
    }
  }
  p = pnearest;
  //p.z += zshift;
}

TASCAR::navmesh_t::~navmesh_t()
{
  for(std::vector<TASCAR::ngon_t*>::iterator it=mesh.begin();it!=mesh.end();++it)
    delete (*it);
}

TASCAR::dynobject_t::dynobject_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc),
    starttime(0),
    c6dof(c6dof_),
    xml_location(NULL),
    xml_orientation(NULL),
    navmesh(NULL)
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
      std::string sloop(sne->get_attribute_value("loop"));
      if( !sloop.empty() )
        location.loop = (sloop == "true");
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
    if( sne && (sne->get_name() == "navmesh") ){
      navmesh = new TASCAR::navmesh_t(sne);
    }
  }
  geometry_update(0);
  c6dof_prev = c6dof_;
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
  c6dof_prev = c6dof_;
  double ltime(time-starttime);
  c6dof_.p = location.interp(ltime);
  TASCAR::pos_t ptmp(c6dof_.p);
  c6dof_.p += dlocation;
  c6dof_.o = orientation.interp(ltime);
  c6dof_.o += dorientation;
  if( navmesh ){
    navmesh->update_pos( c6dof_.p );
    dlocation = c6dof_.p;
    dlocation -= ptmp;
  }
}

TASCAR::pos_t TASCAR::dynobject_t::get_location() const
{
  return c6dof_.p;
}

TASCAR::zyx_euler_t TASCAR::dynobject_t::get_orientation() const
{
  return c6dof_.o;
}

void TASCAR::dynobject_t::get_6dof(pos_t& p,zyx_euler_t& o) const
{
  p = c6dof_.p;
  o = c6dof_.o;
}

void TASCAR::dynobject_t::get_6dof_prev(pos_t& p,zyx_euler_t& o) const
{
  p = c6dof_prev.p;
  o = c6dof_prev.o;
}

TASCAR::dynobject_t::~dynobject_t()
{
  if( navmesh )
    delete navmesh;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
