#include "receivermod.h"
#include "delayline.h"

class ortf_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(double srate,uint32_t chunksize,double maxdist,double c,uint32_t sincorder);
    double fs;
    double dt;
    TASCAR::varidelay_t dline_l;
    TASCAR::varidelay_t dline_r;
    double wl;
    double wr;
    double itd;
    double state_l;
    double state_r;
  };
  ortf_t(xmlpp::Element* xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  uint32_t get_num_channels();
  std::string get_channel_postfix(uint32_t channel) const;
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
private:
  double distance;
  double angle;
  double f6db;
  double fmin;
  uint32_t sincorder;
  double c;
  TASCAR::pos_t dir_l;
  TASCAR::pos_t dir_r;
  TASCAR::pos_t dir_itd;
  double wpow;
  double wmin;
};

ortf_t::data_t::data_t(double srate,uint32_t chunksize,double maxdist,double c,uint32_t sincorder)
  : fs(srate),
    dt(1.0/std::max(1.0,(double)chunksize)),
    dline_l(2*maxdist*srate/c+2+sincorder,srate,c,sincorder,64),
    dline_r(2*maxdist*srate/c+2+sincorder,srate,c,sincorder,64),
    wl(0),wr(0),itd(0),state_l(0),state_r(0)
{
}

ortf_t::ortf_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),
    distance(0.17),
    angle(110*DEG2RAD),
    f6db(1000),
    fmin(60),
    sincorder(0),
    c(340),
    dir_l(1,0,0),
    dir_r(1,0,0),
    dir_itd(0,1,0),
    wpow(1),
    wmin(EPS)
{
  GET_ATTRIBUTE(distance);
  GET_ATTRIBUTE_DEG(angle);
  GET_ATTRIBUTE(f6db);
  GET_ATTRIBUTE(fmin);
  GET_ATTRIBUTE(sincorder);
  GET_ATTRIBUTE(c);
  dir_l.rot_z(0.5*angle);
  dir_r.rot_z(-0.5*angle);
}

void ortf_t::add_pointsource(const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  TASCAR::pos_t prel_norm(prel.normal());
  // calculate panning parameters (as incremental values):
  double wl(pow(0.5-0.5*dot_prod(prel_norm,dir_l),wpow));
  double wr(pow(0.5-0.5*dot_prod(prel_norm,dir_r),wpow));
  if( wl > wmin )
    wl = wmin;
  if( wr > wmin )
    wr = wmin;
  if( !(wl > EPS) )
    wl = EPS;
  if( !(wr > EPS) )
    wr = EPS;
  double dwl((wl - d->wl)*d->dt);
  double dwr((wr - d->wr)*d->dt);
  double ditd((distance*(0.5*dot_prod(prel_norm,dir_itd)+0.5) - d->itd)*d->dt);
  // apply panning:
  uint32_t N(chunk.size());
  for(uint32_t k=0;k<N;++k){
    float v(chunk[k]);
    output[0][k] += (d->state_l = (d->dline_l.get_dist_push(distance-d->itd,v)*(1.0f-d->wl) + d->state_l*d->wl));
    output[1][k] += (d->state_r = (d->dline_r.get_dist_push(d->itd,v)*(1.0f-d->wr) + d->state_r*d->wr));
    d->wl+=dwl;
    d->wr+=dwr;
    d->itd += ditd;
  }
}

void ortf_t::add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
  float* o_l(output[0].d);
  float* o_r(output[1].d);
  const float* i_w(chunk.w().d);
  const float* i_x(chunk.x().d);
  const float* i_y(chunk.y().d);
  //const float* i_z(chunk.z().d);
  for(uint32_t k=0;k<chunk.size();++k){
    *o_l += *i_w + dir_l.x*(*i_x) + dir_l.y*(*i_y);
    *o_r += *i_w + dir_r.x*(*i_x) + dir_r.y*(*i_y);
    ++o_l;
    ++o_r;
    ++i_w;
    ++i_x;
    ++i_y;
  }
  //output[0] += chunk.w() + chunk.x() + chunk.y();
  //output[1] += chunk.w();
}

uint32_t ortf_t::get_num_channels()
{
  return 2;
}

std::string ortf_t::get_channel_postfix(uint32_t channel) const
{
  if( channel == 0)
    return "_l";
  return "_r";
}

TASCAR::receivermod_base_t::data_t* ortf_t::create_data(double srate,uint32_t fragsize)
{
  wpow = log(exp(-M_PI*f6db/srate))/log(0.5);
  //wmin = pow(exp(-M_PI*fmin/srate),wpow);
  wmin = exp(-M_PI*fmin/srate);
  return new data_t(srate,fragsize,distance,c,sincorder);
}

REGISTER_RECEIVERMOD(ortf_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
