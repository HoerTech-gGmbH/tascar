#include "jackiowav.h"

jackio_t::jackio_t(const std::string& ifname,const std::string& ofname,
		   const std::vector<std::string>& ports, const std::string& jackname,int freewheel,int autoconnect,bool verbose)
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
    b_cb(false),
    b_verbose(verbose), wait_(false), cpuload(0), xruns(0)
{
  memset(&sf_inf_in,0,sizeof(sf_inf_in));
  memset(&sf_inf_out,0,sizeof(sf_inf_out));
  log("opening input file "+ifname);
  if( !(sf_in = sf_open(ifname.c_str(),SFM_READ,&sf_inf_in)) )
    throw TASCAR::ErrMsg("unable to open input file \""+ifname+"\" for reading.");
  sf_inf_out = sf_inf_in;
  sf_inf_out.samplerate = get_srate();
  sf_inf_out.channels = std::max(0,(int)(p.size()) - (int)(sf_inf_in.channels));
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
    log("creating output file "+ofname);
    if( !(sf_out = sf_open(ofname.c_str(),SFM_WRITE,&sf_inf_out)) ){
      std::string errmsg("Unable to open output file \""+ofname+"\": ");
      errmsg += sf_strerror(NULL);
      throw TASCAR::ErrMsg(errmsg);
    }
  }
  char c_tmp[1024];
  nframes_total = sf_inf_in.frames;
  sprintf(c_tmp,"%d",nframes_total);
  log("allocating memory for "+std::string(c_tmp)+" audio frames");
  buf_in = new float[std::max((sf_count_t)1,sf_inf_in.channels * sf_inf_in.frames)];
  buf_out = new float[std::max((sf_count_t)1,sf_inf_out.channels * sf_inf_in.frames)];
  memset(buf_out,0,sizeof(float)*sf_inf_out.channels * sf_inf_in.frames);
  memset(buf_in,0,sizeof(float)*sf_inf_in.channels * sf_inf_in.frames);
  log("reading input file into memory");
  sf_readf_float(sf_in,buf_in,sf_inf_in.frames);
  for(unsigned int k=0;k<(unsigned int)sf_inf_out.channels;k++){
    sprintf(c_tmp,"in_%d",k+1);
    log("adding input port "+std::string(c_tmp));
    add_input_port(c_tmp);
  }
  for(unsigned int k=0;k<(unsigned int)sf_inf_in.channels;k++){
    sprintf(c_tmp,"out_%d",k+1);
    log("adding output port "+std::string(c_tmp));
    add_output_port(c_tmp);
  }
}

jackio_t::jackio_t(double duration,const std::string& ofname,
		   const std::vector<std::string>& ports, const std::string& jackname,int freewheel,int autoconnect,bool verbose)
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
    p(ports),
    b_cb(false),
    b_verbose(verbose), wait_(false), cpuload(0), xruns(0)
{
  memset(&sf_inf_in,0,sizeof(sf_inf_in));
  memset(&sf_inf_out,0,sizeof(sf_inf_out));
  sf_inf_out.samplerate = get_srate();
  sf_inf_out.channels = std::max(1,(int)(p.size()));
  sf_inf_out.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT | SF_ENDIAN_FILE;
  if( autoconnect ){
    p.clear();
    p.push_back("system:capture_1");
  }
  if( ofname.size() ){
    log("creating output file "+ofname);
    if( !(sf_out = sf_open(ofname.c_str(),SFM_WRITE,&sf_inf_out)) ){
      std::string errmsg("Unable to open output file \""+ofname+"\": ");
      errmsg += sf_strerror(NULL);
      throw TASCAR::ErrMsg(errmsg);
    }
  }
  char c_tmp[1024];
  sprintf(c_tmp,"%d",nframes_total);
  log("allocating memory for "+std::string(c_tmp)+" audio frames");
  buf_out = new float[sf_inf_out.channels * nframes_total];
  memset(buf_out,0,sizeof(float)*sf_inf_out.channels * nframes_total);
  for(unsigned int k=0;k<(unsigned int)sf_inf_out.channels;k++){
    sprintf(c_tmp,"in_%d",k+1);
    log("add input port "+std::string(c_tmp));
    add_input_port(c_tmp);
  }
}

jackio_t::~jackio_t()
{
  log("cleaning up file handles");
  if( sf_in ){
    sf_close(sf_in);
  }
  if( sf_out ){
    sf_writef_float(sf_out,buf_out,nframes_total);
    sf_close(sf_out);
  }
  log("deallocating memory");
  if( buf_in )
    delete [] buf_in;
  if( buf_out )
    delete [] buf_out;
}


int jackio_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_rolling)
{
  b_cb = true;
  bool record(start);
  if( use_transport )
    record &= tp_rolling;
  if( wait_ )
    record &= tp_frame >= startframe;
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

void jackio_t::log(const std::string& msg)
{
  if( b_verbose )
    std::cerr << msg << std::endl;
}

void jackio_t::run()
{
  log("activating jack client");
  activate();
  for(unsigned int k=0;k<(unsigned int)sf_inf_in.channels;k++)
    if( k<p.size()){
      log("connecting output port to "+p[k]);
      connect_out(k,p[k]);
    }
  for(unsigned int k=0;k<(unsigned int)sf_inf_out.channels;k++)
    if( k+sf_inf_in.channels < p.size() ){
      log("connecting input port to "+p[k+sf_inf_in.channels]);
      connect_in(k,p[k+sf_inf_in.channels],false,true);
    }
  if( freewheel_ ){
    log("switching to freewheeling mode");
    jack_set_freewheel( jc, 1 );
  }
  if( use_transport && (!wait_) ){
    log("locating to startframe");
    tp_stop();
    tp_locate(startframe);
  }
  b_cb = false;
  while( !b_cb ){
    usleep( 5000 );
  }
  start = true;
  if( use_transport && (!wait_) ){
    log("starting transport");
    tp_start();
  }
  log("waiting for data to complete");
  while( !b_quit ){
    usleep( 5000 );
  }
  cpuload = get_cpu_load();
  xruns = get_xruns();
  if( use_transport && (!wait_) ){
    log("stopping transport");
    tp_stop();
  }
  if( freewheel_ ){
    log("deactivating freewheeling mode");
    jack_set_freewheel( jc, 0 );
  }
  log("deactivating jack client");
  deactivate();
}

void jackio_t::set_transport_start(double start_, bool wait)
{
  use_transport = true;
  wait_ = wait;
  startframe = get_srate()*start_;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
