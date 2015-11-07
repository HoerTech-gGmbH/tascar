#include "errorhandling.h"
#include "scene.h"

class nsp_t : public TASCAR::receivermod_base_speaker_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize,uint32_t channels);
    virtual ~data_t();
    // point source speaker weights:
    float* point_w;
    float* point_dw;
    // ambisonic weights:
    float* diff_w;
    float* diff_dw;
    float* diff_x;
    float* diff_dx;
    float* diff_y;
    float* diff_dy;
    float* diff_z;
    float* diff_dz;
    double dt;
  };
  nsp_t(xmlpp::Element* xmlsrc);
  virtual ~nsp_t() {};
  //void write_xml();
  void add_pointsource(const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  uint32_t get_num_channels();
  std::string get_channel_postfix(uint32_t channel) const;
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
private:
  //TASCAR::spk_array_t spkpos;
};


nsp_t::data_t::data_t(uint32_t chunksize,uint32_t channels)
{
  point_w = new float[channels];
  point_dw = new float[channels];
  diff_w = new float[channels];
  diff_dw = new float[channels];
  diff_x = new float[channels];
  diff_dx = new float[channels];
  diff_y = new float[channels];
  diff_dy = new float[channels];
  diff_z = new float[channels];
  diff_dz = new float[channels];
  for(uint32_t k=0;k<channels;k++)
    point_w[k] = point_dw[k] = diff_w[k] = diff_dw[k] = diff_x[k] = 
      diff_dx[k] = diff_y[k] = diff_dy[k] = diff_z[k] = diff_dz[k] = 0;
  dt = 1.0/std::max(1.0,(double)chunksize);
}

nsp_t::data_t::~data_t()
{
  delete [] point_w;
  delete [] point_dw;
  delete [] diff_w;
  delete [] diff_dw;
  delete [] diff_x;
  delete [] diff_dx;
  delete [] diff_y;
  delete [] diff_dy;
  delete [] diff_z;
  delete [] diff_dz;
}

nsp_t::nsp_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_speaker_t(xmlsrc)
{
}

void nsp_t::add_pointsource(const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  TASCAR::pos_t psrc(prel.normal());
  uint32_t kmin(0);
  double dmin(distance(psrc,spkpos[kmin]));
  double dist(0);
  for(unsigned int k=1;k<output.size();k++)
    if( (dist = distance(psrc,spkpos[k]))<dmin ){
      kmin = k;
      dmin = dist;
    }
  for(unsigned int k=0;k<output.size();k++)
    d->point_dw[k] = ((k==kmin) - d->point_w[k])*d->dt;
  for( unsigned int i=0;i<chunk.size();i++){
    for( unsigned int k=0;k<output.size();k++){
      output[k][i] += (d->point_w[k] += d->point_dw[k]) * chunk[i];
    }
  }
}

void nsp_t::add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  spkpos.foa_decode(chunk,output);
  //data_t* d((data_t*)sd);
  //TASCAR::pos_t psrc(prel.normal());
  //uint32_t kmin(0);
  //double dmin(distance(psrc,spkpos[kmin]));
  //double dist(0);
  //for(unsigned int k=1;k<output.size();k++)
  //  if( (dist = distance(psrc,spkpos[k]))<dmin ){
  //    kmin = k;
  //    dmin = dist;
  //  }
  //TASCAR::pos_t px(1,0,0);
  //TASCAR::pos_t py(0,1,0);
  //TASCAR::pos_t pz(0,0,1);
  //for(unsigned int k=0;k<output.size();k++)
  //  d->diff_dw[k] = (0.701 - d->diff_w[k])*d->dt;
  //for(unsigned int k=0;k<output.size();k++)
  //  d->diff_dx[k] = (dot_prod(px,spkpos[k]) - d->diff_x[k])*d->dt;
  //for(unsigned int k=0;k<output.size();k++)
  //  d->diff_dy[k] = (dot_prod(py,spkpos[k]) - d->diff_y[k])*d->dt;
  //for(unsigned int k=0;k<output.size();k++)
  //  d->diff_dz[k] = (dot_prod(pz,spkpos[k]) - d->diff_z[k])*d->dt;
  //for( unsigned int i=0;i<chunk.size();i++){
  //  for( unsigned int k=0;k<output.size();k++){
  //    output[k][i] += (d->diff_w[k] += d->diff_dw[k]) * chunk.w()[i];
  //    output[k][i] += (d->diff_x[k] += d->diff_dx[k]) * chunk.x()[i];
  //    output[k][i] += (d->diff_y[k] += d->diff_dy[k]) * chunk.y()[i];
  //    output[k][i] += (d->diff_z[k] += d->diff_dz[k]) * chunk.z()[i];
  //  }
  //}
}

uint32_t nsp_t::get_num_channels()
{
  return spkpos.size();
}

std::string nsp_t::get_channel_postfix(uint32_t channel) const
{
  char ctmp[1024];
  sprintf(ctmp,".%d%s",channel,spkpos[channel].label.c_str());
  return ctmp;
}


TASCAR::receivermod_base_t::data_t* nsp_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(fragsize,spkpos.size());
}

REGISTER_RECEIVERMOD(nsp_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
