/**
   \file coordinates.h
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

#ifndef COORDINATES_H
#define COORDINATES_H

#include <math.h>
#include <string>
#include <map>
#include <libxml++/libxml++.h>
#include <limits>
#include "defs.h"

template<class T> void make_friendly_number(T& x)
{
  if( (-std::numeric_limits<T>::max() <= x) && (x <= std::numeric_limits<T>::max() ) ){
    if( (0 < x) && (x < std::numeric_limits<T>::min()) )
      x = 0;
    if( (0 > x) && (x > -std::numeric_limits<T>::min()) )
      x = 0;
    return;
  }
  DEBUGS(x);
  x = 0;
}

template<class T> void make_friendly_number_limited(T& x)
{
  if( (-1 <= x) && (x <= 1 ) ){
    if( (0 < x) && (x < std::numeric_limits<T>::min()) )
      x = 0;
    if( (0 > x) && (x > -std::numeric_limits<T>::min()) )
      x = 0;
    return;
  }
  DEBUGS(x);
  x = 0;
}


namespace TASCAR {

  class table1_t : public std::map<double,double> {
  public:
    table1_t();
    double interp(double) const;
  };

  /**
     \brief Coordinates
  */
  class pos_t {
  public:
    double x;
    double y;
    double z;
    /**
       \brief Set point from cartesian coordinates
       \param nx new x value
       \param ny new y value
       \param nz new z value
    */
    void set_cart(double nx,double ny,double nz){ x=nx;y=ny;z=nz;};
    /**
       \brief Set point from spherical coordinates
       \param r radius from center
       \param phi azimuth
       \param theta elevation
    */
    void set_sphere(double r,double phi,double theta){ x=r*cos(phi)*cos(theta);y = r*sin(phi)*cos(theta); z=r*sin(theta);};
    inline double norm2() const {return std::max(1e-10,x*x + y*y + z*z);};
    inline double norm() const {return sqrt(norm2());};
    inline double norm_xy() const {return sqrt(x*x+y*y);};
    inline double azim() const {return atan2(y,x);};
    inline double elev() const {return atan2(z,norm_xy());};
    pos_t normal() const;
    /**
       \brief Rotate around z-axis
    */
    inline pos_t& rot_z(double a) { 
      if( a != 0){
        double xn = cos(a)*x - sin(a)*y;
        double yn = cos(a)*y + sin(a)*x;
        x = xn;
        y = yn;}
      return *this;
    };
    /**
       \brief Rotate around x-axis
    */
    inline pos_t& rot_x(double a) { 
      if( a != 0){
        double zn = cos(a)*z + sin(a)*y;
        double yn = cos(a)*y - sin(a)*z;
        z = zn;
        y = yn;
      }
      return *this;
    };
    /**
       \brief Rotate around y-axis
    */
    inline pos_t& rot_y(double a) {
      if( a != 0){
        double zn = cos(a)*z + sin(a)*x;
        double xn = cos(a)*x - sin(a)*z;
        z = zn;
        x = xn;
      }
      return *this;
    };
    /**
       \brief Default constructor, initialize to origin
    */
    pos_t() : x(0),y(0),z(0) {};
    /**
       \brief Initialize to cartesian coordinates
    */
    pos_t(double nx, double ny, double nz) : x(nx),y(ny),z(nz) {};
    /**
       \brief Format as string in cartesian coordinates
    */
    std::string print_cart(const std::string& delim=", ") const;
    /**
       \brief Format as string in spherical coordinates
    */
    std::string print_sphere(const std::string& delim=", ") const;
  };

  class sphere_t {
  public:
    sphere_t(double r_,double az_, double el_):r(r_),az(az_),el(el_){};
    sphere_t():r(0),az(0),el(0){};
    sphere_t(pos_t c){
      double xy2 = c.x*c.x+c.y*c.y;
      r = sqrt(xy2+c.z*c.z);
      az = atan2(c.y,c.x);
      el = atan2(c.z,sqrt(xy2));
    };
    pos_t cart(){
      double cel(cos(el));
      return pos_t(r*cos(az)*cel,r*sin(az)*cel,r*sin(el));
    };
    double r,az,el;
  };

  /**
     \brief Scale relative to origin
     \param d ratio
  */
  inline sphere_t& operator*=(sphere_t& self,double d) {
    self.r*=d;
    self.az*=d;
    self.el*=d;
    return self;
  };



  class zyx_euler_t {
  public:
    std::string print(const std::string& delim=", ");
    zyx_euler_t(double z_,double y_,double x_):z(z_),y(y_),x(x_){};
    zyx_euler_t():z(0),y(0),x(0){};
    double z;
    double y;
    double x;
  };

  inline zyx_euler_t& operator*=(zyx_euler_t& self,const double& scale){
    self.x*=scale;
    self.y*=scale;
    self.z*=scale;
    return self;
  };

  inline zyx_euler_t& operator+=(zyx_euler_t& self,const zyx_euler_t& other){
    self.x+=other.x;
    self.y+=other.y;
    self.z+=other.z;
    return self;
  };

  /**
     \brief Apply Euler rotation
     \param r Euler rotation
  */
  inline pos_t& operator*=(pos_t& self,const zyx_euler_t& r){
    self.rot_z(r.z);
    self.rot_y(r.y);
    self.rot_x(r.x);
    return self;
  };

  /**
     \brief Apply inverse Euler rotation
     \param r Euler rotation
  */
  inline pos_t& operator/=(pos_t& self,const zyx_euler_t& r){
    self.rot_x(-r.x);
    self.rot_y(-r.y);
    self.rot_z(-r.z);
    return self;
  };

  /**
     \brief Translate a point
     \param p Offset
  */
  inline pos_t& operator+=(pos_t& self,const pos_t& p) {
    self.x+=p.x;
    self.y+=p.y;
    self.z+=p.z;
    return self;
  };
  /**
     \brief Translate a point
     \param p Offset
  */
  inline pos_t operator+(const pos_t& a,const pos_t& b) {
    pos_t tmp(a);
    tmp+=b;
    return tmp;
  };
  /**
     \brief Translate a point
     \param p Inverse offset
  */
  inline pos_t& operator-=(pos_t& self,const pos_t& p) {
    self.x-=p.x;
    self.y-=p.y;
    self.z-=p.z;
    return self;
  };
  /**
     \brief Translate a point
     \param p Offset
  */
  inline pos_t operator-(const pos_t& a,const pos_t& b) {
    pos_t tmp(a);
    tmp-=b;
    return tmp;
  };
  /**
     \brief Scale relative to origin
     \param d inverse ratio
  */
  inline pos_t& operator/=(pos_t& self,double d) {
    self.x/=d;
    self.y/=d;
    self.z/=d;
    return self;
  };
  /**
     \brief Scale relative to origin
     \param d ratio
  */
  inline pos_t& operator*=(pos_t& self,double d) {
    self.x*=d;
    self.y*=d;
    self.z*=d;
    return self;
  };

  /**
     \brief Scale relative to origin, each axis separately
     \param d ratio
  */
  inline pos_t& operator*=(pos_t& self,const pos_t& d) {
    self.x*=d.x;
    self.y*=d.y;
    self.z*=d.z;
    return self;
  };

  /**
     \brief Return distance between two points
  */
  inline double distance(const pos_t& p1, const pos_t& p2)
  {
    return sqrt((p1.x-p2.x)*(p1.x-p2.x) + 
                (p1.y-p2.y)*(p1.y-p2.y) + 
                (p1.z-p2.z)*(p1.z-p2.z));
  }

  inline double dot_prod(const pos_t& p1, const pos_t& p2)
  {
    return p1.x*p2.x+p1.y*p2.y+p1.z*p2.z;
  }

  inline pos_t cross_prod(const pos_t& a,const pos_t& b)
  {
    return pos_t(a.y*b.z - a.z*b.y,
                 a.z*b.x - a.x*b.z,
                 a.x*b.y - a.y*b.x);
  }


  /**
     \ingroup tascar
     \brief List of points connected with a time.
  */
  class track_t : public std::map<double,pos_t> {
  public:
    enum interp_t {
      cartesian, spherical
    };
    track_t();
    /**
       \brief Return the center of a track.
    */
    pos_t center();
    /**
       \brief Return length of a track.
     */
    double length();
    /**
       \brief minimum time
     */
    double t_min(){if( size() ) return begin()->first; else return 0; };
    double t_max(){if( size() ) return rbegin()->first; else return 0; };
    double duration(){return t_max()-t_min();};
    /**
       \brief Return the interpolated position for a given time.
    */
    pos_t interp(double x) const;
    /**
       \brief Shift the time by a constant value
    */
    void shift_time(double dt);
    track_t& operator+=(const pos_t&);
    track_t& operator-=(const pos_t&);
    track_t& operator*=(const pos_t&);
    /**
       \brief Format as string in cartesian coordinates
    */
    std::string print_cart(const std::string& delim=", ");
    /**
       \brief Format as string in spherical coordinates
    */
    std::string print_sphere(const std::string& delim=", ");
    /**
       \brief Tangent projection, transform origin to given point
    */
    void project_tangent(pos_t p);
    /**
       \brief Tangent projection, transform origin to center
    */
    void project_tangent();
    /**
       \brief Rotate around z-axis
    */
    void rot_z(double a);
    /**
       \brief Rotate around x-axis
    */
    void rot_x(double a);
    /**
       \brief Rotate around y-axis
    */
    void rot_y(double a);
    /**
       \brief Smooth a track by convolution with a Hann-window
    */
    void smooth(unsigned int n);
    void resample(double dt);
    /**
       \brief load a track from a gpx file
    */
    void load_from_gpx(const std::string& fname);
    /**
       \brief load a track from a csv file
    */
    void load_from_csv(const std::string& fname);
    /**
       \brief manipulate track based on a set of XML entries
    */
    void edit( xmlpp::Element* m );
    /**
       \brief manipulate track based on a set of XML entries
    */
    void edit( xmlpp::Node::NodeList cmds );
    /**
       \brief set constant velocity
    */
    void set_velocity_const( double vel );
    /**
       \brief set velocity from CSV file
    */
    void set_velocity_csvfile( const std::string& fname, double offset );
    /**
       \brief Export to xml element
    */
    void write_xml( xmlpp::Element* );
    void read_xml( xmlpp::Element* );
    void set_interpt(interp_t p){interpt=p;};
    double get_dist( double time ) const {return time_dist.interp(time);};
    double get_time( double dist ) const {return dist_time.interp(dist);};
    void prepare();
    void fill_gaps(double dt);
  private:
    interp_t interpt;
    table1_t time_dist;
    table1_t dist_time;
  };

  /**
     \brief Read a single track point from an XML trkpt element
  */
  pos_t xml_get_trkpt( xmlpp::Element* pt, time_t& tme );

  std::string xml_get_text( xmlpp::Node* n, const std::string& child );

  class face_t {
  public:
    face_t();
    void set(const pos_t& p0, const zyx_euler_t& o, double width, double height);
    pos_t nearest_on_plane(const pos_t& p0) const;
    pos_t nearest(const pos_t& p0) const;
    face_t& operator+=(const pos_t& p);
    face_t& operator+=(double p);
    const pos_t& get_anchor() const { return anchor;};
    const pos_t& get_e1() const { return e1;};
    const pos_t& get_e2() const { return e2;};
    const pos_t& get_normal() const { return normal;};
  protected:
    void update();
    pos_t anchor;
    pos_t e1;
    pos_t e2;
    pos_t normal;
    double width_;
    double height_;
    zyx_euler_t orient_;
  };

  /**
     \ingroup tascar
     \brief List of points connected with a time.
  */
  class euler_track_t : public std::map<double,zyx_euler_t> {
  public:
    /**
       \brief Return the interpolated orientation for a given time.
    */
    zyx_euler_t interp(double x) const;
    void write_xml( xmlpp::Element* );
    void read_xml( xmlpp::Element* );
    std::string print(const std::string& delim=", ");
  };

  class shoebox_t {
  public:
    shoebox_t();
    shoebox_t(const pos_t& center_,const pos_t& size_,const zyx_euler_t& orientation_);
    pos_t nextpoint(pos_t p);
    pos_t center;
    pos_t size;
    zyx_euler_t orientation;
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
