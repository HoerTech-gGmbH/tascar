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

using namespace TASCAR;

std::string pos_t::print_cart()
{
  std::ostringstream tmp("");
  tmp.precision(20);
  tmp << x << ", " << y << ", " << z;
  return tmp.str();

}

std::string pos_t::print_sphere()
{
  std::ostringstream tmp("");
  tmp.precision(20);
  tmp << norm() << ", " << RAD2DEG*azim() << ", " << RAD2DEG*elev();
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

std::string track_t::print_cart()
{
  std::ostringstream tmp("");
  tmp.precision(20);
  for(iterator i=begin();i!=end();++i){
    tmp << i->first << ", " << i->second.print_cart() << "\n";
  }
  return tmp.str();
}
  
std::string track_t::print_sphere()
{
  std::ostringstream tmp("");
  tmp.precision(20);
  for(iterator i=begin();i!=end();++i){
    tmp << i->first << ", " << i->second.print_sphere() << "\n";
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
  rot_y( PI_2 - c.elev() );
  rot_z( -PI_2 );
  c.set_cart(0,0,-c.norm());
  operator+=(c);
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
}

pos_t track_t::interp(double x)
{
  if( begin() == end() )
    return pos_t();
  iterator lim2 = lower_bound(x);
  if( lim2 == end() )
    return rbegin()->second;
  if( lim2 == begin() )
    return begin()->second;
  if( lim2->first == x )
    return lim2->second;
  iterator lim1 = lim2;
  --lim1;
  pos_t p1(lim1->second);
  pos_t p2(lim2->second);
  double w = (x-lim1->first)/(lim2->first-lim1->first);
  p1 *= (1.0-w);
  p2 *= w;
  p1 += p2;
  return p1;
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
}  

std::string xml_get_text( xmlpp::Node* n, const std::string& child )
{
  if( n ){
    xmlpp::Node::NodeList ch = n->get_children( child );
    if( ch.size() ){
      xmlpp::NodeSet stxt = (*ch.begin())->find("text()");
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
}

/**
   \brief load a track from a gpx file
*/
void track_t::load_from_csv( const std::string& fname )
{
  track_t track;
  std::ifstream fh(fname.c_str());
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
      if( vel_fname.size() ){
        set_velocity_csvfile( vel_fname );
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
      if( dt > 0 ){
        TASCAR::track_t ntrack;
        double t_begin = begin()->first;
        double t_end = rbegin()->first;
        for( double t = t_begin; t <= t_end; t+=dt )
          ntrack[t] = interp(t);
        *this = ntrack;
      }
    }else if( scmd == "time" ){
      // scale...
      double starttime = atof(cmd->get_attribute_value("start").c_str());
      shift_time(starttime - begin()->first);
    }else{
      DEBUG(cmd->get_name());
    }
  }
}

void track_t::edit( xmlpp::Node::NodeList cmds )
{
  for( xmlpp::Node::NodeList::iterator icmd=cmds.begin();icmd != cmds.end();++icmd){
    xmlpp::Element* cmd = dynamic_cast<xmlpp::Element*>(*icmd);
    edit( cmd );
  }
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
}

void track_t::set_velocity_csvfile( const std::string& fname )
{
  std::ifstream fh(fname.c_str());
  std::string v_tm, v_x;
  track_t vmap;
  while( !fh.eof() ){
    getline(fh,v_tm,',');
    getline(fh,v_x);
    if( v_tm.size() && v_x.size() ){
      double tm = atof(v_tm.c_str());
      double x = atof(v_x.c_str());
      vmap[tm] = pos_t(x,0,0);
    }
  }
  fh.close();
  if( vmap.begin() != vmap.end() ){
    set_velocity_const( 1.0 );
    double d = 0;
    double dt = 0.5;
    track_t ntrack;
    for( double tm=vmap.begin()->first; tm <= vmap.rbegin()->first; tm += dt){
      pos_t pv = vmap.interp( tm );
      d += dt*pv.x;
      ntrack[tm] = interp( d );
    }
    *this = ntrack;
  }
}

void track_t::export_to_xml_element( xmlpp::Element* a)
{
  a->add_child_text(print_cart());
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
