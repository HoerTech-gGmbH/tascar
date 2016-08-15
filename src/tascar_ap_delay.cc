#include "audioplugin.h"

class delay_t : public TASCAR::audioplugin_base_t {
public:
  delay_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname);
  void process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos);
  void prepare(double srate,uint32_t fragsize);
  void release();
  ~delay_t();
private:
  double delay;
  uint32_t idelay;
  TASCAR::wave_t* dline;
  uint32_t pos;
};

delay_t::delay_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname)
  : audioplugin_base_t(xmlsrc,name,parentname),
    delay(1),
    dline(NULL),
    pos(0)
{
  GET_ATTRIBUTE(delay);
}

void delay_t::prepare(double srate,uint32_t fragsize)
{
  dline = new TASCAR::wave_t(srate*delay);
  pos = 0;
}

void delay_t::release()
{
  delete dline;
  dline = NULL;
}

delay_t::~delay_t()
{
}

void delay_t::process(TASCAR::wave_t& chunk, const TASCAR::pos_t& p0)
{
  if( dline ){
    for(uint32_t k=0;k<chunk.n;++k){
      float v(dline->d[pos]);
      dline->d[pos] = chunk[k];
      chunk[k] = v;
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
