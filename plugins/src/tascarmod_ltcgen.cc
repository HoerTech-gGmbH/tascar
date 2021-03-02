#include "session.h"
#include <ltc.h>
#include <string.h>

class ltcgen_t : public TASCAR::module_base_t, public jackc_transport_t {
public:
  ltcgen_t( const TASCAR::module_cfg_t& cfg );
  virtual ~ltcgen_t();
  virtual int process(jack_nframes_t, const std::vector<float*>&, const std::vector<float*>&,uint32_t tp_frame, bool tp_rolling);
private:
  double fpsnum;
  double fpsden;
  double volume;
  std::vector<std::string> connect;
  LTCEncoder* encoder;
  ltcsnd_sample_t* enc_buf;
  //
  uint32_t encoded_data;
  uint32_t byteCnt;
  ltcsnd_sample_t* enc_buf_;
  uint32_t lastframe;
};

ltcgen_t::ltcgen_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    jackc_transport_t("ltc."+session->name),
    fpsnum(25),
    fpsden(1),
    volume(-18),
    encoder(NULL),
    enc_buf(NULL),
    encoded_data(0),
    byteCnt(0),
    enc_buf_(enc_buf),
    lastframe(-1)
{
  GET_ATTRIBUTE_(fpsnum);
  GET_ATTRIBUTE_(fpsden);
  GET_ATTRIBUTE_(volume);
  GET_ATTRIBUTE_(connect);
  encoder = ltc_encoder_create(get_srate(),
                               fpsnum/fpsden, LTC_TV_625_50, 0);
  enc_buf = new ltcsnd_sample_t[ltc_encoder_get_buffersize(encoder)];
  memset(enc_buf,0,ltc_encoder_get_buffersize(encoder)*sizeof(ltcsnd_sample_t));
  add_output_port("ltc");
  activate();
  for(std::vector<std::string>::const_iterator it=connect.begin();it!=connect.end();++it)
    jackc_transport_t::connect(get_client_name()+":ltc",*it,true);
}

ltcgen_t::~ltcgen_t()
{
  deactivate();
  ltc_encoder_free( encoder );
}

int ltcgen_t::process(jack_nframes_t n, const std::vector<float*>& sIn, const std::vector<float*>& sOut,uint32_t tp_frame, bool tp_rolling)
{
  float smult = pow(10, volume/20.0)/(90.0);
  if( tp_frame != lastframe + n){
    double sec(tp_frame*t_sample);
    SMPTETimecode st;
    memset(st.timezone,0,6);
    st.years = 0;
    st.months = 0;
    st.days = 0;
    st.hours = (int)floor(sec/3600.0);
    st.mins  = (int)floor((sec-3600.0*floor(sec/3600.0))/60.0);
    st.secs  = (int)floor(sec)%60;
    st.frame = (int)floor((sec-floor(sec))*(double)fpsnum/(double)fpsden);
    ltc_encoder_set_timecode(encoder, &st);
  }
  lastframe = tp_frame;
  if( sOut.size() ){
    for( uint32_t k=0;k<n;++k){
      if( !encoded_data ){
        if( byteCnt >= 10 ){
          byteCnt = 0;
          if( tp_rolling )
            ltc_encoder_inc_timecode(encoder);
        }
	ltc_encoder_encode_byte(encoder, byteCnt, 1.0);
        byteCnt++;
        encoded_data = ltc_encoder_get_buffer(encoder, enc_buf);
        enc_buf_ = enc_buf;
      }
      if( encoded_data ){
        sOut[0][k] = smult*((float)(*enc_buf_) - 128.0f);
        ++enc_buf_;
        --encoded_data;
      }
    }
  }
  return 0;
}


REGISTER_MODULE(ltcgen_t);


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

