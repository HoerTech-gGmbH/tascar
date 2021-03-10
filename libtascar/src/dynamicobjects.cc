#include "dynamicobjects.h"
#include <fstream>
#include "errorhandling.h"

TASCAR::navmesh_t::navmesh_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc),
    maxstep(0.5),
    zshift(0)
{
  std::string importraw;
  GET_ATTRIBUTE(maxstep,"m","maximum step height of object");
  GET_ATTRIBUTE(importraw,"","file name of vertex list");
  GET_ATTRIBUTE(zshift,"m","shift object vertically");
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
    : xml_element_t(xmlsrc), starttime(0), sampledorientation(0), c6dof(c6dof_),
      c6dof_nodelta(c6dof_nodelta_), xml_location(NULL), xml_orientation(NULL),
      navmesh(NULL)
{
  get_attribute("start", starttime, "s",
                "time when rendering of object starts");
  GET_ATTRIBUTE(sampledorientation, "m",
                "sample orientation by line fit into curve");
  GET_ATTRIBUTE(localpos, "m", "local position");
  GET_ATTRIBUTE(dlocation, "m", "delta location");
  GET_ATTRIBUTE_NOUNIT(dorientation, "delta orientation");
  for(auto sn : e->get_children()) {
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(sn));
    if(sne && (sne->get_name() == "position")) {
      xml_location = sne;
      location.read_xml(sne);
    }
    if(sne && (sne->get_name() == "orientation")) {
      xml_orientation = sne;
      orientation.read_xml(sne);
    }
    if(sne && (sne->get_name() == "creator")) {
      for(auto node : sne->get_children())
        location.edit(dynamic_cast<xmlpp::Element*>(node));
      TASCAR::track_t::iterator it_old = location.end();
      double old_azim(0);
      double new_azim(0);
      for(TASCAR::track_t::iterator it = location.begin(); it != location.end();
          ++it) {
        if(it_old != location.end()) {
          pos_t p = it->second;
          p -= it_old->second;
          new_azim = p.azim();
          while(new_azim - old_azim > M_PI)
            new_azim -= 2 * M_PI;
          while(new_azim - old_azim < -M_PI)
            new_azim += 2 * M_PI;
          orientation[it_old->first] = zyx_euler_t(new_azim, 0, 0);
          old_azim = new_azim;
        }

        if((it_old == location.end()) ||
           (TASCAR::distance(it->second, it_old->second) > 0))
          it_old = it;
      }
      double loop(0);
      get_attribute_value(sne, "loop", loop);
      if(loop > 0) {
        location.loop = loop;
        orientation.loop = loop;
      }
    }
    if(sne && (sne->get_name() == "navmesh")) {
      navmesh = new TASCAR::navmesh_t(sne);
    }
  }
  location.prepare();
  geometry_update(0);
  c6dof_prev = c6dof_;
}

void TASCAR::dynobject_t::geometry_update(double time)
{
  c6dof_prev = c6dof_;
  double ltime(time-starttime);
  c6dof_.position = location.interp(ltime);
  TASCAR::pos_t ptmp(c6dof_.position);
  c6dof_nodelta_.position = c6dof_.position;
  c6dof_.position += dlocation;
  if( sampledorientation == 0 )
    c6dof_.orientation = orientation.interp(ltime);
  else{
    double tp(location.get_time(location.get_dist(ltime)-sampledorientation));
    TASCAR::pos_t pdt(c6dof_.position);
    pdt -= location.interp(tp);
    if( sampledorientation < 0 )
      pdt *= -1.0;
    c6dof_.orientation.z = pdt.azim();
    c6dof_.orientation.y = pdt.elev();
    c6dof_.orientation.x = 0.0;
  }
  c6dof_nodelta_.orientation = c6dof_.orientation;
  c6dof_.orientation += dorientation;
  if( navmesh ){
    navmesh->update_pos( c6dof_.position );
    dlocation = c6dof_.position;
    dlocation -= ptmp;
  }
  ptmp = localpos;
  ptmp *= c6dof_.orientation;
  c6dof_.position += ptmp;
}

TASCAR::pos_t TASCAR::dynobject_t::get_location() const
{
  return c6dof_.position;
}

TASCAR::zyx_euler_t TASCAR::dynobject_t::get_orientation() const
{
  return c6dof_.orientation;
}

void TASCAR::dynobject_t::get_6dof(pos_t& p,zyx_euler_t& o) const
{
  p = c6dof_.position;
  o = c6dof_.orientation;
}

void TASCAR::dynobject_t::get_6dof_prev(pos_t& p,zyx_euler_t& o) const
{
  p = c6dof_prev.position;
  o = c6dof_prev.orientation;
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
