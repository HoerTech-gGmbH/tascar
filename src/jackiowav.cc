#include "jackiowav.h"

jackio_t::jackio_t(const std::string& ifname,const std::string& ofname,
		   const std::vector<std::string>& ports, const std::string& jackname,int freewheel,int autoconnect)
  : jackc_transport_t(jackname),
    sf_in(NULL),
    sf_out(NULL),
    buf_in(NULL),
    buf_out(NULL),
    pos(0),
    b_quit(false),
    start(false),
    freewheel_(freewheel),
    use_transport(false),
    startframe(0),
    nframes_total(0),
    p(ports),
    b_cb(false)
{
  if( !(sf_in = sf_open(ifname.c_str(),SFM_READ,&sf_inf_in)) )
    throw TASCAR::ErrMsg("unable to open input file \""+ifname+"\" for reading.");
  sf_inf_out = sf_inf_in;
  sf_inf_out.samplerate = get_srate();
  sf_inf_out.channels = std::max(1,(int)(p.size()) - (int)(sf_inf_in.channels));
  sf_inf_out.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT | SF_ENDIAN_FILE;
  if( autoconnect ){
    p.clear();
    for(int ch=0;ch<sf_inf_in.channels; ch++){
      char ctmp[100];
      sprintf(ctmp,"system:playback_%d",ch+1);
      p.push_back(ctmp);
    }
    p.push_back("system:capture_1");
  }
  if( ofname.size() ){
    if( !(sf_out = sf_open(ofname.c_str(),SFM_WRITE,&sf_inf_out)) )
      throw TASCAR::ErrMsg("unable to open output file");
  }
  nframes_total = sf_inf_in.frames;
  buf_in = new float[sf_inf_in.channels * sf_inf_in.frames];
  buf_out = new float[sf_inf_out.channels * sf_inf_in.frames];
  memset(buf_out,0,sizeof(float)*sf_inf_out.channels * sf_inf_in.frames);
  memset(buf_in,0,sizeof(float)*sf_inf_in.channels * sf_inf_in.frames);
  sf_readf_float(sf_in,buf_in,sf_inf_in.frames);
  char c_tmp[100];
  for(unsigned int k=0;k<(unsigned int)sf_inf_out.channels;k++){
    sprintf(c_tmp,"in_%d",k+1);
    add_input_port(c_tmp);
  }
  for(unsigned int k=0;k<(unsigned int)sf_inf_in.channels;k++){
    sprintf(c_tmp,"out_%d",k+1);
    add_output_port(c_tmp);
  }
}

jackio_t::jackio_t(double duration,const std::string& ofname,
		   const std::vector<std::string>& ports, const std::string& jackname,int freewheel,int autoconnect)
  : jackc_transport_t(jackname),
    sf_in(NULL),
    sf_out(NULL),
    buf_in(NULL),
    buf_out(NULL),
    pos(0),
    b_quit(false),
    start(false),
    freewheel_(freewheel),
    use_transport(false),
    startframe(0),
    nframes_total(std::max(1u,uint32_t(get_srate()*duration))),
    p(ports)
{
  //if( !(sf_in = sf_open(ifname.c_str(),SFM_READ,&sf_inf_in)) )
  //  throw TASCAR::ErrMsg("unable to open input file \""+ifname+"\" for reading.");
  memset(&sf_inf_out,0,sizeof(sf_inf_out));
  sf_inf_out.samplerate = get_srate();
  sf_inf_out.channels = std::max(1,(int)(p.size()));
  sf_inf_out.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT | SF_ENDIAN_FILE;
  if( autoconnect ){
    p.clear();
    p.push_back("system:capture_1");
  }
  if( ofname.size() ){
    if( !(sf_out = sf_open(ofname.c_str(),SFM_WRITE,&sf_inf_out)) )
      throw TASCAR::ErrMsg("unable to open output file");
  }
  //buf_in = new float[1];
  buf_out = new float[sf_inf_out.channels * nframes_total];
  memset(buf_out,0,sizeof(float)*sf_inf_out.channels * nframes_total);
  //memset(buf_in,0,sizeof(float)*sf_inf_in.channels * sf_inf_in.frames);
  //sf_readf_float(sf_in,buf_in,sf_inf_in.frames);
  char c_tmp[100];
  for(unsigned int k=0;k<(unsigned int)sf_inf_out.channels;k++){
    sprintf(c_tmp,"in_%d",k+1);
    add_input_port(c_tmp);
  }
}

jackio_t::~jackio_t()
{
  sf_close(sf_in);
  if( sf_out ){
    sf_writef_float(sf_out,buf_out,nframes_total);
    sf_close(sf_out);
  }
}


int jackio_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_running)
{
  b_cb = true;
  bool record(start);
  if( use_transport )
    record &= tp_running;
  //DEBUGS(record);
  //DEBUGS(tp_frame);
  //DEBUGS(tp_running);
  //DEBUGS(pos);
  //DEBUGS(nframes_total);
  for(unsigned int k=0;k<nframes;k++){
    if( record && (pos < nframes_total) ){
      if( buf_in )
        for(unsigned int ch=0;ch<outBuffer.size();ch++)
          outBuffer[ch][k] = buf_in[sf_inf_in.channels*pos+ch];
      for(unsigned int ch=0;ch<inBuffer.size();ch++)
	buf_out[sf_inf_out.channels*pos+ch] = inBuffer[ch][k];
      pos++;
    }else{
      if( pos >= nframes_total)
	b_quit = true;
      for(unsigned int ch=0;ch<outBuffer.size();ch++)
	outBuffer[ch][k] = 0;
    }
  }
  return 0;
}

void jackio_t::run()
{
  activate();
  if( !use_transport ){
    for(unsigned int k=0;k<(unsigned int)sf_inf_in.channels;k++)
      if( k<p.size())
        connect_out(k,p[k]);
  for(unsigned int k=0;k<(unsigned int)sf_inf_out.channels;k++)
    if( k+sf_inf_in.channels < p.size() )
      connect_in(k,p[k+sf_inf_in.channels]);
  }else{
    for(unsigned int k=0;k<(unsigned int)sf_inf_out.channels;k++)
      if( k < p.size() )
        connect_in(k,p[k]);
  }
  //sleep( 1 );
  if( freewheel_ )
    jack_set_freewheel( jc, 1 );
  if( use_transport ){
    tp_stop();
    tp_locate(startframe);
  }
  b_cb = false;
  while( !b_cb ){
    usleep( 5000 );
  }
  start = true;
  if( use_transport ){
    tp_start();
  }
  while( !b_quit ){
    usleep( 5000 );
  }
  if( use_transport ){
    tp_stop();
  }
  if( freewheel_ )
    jack_set_freewheel( jc, 0 );
  deactivate();
}

void jackio_t::set_transport_start(double start)
{
  use_transport = true;
  startframe = get_srate()*start;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
