#include "sinkmod.h"
#include "errorhandling.h"
#include "scene.h"

class nsp_t : public TASCAR::sinkmod_base_t {
public:
  class data_t : public TASCAR::sinkmod_base_t::data_t {
  public:
    data_t(uint32_t chunksize,uint32_t channels);
    virtual ~data_t();
    // ambisonic weights:
    float* w;
    float* dw;
    float* x;
    float* dx;
    float* y;
    float* dy;
    float* z;
    float* dz;
    double dt;
  };
  nsp_t(xmlpp::Element* xmlsrc);
  virtual ~nsp_t() {};
  void write_xml();
  void add_pointsource(const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, sinkmod_base_t::data_t*);
  void add_diffusesource(const TASCAR::pos_t& prel, const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, sinkmod_base_t::data_t*);
  uint32_t get_num_channels();
  sinkmod_base_t::data_t* create_data(double srate,uint32_t fragsize);
private:
  std::vector<TASCAR::Scene::spk_pos_t> spkpos;
};


nsp_t::data_t::data_t(uint32_t chunksize,uint32_t channels)
{
  w = new float[channels];
  dw = new float[channels];
  x = new float[channels];
  dx = new float[channels];
  y = new float[channels];
  dy = new float[channels];
  z = new float[channels];
  dz = new float[channels];
  for(uint32_t k=0;k<channels;k++)
    w[k] = dw[k] = x[k] = dx[k] = y[k] = dy[k] = z[k] = dz[k] = 0;
  dt = 1.0/std::max(1.0,(double)chunksize);
}

nsp_t::data_t::~data_t()
{
  delete [] w;
  delete [] dw;
  delete [] x;
  delete [] dx;
  delete [] y;
  delete [] dy;
  delete [] z;
  delete [] dz;
}

nsp_t::nsp_t(xmlpp::Element* xmlsrc)
  : TASCAR::sinkmod_base_t(xmlsrc)
{
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "speaker" )){
      spkpos.push_back(TASCAR::Scene::spk_pos_t(sne));
    }
  }
}

void nsp_t::add_pointsource(const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, sinkmod_base_t::data_t* sd)
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
    d->dw[k] = ((k==kmin) - d->w[k])*d->dt;
  for( unsigned int i=0;i<chunk.size();i++){
    for( unsigned int k=0;k<output.size();k++){
      output[k][i] += (d->w[k] += d->dw[k]) * chunk[i];
    }
  }
}

void nsp_t::add_diffusesource(const TASCAR::pos_t& prel, const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, sinkmod_base_t::data_t* sd)
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
  TASCAR::pos_t px(1,0,0);
  TASCAR::pos_t py(0,1,0);
  TASCAR::pos_t pz(0,0,1);
  for(unsigned int k=0;k<output.size();k++)
    d->dw[k] = (0.701 - d->w[k])*d->dt;
  for(unsigned int k=0;k<output.size();k++)
    d->dx[k] = (dot_prod(px,spkpos[k]) - d->x[k])*d->dt;
  for(unsigned int k=0;k<output.size();k++)
    d->dy[k] = (dot_prod(py,spkpos[k]) - d->y[k])*d->dt;
  for(unsigned int k=0;k<output.size();k++)
    d->dz[k] = (dot_prod(pz,spkpos[k]) - d->z[k])*d->dt;
  for( unsigned int i=0;i<chunk.size();i++){
    for( unsigned int k=0;k<output.size();k++){
      output[k][i] += (d->w[k] += d->dw[k]) * chunk.w()[i];
      output[k][i] += (d->x[k] += d->dx[k]) * chunk.x()[i];
      output[k][i] += (d->y[k] += d->dy[k]) * chunk.y()[i];
      output[k][i] += (d->z[k] += d->dz[k]) * chunk.z()[i];
    }
  }
}

uint32_t nsp_t::get_num_channels()
{
  return spkpos.size();
}

TASCAR::sinkmod_base_t::data_t* nsp_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(fragsize,spkpos.size());
}

REGISTER_SINKMOD(nsp_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
