/**
   \file multipan.cc
   \brief "coordinates" provide classes for coordinate handling
   \ingroup libtascar
   \author Giso Grimm
   \date 2012

   \section license License (LGPL)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; version 2 of the
   License.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/
#include "coordinates.h"
#include <sstream>
#include "defs.h"
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include "errorhandling.h"

static uint32_t cnt_interp = 0;

uint32_t get_cnt_interp()
{
  uint32_t tmp(cnt_interp);
  cnt_interp = 0;
  return tmp;
}

using namespace TASCAR;

std::string pos_t::print_cart(const std::string& delim) const
{
  std::ostringstream tmp("");
  tmp.precision(12);
  tmp << x << delim << y << delim << z;
  return tmp.str();

}

std::string pos_t::print_sphere(const std::string& delim) const
{
  std::ostringstream tmp("");
  tmp.precision(12);
  tmp << norm() << delim << RAD2DEG*azim() << delim << RAD2DEG*elev();
  return tmp.str();

}

pos_t track_t::center()
{
  pos_t c;
  for(iterator i=begin();i!=end();++i){
    c += i->second;
  }
  if( size() )
    c /= size();
  return c;
}

track_t& track_t::operator+=(const pos_t& x)
{
  for(iterator i=begin();i!=end();++i){
    i->second += x;
  }
  return *this;
}

track_t& track_t::operator-=(const pos_t& x)
{
  for(iterator i=begin();i!=end();++i){
    i->second -= x;
  }
  return *this;
}

std::string track_t::print_cart(const std::string& delim)
{
  std::ostringstream tmp("");
  tmp.precision(12);
  for(iterator i=begin();i!=end();++i){
    tmp << i->first << delim << i->second.print_cart(delim) << "\n";
  }
  return tmp.str();
}
  
std::string track_t::print_sphere(const std::string& delim)
{
  std::ostringstream tmp("");
  tmp.precision(12);
  for(iterator i=begin();i!=end();++i){
    tmp << i->first << delim << i->second.print_sphere(delim) << "\n";
  }
  return tmp.str();
}

void track_t::project_tangent()
{
  project_tangent(center());
}

void track_t::project_tangent(pos_t c)
{
  rot_z( -c.azim() );
  rot_y( (PI_2 - c.elev()) );
  rot_z( -PI_2 );
  c.set_cart(0,0,-c.norm());
  operator+=(c);
  //operator*=(pos_t(1,-1,1));
}

void track_t::rot_z(double a)
{
  for(iterator i=begin();i!=end();++i)
    i->second.rot_z(a);
}

void track_t::rot_x(double a)
{
  for(iterator i=begin();i!=end();++i)
    i->second.rot_x(a);
}

void track_t::rot_y(double a)
{
  for(iterator i=begin();i!=end();++i)
    i->second.rot_y(a);
}


track_t& track_t::operator*=(const pos_t& x)
{
  for(iterator i=begin();i!=end();++i){
    i->second *= x;
  }
  return *this;
}

void track_t::shift_time(double dt)
{
  track_t nt;
  for(iterator i=begin();i!=end();++i){
    nt[i->first+dt] = i->second;
  }
  *this = nt;
  prepare();
}

pos_t track_t::interp(double x) const
{
  cnt_interp++;
  if( begin() == end() )
    return pos_t();
  const_iterator lim2 = lower_bound(x);
  if( lim2 == end() )
    return rbegin()->second;
  if( lim2 == begin() )
    return begin()->second;
  if( lim2->first == x )
    return lim2->second;
  const_iterator lim1 = lim2;
  --lim1;
  if( interpt == track_t::cartesian ){
    // cartesian interpolation:
    pos_t p1(lim1->second);
    pos_t p2(lim2->second);
    double w = (x-lim1->first)/(lim2->first-lim1->first);
    make_friendly_number(w);
    p1 *= (1.0-w);
    p2 *= w;
    p1 += p2;
    return p1;
  }else{
    // spherical interpolation:
    sphere_t p1(lim1->second);
    sphere_t p2(lim2->second);
    double w = (x-lim1->first)/(lim2->first-lim1->first);
    make_friendly_number(w);
    p1 *= (1.0-w);
    p2 *= w;
    p1.r += p2.r;
    p1.az += p2.az;
    p1.el += p2.el;
    return p1.cart();
  }
}

void track_t::smooth( unsigned int n )
{
  unsigned int n_in= size();
  track_t sm;
  std::vector<pos_t> vx;
  std::vector<double> vt;
  vx.resize(n_in);
  vt.resize(n_in);
  unsigned int n2 = n/2;
  unsigned int k=0;
  for(iterator i=begin();i!=end();++i){
    vt[k] = i->first;
    vx[k] = i->second;
    k++;
  }
  std::vector<double> wnd;
  wnd.resize(n);
  double wsum(0);
  for(k = 0;k<n;k++){
    wnd[k] = 0.5 - 0.5*cos(PI2*(k+1)/(n+1));
    wsum+=wnd[k];
  }
  make_friendly_number(wsum);
  for(k = 0;k<n;k++){
    wnd[k]/=wsum;
  }
  for( k=0;k<n_in;k++ ){
    pos_t ps;
    for( unsigned int kw=0;kw<n;kw++){
      pos_t p = vx[std::min(std::max(k+kw,n2)-n2,n_in-1)];
      p *= wnd[kw];
      ps += p;
    }
    sm[vt[k]] = ps;
  }
  *this = sm;
  prepare();
}  

std::string TASCAR::xml_get_text( xmlpp::Node* n, const std::string& child )
{
  if( n ){
    if( child.size() ){
      xmlpp::Node::NodeList ch = n->get_children( child );
      if( ch.size() ){
        xmlpp::NodeSet stxt = (*ch.begin())->find("text()");
        xmlpp::TextNode* txt = dynamic_cast<xmlpp::TextNode*>(*(stxt.begin()));
        if( txt ){
          return txt->get_content();
        }
      }
    }else{
      xmlpp::NodeSet stxt = n->find("text()");
      xmlpp::TextNode* txt = dynamic_cast<xmlpp::TextNode*>(*(stxt.begin()));
      if( txt ){
        return txt->get_content();
      }
    }
  }
  return "";
}


pos_t TASCAR::xml_get_trkpt( xmlpp::Element* pt, time_t& tme )
{
  double lat = atof(pt->get_attribute_value("lat").c_str());
  double lon = atof(pt->get_attribute_value("lon").c_str());
  std::string stm = xml_get_text( pt, "time" );
  struct tm bdtm;
  tme = 0;
  memset(&bdtm,0,sizeof(bdtm));
  if( strptime(stm.c_str(),"%Y-%m-%dT%T",&bdtm) ){
    tme = mktime( &bdtm );
  }
  std::string selev = xml_get_text( pt, "ele" );
  double elev = 0;
  if( selev.size() )
    elev = atof(selev.c_str());
  TASCAR::pos_t p;
  p.set_sphere(R_EARTH+elev,DEG2RAD*lon,DEG2RAD*lat);
  return p;
}

/**
   \brief load a track from a gpx file
*/
void track_t::load_from_gpx( const std::string& fname )
{
  double ttinc(0);
  track_t track;
  xmlpp::DomParser parser(fname);
  xmlpp::Element* root = parser.get_document()->get_root_node();
  if( root ){
    xmlpp::Node::NodeList xmltrack = root->get_children("trk");
    if( xmltrack.size() ){
      xmlpp::Node::NodeList trackseg = (*(xmltrack.begin()))->get_children("trkseg");
      if( trackseg.size() ){
        xmlpp::Node::NodeList trackpt = (*(trackseg.begin()))->get_children("trkpt");
        for(xmlpp::Node::NodeList::iterator itrackpt=trackpt.begin();itrackpt!=trackpt.end();++itrackpt){
          xmlpp::Element* loc = dynamic_cast<xmlpp::Element*>(*itrackpt);
          if( loc ){
            time_t tm;
            pos_t p = xml_get_trkpt( loc, tm );
            double ltm(tm);
            if( ltm == 0 )
              ltm = ttinc;
            track[ltm] = p;
            ttinc += 1.0;
          }
        }
      }
    }
  }
  *this = track;
  prepare();
}

/**
   \brief load a track from a gpx file
*/
void track_t::load_from_csv( const std::string& fname )
{
  track_t track;
  std::ifstream fh(fname.c_str());
  if( fh.fail() )
    throw TASCAR::ErrMsg("Unable to open track csv file \""+fname+"\".");
  std::string v_tm, v_x, v_y, v_z;
  while( !fh.eof() ){
    getline(fh,v_tm,',');
    getline(fh,v_x,',');
    getline(fh,v_y,',');
    getline(fh,v_z);
    if( v_tm.size() && v_x.size() && v_y.size() && v_z.size() ){
      double tm = atof(v_tm.c_str());
      double x = atof(v_x.c_str());
      double y = atof(v_y.c_str());
      double z = atof(v_z.c_str());
      track[tm] = pos_t(x,y,z);
    }
  }
  fh.close();
  *this = track;
  prepare();
}

void track_t::edit( xmlpp::Element* cmd )
{
  if( cmd ){
    std::string scmd(cmd->get_name());
    if( scmd == "load" ){
      std::string filename = cmd->get_attribute_value("name");
      std::string filefmt = cmd->get_attribute_value("format");
      if( filefmt == "gpx" ){
        load_from_gpx(filename);
      }else if( filefmt == "csv" ){
        load_from_csv(filename);
      }else{
        DEBUG("invalid file format");
        DEBUG(filefmt);
      }
    }else if( scmd == "origin" ){
      std::string normtype = cmd->get_attribute_value("src");
      std::string normmode = cmd->get_attribute_value("mode");
      TASCAR::pos_t orig;
      if( normtype == "center" ){
        orig = center();
      }else if( normtype == "trkpt" ){
        xmlpp::Node::NodeList trackpt = cmd->get_children("trkpt");
        xmlpp::Element* loc = dynamic_cast<xmlpp::Element*>(*(trackpt.begin()));
        if( loc ){
          time_t tm;
          orig = xml_get_trkpt( loc, tm );
        }
      }
      if( normmode == "tangent" ){
        project_tangent( orig );
      }else if( normmode == "translate" ){
        *this -= orig;
      }
    }else if( scmd == "addpoints" ){
      std::string fmt = cmd->get_attribute_value("format");
      //TASCAR::pos_t orig;
      if( fmt == "trkpt" ){
        double ttinc(0);
        if( rbegin() != rend() )
          ttinc = rbegin()->first;
        xmlpp::Node::NodeList trackpt = cmd->get_children("trkpt");
        for(xmlpp::Node::NodeList::iterator itrackpt=trackpt.begin();itrackpt!=trackpt.end();++itrackpt){
          xmlpp::Element* loc = dynamic_cast<xmlpp::Element*>(*itrackpt);
          if( loc ){
            time_t tm;
            pos_t p = xml_get_trkpt( loc, tm );
            double ltm(tm);
            if( ltm == 0 )
              ltm = ttinc;
            (*this)[ltm] = p;
            ttinc += 1.0;
          }
        }
      }
    }else if( scmd == "velocity" ){
      std::string vel(cmd->get_attribute_value("const"));
      if( vel.size() ){
        double v(atof(vel.c_str()));
        set_velocity_const( v );
      }
      std::string vel_fname(cmd->get_attribute_value("csvfile"));
      std::string s_offset(cmd->get_attribute_value("start"));
      if( vel_fname.size() ){
        double v_offset(0);
        if( s_offset.size() )
          v_offset = atof(s_offset.c_str());
        set_velocity_csvfile( vel_fname, v_offset );
      }
    }else if( scmd == "rotate" ){
      rot_z(DEG2RAD*atof(cmd->get_attribute_value("angle").c_str()));
    }else if( scmd == "scale" ){
      TASCAR::pos_t scale(atof(cmd->get_attribute_value("x").c_str()),
                          atof(cmd->get_attribute_value("y").c_str()),
                          atof(cmd->get_attribute_value("z").c_str()));
      *this *= scale;
    }else if( scmd == "translate" ){
      TASCAR::pos_t dx(atof(cmd->get_attribute_value("x").c_str()),
                       atof(cmd->get_attribute_value("y").c_str()),
                       atof(cmd->get_attribute_value("z").c_str()));
      *this += dx;
    }else if( scmd == "smooth" ){
      unsigned int n = atoi(cmd->get_attribute_value("n").c_str());
      if( n )
        smooth( n );
    }else if( scmd == "resample" ){
      double dt = atof(cmd->get_attribute_value("dt").c_str());
      resample(dt);
    }else if( scmd == "trim" ){
      prepare();
      double d_start = atof(cmd->get_attribute_value("start").c_str());
      double d_end = atof(cmd->get_attribute_value("end").c_str());
      double t_start(get_time(d_start));
      double t_end(get_time(length()-d_end));
      TASCAR::track_t ntrack;
      for( TASCAR::track_t::iterator it=begin();it != end(); ++it){
        if( (t_start < it->first) && (t_end > it->first) ){
          ntrack[it->first] = it->second;
        }
      }
      ntrack[t_start] = interp(t_start);
      ntrack[t_end] = interp(t_end);
      *this = ntrack;
      prepare();
    }else if( scmd == "time" ){
      // scale...
      std::string att_start(cmd->get_attribute_value("start"));
      if( att_start.size() ){
        double starttime = atof(att_start.c_str());
        shift_time(starttime - begin()->first);
      }
      std::string att_scale(cmd->get_attribute_value("scale"));
      if( att_scale.size() ){
        double scaletime = atof(att_scale.c_str());
        TASCAR::track_t ntrack;
        for( TASCAR::track_t::iterator it=begin();it != end(); ++it){
          ntrack[scaletime*it->first] = it->second;
        }
        *this = ntrack;
        prepare();
      }
    }else{
      DEBUG(cmd->get_name());
    }
  }
  prepare();
}

void track_t::resample( double dt )
{
  if( dt > 0 ){
    TASCAR::track_t ntrack;
    double t_begin = begin()->first;
    double t_end = rbegin()->first;
    for( double t = t_begin; t <= t_end; t+=dt )
      ntrack[t] = interp(t);
    *this = ntrack;
  }
  prepare();
}

void track_t::edit( xmlpp::Node::NodeList cmds )
{
  for( xmlpp::Node::NodeList::iterator icmd=cmds.begin();icmd != cmds.end();++icmd){
    xmlpp::Element* cmd = dynamic_cast<xmlpp::Element*>(*icmd);
    edit( cmd );
  }
  prepare();
}

void track_t::set_velocity_const( double v )
{
  if( v != 0 ){
    double t0(0);
    TASCAR::pos_t p0;
    if( begin() != end() ){
      t0 = begin()->first;
      p0 = begin()->second;
    }
    TASCAR::track_t ntrack;
    for( TASCAR::track_t::iterator it=begin();it != end(); ++it){
      double d = distance(p0,it->second);
      p0 = it->second;
      t0 += d/v;
      ntrack[t0] = p0;
    }
    *this = ntrack;
  }
  prepare();
}

void track_t::set_velocity_csvfile( const std::string& fname, double offset )
{
  std::ifstream fh(fname.c_str());
  if( fh.fail() )
    throw TASCAR::ErrMsg("Unable to open velocity csv file \""+fname+"\".");
  std::string v_tm, v_x;
  track_t vmap;
  while( !fh.eof() ){
    getline(fh,v_tm,',');
    getline(fh,v_x);
    if( v_tm.size() && v_x.size() ){
      double tm = atof(v_tm.c_str());
      double x = atof(v_x.c_str());
      vmap[tm-offset] = pos_t(x,0,0);
    }
  }
  fh.close();
  if( vmap.begin() != vmap.end() ){
    set_velocity_const( 1.0 );
    double d = 0;
    double dt = 0.5;
    track_t ntrack;
    for( double tm=std::max(0.0,vmap.begin()->first); tm <= vmap.rbegin()->first; tm += dt){
      pos_t pv = vmap.interp( tm );
      d += dt*pv.x;
      ntrack[tm] = interp( d );
    }
    *this = ntrack;
  }
  prepare();
}

double track_t::length()
{
  if( !size() )
    return 0;
  double l(0);
  pos_t p = begin()->second;
  for(iterator i=begin();i!=end();++i){
    l+=distance(p,i->second);
    p = i->second;
  }
  return l;
}

track_t::track_t()
  : interpt(cartesian)
{
}

void track_t::write_xml( xmlpp::Element* a)
{
  switch( interpt ){
  case TASCAR::track_t::cartesian:
    //a->set_attribute("interpolation","cartesian");
    break;
  case TASCAR::track_t::spherical:
    a->set_attribute("interpolation","spherical");
    break;
  }
  xmlpp::Node::NodeList ch = a->get_children();
  for(xmlpp::Node::NodeList::reverse_iterator sn=ch.rbegin();sn!=ch.rend();++sn)
    a->remove_child(*sn);
  a->add_child_text(print_cart(" "));
}

void track_t::read_xml( xmlpp::Element* a )
{
  track_t ntrack;
  if( a->get_attribute_value("interpolation") == "spherical" )
    ntrack.set_interpt(TASCAR::track_t::spherical);
  std::stringstream ptxt(xml_get_text(a,""));
  while( !ptxt.eof() ){
    double t(-1);
    pos_t p;
    ptxt >> t;
    if( !ptxt.eof() ){
      ptxt >> p.x >> p.y >> p.z;
      ntrack[t] = p;
    }
  }
  *this = ntrack;
}

table1_t::table1_t()
{
}

double table1_t::interp( double x ) const
{
  if( begin() == end() )
    return 0;
  const_iterator lim2 = lower_bound(x);
  if( lim2 == end() )
    return rbegin()->second;
  if( lim2 == begin() )
    return begin()->second;
  if( lim2->first == x )
    return lim2->second;
  const_iterator lim1 = lim2;
  --lim1;
  // cartesian interpolation:
  double p1(lim1->second);
  double p2(lim2->second);
  double w = (x-lim1->first)/(lim2->first-lim1->first);
  make_friendly_number(w);
  p1 *= (1.0-w);
  p2 *= w;
  p1 += p2;
  return p1;
}

void track_t::prepare()
{
  time_dist.clear();
  dist_time.clear();
  if( size() ){
    double l(0);
    pos_t p0(begin()->second);
    for(const_iterator it=begin();it!=end();++it){
      l+=distance(it->second,p0);
      time_dist[it->first] = l;
      dist_time[l] = it->first;
      p0 = it->second;
    }
  }
}

zyx_euler_t euler_track_t::interp(double x) const
{
  //DEBUG(size());
  if( begin() == end() ){
    //DEBUG("exit");
    return zyx_euler_t();
  }
  //DEBUG(x);
  //DEBUG(begin()->first);
  const_iterator lim2 = lower_bound(x);
  if( lim2 == end() )
    return rbegin()->second;
  if( lim2 == begin() )
    return begin()->second;
  if( lim2->first == x )
    return lim2->second;
  const_iterator lim1 = lim2;
  --lim1;
  zyx_euler_t p1(lim1->second);
  zyx_euler_t p2(lim2->second);
  double w = (x-lim1->first)/(lim2->first-lim1->first);
  make_friendly_number(w);
  p1 *= (1.0-w);
  p2 *= w;
  p1 += p2;
  return p1;
}

void euler_track_t::write_xml( xmlpp::Element* a)
{
  a->add_child_text(print(" "));
}

void euler_track_t::read_xml( xmlpp::Element* a )
{
  euler_track_t ntrack;
  std::stringstream ptxt(xml_get_text(a,""));
  while( !ptxt.eof() ){
    double t(-1);
    zyx_euler_t p;
    ptxt >> t;
    if( !ptxt.eof() ){
      ptxt >> p.z >> p.y >> p.x;
      p *= DEG2RAD;
      ntrack[t] = p;
    }
  }
  *this = ntrack;
}

std::string euler_track_t::print(const std::string& delim)
{
  std::ostringstream tmp("");
  tmp.precision(12);
  for(iterator i=begin();i!=end();++i){
    tmp << i->first << delim << i->second.print(delim) << "\n";
  }
  return tmp.str();
}

std::string zyx_euler_t::print(const std::string& delim)
{
  std::ostringstream tmp("");
  tmp.precision(12);
  tmp << RAD2DEG*z << delim << RAD2DEG*y << delim << RAD2DEG*x;
  return tmp.str();

}

void track_t::fill_gaps(double dt)
{
  if( empty() )
    return;
  track_t nt;
  double lt(begin()->first);
  for(iterator i=begin();i!=end();++i){
    nt[i->first] = i->second;
    double tdt(i->first-lt);
    unsigned int n(tdt/dt);
    if( n > 0 ){
      double ldt = tdt/n;
      for( double t=lt+ldt;t<i->first;t+=ldt )
        nt[t] = interp(t);
    }
    lt = i->first;
  }
  *this = nt;
  prepare();
}

face_t::face_t()
{
  set(pos_t(),zyx_euler_t(),2,1);
}

void face_t::set(const pos_t& p0, const zyx_euler_t& o, double width, double height)
{
  anchor = p0;
  e1 = pos_t(0,width,0);
  e2 = pos_t(0,0,height);
  e1 *= o;
  e2 *= o;
  width_ = width;
  height_ = height;
  orient_ = o;
  update();
}

void face_t::update()
{
  normal = cross_prod(e1,e2);
  normal /= normal.norm();
}

pos_t face_t::nearest_on_plane( const pos_t& p0 ) const
{
  double plane_dist = dot_prod(normal,anchor-p0);
  pos_t p0d = normal;
  p0d *= plane_dist;
  p0d += p0;
  return p0d;
}

pos_t face_t::nearest( const pos_t& p0 ) const
{
  // compensate orientation and calculate on two-dimensional plane:
  pos_t p0d(p0);
  p0d -= anchor;
  p0d /= orient_;
  p0d.x = 0;
  p0d.y = std::max(0.0,std::min(width_,p0d.y));
  p0d.z = std::max(0.0,std::min(height_,p0d.z));
  p0d *= orient_;
  p0d += anchor;
  return p0d;
}

pos_t pos_t::normal() const
{
  pos_t r(*this);
  r /= norm();
  return r;
}

void pos_t::normalize()
{
  *this /= norm();
}

bool pos_t::has_infinity() const
{
  if( x == std::numeric_limits<double>::infinity() )
    return true;
  if( y == std::numeric_limits<double>::infinity() )
    return true;
  if( z == std::numeric_limits<double>::infinity() )
    return true;
  return false;
}

shoebox_t::shoebox_t()
{
}

shoebox_t::shoebox_t(const pos_t& center_,const pos_t& size_,const zyx_euler_t& orientation_)
  : center(center_),
    size(size_),
    orientation(orientation_)
{
}

pos_t shoebox_t::nextpoint(pos_t p)
{
  p -= center;
  p /= orientation;
  //DEBUG(size.print_cart());
  pos_t prel;
  if( p.x > 0 )
    prel.x = std::max(0.0,p.x-0.5*size.x);
  else
    prel.x = std::min(0.0,p.x+0.5*size.x);
  if( p.y > 0 )
    prel.y = std::max(0.0,p.y-0.5*size.y);
  else
    prel.y = std::min(0.0,p.y+0.5*size.y);
  if( p.z > 0 )
    prel.z = std::max(0.0,p.z-0.5*size.z);
  else
    prel.z = std::min(0.0,p.z+0.5*size.z);
  return prel;
}

face_t& face_t::operator+=(const pos_t& p)
{
  anchor += p;
  update();
  return *this;
}

face_t& face_t::operator+=(double p)
{
  pos_t n(normal);
  n *= p;
  return (*this += n);
}

double TASCAR::drand()
{
  return (double)random()/(double)(RAND_MAX+1.0);
}

ngon_t::ngon_t()
  : N(4)
{
  std::vector<pos_t> nverts;
  nverts.push_back(pos_t(0,0,0));
  nverts.push_back(pos_t(1,0,0));
  nverts.push_back(pos_t(1,0,2));
  nverts.push_back(pos_t(0,0,2));
  nonrt_set(pos_t(),zyx_euler_t(),nverts);
}

void ngon_t::nonrt_set(const pos_t& p0, const zyx_euler_t& o, const std::vector<pos_t>& verts)
{
  if( verts.size() < 3 )
    throw TASCAR::ErrMsg("A polygon needs at least three vertices.");
  local_verts_ = verts;
  N = verts.size();
  verts_.resize(N);
  edges_.resize(N);
  vert_normals_.resize(N);
  apply_rot_loc(p0,o);
  //for(std::vector<pos_t>::iterator it=verts_.begin();it!=verts_.end();++it){
  //  *it *= o;
  //  *it += p0;
  //}
  //anchor = p0;
  //e1 = pos_t(0,width,0);
  //e2 = pos_t(0,0,height);
  //e1 *= o;
  //e2 *= o;
  //width_ = width;
  //height_ = height;
  //orient_ = o;
  //update();
}

void ngon_t::apply_rot_loc(const pos_t& p0,const zyx_euler_t& o)
{
  std::vector<pos_t>::iterator i_local_vert(local_verts_.begin());
  for(std::vector<pos_t>::iterator i_vert=verts_.begin();i_vert!=verts_.end();++i_vert){
    *i_vert = *i_local_vert;
    *i_vert *= o;
    *i_vert += p0;
    i_local_vert++;
  }
  std::vector<pos_t>::iterator i_vert(verts_.begin());
  std::vector<pos_t>::iterator i_next_vert(i_vert+1);
  for(std::vector<pos_t>::iterator i_edge=edges_.begin();i_edge!=edges_.end();++i_edge){
    *i_edge = *i_next_vert;
    *i_edge -= *i_vert;
    i_next_vert++;
    if( i_next_vert == verts_.end() )
      i_next_vert = verts_.begin();
    i_vert++;
  }
  pos_t rot;
  std::vector<pos_t>::iterator i_prev_edge(edges_.end()-1);
  for(std::vector<pos_t>::iterator i_edge=edges_.begin();i_edge!=edges_.end();++i_edge){
    rot += cross_prod(*i_prev_edge,*i_edge);
    i_prev_edge = i_edge;
  }
  rot /= rot.norm();
  normal = rot;
  // vertex normals, used to calculate inside/outside:
  i_prev_edge = edges_.end()-1;
  std::vector<pos_t>::iterator i_edge(edges_.begin());
  for(std::vector<pos_t>::iterator i_vert_normal=vert_normals_.begin();i_vert_normal!=vert_normals_.end();++i_vert_normal){
    *i_vert_normal = cross_prod(i_edge->normal() + i_prev_edge->normal(),rot).normal();
    i_prev_edge = i_edge;
    i_edge++;
  }
}

//void ngon_t::update()
//{
//  normal = cross_prod(e1,e2);
//  normal /= normal.norm();
//}

pos_t ngon_t::nearest_on_plane( const pos_t& p0 ) const
{
  double plane_dist = dot_prod(normal,verts_[0]-p0);
  pos_t p0d = normal;
  p0d *= plane_dist;
  p0d += p0;
  return p0d;
}

pos_t edge_nearest(const pos_t& v,const pos_t& d,const pos_t& p0)
{
  pos_t p0p1(p0-v);
  double l(d.norm());
  pos_t n(d);
  n /= l;
  double r(0.0);
  if( !p0p1.is_null() )
    r = dot_prod(n,p0p1.normal())*p0p1.norm();
  if( r < 0 )
    return v;
  if( r > l )
    return v+d;
  pos_t p0d(n);
  p0d *= r;
  p0d += v;
  return p0d;
}

pos_t ngon_t::nearest_on_edge(const pos_t& p0,uint32_t* pk0) const
{
  pos_t ne(edge_nearest(verts_[0],edges_[0],p0));
  double d(distance(ne,p0));
  uint32_t k0(0);
  for( uint32_t k=1;k<N;k++){
    pos_t ln(edge_nearest(verts_[k],edges_[k],p0));
    double ld;
    if( (ld = distance(ln,p0)) < d ){
      ne = ln;
      d = ld;
      k0 = k;
    }
  }
  if( pk0 )
    *pk0 = k0;
  return ne;
}

pos_t ngon_t::nearest( const pos_t& p0 ) const
{
  uint32_t k0(0);
  pos_t ne(nearest_on_edge(p0,&k0));
  // is inside?
  bool is_outside(false);
  pos_t dp0(ne-p0);
  if( dp0.is_null() )
    is_outside = true;
  else
    // caclulate edge normal:
    is_outside = (dot_prod(vert_normals_[k0],ne-p0) >= 0);
  if( is_outside )
    return ne;
  return nearest_on_plane(p0);
}

std::string ngon_t::print(const std::string& delim) const
{
  std::ostringstream tmp("");
  tmp.precision(12);
  for(std::vector<pos_t>::const_iterator i_vert=verts_.begin();i_vert!=verts_.end();++i_vert){
    if( i_vert != verts_.begin() )
      tmp << delim;
    tmp << i_vert->print_cart(delim);
  }
  return tmp.str();
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
