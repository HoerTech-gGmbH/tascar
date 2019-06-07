#include "alsamidicc.h"
#include "glabsensorplugin.h"
#include "errorhandling.h"
#include <lsl_cpp.h>
#include <thread>


using namespace TASCAR;

class midicc_t : public sensorplugin_drawing_t, public midi_ctl_t {
public:
  midicc_t( const sensorplugin_cfg_t& cfg );
  virtual ~midicc_t();
  virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
  virtual std::string type() const;
  virtual void emit_event(int channel, int param, int value);
private:
  void send_service();
  std::string connect;
  std::vector<uint16_t> controllers_;
  std::string unknown_label;
  std::vector<double> range;
  std::vector<double> data;
  lsl::stream_outlet* outlet;
  std::thread srv;
  bool run_service;
};

midicc_t::midicc_t( const sensorplugin_cfg_t& cfg )
  : sensorplugin_drawing_t( cfg ),
    TASCAR::midi_ctl_t( "glabsensor"+cfg.modname ),
    run_service( true )
{
  if( name.empty() )
    name = cfg.modname;
  range.resize(2);
  range[0] = 0;
  range[1] = 1;
  GET_ATTRIBUTE(connect);
  GET_ATTRIBUTE(range);
  if( range.size() != 2 )
    throw ErrMsg("Range needs two entries");
  if( range[0] == range[1] )
    throw ErrMsg("Range values must differ"); 
  std::vector<std::string> controllers;
  GET_ATTRIBUTE(controllers);
  GET_ATTRIBUTE(data);
  for(uint32_t k=0;k<controllers.size();++k){
    size_t cpos(controllers[k].find("/"));
    if( cpos != std::string::npos ){
      uint32_t channel(atoi(controllers[k].substr(0,cpos-1).c_str()));
      uint32_t param(atoi(controllers[k].substr(cpos+1,controllers[k].size()-cpos-1).c_str()));
      controllers_.push_back(256*channel+param);
    }else{
      throw TASCAR::ErrMsg("Invalid controller name "+controllers[k]);
    }
  }
  if( controllers_.size() == 0 )
    throw TASCAR::ErrMsg("No controllers defined.");
  data.resize(controllers_.size());
  if( !connect.empty() ){
    connect_input(connect);
    connect_output(connect);
  }
  lsl::stream_info streaminfo(get_name(),modname,controllers_.size(),100.0,lsl::cf_double64);
  outlet = new lsl::stream_outlet(streaminfo);
  srv = std::thread(&midicc_t::send_service,this);
  start_service();
  for(uint32_t k=0;k<controllers_.size();++k){
    uint8_t v(std::max(0.0,std::min(127.0,(data[k]-range[0])/(range[1]-range[0])*127.0)));
    int channel(controllers_[k] >> 8);
    int param(controllers_[k] & 0xff);
    send_midi( channel, param, v );
  }
}

midicc_t::~midicc_t()
{
  stop_service();
  run_service = false;
  srv.join();
}

void midicc_t::send_service()
{
  while( run_service ){
    // wait for 10 ms:
    for( uint32_t k=0;k<10;++k)
      if( run_service )
        usleep( 1000 );
    if( run_service ){
      outlet->push_sample( data );
      alive();
    }
  }
}

void midicc_t::draw(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
  cr->save();
  double linewidth(2.0);
  cr->set_line_width( linewidth );
  cr->set_font_size( 6*linewidth );
  cr->set_source_rgb( 0, 0, 0 );
  double y0(height-20);
  //cr->move_to(0,y0);
  //cr->line_to(width,y0);
  //cr->move_to(0,20);
  //cr->line_to(width,20);
  //cr->stroke();
  cr->save();
  cr->set_source_rgb( 0.5, 0.5, 0.5 );
  for(uint32_t k=0;k<controllers_.size();++k){
    double x0(width*(k+0.5)/controllers_.size());
    double x1(width*(k+0.05)/controllers_.size());
    double x2(width*(k+0.95)/controllers_.size());
    cr->move_to( x1, y0 );
    cr->line_to( x1, 20 );
    cr->line_to( x2, 20 );
    cr->line_to( x2, y0 );
    cr->line_to( x1, y0 );
    cr->stroke();
    char ctmp[256];
    snprintf(ctmp,256,"%d/%d",controllers_[k] >> 8, controllers_[k] & 0xff );
    ctmp[255] = 0;
    ctext_at( cr, x0, 10, ctmp);
  }
  cr->restore();
  std::vector<double> ldata(data);
  cr->save();
  cr->set_line_width( 0.6*width/ldata.size() );
  cr->set_source_rgb( 1.0, 0.2, 0.2 );
  for(uint32_t k=0;k<ldata.size();++k){
    if( ldata[k] != HUGE_VAL ){
      double x(width*(k+0.5)/ldata.size());
      double y1(y0 - (height-40)*(ldata[k]-range[0])/(range[1]-range[0]));
      cr->move_to(x,y0);
      cr->line_to(x,y1);
      cr->stroke();
    }
  }
  cr->restore();
  cr->move_to( 10, height );
  cr->show_text( unknown_label );
  cr->restore();
}

std::string midicc_t::type() const
{
  return "midicc";
}

void midicc_t::emit_event(int channel, int param, int value)
{
  uint32_t ctl(256*channel+param);
  bool known = false;
  for(uint32_t k=0;k<controllers_.size();++k){
    if( controllers_[k] == ctl ){
      data[k] = value*(range[1]-range[0])/127.0+range[0];
      known = true;
    }
  }
  if( !known ){
    char ctmp[256];
    snprintf(ctmp,256,"%d/%d: %d",channel,param,value);
    ctmp[255] = 0;
    unknown_label = ctmp;
  }
}

REGISTER_SENSORPLUGIN(midicc_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
