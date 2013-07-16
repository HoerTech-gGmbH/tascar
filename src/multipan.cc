/**
   \file multipan.cc
   \brief "dynpan" is a multichannel panning tool implementing several panning methods.
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
#include "multipan.h"
#include <string.h>
#include <iostream>
#include <stdio.h>
#include "defs.h"

using namespace TASCAR;

int powi(int base, unsigned exp) {
    int result = 1, squares = base;
    for (;exp; (exp = exp >> 1U), squares *= squares)
        if (exp & 1) result *= squares;
    return result;
}

double powi(double base, unsigned exp) {
  double result = 1.0, squares = base;
  for (;exp; (exp = exp >> 1U), squares *= squares)
    if (exp & 1) result *= squares;
  return result;
}

TASCAR::speakerlayout_t::speakerlayout_t(const std::vector<pos_t>& pspk,const std::vector<std::string>& dest,const std::vector<std::string>& name)
  : n(pspk.size()),
    p(pspk),
    p_norm(pspk),
    dest_(dest),
    name_(name)
{
  dest_.resize(n);
  name_.resize(n);
  d_max = 0;
  d_min = p[0].norm();
  for(unsigned int k=0;k<n;k++){
    d.push_back(p[k].norm());
    az.push_back(p[k].azim());
    p_norm[k] /= d[k];
    if( d[k] > d_max )
      d_max = d[k];
    if( d[k] < d_min )
      d_min = d[k];
    if( !(name_[k].size()) ){
      char ctmp[100];
      sprintf(ctmp,"out_%d",k+1);
      name_[k] = ctmp;
    }
  }
}

void speakerlayout_t::print()
{
  std::cout << "# " << n << " speakers\n";
  std::cout << "# no d az el x y z dest\n";
  for(unsigned int k=0;k<n;k++){
    std::cout << k << " " << d[k] << " " << RAD2DEG*az[k] << " " << RAD2DEG*p[k].elev() << " " << 
      0.001*floor(1000.0*p[k].x) << " " << 
      0.001*floor(1000.0*p[k].y) << " " << 
      0.001*floor(1000.0*p[k].z) << " " << dest_[k] << "\n";
  }
}

pan_nearest_t::pan_nearest_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling)
  : pan_base_t(spk,dline,subsampling),
    k_nearest(0)
{
}

void pan_nearest_t::updatepar()
{
  pos_t src_norm(src);
  src_norm /= src_norm.norm();
  double d;
  double d_min = 2.0;
  for(unsigned int k=0; k<spk_.n;k++){
    if( (d = distance(spk_.p_norm[k],src_norm)) < d_min ){
      k_nearest = k;
      d_min = d;
    }
  }
}

void pan_nearest_t::process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer)
{
  for( unsigned int i=0;i<n;i++){
    delayline.push(vIn[i]);
    src.set_cart(vX[i],vY[i],vZ[i]);
    inct();
    double d = src.norm();
    outBuffer[k_nearest][i] += delayline.get_dist( d - spk_.d_min )/std::max(1.0,d);
  }
}

pan_vbap_t::pan_vbap_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling)
  : pan_base_t(spk,dline,subsampling),
    k_nearest(0),
    k_next(0),
    w_nearest(0.0),
    w_next(0.0),
    d_w_nearest(0.0),
    d_w_next(0.0),
    prev_k_nearest(0),
    prev_k_next(0),
    prev_w_nearest(0.0),
    prev_w_next(0.0),
    d_prev_w_nearest(0.0),
    d_prev_w_next(0.0)
{
  updatepar();
}

void pan_vbap_t::updatepar()
{
  prev_k_nearest = k_nearest;
  prev_k_next = k_next;
  prev_w_nearest = w_nearest;
  prev_w_next = w_next;
  // src_norm is normalized vector of virtual source:
  pos_t src_norm(src);
  double s_nm = src_norm.norm();
  if( s_nm > 0 )
    src_norm /= s_nm;
  // find closest speakers:
  double d;
  double d_min = 2.0;
  for(unsigned int k=0; k<spk_.n;k++){
    if( (d = distance(spk_.p_norm[k],src_norm)) < d_min ){
      k_nearest = k;
      d_min = d;
    }
  }
  double d_min2 = 2.0;
  for(unsigned int k=0; k<spk_.n;k++){
    if( (k!=k_nearest) && ((d = distance(spk_.p_norm[k],src_norm)) < d_min2) ){
      k_next = k;
      d_min2 = d;
    }
  }
  // now spk_.p_norm[k_nearest] and spk_.p_norm[k_next] are normalized
  // vectors of closest speakers
  // next: invert speaker matrix:
  double l11 = spk_.p_norm[k_nearest].x;
  double l12 = spk_.p_norm[k_nearest].y;
  double l21 = spk_.p_norm[k_next].x;
  double l22 = spk_.p_norm[k_next].y;
  double det_speaker(l11*l22 - l21*l12);
  if( det_speaker != 0 )
    det_speaker = 1.0/det_speaker;
  double linv11 =  det_speaker*l22;
  double linv12 = -det_speaker*l12;
  double linv21 = -det_speaker*l21;
  double linv22 =  det_speaker*l11;
  // calculate speaker weights:
  double g1 = src_norm.x*linv11+src_norm.y*linv21;
  double g2 = src_norm.x*linv12+src_norm.y*linv22;
  double w = sqrt(g1*g1+g2*g2);
  if( w > 0 )
    w = 1.0/w;
  w_nearest = 0.0;
  w_next = 0.0;
  d_w_nearest = w * g1 * dt1;
  d_w_next = w * g2 * dt1;
  d_prev_w_nearest = -prev_w_nearest * dt1;
  d_prev_w_next = -prev_w_next * dt1;
}

void pan_vbap_t::process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer)
{
  for( unsigned int i=0;i<n;i++){
    delayline.push(vIn[i]);
    src.set_cart(vX[i],vY[i],vZ[i]);
    inct();
    double d = src.norm();
    double v_in = delayline.get_dist( d - spk_.d_min )/std::max(1.0,d);
    outBuffer[k_nearest][i] += (w_nearest += d_w_nearest) * v_in;
    outBuffer[k_next][i] += (w_next += d_w_next) * v_in;
    outBuffer[prev_k_nearest][i] += (prev_w_nearest += d_prev_w_nearest) * v_in;
    outBuffer[prev_k_next][i] += (prev_w_next += d_prev_w_next) * v_in;
  }
}

pan_amb_basic_t::pan_amb_basic_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling, uint32_t maxorder)
  : pan_base_t(spk,dline,subsampling),
    order(1)
{
  spk_delay.resize(spk_.n);
  spk_gain.resize(spk_.n);
  w_current.resize(spk_.n);
  dw.resize(spk_.n);
  for(unsigned int k=0;k<spk_.n;k++){
    spk_delay[k] = spk_.d_max - spk_.d[k];
    spk_gain[k] = spk_.d[k]/spk_.d_max;
  }
  set_order( std::min( maxorder, (spk_.n-2)/2) );
}

void pan_amb_basic_t::set_order(unsigned int order_)
{
  order = order_;
  //DEBUG(order);
}

void pan_amb_basic_t::updatepar()
{
  double az_src = src.azim();
  for(unsigned int k=0;k<spk_.n;k++){
    double az = az_src - spk_.az[k];
    //double denom = sin( 0.5*az );
    //while( denom == 0 ){
    //  az += 1.0e-10;
    //  denom = sin( 0.5*az );
    //}
    //double nomin = sin( scale_order*az );
    double w = 0.5;
    for(unsigned int l=1;l<=order;l++)
      w += cos(l*az);
    w *= 2.0;
    w /= (double)spk_.n;
    w *= spk_gain[k];
    dw[k] = (w - w_current[k])*dt1;
  }
}

void pan_amb_basic_t::process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer)
{
  for( unsigned int i=0;i<n;i++){
    delayline.push(vIn[i]);
    src.set_cart(vX[i],vY[i],vZ[i]);
    inct();
    double d = src.norm();
    for( unsigned int k=0;k<spk_.n;k++){
      outBuffer[k][i] += (w_current[k] += dw[k]) * delayline.get_dist( d + spk_delay[k] - spk_.d_min )/std::max(1.0,d);
    }
  }
}

pan_amb_inphase_t::pan_amb_inphase_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling, uint32_t maxorder)
  : pan_base_t(spk,dline,subsampling),
    scale_order(0.5)
{
  spk_delay.resize(spk_.n);
  spk_gain.resize(spk_.n);
  w_current.resize(spk_.n);
  dw.resize(spk_.n);
  for(unsigned int k=0;k<spk_.n;k++){
    spk_delay[k] = spk_.d_max - spk_.d[k];
    spk_gain[k] = spk_.d[k]/spk_.d_max;
  }
  set_order( std::min( maxorder, (spk_.n-2)/2) );
}

void pan_amb_inphase_t::set_order(double order)
{
  scale_order = 2.0*order;
}

void pan_amb_inphase_t::updatepar()
{
  double az_src = src.azim();
  for(unsigned int k=0;k<spk_.n;k++){
    double az = az_src - spk_.az[k];
    double w = pow(cos(0.5*az),scale_order);
    w *= spk_gain[k];
    dw[k] = (w - w_current[k])*dt1;
  }
}

void pan_amb_inphase_t::process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer)
{
  for( unsigned int i=0;i<n;i++){
    delayline.push(vIn[i]);
    src.set_cart(vX[i],vY[i],vZ[i]);
    inct();
    double d = src.norm();
    for( unsigned int k=0;k<spk_.n;k++){
      outBuffer[k][i] += (w_current[k] += dw[k]) * delayline.get_dist( d + spk_delay[k] - spk_.d_min )/std::max(1.0,d);
    }
  }
}


pan_wfs_t::pan_wfs_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling)
  : pan_base_t(spk,dline,subsampling),
    wfs_gain(M_PI/spk.n)
{
  spk_dist.resize(spk_.n);
  d_spk_dist.resize(spk_.n);
  w.resize(spk_.n);
  d_w.resize(spk_.n);
}

void pan_wfs_t::updatepar()
{
  double az = src.azim();
  for(unsigned int k=0;k<spk_.n;k++){
    double d = distance(src,spk_.p[k]);
    double azspk = spk_.az[k] - az;
    double gc = std::max(0.0,cos(azspk) * wfs_gain / std::max(1.0,d) );
    d_w[k] = (gc - w[k])*dt1;
    d_spk_dist[k] = (d - spk_dist[k])*dt1;
  }
}

void pan_wfs_t::process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer)
{
  for( unsigned int i=0;i<n;i++){
    delayline.push(vIn[i]);
    src.set_cart(vX[i],vY[i],vZ[i]);
    inct();
    for(unsigned int k=0;k<spk_.n;k++){
      double gc = (w[k] += d_w[k]);
      double d = (spk_dist[k] += d_spk_dist[k]);
      if( gc > 0 )
        outBuffer[k][i] += gc*delayline.get_dist( d );
    }
  }
}

pan_cardioid_t::pan_cardioid_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling)
  : pan_base_t(spk,dline,subsampling)
{
  spk_dist.resize(spk_.n);
  d_spk_dist.resize(spk_.n);
  w.resize(spk_.n);
  dw.resize(spk_.n);
}

void pan_cardioid_t::updatepar()
{
  double az_src = src.azim();
  double d = src.norm();
  for(unsigned int k=0;k<spk_.n;k++){
    double az = az_src - spk_.az[k];
    double wn = 0.5*(1.0+cos(az))/std::max(0.1,d);
    dw[k] = (wn - w[k])*dt1;
    d_spk_dist[k] = (distance(src,spk_.p[k]) - spk_dist[k])*dt1;
  }
}

void pan_cardioid_t::process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer)
{
  for( unsigned int i=0;i<n;i++){
    delayline.push(vIn[i]);
    src.set_cart(vX[i],vY[i],vZ[i]);
    inct();
    for(unsigned int k=0;k<spk_.n;k++){
      double gc = (w[k] += dw[k]);
      double d = (spk_dist[k] += d_spk_dist[k]);
      outBuffer[k][i] += gc*delayline.get_dist( d );
    }
  }
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
