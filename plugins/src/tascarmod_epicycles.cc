/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include "session.h"

#include "osc_helper.h"
#include <complex>
#include <stdlib.h>

const std::complex<float> i_f(0.0, 1.0);

/**
   \brief Classes mainly used for artistic purposes

   Members are mostly dirty hacks and undocumented...
 */
namespace HoS {

  class srvvars_t : public TASCAR::xml_element_t {
  public:
    srvvars_t(tsccfg::node_t);
    std::string targetaddr;
    std::string path;
  };

  class par_t {
  public:
    par_t();
    void mix_static(float g, const par_t& p1, const par_t& p2);
    //void assign_static(const par_t& p1);
    void mix_dynamic(float g, const par_t& p1, const par_t& p2);
    void assign_dynamic(const par_t& p1);
    float phi0;     // starting phase
    float incphi0;  // phi increment
    float random;   // random component
    float f;        // main rotation frequency
    float r;        // main radius
    float theta;    // ellipse main axis rotation
    float e;        // excentricity
    float f_epi;    // epicycle frequency
    float r_epi;    // epicycle radius
    float phi0_epi; // starting phase epicycle
  };

  class parameter_t : public srvvars_t {
  public:
    parameter_t(tsccfg::node_t, TASCAR::osc_server_t* o);
    ~parameter_t();
    void locate0(float time);
    void az(float az_);
    void apply(float time);
    void set_stopat(float sa)
    {
      stopat = sa;
      b_stopat = true;
    };
    void set_applyat(float sa, float t)
    {
      applyat = sa;
      applyat_time = t;
      b_applyat = true;
    };
    void set_par_fupdate(float f_update_) { f_update = f_update_; };
    void send_phi(const char* addr)
    {
      if(lo_addr)
        lo_send(lo_addr, addr, "f", _az * RAD2DEG);
    }

  protected:
    HoS::par_t par_osc;
    HoS::par_t par_current;
    HoS::par_t par_previous;
    float stopat;
    bool b_stopat;
    float applyat;
    float applyat_time;
    bool b_applyat;
    // ellipse parameters:
    float e2;
    float r2;
    // normalized angular velocity:
    double w_main;
    double w_epi;
    // panning parameters:
    lo_address lo_addr;
    bool b_quit;
    float f_update;
    unsigned int t_locate; // slowly locate to position
    float locate_gain;
    unsigned int t_apply; // slowly apply parameters
    float apply_gain;
    // control parameter for panner:
    float _az;
    float _rho;
    float lastphi;
    float phi;
    float phi_epi;
    float home;
    bool b_home;
    pthread_mutex_t mtx;

  private:
  };

}; // namespace HoS

namespace OSC {

  int _sendphi(const char* , const char* types, lo_arg** argv, int argc,
               lo_message , void* user_data)
  {
    if((argc == 1) && (types[0] == 's')) {
      ((HoS::parameter_t*)user_data)->send_phi(&(argv[0]->s));
      return 0;
    }
    return 1;
  }

  int _locate(const char* , const char* types, lo_arg** argv, int argc,
              lo_message , void* user_data)
  {
    if((argc == 1) && (types[0] == 'f')) {
      ((HoS::parameter_t*)user_data)->locate0(argv[0]->f);
      return 0;
    }
    return 1;
  }

  int _stopat(const char* , const char* types, lo_arg** argv, int argc,
              lo_message , void* user_data)
  {
    if((argc == 1) && (types[0] == 'f')) {
      ((HoS::parameter_t*)user_data)->set_stopat(DEG2RAD * (argv[0]->f));
      return 0;
    }
    return 1;
  }

  int _applyat(const char* , const char* types, lo_arg** argv, int argc,
               lo_message , void* user_data)
  {
    if((argc == 2) && (types[0] == 'f') && (types[1] == 'f')) {
      ((HoS::parameter_t*)user_data)
          ->set_applyat(DEG2RAD * (argv[0]->f), argv[1]->f);
      return 0;
    }
    return 1;
  }

  int _apply(const char* , const char* types, lo_arg** argv, int argc,
             lo_message , void* user_data)
  {
    if((argc == 1) && (types[0] == 'f')) {
      ((HoS::parameter_t*)user_data)->apply(argv[0]->f);
      return 0;
    }
    return 1;
  }

  int _az(const char* , const char* types, lo_arg** argv, int argc,
          lo_message , void* user_data)
  {
    if((argc == 1) && (types[0] == 'f')) {
      ((HoS::parameter_t*)user_data)->az(argv[0]->f);
      return 0;
    }
    return 1;
  }

} // namespace OSC

void HoS::par_t::mix_static(float g, const par_t& p1, const par_t& p2)
{
  float g1 = 1.0f - g;
  float dphi(std::arg(std::exp(i_f * (p2.phi0 - p1.phi0))));
  phi0 = p1.phi0 + g1 * dphi;
  phi0_epi = std::arg(g * std::exp(i_f * p1.phi0_epi) +
                      g1 * std::exp(i_f * p2.phi0_epi));
}

//void HoS::par_t::assign_static(const par_t& p1)
//{
//#define MIX_(x) x = p1.x;
//  MIX_(phi0);
//  MIX_(phi0_epi);
//#undef MIX_
//}

void HoS::par_t::mix_dynamic(float g, const par_t& p1, const par_t& p2)
{
  float g1 = 1.0f - g;
#define MIX_(x) x = g * p1.x + g1 * p2.x;
  MIX_(random);
  MIX_(f);
  MIX_(r);
  MIX_(theta);
  MIX_(e);
  MIX_(f_epi);
  MIX_(r_epi);
#undef MIX_
}

void HoS::par_t::assign_dynamic(const par_t& p1)
{
#define MIX_(x) x = p1.x;
  MIX_(random);
  MIX_(f);
  MIX_(r);
  MIX_(theta);
  MIX_(e);
  MIX_(f_epi);
  MIX_(r_epi);
#undef MIX_
}

HoS::par_t::par_t()
    : phi0(0), random(0), f(0), r(1.0), theta(0), e(0), f_epi(0), r_epi(0),
      phi0_epi(0)
{
}

void HoS::parameter_t::az(float az_)
{
  par_osc.phi0 = DEG2RAD * az_;
  locate0(0);
}

void HoS::parameter_t::locate0(float time)
{
  if(pthread_mutex_lock(&mtx) == 0) {
    time *= f_update;
    if(time > 0)
      locate_gain = 1.0f / time;
    else {
      locate_gain = 0;
    }
    t_locate = 1 + time;
    par_osc.phi0 += par_osc.incphi0;
    par_previous.phi0 = phi;
    par_previous.phi0_epi = phi_epi;
    par_current.phi0 = phi;
    par_current.phi0_epi = phi_epi;
    // par_previous.assign_static( par_current );
    pthread_mutex_unlock(&mtx);
  }
}

void HoS::parameter_t::apply(float time)
{
  time *= f_update;
  t_apply = 0;
  if(time > 0)
    apply_gain = 1.0f / time;
  else {
    apply_gain = 0;
  }
  t_apply = 1 + time;
  b_stopat = false;
  b_applyat = false;
}

HoS::parameter_t::parameter_t(tsccfg::node_t e, TASCAR::osc_server_t* o)
    : srvvars_t(e), stopat(0), b_stopat(false), applyat(0), applyat_time(0),
      b_applyat(false), lo_addr(lo_address_new_from_url(targetaddr.c_str())),
      b_quit(false), f_update(1), t_locate(0), t_apply(0), lastphi(0), phi(0),
      phi_epi(0), home(0), b_home(false)
{
  GET_ATTRIBUTE_DEG(home, "Home direction of sound source");
  pthread_mutex_init(&mtx, NULL);
  if(lo_addr)
    lo_address_set_ttl(lo_addr, 1);
  o->add_bool_true(path + "/gohome", &b_home);
  o->add_float_degree(path + "/home", &home);
#define REGISTER_FLOAT_VAR(x) o->add_float(path + "/" + #x, &(par_osc.x))
#define REGISTER_FLOAT_VAR_DEGREE(x)                                           \
  o->add_float_degree(path + "/" + #x, &(par_osc.x))
#define REGISTER_CALLBACK(x, fmt)                                              \
  o->add_method(path + "/" + #x, fmt, OSC::_##x, this)
  REGISTER_FLOAT_VAR_DEGREE(phi0);
  REGISTER_FLOAT_VAR_DEGREE(incphi0);
  REGISTER_FLOAT_VAR(random);
  REGISTER_FLOAT_VAR(f);
  REGISTER_FLOAT_VAR(r);
  REGISTER_FLOAT_VAR_DEGREE(theta);
  REGISTER_FLOAT_VAR(e);
  REGISTER_FLOAT_VAR(f_epi);
  REGISTER_FLOAT_VAR(r_epi);
  REGISTER_FLOAT_VAR_DEGREE(phi0_epi);
  REGISTER_CALLBACK(sendphi, "s");
  REGISTER_CALLBACK(locate, "f");
  REGISTER_CALLBACK(apply, "f");
  REGISTER_CALLBACK(stopat, "f");
  REGISTER_CALLBACK(applyat, "ff");
  REGISTER_CALLBACK(az, "f");
#undef REGISTER_FLOAT_VAR
#undef REGISTER_CALLBACK
}

HoS::parameter_t::~parameter_t() {}

HoS::srvvars_t::srvvars_t(tsccfg::node_t e) : xml_element_t(e)
{
  GET_ATTRIBUTE(targetaddr, "",
                "Target url where the current position is sent to on trigger");
  GET_ATTRIBUTE(path, "", "Path prefix of plugin");
}

class epicycles_t : public TASCAR::actor_module_t, private HoS::parameter_t {
public:
  epicycles_t(const TASCAR::module_cfg_t& cfg);
  ~epicycles_t();
  void configure();
  void update(uint32_t frame, bool running);

private:
  bool use_transport;
};

epicycles_t::epicycles_t(const TASCAR::module_cfg_t& cfg)
    : actor_module_t(cfg), HoS::parameter_t(cfg.xmlsrc, cfg.session),
      use_transport(true)
{
  actor_module_t::GET_ATTRIBUTE_BOOL(
      use_transport, "Update traces only while transport is running");
}

epicycles_t::~epicycles_t() {}

double phidiff(double p1, double p2)
{
  return atan2(sin(p1) * cos(p2) - cos(p1) * sin(p2),
               cos(p1) * cos(p2) + sin(p1) * sin(p2));
}

bool iscrossing(double phi0, double phi, double lastphi)
{
  double pd1(phidiff(phi0, phi));
  double pd2(phidiff(phi0, lastphi));
  return (((pd1 >= 0) && (pd2 < 0)) || ((pd1 < 0) && (pd2 >= 0))) &&
         (fabs(pd1) < TASCAR_PI2) && (fabs(pd2) < TASCAR_PI2);
}

void epicycles_t::configure()
{
  actor_module_t::configure();
  f_update = f_sample / (double)n_fragment;
}

void epicycles_t::update(uint32_t , bool running)
{
  if(b_home) {
    par_osc.phi0 = home;
    b_home = false;
  }
  if(running || (!use_transport)) {
    if(pthread_mutex_trylock(&mtx) == 0) {
      // optionally apply dynamic parameters:
      if(t_apply) {
        t_apply--;
        float g = t_apply * apply_gain;
        par_current.mix_dynamic(g, par_previous, par_osc);
      } else {
        par_previous.assign_dynamic(par_current);
      }
      // optionally reset static parameters:
      if(t_locate) {
        t_locate--;
        float g = t_locate * locate_gain;
        par_current.mix_static(g, par_previous, par_osc);
        phi = par_current.phi0;
        phi_epi = par_current.phi0_epi;
      } else {
        // par_previous.assign_static( par_current );
        par_previous.phi0 = phi;
        par_previous.phi0_epi = phi_epi;
        par_current.phi0 = phi;
        par_current.phi0_epi = phi_epi;
      }
      pthread_mutex_unlock(&mtx);
    }
    // ellipse parameters:
    par_current.e = std::min(par_current.e, 0.9999f);
    e2 = par_current.e * par_current.e;
    r2 = par_current.r / sqrt(1.0 - e2);
    // normalized angular velocity:
    w_main =
        par_current.f * TASCAR_2PI * par_current.r * par_current.r / f_update;
    w_epi = par_current.f_epi * TASCAR_2PI / f_update;
    // random component:
    double r;
    r = (2.0 * TASCAR::drand()) - 0.7;
    r *= r * r;
    r *= par_current.random;
    w_main += r * n_fragment / 64.0;
    r = (2.0 * TASCAR::drand()) - 0.7;
    r *= r * r;
    r *= par_current.random;
    w_epi += r * n_fragment / 64.0;
    // panning parameters:
    // get ellipse in polar coordinates:
    float rho = r2 * (1.0f - e2) /
                (1.0f - par_current.e * cosf(phi - par_current.theta));
    // add epicycles:
    float x = rho * cosf(phi) + par_current.r_epi * cosf(phi_epi);
    float y = rho * sinf(phi) + par_current.r_epi * sinf(phi_epi);
    // calc resulting polar coordinates:
    _az = atan2f(y, x);
    _rho = sqrtf(x * x + y * y);
    // increment phi and phi_epi:
    phi += w_main / std::max(rho * rho, EPSf);
    while(phi > TASCAR_PIf)
      phi -= TASCAR_2PIf;
    while(phi < -TASCAR_PIf)
      phi += TASCAR_2PIf;
    phi_epi += w_epi;
    while(phi_epi > TASCAR_PIf)
      phi_epi -= TASCAR_2PIf;
    while(phi_epi < -TASCAR_PIf)
      phi_epi += TASCAR_2PIf;
    if(b_stopat && iscrossing(stopat, phi, lastphi)) {
      par_current.f = 0;
      par_osc.f = 0;
      par_previous.f = 0;
      phi = stopat;
      b_stopat = false;
    }
    if(b_applyat && iscrossing(applyat, phi, lastphi)) {
      par_current.f = 0;
      par_osc.f = 0;
      par_previous.f = 0;
      phi = applyat;
      b_applyat = false;
      apply(applyat_time);
    }
    lastphi = phi;
    // apply position:
    TASCAR::pos_t p0(x, y, 0);
    for(std::vector<TASCAR::named_object_t>::iterator iobj = obj.begin();
        iobj != obj.end(); ++iobj) {
      iobj->obj->dlocation = p0;
    }
  }
}

REGISTER_MODULE(epicycles_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
