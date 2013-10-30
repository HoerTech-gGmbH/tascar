/**
   \file tascar_jackio.cc
   \ingroup apptascar
   \brief simultaneously play a sound file and record from jack
   \author Giso Grimm
   \date 2012

   \section license License (GPL)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; version 2 of the
   License.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "tascar.h"
#include <getopt.h>
#include "errorhandling.h"
#include <unistd.h>

/**
\ingroup apptascar
*/
class jackio_t : public jackc_t {
public:
  /**
     \param ifname Input file name
     \param ofname Output file name
     \param ports Output and Input ports (the first N ports are assumed to be output ports, N = number of channels in input file)
     \param freewheel Optionally use freewheeling mode
   */
  jackio_t(const std::string& ifname,const std::string& ofname,
	   const std::vector<std::string>& ports,const std::string& jackname = "jackio",int freewheel = 0,int autoconnect = 0);
  ~jackio_t();
  /**
     \brief start processing
   */
  void run();
private:
  SNDFILE* sf_in;
  SNDFILE* sf_out;
  SF_INFO sf_inf_in;
  SF_INFO sf_inf_out;
  float* buf_in;
  float* buf_out;
  unsigned int pos;
  bool b_quit;
  bool start;
  bool freewheel_;
  std::vector<std::string> p;
  int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer);
};

jackio_t::jackio_t(const std::string& ifname,const std::string& ofname,
		   const std::vector<std::string>& ports, const std::string& jackname,int freewheel,int autoconnect)
  : jackc_t(jackname),
    sf_in(NULL),
    sf_out(NULL),
    buf_in(NULL),
    buf_out(NULL),
    pos(0),
    b_quit(false),
    start(false),
    freewheel_(freewheel),
    p(ports)
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

jackio_t::~jackio_t()
{
  sf_close(sf_in);
  if( sf_out ){
    sf_writef_float(sf_out,buf_out,sf_inf_in.frames);
    sf_close(sf_out);
  }
}


int jackio_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer)
{
  for(unsigned int k=0;k<nframes;k++){
    if( start && (pos < sf_inf_in.frames) ){
      for(unsigned int ch=0;ch<outBuffer.size();ch++)
	outBuffer[ch][k] = buf_in[sf_inf_in.channels*pos+ch];
      for(unsigned int ch=0;ch<inBuffer.size();ch++)
	buf_out[sf_inf_out.channels*pos+ch] = inBuffer[ch][k];
      pos++;
    }else{
      if( pos >= sf_inf_in.frames)
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
  for(unsigned int k=0;k<(unsigned int)sf_inf_in.channels;k++)
    if( k<p.size())
      connect_out(k,p[k]);
  for(unsigned int k=0;k<(unsigned int)sf_inf_out.channels;k++)
    if( k+sf_inf_in.channels < p.size() )
      connect_in(k,p[k+sf_inf_in.channels]);
  //sleep( 1 );
  if( freewheel_ )
    jack_set_freewheel( jc, 1 );
  start = true;
  while( !b_quit ){
    usleep( 5000 );
  }
  if( freewheel_ )
    jack_set_freewheel( jc, 0 );
  deactivate();
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_jackio [options] input.wav [ ports [...]]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}


int main(int argc, char** argv)
{
  try{
    const char *options = "fo:chu";
    struct option long_options[] = { 
      { "freewheeling", 0, 0, 'f' },
      { "output-file",  1, 0, 'o' },
      { "jack-name",    1, 0, 'n' },
      { "autoconnect",  0, 0, 'c' },
      { "unlink",       0, 0, 'u' },
      { "help",         0, 0, 'h' },
      { 0, 0, 0, 0 }
    };
    int opt(0);
    int option_index(0);
    bool b_unlink(false);
    std::string ifname;
    std::string ofname;
    std::string jackname("tascar_jackio");
    int freewheel(0);
    int autoconnect(0);
    std::vector<std::string> ports;
    while( (opt = getopt_long(argc, argv, options,
                              long_options, &option_index)) != EOF){
      switch(opt){
      case 'f':
        freewheel = 1;
        break;
      case 'c':
        autoconnect = 1;
        break;
      case 'u':
        b_unlink = true;
        break;
      case 'o':
        ofname = optarg;
        break;
      case 'n':
        jackname = optarg;
        break;
      case 'h':
        usage(long_options);
        return -1;
      }
    }
    if( optind < argc )
      ifname = argv[optind++];
    while( optind < argc ){
      ports.push_back( argv[optind++] );
    }
    if( ifname.size() ){
      jackio_t jio(ifname,ofname,ports,jackname,freewheel,autoconnect);
      jio.run();
      if( b_unlink )
        unlink(ifname.c_str());
    }
  }
  catch( const std::exception& e ){
    std::cerr << e.what() << std::endl;
    return 1;
  }
  catch( const char* e ){
    std::cerr << e << std::endl;
    return 1;
  }
  return 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
