#include "render_sinks.h"
#include "tascar.h"

using namespace TASCAR;
using namespace AMB33;

mirror_pan_t::mirror_pan_t(double srate, uint32_t fragsize, double maxdist_)
  : trackpan_amb33_t(srate,fragsize,maxdist_),
    c1_current(0),
    dc1(0),
    c2_current(0),
    dc2(0),
    my(0)
{
}

void mirror_pan_t::process(uint32_t n, float* vIn, const std::vector<float*>& outBuffer, uint32_t tp_frame, double tp_time, bool tp_rolling, sound_t* snd,face_object_t* face)
{
  if( n != fragsize_ )
    throw ErrMsg("fragsize changed");
  double ldt(dt_sample);
  if( !tp_rolling )
    ldt = 0;
  if( face->isactive(tp_time) && vIn ){
    double nexttime(tp_time+ldt*n);
    // first get sound position in global coordinates:
    pos_t psnd(snd->get_pos_global(nexttime));
    // then get mirror object:
    mirror_t m(face->get_mirror(nexttime,psnd));
    // now transform mirror source relative to listener:
    m.p -= snd->get_reference()->get_location(nexttime);
    m.p /= snd->get_reference()->get_orientation(nexttime);
    // update panner and low pass filters:
    updatepar(m.p);
    dc1 = (m.c1-c1_current)*dt_update;
    dc2 = (m.c2-c2_current)*dt_update;
    //DEBUG(m.p.print_cart());
    //DEBUG(m.c1);
    //DEBUG(m.c2);
    // iterate through fragment:
    for( unsigned int i=0;i<n;i++){
      delayline.push(vIn[i]);
      float d = (d_current+=dd);
      float d1 = 1.0/std::max(0.3f,d);
      if( tp_rolling ){
        float c1(clp_current+=dclp);
        float c2(1.0f-c1);
        float mc1(c1_current+=dc1);
        float mc2(c2_current+=dc2);
        y = c2*y+c1*delayline.get_dist(d)*d1;
        my = mc2*my+mc1*y;
        //my = y;
        for( unsigned int k=0;k<idx::channels;k++){
          outBuffer[k][i] += (w_current[k] += dw[k]) * my;
        }
      }
    }
  }
}
  
trackpan_amb33_t::trackpan_amb33_t(double srate, uint32_t fragsize, double maxdist_)
//  : delayline(3*srate,srate,340.0), 
  : delayline(maxdist_/340.0*srate+2,srate,340.0), 
    srate_(srate),
    dt_sample(1.0/srate_),
    dt_update(1.0/(double)fragsize),
    fragsize_(fragsize),
    y(0),
    dscale(srate/(340.0*7782.0)),
    maxdist(maxdist_)
{
  for(unsigned int k=0;k<idx::channels;k++)
    _w[k] = w_current[k] = dw[k] = 0;
  clp_current = dclp = 0;
}

trackpan_amb33_t::trackpan_amb33_t(const trackpan_amb33_t& src)
  : delayline(src.maxdist/340.0*src.srate_+2,src.srate_,340.0),
    srate_(src.srate_),
    dt_sample(1.0/srate_),
    dt_update(1.0/(double)(src.fragsize_)),
    fragsize_(src.fragsize_),
    y(0),
    dscale(src.srate_/(340.0*7782.0)),
    maxdist(src.maxdist)
{
  for(unsigned int k=0;k<idx::channels;k++)
    _w[k] = w_current[k] = dw[k] = 0;
  clp_current = dclp = 0;
}

trackpan_amb33_t::~trackpan_amb33_t()
{
  //DEBUG(data);
}

void trackpan_amb33_t::updatepar(pos_t src_)
{
  //DEBUG(src_.print_cart());
  float az = src_.azim();
  float el = src_.elev();
  float nm = src_.norm();
  //DEBUG(az);
  //DEBUG(el);
  float t, x2, y2, z2;
  dd = (nm-d_current)*dt_update;
  // this is taken from AMB plugins by Fons and Joern:
  _w[idx::w] = MIN3DB;
  t = cosf (el);
  _w[idx::x] = t * cosf (az);
  _w[idx::y] = t * sinf (az);
  _w[idx::z] = sinf (el);
  x2 = _w[idx::x] * _w[idx::x];
  y2 = _w[idx::y] * _w[idx::y];
  z2 = _w[idx::z] * _w[idx::z];
  _w[idx::u] = x2 - y2;
  _w[idx::v] = 2 * _w[idx::x] * _w[idx::y];
  _w[idx::s] = 2 * _w[idx::z] * _w[idx::x];
  _w[idx::t] = 2 * _w[idx::z] * _w[idx::y];
  _w[idx::r] = (3 * z2 - 1) / 2;
  _w[idx::p] = (x2 - 3 * y2) * _w[idx::x];
  _w[idx::q] = (3 * x2 - y2) * _w[idx::y];
  t = 2.598076f * _w[idx::z]; 
  _w[idx::n] = t * _w[idx::u];
  _w[idx::o] = t * _w[idx::v];
  t = 0.726184f * (5 * z2 - 1);
  _w[idx::l] = t * _w[idx::x];
  _w[idx::m] = t * _w[idx::y];
  _w[idx::k] = _w[idx::z] * (5 * z2 - 3) / 2;
  for(unsigned int k=0;k<idx::channels;k++)
    dw[k] = (_w[k] - w_current[k])*dt_update;
  dclp = (exp(-nm*dscale) - clp_current)*dt_update;
}

void trackpan_amb33_t::process(uint32_t n, float* vIn, const std::vector<float*>& outBuffer,uint32_t tp_frame, double tp_time,bool tp_rolling, sound_t* snd)
{
  if( n != fragsize_ )
    throw ErrMsg("fragsize changed");
  double ldt(dt_sample);
  if( !tp_rolling )
    ldt = 0;
  if( snd->isactive(tp_time) && vIn ){
    updatepar(snd->get_pos(tp_time+ldt*n));
    for( unsigned int i=0;i<n;i++){
      delayline.push(vIn[i]);
      float d = (d_current+=dd);
      float d1 = 1.0/std::max(0.3f,d);
      if( tp_rolling ){
        float c1(clp_current+=dclp);
        float c2(1.0f-c1);
        y = c2*y+c1*delayline.get_dist(d)*d1;
        for( unsigned int k=0;k<idx::channels;k++){
          outBuffer[k][i] += (w_current[k] += dw[k]) * y;
        }
      }
    }
  }
}

  
reverb_line_t::reverb_line_t(double srate, uint32_t fragsize, double maxdist_)
//  : delayline(3*srate,srate,340.0), 
  : delayline(maxdist_/340.0*srate+2,srate,340.0), 
    srate_(srate),
    dt_sample(1.0/srate_),
    dt_update(1.0/(double)fragsize),
    fragsize_(fragsize),
    d_global_current(0.0),
    d_border_current(0.0),
    clp_current(1.0),
    y(0),
    dscale(srate/(340.0*7782.0)),
    maxdist(maxdist_)
{
  clp_current = dclp = 0;
}

reverb_line_t::reverb_line_t(const reverb_line_t& src)
  : delayline(src.maxdist/340.0*src.srate_+2,src.srate_,340.0),
    srate_(src.srate_),
    dt_sample(1.0/srate_),
    dt_update(1.0/(double)(src.fragsize_)),
    fragsize_(src.fragsize_),
    d_global_current(0.0),
    d_border_current(0.0),
    clp_current(1.0),
    y(0),
    dscale(src.srate_/(340.0*7782.0)),
    maxdist(src.maxdist)
{
  clp_current = dclp = 0;
}

void reverb_line_t::process(uint32_t n, float* vIn, float* vOut,uint32_t tp_frame, double tp_time,bool tp_rolling, sound_t* snd, diffuse_reverb_t* reverb)
{
  if( n != fragsize_ )
    throw ErrMsg("fragsize changed");
  if( snd->isactive(tp_time) && vIn ){
    double ldt(dt_sample);
    if( !tp_rolling )
      ldt = 0;
    pos_t p(snd->get_pos_global(tp_time+ldt*n));
    double d_global(p.norm());
    double d_border(0.25*reverb->size.norm()+reverb->border_distance(p));
    //DEBUG(d_global);
    //DEBUG(d_border);
    dd_global = (d_global-d_global_current)*dt_update;
    dd_border = (d_border-d_border_current)*dt_update;
    dclp = (exp(-d_border*dscale) - clp_current)*dt_update;
    for( unsigned int i=0;i<n;i++){
      delayline.push(vIn[i]);
      d_global = (d_global_current+=dd_global);
      d_border = (d_border_current+=dd_border);
      float d1 = 1.0/std::max(1.0,d_border);
      float c1(clp_current+=dclp);
      float c2(1.0f-c1);
      if( tp_rolling ){
        y = c2*y+c1*delayline.get_dist(d_global)*d1;
        vOut[i] += y;
      }
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
