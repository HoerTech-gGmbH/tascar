/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
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

#include "errorhandling.h"
#include "scene.h"

class simplex_t {
public:
  simplex_t() : c1(-1),c2(-1){};
  bool get_gain( const TASCAR::pos_t& p){
    g1 = p.x*l11+p.y*l21;
    g2 = p.x*l12+p.y*l22;
    if( (g1>=0.0) && (g2>=0.0) ){
      double w(sqrt(g1*g1+g2*g2));
      if( w > 0 )
        w = 1.0/w;
      g1*=w;
      g2*=w;
      return true;
    }
    return false;
  };
  uint32_t c1;
  uint32_t c2;
  double l11;
  double l12;
  double l21;
  double l22;
  double g1;
  double g2;
};

class rec_itu51_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t channels);
    virtual ~data_t();
    // loudspeaker driving weights:
    float* wp;
    // differential driving weights:
    float* dwp;
  };
  rec_itu51_t(tsccfg::node_t xmlsrc);
  virtual ~rec_itu51_t() {};
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const;
  void configure();
  void release( );
  void postproc( std::vector<TASCAR::wave_t>& output );
  std::vector<simplex_t> simplices;
  std::vector<TASCAR::pos_t> spkpos;
  std::vector<TASCAR::wave_t*> render_buffer;
  TASCAR::pos_t dir_L;
  TASCAR::pos_t dir_R;
  TASCAR::pos_t dir_Ls;
  TASCAR::pos_t dir_Rs;
  double diffusegainfront;
  double diffusegainrear;
  double fc;
  std::vector<TASCAR::biquad_t> filter;
  TASCAR::biquad_t allpL;
  TASCAR::biquad_t allpR;
  TASCAR::biquad_t allpC;
  TASCAR::biquad_t allpLs;
  TASCAR::biquad_t allpRs;
  double r;
};

void rec_itu51_t::configure()
{
  TASCAR::receivermod_base_t::configure();
  n_channels = 6;
  render_buffer.clear();
  // render buffer gets 5 channels: L R Ls Rs LFE
  for(uint32_t k = 0; k < 5; ++k) {
    render_buffer.push_back(new TASCAR::wave_t(n_fragment));
  }
  filter.resize(6);
  for(auto it = filter.begin(); it != filter.end(); ++it)
    it->set_highpass(fc, f_sample);
  filter[3].set_lowpass(fc, f_sample, true);
  // set_gzp( 1.0, 1.0, 0.0, pow(10.0,-2.0*fc/fs), fc/fs*TASCAR_2PI );
  const double f0(0.125 * fc / f_sample * TASCAR_2PI);
  allpL.set_gzp(1.0, 1, -f0, 1.0 / r, f0);
  const double g(std::abs(allpL.response(TASCAR_PI)));
  allpL.set_gzp(1.0 / g, 1, -f0, 1.0 / r, f0);
  allpR.set_gzp(1.0 / g, 1, -f0, 1.0 / r, f0);
  allpC.set_gzp(1.0 / g, 1, -f0, 1.0 / r, f0);
  allpLs.set_gzp(1.0 / g, 1, -f0, 1.0 / r, f0);
  allpRs.set_gzp(1.0 / g, 1, -f0, 1.0 / r, f0);
  labels.clear();
  labels.push_back(".0L");
  labels.push_back(".1R");
  labels.push_back(".2C");
  labels.push_back(".3LFE");
  labels.push_back(".4Ls");
  labels.push_back(".5Rs");
}

void rec_itu51_t::release( )
{
  TASCAR::receivermod_base_t::release();
  for( auto it=render_buffer.begin();it!=render_buffer.end();++it)
    delete (*it);
  render_buffer.clear();
}

void rec_itu51_t::postproc( std::vector<TASCAR::wave_t>& output )
{
  TASCAR::receivermod_base_t::postproc( output );
  *(render_buffer[0]) *= diffusegainfront;
  *(render_buffer[1]) *= diffusegainfront;
  *(render_buffer[2]) *= diffusegainrear;
  *(render_buffer[3]) *= diffusegainrear;
  *(render_buffer[4]) *= diffusegainrear;
  output[0] += *(render_buffer[0]);
  output[1] += *(render_buffer[1]);
  output[3] += *(render_buffer[2]);
  output[4] += *(render_buffer[3]);
  output[5] += *(render_buffer[4]);
  for( auto it=render_buffer.begin();it!=render_buffer.end();++it)
    (*it)->clear();
  for( uint32_t ch=0;ch<std::min(output.size(),filter.size());++ch)
    filter[ch].filter( output[ch] );
  allpL.filter(output[0]);
  allpR.filter(output[1]);
  allpC.filter(output[2]);
  allpLs.filter(output[4]);
  allpRs.filter(output[5]);
}

rec_itu51_t::data_t::data_t( uint32_t channels )
{
  wp = new float[channels];
  dwp = new float[channels];
  for(uint32_t k=0;k<channels;++k)
    wp[k] = dwp[k] = 0;
}

rec_itu51_t::data_t::~data_t()
{
  delete [] wp;
  delete [] dwp;
}

rec_itu51_t::rec_itu51_t(tsccfg::node_t xmlsrc)
    : TASCAR::receivermod_base_t(xmlsrc), dir_L(sqrt(0.5), sqrt(0.5), 0),
      dir_R(sqrt(0.5), -sqrt(0.5), 0), dir_Ls(-sqrt(0.5), sqrt(0.5), 0),
      dir_Rs(-sqrt(0.5), -sqrt(0.5), 0), diffusegainfront(0.5),
      diffusegainrear(1), fc(80), r(1.01)
{
  GET_ATTRIBUTE(fc, "Hz", "LFE cut off frequency");
  GET_ATTRIBUTE_DB(diffusegainfront, "Diffuse gain for frontal speakers");
  GET_ATTRIBUTE_DB(diffusegainrear, "Diffuse gain for rear speakers");
  // L:
  spkpos.push_back(TASCAR::pos_t(1, 1, 0));
  // R:
  spkpos.push_back(TASCAR::pos_t(1, -1, 0));
  // C:
  spkpos.push_back(TASCAR::pos_t(1, 0, 0));
  // Ls:
  spkpos.push_back(TASCAR::pos_t(-1, 1, 0));
  // Rs:
  spkpos.push_back(TASCAR::pos_t(-1, -1, 0));
  if(spkpos.size() < 2)
    throw TASCAR::ErrMsg("At least two loudspeakers are required for 2D-VBAP.");
  std::vector<TASCAR::pos_t> hull;
  for(uint32_t k = 0; k < spkpos.size(); ++k) {
    if(fabs(spkpos[k].z) > EPS)
      throw TASCAR::ErrMsg("Channel elevation must be zero for 2D VBAP");
    hull.push_back(spkpos[k]);
  }
  auto spklist = hull;
  std::sort(hull.begin(), hull.end(),
            [](const TASCAR::pos_t& a, const TASCAR::pos_t& b) {
              return a.azim() < b.azim();
            });
  hull.push_back(hull[0]);
  // identify channel numbers of simplex vertices and store inverted
  // loudspeaker matrices:
  for(uint32_t khull = 0; khull < hull.size() - 1; ++khull) {
    simplex_t sim;
    sim.c1 = spklist.size();
    sim.c2 = spklist.size();
    for(uint32_t k = 0; k < spklist.size(); ++k)
      if(spklist[k] == hull[khull])
        sim.c1 = k;
    for(uint32_t k = 0; k < spklist.size(); ++k)
      if(spklist[k] == hull[khull + 1])
        sim.c2 = k;
    if((sim.c1 >= spklist.size()) || (sim.c2 >= spklist.size()))
      throw TASCAR::ErrMsg("Simplex vertex not found in speaker list.");
    double l11(spkpos[sim.c1].x);
    double l12(spkpos[sim.c1].y);
    double l21(spkpos[sim.c2].x);
    double l22(spkpos[sim.c2].y);
    double det_speaker(l11 * l22 - l21 * l12);
    if(det_speaker != 0)
      det_speaker = 1.0 / det_speaker;
    sim.l11 = det_speaker * l22;
    sim.l12 = -det_speaker * l12;
    sim.l21 = -det_speaker * l21;
    sim.l22 = det_speaker * l11;
    simplices.push_back(sim);
  }
}

/*
  See receivermod_base_t::add_pointsource() in file receivermod.h for details.
*/
void rec_itu51_t::add_pointsource( const TASCAR::pos_t& prel,
                                  double width,
                                  const TASCAR::wave_t& chunk,
                                  std::vector<TASCAR::wave_t>& output,
                                  receivermod_base_t::data_t* sd)
{
  // N is the number of main loudspeakers = 5:
  uint32_t N(spkpos.size());

  // d is the internal state variable for this specific
  // receiver-source-pair:
  data_t* d((data_t*)sd);//it creates the variable d

  // psrc_normal is the normalized source direction in the receiver
  // coordinate system:
  TASCAR::pos_t psrc_normal(prel.normal());

  for(unsigned int k=0;k<N;k++)
    d->dwp[k] = - d->wp[k]*t_inc;
  uint32_t k=0;
  for( auto it=simplices.begin();it!=simplices.end();++it){
    if( it->get_gain(psrc_normal) ){
      d->dwp[it->c1] = (it->g1 - d->wp[it->c1])*t_inc;
      d->dwp[it->c2] = (it->g2 - d->wp[it->c2])*t_inc;
    }
    ++k;
  }
  // i is time (in samples):
  for( unsigned int i=0;i<chunk.size();i++){
    //output in each louspeaker k at sample i:
    // omit LFE channel (not a main speaker)
    output[0][i] += (d->wp[0] += d->dwp[0]) * chunk[i];
    output[1][i] += (d->wp[1] += d->dwp[1]) * chunk[i];
    output[2][i] += (d->wp[2] += d->dwp[2]) * chunk[i];
    output[4][i] += (d->wp[3] += d->dwp[3]) * chunk[i];
    output[5][i] += (d->wp[4] += d->dwp[4]) * chunk[i];
  }
  // add omni signal to LFE channel:
  output[3] += chunk;
}

TASCAR::receivermod_base_t::data_t* rec_itu51_t::create_state_data(double srate,uint32_t fragsize) const
{
  return new data_t(spkpos.size());
}

void rec_itu51_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
  float* o_l(render_buffer[0]->d);
  float* o_r(render_buffer[1]->d);
  float* o_ls(render_buffer[2]->d);
  float* o_rs(render_buffer[3]->d);
  float* o_lfe(render_buffer[4]->d);
  const float* i_w(chunk.w().d);
  const float* i_x(chunk.x().d);
  const float* i_y(chunk.y().d);
  // decode diffuse sound field in microphone directions:
  for(uint32_t k=0;k<chunk.size();++k){
    *o_l += *i_w + dir_L.x*(*i_x) + dir_L.y*(*i_y);
    *o_r += *i_w + dir_R.x*(*i_x) + dir_R.y*(*i_y);
    *o_ls += *i_w + dir_Ls.x*(*i_x) + dir_Ls.y*(*i_y);
    *o_rs += *i_w + dir_Rs.x*(*i_x) + dir_Rs.y*(*i_y);
    *o_lfe += *i_w;
    ++o_l;
    ++o_r;
    ++o_ls;
    ++o_rs;
    ++o_lfe;
    ++i_w;
    ++i_x;
    ++i_y;
  }
}

REGISTER_RECEIVERMOD(rec_itu51_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
