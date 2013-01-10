/**
   \file ambpan.h
   \ingroup apphos
   \brief Ambisonics panner
   \date 2011

   \section license License (GPL)

   Copyright (C) 2011 Giso Grimm
   
   Copyright (C) 2009 Fons Adriaensen <fons@kokkinizita.net>
   and Jörn Nettingsmeier <nettings@stackingdwarves.net>
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; version 2 of the
   License.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

#ifndef AMBPAN_H
#define AMBPAN_H

#define MIN3DB  0.707107f

namespace HoS {

  /**
     \brief 3rd order horizontal ambisonics panner
  */
  class amb_coeff {
  public:
    /**
       \param dt update time interval
    */
    amb_coeff(unsigned int dt):dt1(1.0f/(float)dt),g(0.0f),x(0.0f),y(0.0f),u(0.0f),v(0.0f),p(0.0f),q(0.0f){set(0.0f,0.0f,0.0f);};
    /**
       \brief Set panning parameters
       \param az Azimuth (radiants)
       \param el Pseudo-elevation (radiants)
       \param gain Linear broadband gain
    */
    void set(float az, float el, float gain)
    {
      float _t(cosf(el));
      gain *= 2.0f-fabsf(_t);
      float _x(_t*cosf(az));
      float _y(_t*sinf(az));
      float _x2(_x * _x);
      float _y2(_y * _y);
      float _u(_x2-_y2);
      float _v(2.0f*_x*_y);
      float _p((_x2-3.0f*_y2)*_x);
      float _q((3.0f*_x2-_y2)*_y);
      dg = (gain-g)*dt1;
      dx = (_x-x)*dt1;
      dy = (_y-y)*dt1;
      du = (_u-u)*dt1;
      dv = (_v-v)*dt1;
      dp = (_p-p)*dt1;
      dq = (_q-q)*dt1;
    };
    /** 
        \brief Apply panning to one time frame

        \param t Input sample
        \param W zeroth-order output
        \param X first-order output
        \param Y first-order output
        \param U second-order output
        \param V second-order output
        \param P third-order output
        \param Q third-order output
    */
    inline void addpan30(float t,float& W, float& X, float& Y, float& U, float& V, float& P, float& Q)
    {
      t *= ( g+=dg );
      W += t * MIN3DB;
      X += t * ( x+=dx );
      Y += t * ( y+=dy );
      U += t * ( u+=du );
      V += t * ( v+=dv );
      P += t * ( p+=dp );
      Q += t * ( q+=dq );
    };
    /**
       \brief Clear components of one frame
       \param W zeroth-order output
       \param X first-order output
       \param Y first-order output
       \param U second-order output
       \param V second-order output
       \param P third-order output
       \param Q third-order output
    */
    inline void clear(float& W, 
                      float& X, float& Y, 
                      float& U, float& V, 
                      float& P, float& Q)
    {
      W = 0.0f;
      X = 0.0f;
      Y = 0.0f;
      U = 0.0f;
      V = 0.0f;
      P = 0.0f;
      Q = 0.0f;
    }
  private:
    float dt1;
    float g, x, y, u, v, p, q;
    float dg, dx, dy, du, dv, dp, dq;
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

