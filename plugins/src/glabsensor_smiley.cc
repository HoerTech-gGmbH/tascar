#include "glabsensorplugin.h"

using namespace TASCAR;

class smiley_t : public sensorplugin_drawing_t {
public:
  smiley_t( const TASCAR::sensorplugin_cfg_t& cfg );
  void add_variables( TASCAR::osc_server_t* srv );
  void draw(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
private:
  float happiness;
  bool hide;
};

smiley_t::smiley_t( const sensorplugin_cfg_t& cfg )
  : sensorplugin_drawing_t(cfg),
    happiness(1.0),
    hide(false)
{
}

void smiley_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_float("/happy",&happiness);
  srv->add_bool("/hide",&hide);
}

void smiley_t::draw(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
  if( !hide ){
    if( happiness < -0.8 )
      add_critical( "!$&!//&!" );
    else if( happiness < 0.1 )
      add_warning( "I am not happy." );
    float s(std::min(width,height));
    cr->arc(0.5*width, 0.5*height, 0.2*s, 0.0, 2 * M_PI);
    cr->stroke();
    cr->arc(0.5*width-0.1*s, 0.5*height-0.05*s, 0.02*s, 0.0, 2 * M_PI);
    cr->arc(0.5*width+0.1*s, 0.5*height-0.05*s, 0.02*s, 0.0, 2 * M_PI);
    cr->fill();
    cr->move_to(0.5*width-0.1*s, 0.5*height+0.1*s-0.05*s*happiness);
    for(uint32_t k=0;k<32;++k){
      float x(((float)k-15.0)/16.0);
      cr->line_to(0.5*width+x*0.1*s,0.5*height+0.1*s-0.05*s*happiness*x*x);
    }
    cr->stroke();
    alive();
  }
}

REGISTER_SENSORPLUGIN(smiley_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
