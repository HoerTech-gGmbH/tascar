#include "audioplugin.h"
#include "delayline.h"

class feedbackdelay_t : public TASCAR::audioplugin_base_t {
public:
  feedbackdelay_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  void add_variables( TASCAR::osc_server_t* srv );
  ~feedbackdelay_t();
private:
  size_t maxdelay;
  double f;
  double feedback;
  TASCAR::varidelay_t* dl;
};

feedbackdelay_t::feedbackdelay_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    maxdelay(44100),
    f(1000),
    feedback(0.5),
    dl(NULL)
{
  GET_ATTRIBUTE(maxdelay);
  GET_ATTRIBUTE(f);
  GET_ATTRIBUTE(feedback);
  dl = new TASCAR::varidelay_t(maxdelay, 1.0, 1.0, 0, 1);
}

feedbackdelay_t::~feedbackdelay_t()
{
  delete dl;
}

void feedbackdelay_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double("/f",&f);
  srv->add_double("/feedback",&feedback);
}

void feedbackdelay_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp)
{
  // sample delay; subtract 1 to account for order of read/write:
  double d(f_sample/f-1.0);
  // operate only on first channel:
  float* vsigbegin(chunk[0].d);
  float* vsigend(vsigbegin+chunk[0].n);
  for(float* v=vsigbegin;v!=vsigend;++v){
    float v_out(dl->get(d));
    dl->push(v_out*feedback + *v);
    *v = v_out;
  }
}

REGISTER_AUDIOPLUGIN(feedbackdelay_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
