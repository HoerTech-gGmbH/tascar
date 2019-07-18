#include "audioplugin.h"

class delay_t : public TASCAR::audioplugin_base_t {
public:
  delay_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  void prepare( chunk_cfg_t& cf_ );
  void release();
  ~delay_t();
private:
  double delay;
  uint32_t idelay;
  TASCAR::wave_t* dline;
  uint32_t pos;
};

delay_t::delay_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    delay(1),
    dline(NULL),
    pos(0)
{
  GET_ATTRIBUTE(delay);
}

void delay_t::prepare( chunk_cfg_t& cf_ )
{
  audioplugin_base_t::prepare( cf_ );
  dline = new TASCAR::wave_t(std::max(1.0,f_sample*delay));
  pos = 0;
}

void delay_t::release()
{
  audioplugin_base_t::release();
  delete dline;
  dline = NULL;
}

delay_t::~delay_t()
{
}

void delay_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& p0, const TASCAR::transport_t& tp)
{
  if( dline && (delay > 0) ){
    for(uint32_t k=0;k<chunk[0].n;++k){
      float v(dline->d[pos]);
      dline->d[pos] = chunk[0][k];
      chunk[0][k] = v;
      if( pos )
        pos--;
      else
        pos = dline->n-1;
    }
  }
}

REGISTER_AUDIOPLUGIN(delay_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
