#include "errorhandling.h"
#include "scene.h"

class rec_wfs_t : public TASCAR::receivermod_base_speaker_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t( uint32_t channels, float fs, float rmax, float c );
    // loudspeaker driving weights:
    std::vector<float> w;
    // loudspeaker delays:
    std::vector<float> d;
    // delay lines:
    std::vector<TASCAR::varidelay_t> dline;
  };
  rec_wfs_t(tsccfg::node_t xmlsrc);
  virtual ~rec_wfs_t() {};
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const;
  void add_variables( TASCAR::osc_server_t* srv );
private:
  float c;
  bool planewave;
  // loudspeaker driving weights + increments:
  std::vector<float> w;
  std::vector<float> dw;
  // loudspeaker delays + increments:
  std::vector<float> d;
  std::vector<float> dd;
};

rec_wfs_t::data_t::data_t( uint32_t channels, float fs, float rmax, float c )
  :  w(channels,0.0f),
     d(channels,0.0f),
     dline(channels,TASCAR::varidelay_t(rmax/c*fs, fs, c, 0, 1 ))
{
}

rec_wfs_t::rec_wfs_t(tsccfg::node_t xmlsrc)
  : TASCAR::receivermod_base_speaker_t(xmlsrc),
  c(340),
  planewave(true),
  w(spkpos.size(),0.0f),
  dw(spkpos.size(),0.0f),
  d(spkpos.size(),0.0f),
  dd(spkpos.size(),0.0f)
{
  GET_ATTRIBUTE(c,"m/s","Speed of sound");
  GET_ATTRIBUTE_BOOL(planewave,"Simlate always plane waves independent of distance");
  typeidattr.push_back("c");
  typeidattr.push_back("planewave");
}

void rec_wfs_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_bool( "/planewave", &planewave );
}

/*
  See receivermod_base_t::add_pointsource() in file receivermod.h for details.
*/
void rec_wfs_t::add_pointsource( const TASCAR::pos_t& prel,
                                  double width,
                                  const TASCAR::wave_t& chunk,
                                  std::vector<TASCAR::wave_t>& output,
                                  receivermod_base_t::data_t* sd)
{
  // N is the number of loudspeakers:
  uint32_t N(output.size());
  // 'state' is the internal state variable for this specific
  // receiver-source-pair:
  data_t* state((data_t*)sd);
  // psrc is the normalized source direction in the receiver
  // coordinate system:
  TASCAR::pos_t psrc(prel.normal());
  double rmax(spkpos.get_rmax());
  double rmaxprelnorm(rmax-prel.norm());
  // calculate final panning parameters:
  float wsum(0.0f);
  for(uint32_t ch=0;ch<N;++ch){
    w[ch] = std::max(0.0,TASCAR::dot_prod( psrc, spkpos[ch].unitvector ));
    wsum += w[ch];
    if( planewave ){
      d[ch] = rmax - spkpos[ch].r*w[ch];
    }else{
      d[ch] = std::max(0.0,rmaxprelnorm + distance(spkpos[ch],prel));
    }
  }
  if( wsum > 0 )
    wsum = 1.0f/wsum;
  else
    wsum = 1.0f;
  for(uint32_t ch=0;ch<N;++ch){
    w[ch]*=wsum;
    dw[ch] = (w[ch] - state->w[ch])*t_inc;
    dd[ch] = (d[ch] - state->d[ch])*t_inc;
  }
  //throw TASCAR::ErrMsg("Stop");
  // apply panning:
  for(uint32_t t=0;t<chunk.n;++t){
    float v(chunk[t]);
    for(uint32_t ch=0;ch<N;++ch){
      output[ch][t] += state->dline[ch].get_dist_push( (state->d[ch]+=dd[ch]),v)*(state->w[ch]+=dw[ch]);
    }
  }
  // copy final value:
  for(uint32_t ch=0;ch<N;++ch){
    state->w[ch] = w[ch];
    state->d[ch] = d[ch];
  }
}

TASCAR::receivermod_base_t::data_t* rec_wfs_t::create_state_data(double srate,uint32_t fragsize) const
{
  return new data_t(spkpos.size(), srate, 2.0*spkpos.get_rmax(), c );
}

REGISTER_RECEIVERMOD(rec_wfs_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
