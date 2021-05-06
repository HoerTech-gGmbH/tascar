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

#include "audioplugin.h"
#include "filterclass.h"

static int osc_set_message(const char* path, const char* types, lo_arg** argv,
                           int argc, lo_message msg, void* user_data)
{
  if(user_data) {
    TASCAR::msg_t* tmsg((TASCAR::msg_t*)user_data);
    lo_message_free(tmsg->msg);
    tmsg->msg = lo_message_clone(msg);
  }
  return 0;
}

class metronome_t : public TASCAR::audioplugin_base_t {
public:
  metronome_t(const TASCAR::audioplugin_cfg_t& cfg);
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos,
                  const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void add_variables(TASCAR::osc_server_t* srv);
  void configure();
  void release();
  ~metronome_t();

private:
  void update_par();
  bool changeonone;
  double bpm;
  uint32_t bpb;
  double a1;
  double ao;
  double fres1;
  double freso;
  double q1;
  double qo;
  bool sync;
  bool bypass;
  bool bypass_;
  int64_t t;
  int64_t beat;
  TASCAR::resonance_filter_t f1;
  TASCAR::resonance_filter_t fo;
  uint32_t bpb_;
  double a1_;
  double ao_;
  int64_t period;
  uint32_t dispatchin;
  TASCAR::osc_server_t* srv_;
  TASCAR::msg_t msg;
};

metronome_t::metronome_t(const TASCAR::audioplugin_cfg_t& cfg)
    : audioplugin_base_t(cfg), changeonone(false), bpm(120), bpb(4), a1(0.002),
      ao(0.001), fres1(1000), freso(600), q1(0.997), qo(0.997), sync(false),
      bypass(false), bypass_(false), t(0), beat(0), bpb_(4), a1_(0.002),
      ao_(0.001), period(0), dispatchin(0), srv_(NULL),
      msg(find_or_add_child("msg"))
{
  GET_ATTRIBUTE_BOOL(changeonone,"Apply OSC parameter changes on next bar");
  GET_ATTRIBUTE(bpm,"","Beats per minute");
  GET_ATTRIBUTE(bpb,"","Beats per bar");
  GET_ATTRIBUTE_DBSPL(a1,"Amplitude of first beat");
  GET_ATTRIBUTE_DBSPL(ao,"Amplitude of other beats");
  GET_ATTRIBUTE_BOOL(sync,"Use object time synchronization");
  GET_ATTRIBUTE(fres1,"Hz","Resonance frequency of first beat");
  GET_ATTRIBUTE(freso,"Hz","Resonance frequency of other beats");
  GET_ATTRIBUTE(q1,"","Filter resonance of first beat");
  GET_ATTRIBUTE(qo,"","Filter resonance of other beats");
  GET_ATTRIBUTE_BOOL(bypass,"Load in bypass mode");
}

void metronome_t::configure()
{
  audioplugin_base_t::configure();
  update_par();
}

void metronome_t::release()
{
  audioplugin_base_t::release();
}

void metronome_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv_ = srv;
  srv->add_bool("/changeonone", &changeonone);
  srv->add_double("/bpm", &bpm);
  srv->add_uint("/bpb", &bpb);
  srv->add_double_dbspl("/a1", &a1);
  srv->add_double_dbspl("/ao", &ao);
  srv->add_bool("/sync", &sync);
  srv->add_double("/filter/f1", &fres1);
  srv->add_double("/filter/fo", &freso);
  srv->add_double("/filter/q1", &q1);
  srv->add_double("/filter/qo", &qo);
  srv->add_bool("/bypass", &bypass);
  srv->add_uint("/dispatchin", &dispatchin);
  srv->add_method("/dispatchmsg", NULL, &osc_set_message, &msg);
  srv->add_string("/dispatchpath", &(msg.path));
}

metronome_t::~metronome_t() {}

void metronome_t::update_par()
{
  bypass_ = bypass;
  f1.set_fq(fres1 * t_sample, q1);
  fo.set_fq(freso * t_sample, qo);
  if(bpm < 6.9444e-04)
    bpm = 6.9444e-04;
  period = (60 * (int64_t)f_sample) / (int64_t)bpm;
  ao_ = ao;
  a1_ = a1;
  bpb_ = bpb;
}

void metronome_t::ap_process(std::vector<TASCAR::wave_t>& chunk,
                             const TASCAR::pos_t& pos,
                             const TASCAR::zyx_euler_t&,
                             const TASCAR::transport_t& tp)
{
  // apply parameters:
  if(!changeonone)
    update_par();
  float v(0);
  size_t Nch(chunk.size());
  uint32_t Nsmp(0);
  if(Nch > 0)
    Nsmp = chunk[0].n;
  // TASCAR::wave_t& aud(chunk[0]);
  int64_t objecttime(tp.object_time_samples);
  for(uint32_t k = 0; k < Nsmp; ++k) {
    if(sync) {
      // synchronize to time line:
      t = objecttime;
      if(tp.rolling)
        ++objecttime;
      ldiv_t tmp(ldiv(t, period));
      t = tmp.rem;
      beat = tmp.quot;
      if(bpb_ > 0) {
        tmp = ldiv(beat, bpb_);
        beat = tmp.rem;
      } else
        beat = 0;
    } else {
      // free-run mode, in
      if(t >= period) {
        t = 0;
        ++beat;
        if(beat >= bpb_)
          beat = 0;
      }
    }
    if(t || (sync && !tp.rolling)) {
      // this is not a beat sample:
      v = 0.0f;
    } else {
      // now we are exactly on a beat.
      // if beat != 0 then this is not a one:
      if(beat)
        v = ao_;
      else {
        if(dispatchin) {
          // count down "dispatchin". If zero, then dispatch message.
          --dispatchin;
          if((!dispatchin) && srv_)
            srv_->dispatch_data_message(msg.path.c_str(), msg.msg);
        }
        if(changeonone)
          update_par();
        v = a1_;
      }
    }
    if(!bypass_) {
      // beat-dependent filter properties:
      if(beat)
        v = fo.filter_unscaled(v);
      else
        v = f1.filter_unscaled(v);
      for(uint32_t c = 0; c < Nch; c++)
        chunk[c].d[k] += v;
    }
    ++t;
  }
}

REGISTER_AUDIOPLUGIN(metronome_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
