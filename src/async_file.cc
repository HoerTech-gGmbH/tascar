#include "async_file.h"
#include "tascar.h"
#include <string.h>
#include <iostream>

static std::string async_file_error("");

void lprintbuf(float* buf, unsigned int n)
{
  std::cout << "N=" << n;
  for(unsigned int k=0;k<n;k++)
    std::cout << " " << k << ":" << buf[k];
  std::cout << std::endl;
}

TASCAR::looped_sndfile_t::looped_sndfile_t(const std::string& fname,uint32_t loopcnt)
  : sfile(sf_open(fname.c_str(),SFM_READ,&sf_inf)),
    loopcnt_(loopcnt),
    filepos_looped(0)
{
  if( !sfile ){
    async_file_error = "unable to open sound file '" + fname + "'.";
    throw ErrMsg(async_file_error.c_str());
  }
  if( !(sf_inf.seekable ) ){
    async_file_error = "the sound file '" + fname + "' is not seekable.";
    throw ErrMsg(async_file_error.c_str());
  }
  if( !sf_inf.frames ){
    async_file_error = "the sound file '" + fname + "' is empty.";
    throw ErrMsg(async_file_error.c_str());
  }
}

TASCAR::looped_sndfile_t::~looped_sndfile_t()
{
  sf_close(sfile);
}

uint32_t TASCAR::looped_sndfile_t::readf_float( float* buf, uint32_t frames )
{
  // check how many frames we should read due to looping end:
  uint32_t frames_to_read = frames;
  if( loopcnt_ > 0 )
    frames_to_read = std::min( (uint32_t)(loopcnt_ * sf_inf.frames - filepos_looped), frames );
  // now read from file until all frames are read, rewind if coming
  // across file end:
  uint32_t rframes = 0;
  while( rframes < frames_to_read ){
    uint32_t requested_frames = frames_to_read - rframes;
    uint32_t loc_rframes = sf_readf_float( sfile, &(buf[rframes*sf_inf.channels]), requested_frames );
    if( loc_rframes < requested_frames ){
      // end reached, start from beginning:
      sf_seek( sfile, 0, SEEK_SET );
    }
    rframes += loc_rframes;
  }
  filepos_looped += frames_to_read;
  return frames_to_read;
}

uint32_t TASCAR::looped_sndfile_t::seekf( uint32_t frame )
{
  if( (loopcnt_ > 0) && (frame >= loopcnt_ * sf_inf.frames ) ){
    // seeking beyond the end, return virtual end of file:
    sf_seek( sfile, sf_inf.frames, SEEK_SET );
    //filepos_real = sf_inf.frames;
    return (filepos_looped = loopcnt_ * sf_inf.frames);
  }
  uint32_t new_real_pos = frame % sf_inf.frames;
  sf_seek( sfile, new_real_pos, SEEK_SET );
  //filepos_real = new_real_pos;
  return (filepos_looped = frame);
}

uint32_t TASCAR::looped_sndfile_t::get_loopedframes() const
{
  if( loopcnt_ ) return loopcnt_ * sf_inf.frames;
  return (uint32_t)(-1);
}


TASCAR::inftime_looped_sndfile_t::inftime_looped_sndfile_t(const std::string& fname,uint32_t loopcnt)
  : looped_sndfile_t(fname,loopcnt),virtual_filepos(0)
{
}

uint32_t TASCAR::inftime_looped_sndfile_t::readf_float( float* buf, uint32_t frames )
{
  if( (virtual_filepos + (int32_t)frames < 0) || ((virtual_filepos>0)&&((uint32_t)(virtual_filepos) > get_loopedframes() )) ){
    // requested data is not overlapping with the file:
    memset(buf,0,frames*sf_inf.channels*sizeof(float));
  }else{
    if( virtual_filepos >= 0 ){
      // data can be read from beginning of buf:
      uint32_t rcnt = TASCAR::looped_sndfile_t::readf_float( buf, frames );
      if( rcnt < frames )
        memset( &(buf[rcnt*sf_inf.channels]),0,(frames-rcnt)*sf_inf.channels*sizeof(float));
    }else{
      // fill beginning of buf with zeros, then read from file:
      uint32_t frames_to_read(virtual_filepos+(int32_t)frames);
      uint32_t offset(frames-frames_to_read);
      memset(buf,0,offset*sf_inf.channels*sizeof(float));
      float* nbuf(&(buf[offset*sf_inf.channels]));
      uint32_t rcnt = TASCAR::looped_sndfile_t::readf_float( nbuf, frames_to_read );
      if( rcnt < frames_to_read )
        memset( &(nbuf[rcnt*sf_inf.channels]),0,(frames_to_read-rcnt)*sf_inf.channels*sizeof(float));
    }
  }
  virtual_filepos += frames;
  return frames;
}

int32_t TASCAR::inftime_looped_sndfile_t::seekf_inf( int32_t frame )
{
  seekf( std::max(0,frame) );
  return (virtual_filepos=frame);
}

TASCAR::async_sndfile_t::async_sndfile_t(uint32_t numchannels,uint32_t buffer_length,uint32_t fragsize)
  : service_running(false),
    run_service(true),
    numchannels_(numchannels),
    buffer_length_(buffer_length),
    fragsize_(fragsize),
    rb(buffer_length,numchannels),
    requested_startframe(0),
    need_data(false),
    sfile(NULL),
    file_firstchannel(0),
    file_buffer(NULL),
    read_fragment_buf(new float[numchannels*fragsize]),
    disk_fragment_buf(new float[numchannels*fragsize]),
    file_frames(1),
    file_channels(1),
    gain_(1.0),
    xrun(0),
    min_read_chunk(std::min(8192u,buffer_length_>>1))
{
  pthread_mutex_init( &mtx_file, NULL );
}

TASCAR::async_sndfile_t::async_sndfile_t( const async_sndfile_t& src)
  : service_running(false),
    run_service(true),
    numchannels_(src.numchannels_),
    buffer_length_(src.buffer_length_),
    fragsize_(src.fragsize_),
    rb(src.buffer_length_,src.numchannels_),
    requested_startframe(0),
    need_data(false),
    sfile(NULL),
    file_firstchannel(0),
    file_buffer(NULL),
    read_fragment_buf(new float[numchannels_*fragsize_]),
    disk_fragment_buf(new float[numchannels_*fragsize_]),
    file_frames(1),
    file_channels(1),
    gain_(1.0),
    xrun(0),
    min_read_chunk(std::min(8192u,buffer_length_>>1))
{
  pthread_mutex_init( &mtx_file, NULL );
}

void TASCAR::async_sndfile_t::start_service()
{
  if( !service_running){
    run_service = true;
    int err = pthread_create( &srv_thread, NULL, &TASCAR::async_sndfile_t::service, this);
    if( err < 0 )
      throw ErrMsg("pthread_create failed");
    service_running = true;
  }
}

void TASCAR::async_sndfile_t::stop_service()
{
  if( service_running){
    run_service = false;
    // first terminate disk thread:
    pthread_join( srv_thread, NULL );
    // then clean-up the mutexes:
    pthread_mutex_trylock( &mtx_file );
    pthread_mutex_unlock( &mtx_file);
    service_running = false;
  }
}

void * TASCAR::async_sndfile_t::service(void* h)
{
  ((TASCAR::async_sndfile_t*)h)->service();
  return NULL;
}

void TASCAR::async_sndfile_t::service()
{
  while( run_service ){
    usleep( 500 );
    if( rb.relocation_requested() ){
      rb.lock_relocate();
      pthread_mutex_lock( &mtx_file);
      if( sfile ){
        int32_t requested_location(rb.get_requested_location());
        sfile->seekf_inf( requested_location - file_firstframe );
      }
      pthread_mutex_unlock( &mtx_file);
      rb.unlock_relocate();
    }
    if( rb.write_space() >= min_read_chunk ){
      pthread_mutex_lock( &mtx_file);
      if( sfile ){
        uint32_t rcnt = sfile->readf_float( file_buffer, std::min( fragsize_, rb.write_space()) );
        for( unsigned int ch=0;ch<numchannels_;ch++)
          for( unsigned int k=0;k<rcnt;k++)
            disk_fragment_buf[numchannels_*k+ch] = gain_*file_buffer[file_channels*k+(ch+file_firstchannel)];
        //DEBUG(file_buffer[0]);
        //DEBUG(file_buffer[1]);
        rb.write( disk_fragment_buf, rcnt );
      }else{
        rb.write_zeros( rb.write_space() );
      }
      pthread_mutex_unlock( &mtx_file);
    }
  }
  //DEBUG("service2");
}

TASCAR::async_sndfile_t::~async_sndfile_t()
{
  stop_service();
  pthread_mutex_destroy( &mtx_file );
  if( sfile ){
    delete  sfile;
    sfile = NULL;
  }
  if( file_buffer ){
    delete [] file_buffer;
    file_buffer = NULL;
  }
  delete [] read_fragment_buf;
  delete [] disk_fragment_buf;
  //DEBUGMSG("destructor done");
}

void TASCAR::async_sndfile_t::request_data( int32_t firstframe, uint32_t n, uint32_t channels, float** buf )
{
  if( channels != numchannels_ )
    throw ErrMsg("request_data channel count mismatch");
  if( n > fragsize_ )
    throw ErrMsg("requested number of frames is larger than fragsize");
  // do we have to relocate file?
  int32_t rbpos(rb.get_current_location());
  if( rbpos != firstframe ){
    if( (firstframe < rbpos) || (firstframe+n > rbpos+rb.read_space()) ){
      rb.set_locate( firstframe );
    }else{
      // firstframe is > rbpos and firstframe is within filled buffer:
      rb.read_skip( firstframe-rbpos );
    }
  }
  int32_t current_pos(0);
  uint32_t rframes = rb.read( read_fragment_buf, n, &current_pos );
  if( (current_pos != firstframe) || (rframes<n) )
    xrun++;
  if( current_pos == firstframe ){
    // copy and de-interlace buffer:
    for( unsigned int ch=0; ch<channels; ch++ ){
      for( unsigned int k=0; k<rframes; k++ )
        buf[ch][k] += read_fragment_buf[k*channels+ch];
    }
  }
}

void TASCAR::async_sndfile_t::open(const std::string& fname, uint32_t firstchannel, int32_t first_frame, double gain, uint32_t loop)
{
  //DEBUG(fname);
  //DEBUG(firstchannel);
  //DEBUG(first_frame);
  //DEBUG(gain);
  //DEBUG(loop);
  if( pthread_mutex_lock( &mtx_file ) != 0 )
    return;
  if( sfile ){
    delete sfile;
    sfile = NULL;
  }
  if( file_buffer ){
    delete [] file_buffer;
    file_buffer = NULL;
  }
  gain_ = gain;
  sfile = new inftime_looped_sndfile_t(fname, loop );
  file_frames = sfile->get_frames();
  file_channels = sfile->get_channels();
  if( file_channels < numchannels_+firstchannel ){
    delete sfile;
    sfile = NULL;
    pthread_mutex_unlock( &mtx_file );
    throw ErrMsg("the input sound file does not provide sufficient number of channels");
  }
  file_firstchannel = std::min(firstchannel, file_channels-numchannels_);
  file_buffer = new float[fragsize_ * file_channels];
  file_firstframe = first_frame;
  pthread_mutex_unlock( &mtx_file );
}

TASCAR::ringbuffer_t::ringbuffer_t(uint32_t size,uint32_t channels_)
  : data(new float[size*channels_]),
    pos(size),
    channels(channels_)
{
  //DEBUG(this);
  //DEBUG(data);
  reset();
  pthread_mutex_init( &mtx_read_access, NULL );
  pthread_mutex_init( &mtx_write_access, NULL );
}
 
TASCAR::ringbuffer_t::~ringbuffer_t()
{
  //DEBUG(this);
  //DEBUG(data);
  pthread_mutex_trylock( &mtx_write_access );
  pthread_mutex_unlock( &mtx_write_access);
  pthread_mutex_destroy( &mtx_write_access );
  pthread_mutex_trylock( &mtx_read_access );
  pthread_mutex_unlock( &mtx_read_access);
  pthread_mutex_destroy( &mtx_read_access );
  delete [] data;
}
uint32_t TASCAR::ringbuffer_t::pos_t::rspace()
{
  if( w >= r ) return w-r;
  return w+l-r;
}

uint32_t TASCAR::ringbuffer_t::pos_t::wspace()
{
  if( r > w ) return r-w-1;
  return r+l-w-1;
}

uint32_t TASCAR::ringbuffer_t::read_space()
{
  if( pthread_mutex_trylock( &mtx_read_access ) == 0 ){
    uint32_t retv(get_pos().rspace());
    pthread_mutex_unlock( &mtx_read_access );
    return retv;
  }
  return 0;
}

uint32_t TASCAR::ringbuffer_t::write_space()
{
  if( pthread_mutex_trylock( &mtx_write_access ) == 0 ){
    uint32_t retv(get_pos().wspace());
    pthread_mutex_unlock( &mtx_write_access );
    return retv;
  }
  return 0;
}

uint32_t TASCAR::ringbuffer_t::read( float* buf, uint32_t frames, int32_t* current_loc )
{
  if( current_loc )
    *current_loc = INVALID_LOCATION;
  if( pthread_mutex_trylock( &mtx_read_access ) == 0 ){
    //printbuf(data,pos.l);
    //DEBUG(frames);
    //DEBUG(current_location);
    if( current_loc )
      *current_loc = current_location;
    pos_t tmp_p(get_pos());
    uint32_t frames_to_read = std::min(frames,tmp_p.rspace());
    uint32_t r1(std::min(tmp_p.r+frames_to_read,tmp_p.l)-tmp_p.r);
    tmp_p.r += frames_to_read;
    //DEBUG(r1);
    if( r1 && buf )
      memcpy( buf, &(data[pos.r*channels]), r1*sizeof(float)*channels);
    if( r1 < frames_to_read ){
      if( buf )
        memcpy( &(buf[r1*channels]), data, (frames_to_read-r1)*sizeof(float)*channels);
      tmp_p.r = (frames_to_read-r1);
    }
    //printbuf(buf,frames_to_read);
    pos.r = tmp_p.r;
    current_location += frames_to_read;
    pthread_mutex_unlock( &mtx_read_access );
    return frames_to_read;
  }
  return 0;
}

uint32_t TASCAR::ringbuffer_t::read_skip( uint32_t frames, int32_t* current_loc )
{
  return read( NULL, frames, current_loc );
}

uint32_t TASCAR::ringbuffer_t::write( float* buf, uint32_t frames )
{
  //DEBUGMSG("");
  if( pthread_mutex_trylock( &mtx_write_access ) == 0 ){
    //DEBUG(frames);
    //DEBUG(pos.w);
    pos_t tmp_p(get_pos());
    uint32_t frames_to_write = std::min(frames,tmp_p.wspace());
    uint32_t r1(std::min(tmp_p.w+frames_to_write,tmp_p.l)-tmp_p.w);
    tmp_p.w += frames_to_write;
    //DEBUG(r1);
    //printbuf(buf,frames);
    if( r1 ){
      if( buf )
        memcpy( &(data[pos.w*channels]), buf, r1*sizeof(float)*channels);
      else
        memset( &(data[pos.w*channels]), 0, r1*sizeof(float)*channels);
    }
    if( r1 < frames_to_write ){
      if( buf )
        memcpy( data, &(buf[r1*channels]), (frames_to_write-r1)*sizeof(float)*channels);
      else
        memset( data, 0, (frames_to_write-r1)*sizeof(float)*channels);
      tmp_p.w = (frames_to_write-r1);
    }
    //printbuf(data,pos.l);
    pos.w = tmp_p.w;
    //DEBUG(pos.w);
    pthread_mutex_unlock( &mtx_write_access );
    return frames_to_write;
  }
  return 0;
}

uint32_t TASCAR::ringbuffer_t::write_zeros( uint32_t frames )
{
  return write( NULL, frames );
}

void TASCAR::ringbuffer_t::reset()
{
  //DEBUG(this);
  pos.r = 0;
  pos.w = 0;
  current_location = 0;
  requested_location = INVALID_LOCATION;
}

void TASCAR::ringbuffer_t::lock_relocate()
{
  pthread_mutex_lock( &mtx_read_access );
  pthread_mutex_lock( &mtx_write_access );
}

void TASCAR::ringbuffer_t::unlock_relocate()
{
  //DEBUG(requested_location);
  uint32_t new_location(requested_location);
  reset();
  current_location = new_location;
  pthread_mutex_unlock( &mtx_read_access );
  pthread_mutex_unlock( &mtx_write_access );
}

void TASCAR::ringbuffer_t::set_locate(int32_t l)
{
  requested_location = l;
  //DEBUG(requested_location);
}

// set_locate(l) => request_location = l;
// 

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
