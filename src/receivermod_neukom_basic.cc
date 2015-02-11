#include "errorhandling.h"
#include "scene.h"

class neukom_basic_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
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
  neukom_basic_t(xmlpp::Element* xmlsrc);
  virtual ~neukom_basic_t() {};
  void write_xml();
  void add_pointsource(const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffusesource(const TASCAR::pos_t& prel, const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  uint32_t get_num_channels();
  std::string get_channel_postfix(uint32_t channel) const;
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
private:
  std::vector<TASCAR::Scene::spk_pos_t> spkpos;
  std::vector<double> spk_az;
  std::vector<double> spk_gain;
  uint32_t order;
};

void neukom_basic_t::write_xml()
{
  SET_ATTRIBUTE(order);
}

neukom_basic_t::data_t::data_t(uint32_t chunksize,uint32_t channels)
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

neukom_basic_t::data_t::~data_t()
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

neukom_basic_t::neukom_basic_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc)
{
  GET_ATTRIBUTE(order);
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "speaker" )){
      spkpos.push_back(TASCAR::Scene::spk_pos_t(sne));
    }
  }
  double maxd(0);
  for(uint32_t k=0;k<spkpos.size();k++){
    if( maxd < spkpos[k].norm() )
      maxd = spkpos[k].norm();
  }
  for(uint32_t k=0;k<spkpos.size();k++){
    spk_az.push_back(spkpos[k].azim());
    spk_gain.push_back(maxd/spkpos[k].norm());
  }
}

void neukom_basic_t::add_pointsource(const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  double az_src(prel.azim());
  for(unsigned int k=0;k<output.size();k++){
    double az = az_src - spk_az[k];
    double w = 0.5;
    for(unsigned int l=1;l<=order;l++)
      w += cos(l*az);
    w *= 2.0;
    w /= (double)spkpos.size();
    w *= spk_gain[k];
    d->dw[k] = (w - d->w[k])*d->dt;
  }
  for( unsigned int i=0;i<chunk.size();i++){
    for( unsigned int k=0;k<output.size();k++){
      output[k][i] += (d->w[k] += d->dw[k]) * chunk[i];
    }
  }
}

void neukom_basic_t::add_diffusesource(const TASCAR::pos_t& prel, const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
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

uint32_t neukom_basic_t::get_num_channels()
{
  return spkpos.size();
}

std::string neukom_basic_t::get_channel_postfix(uint32_t channel) const
{
  char ctmp[1024];
  sprintf(ctmp,".%d",channel);
  return ctmp;
}


TASCAR::receivermod_base_t::data_t* neukom_basic_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(fragsize,spkpos.size());
}

REGISTER_RECEIVERMOD(neukom_basic_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
