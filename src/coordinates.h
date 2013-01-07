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

namespace TASCAR {
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
    inline double norm2() const {return x*x + y*y + z*z;};
    inline double norm() const {return sqrt(norm2());};
    inline double norm_xy() const {return sqrt(x*x+y*y);};
    inline double azim() const {return atan2(y,x);};
    inline double elev() const {return atan2(z,norm_xy());};
    /**
       \brief Rotate around z-axis
    */
    inline pos_t& rot_z(double a) { 
      double xn = cos(a)*x - sin(a)*y;
      double yn = cos(a)*y + sin(a)*x;
      x = xn;
      y = yn;
      return *this;
    };
    /**
       \brief Rotate around x-axis
    */
    inline pos_t& rot_x(double a) { 
      double zn = cos(a)*z + sin(a)*y;
      double yn = cos(a)*y - sin(a)*z;
      z = zn;
      y = yn;
      return *this;
    };
    /**
       \brief Rotate around y-axis
    */
    inline pos_t& rot_y(double a) { 
      double zn = cos(a)*z + sin(a)*x;
      double xn = cos(a)*x - sin(a)*z;
      z = zn;
      x = xn;
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
    std::string print_cart();
    /**
       \brief Format as string in spherical coordinates
     */
    std::string print_sphere();
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
     \param p Inverse offset
  */
  inline pos_t& operator-=(pos_t& self,const pos_t& p) {
    self.x-=p.x;
    self.y-=p.y;
    self.z-=p.z;
    return self;
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

  /**
     \ingroup tascar
     \brief List of points connected with a time.
   */
  class track_t : public std::map<double,pos_t> {
  public:
    /**
       \brief Return the center of a track.
     */
    pos_t center();
    /**
       \brief Return the interpolated position for a given time.
     */
    pos_t interp(double x);
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
    std::string print_cart();
    /**
       \brief Format as string in spherical coordinates
     */
    std::string print_sphere();
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
    void set_velocity_csvfile( const std::string& fname );
  };

  /**
     \brief Read a single track point from an XML trkpt element
  */
  pos_t xml_get_trkpt( xmlpp::Element* pt, time_t& tme );

  class face_t {
  protected:
    pos_t normal;
    pos_t anchor;
    pos_t dimension;
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
