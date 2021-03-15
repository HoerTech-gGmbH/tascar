#include "errorhandling.h"
#include "scene.h"
#include <math.h>

#include "vbap3d.h"

class rec_vbap_t : public TASCAR::receivermod_base_speaker_t {
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
  rec_vbap_t(tsccfg::node_t xmlsrc);
  virtual ~rec_vbap_t() {};
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
private:
  TASCAR::vbap3d_t vbap;
};

rec_vbap_t::data_t::data_t( uint32_t channels )
{
  wp = new float[channels];
  dwp = new float[channels];
  for(uint32_t k=0;k<channels;++k)
    wp[k] = dwp[k] = 0;
}

rec_vbap_t::data_t::~data_t()
{
  delete [] wp;
  delete [] dwp;
}

rec_vbap_t::rec_vbap_t(tsccfg::node_t xmlsrc)
  : TASCAR::receivermod_base_speaker_t(xmlsrc),
  vbap(spkpos.get_positions())
{
}

/*
  See receivermod_base_t::add_pointsource() in file receivermod.h for details.
*/
void rec_vbap_t::add_pointsource( const TASCAR::pos_t& prel,
                                  double width,
                                  const TASCAR::wave_t& chunk,
                                  std::vector<TASCAR::wave_t>& output,
                                  receivermod_base_t::data_t* sd)
{
  // N is the number of loudspeakers:
  uint32_t N(vbap.numchannels);
  if( N > output.size() ){
    DEBUG(N);
    DEBUG(output.size());
    throw TASCAR::ErrMsg("Invalid number of channels");
  }

  // d is the internal state variable for this specific
  // receiver-source-pair:
  data_t* d((data_t*)sd);//it creates the variable d

  vbap.encode( prel.normal() );

  for(unsigned int k=0;k<N;k++)
    d->dwp[k] = (vbap.weights[k] - d->wp[k])*t_inc;
  // i is time (in samples):
  for( unsigned int i=0;i<chunk.size();i++){
    // k is output channel number:
    for( unsigned int k=0;k<N;k++){
      //output in each louspeaker k at sample i:
      output[k][i] += (d->wp[k] += d->dwp[k]) * chunk[i];
      // This += is because we sum up all the sources for which we
      // call this func
    }
  }
}

TASCAR::receivermod_base_t::data_t* rec_vbap_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(spkpos.size());
}

REGISTER_RECEIVERMOD(rec_vbap_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
