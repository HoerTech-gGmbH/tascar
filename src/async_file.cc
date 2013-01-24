#include "async_file.h"
#include "tascar.h"
#include <string.h>
#include <iostream>

static std::string async_file_error("");

TASCAR::chunk_t::chunk_t(uint32_t chunksize, uint32_t numchannels)
  : firstframe(-1),
    len(chunksize),
    channels(numchannels),
    data(new float[std::max(chunksize*channels,1u)])
{
  //DEBUG(this);
  //DEBUG(data);
}

TASCAR::chunk_t::~chunk_t()
{
  //DEBUG(this);
  //DEBUG(data);
  delete [] data;
}

TASCAR::looped_sndfile_t::looped_sndfile_t(const std::string& fname,uint32_t loopcnt)
  : sfile(sf_open(fname.c_str(),SFM_READ,&sf_inf)),
    loopcnt_(loopcnt),
    filepos_looped(0)
{
  if( !sfile ){
    async_file_error = "unable to open sound file '" + fname + "'.";
    throw async_file_error.c_str();
  }
  if( !(sf_inf.seekable ) ){
    async_file_error = "the sound file '" + fname + "' is not seekable.";
    throw async_file_error.c_str();
  }
  if( !sf_inf.frames ){
    async_file_error = "the sound file '" + fname + "' is empty.";
    throw async_file_error.c_str();
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

TASCAR::async_sndfile_t::async_sndfile_t(uint32_t numchannels,uint32_t chunksize)
  : service_running(false),
    run_service(true),
    numchannels_(numchannels),
    chunksize_(chunksize),
    ch1(chunksize,numchannels),
    ch2(chunksize,numchannels),
    current(&ch1),
    next(&ch2),
    requested_startframe(0),
    need_data(false),
    sfile(NULL),
    file_firstchannel(0),
    file_buffer(NULL),
    file_frames(1),
    file_channels(1),
    gain_(1.0)
{
  pthread_mutex_init( &mtx_readrequest, NULL );
  pthread_mutex_init( &mtx_next, NULL );
  pthread_mutex_init( &mtx_wakeup, NULL );
  pthread_mutex_init( &mtx_file, NULL );
  //DEBUG(service_running);
  //DEBUG(ch1.data);
  //DEBUG(ch2.data);
  //DEBUGMSG("explicit");
}

TASCAR::async_sndfile_t::async_sndfile_t( const async_sndfile_t& src)
  : service_running(false),
    run_service(true),
    numchannels_(src.numchannels_),
    chunksize_(src.chunksize_),
    ch1(chunksize_,numchannels_),
    ch2(chunksize_,numchannels_),
    current(&ch1),
    next(&ch2),
    requested_startframe(0),
    need_data(false),
    sfile(NULL),
    file_firstchannel(0),
    file_buffer(NULL),
    file_frames(1),
    file_channels(1),
    gain_(1.0)
{
  pthread_mutex_init( &mtx_readrequest, NULL );
  pthread_mutex_init( &mtx_next, NULL );
  pthread_mutex_init( &mtx_wakeup, NULL );
  pthread_mutex_init( &mtx_file, NULL );
  //DEBUG(src.service_running);
  //DEBUG(ch1.data);
  //DEBUG(ch2.data);
  //DEBUGMSG("copy");
}

void TASCAR::async_sndfile_t::start_service()
{
  //DEBUG(1);
  if( !service_running){
    //DEBUG(1);
    run_service = true;
    int err = pthread_create( &srv_thread, NULL, &TASCAR::async_sndfile_t::service, this);
    if( err < 0 )
      throw "pthread_create failed";
    service_running = true;
  }
}

void TASCAR::async_sndfile_t::stop_service()
{
  if( service_running){
    //DEBUG(1);
    run_service = false;
    // first terminate disk thread:
    pthread_mutex_trylock( &mtx_wakeup );
    pthread_mutex_unlock( &mtx_wakeup);
    pthread_join( srv_thread, NULL );
    // then clean-up the mutexes:
    pthread_mutex_trylock( &mtx_readrequest );
    pthread_mutex_unlock( &mtx_readrequest);
    //
    pthread_mutex_trylock( &mtx_next );
    pthread_mutex_unlock( &mtx_next);
    //
    pthread_mutex_trylock( &mtx_file );
    pthread_mutex_unlock( &mtx_file);
    service_running = false;
    //DEBUG(1);
  }
}

void * TASCAR::async_sndfile_t::service(void* h)
{
  ((TASCAR::async_sndfile_t*)h)->service();
  return NULL;
}

void TASCAR::async_sndfile_t::service()
{
  //DEBUG("service1");
  while( run_service ){
    pthread_mutex_lock( &mtx_wakeup );
    if( run_service ){
      pthread_mutex_lock( &mtx_readrequest );
      bool l_need_data(need_data);
      uint32_t l_requested_startframe(requested_startframe);
      pthread_mutex_unlock( &mtx_readrequest );
      if( l_need_data ){
        //DEBUG("pre_slave");
        slave_read_file( l_requested_startframe );
        //DEBUG("post_slave");
      }
    }
  }
  //DEBUG("service2");
}

TASCAR::async_sndfile_t::~async_sndfile_t()
{
  //DEBUG(1);
  stop_service();
  //DEBUG(1);
  // then clean-up the mutexes:
  pthread_mutex_destroy( &mtx_readrequest );
  pthread_mutex_destroy( &mtx_next );
  pthread_mutex_destroy( &mtx_wakeup );
  //
  pthread_mutex_destroy( &mtx_file );
  //DEBUG(1);
  if( sfile ){
    delete  sfile;
    sfile = NULL;
  }
  if( file_buffer ){
    delete [] file_buffer;
    file_buffer = NULL;
  }
  //DEBUGMSG("destructor done");
}

void TASCAR::async_sndfile_t::request_data( uint32_t firstframe, uint32_t n, uint32_t channels, float** buf )
{
  if( channels != numchannels_ ){
    //DEBUG(channels);
    //DEBUG(numchannels_);
    throw "request_data channel count mismatch";
  }
  //DEBUG(firstframe);
  uint32_t next_request(current->firstframe+current->len);
  // flag to indicate wether the buffer can be used in the next request
  bool buffer_used_up(false);
  // if the requested data is not available then try to swap buffers
  if( (firstframe < current->firstframe) || (firstframe >= current->firstframe+current->len) ){
    if( pthread_mutex_trylock( &mtx_next ) == 0 ){
      //DEBUGMSG("swapping_buffers");
      chunk_t* newc(current);
      current = next;
      next = newc;
      next->firstframe = -1;
      pthread_mutex_unlock( &mtx_next );
      //DEBUGMSG("swapping_buffers_done");
    }else{
      //DEBUGMSG("swapping_buffers_failed");
    }
  }
  // is the requested data (at least partially) available?
  if( (firstframe >= current->firstframe) && (firstframe < current->firstframe+current->len) ){
    // copy all available data
    //DEBUGMSG("copy all available data");
    uint32_t offset = firstframe - current->firstframe;
    uint32_t len = std::min(n,current->len - offset);
    for( uint32_t ch=0;ch<numchannels_;ch++){
      for( uint32_t k=0;k<len;k++){
        buf[ch][k] = current->data[(offset+k)*numchannels_+ch];
      }
    }
    // if not completely full then clear data and mark current as unusable:
    if( len < n ){
      for( uint32_t ch=0;ch<numchannels_;ch++){
        memset(&(buf[ch][len]), 0, sizeof(float)*(n-len) );
      }
      buffer_used_up = true;
    }else{
      // check if this buffer can still be used in the next cycle:
      if( firstframe + 2*n > current->firstframe+current->len ){
	buffer_used_up = true;
      }
    }
  }else{
    for( uint32_t ch=0;ch<numchannels_;ch++){
      memset(buf[ch], 0, sizeof(float)*n );
    }
    buffer_used_up = true;
  }
  // if the current buffer is used up, then request new data:
  if( buffer_used_up ){
    //DEBUGMSG("request new data");
    next_request = firstframe;
    rt_request_from_slave( next_request );
  }
  //DEBUGMSG("exit_request");
}

void TASCAR::async_sndfile_t::open(const std::string& fname, uint32_t firstchannel, int32_t first_frame, double gain, uint32_t loop)
{
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
  sfile = new looped_sndfile_t(fname, loop );
  file_frames = sfile->get_frames();
  file_channels = sfile->get_channels();
  if( file_channels < numchannels_ ){
    delete sfile;
    sfile = NULL;
    pthread_mutex_unlock( &mtx_file );
    throw "the input sound file does not provide sufficient number of channels";
  }
  file_firstchannel = std::min(firstchannel, file_channels-numchannels_);
  file_buffer = new float[chunksize_ * file_channels];
  file_pos = 0;
  file_firstframe = first_frame;
  if( loop )
    file_lastframe = first_frame + (int32_t)loop * file_frames - 1;
  else
    file_lastframe = (1 << 30);
  pthread_mutex_lock( &mtx_next );
  next->firstframe = -1;
  pthread_mutex_unlock( &mtx_next );
  pthread_mutex_unlock( &mtx_file );
}

void TASCAR::async_sndfile_t::rt_request_from_slave( uint32_t n )
{
  if( pthread_mutex_trylock( &mtx_readrequest ) == 0 ){
    requested_startframe = n;
    need_data = true;
    pthread_mutex_unlock( &mtx_readrequest );
  }
  pthread_mutex_trylock( &mtx_wakeup );
  pthread_mutex_unlock( &mtx_wakeup );
}

void TASCAR::async_sndfile_t::slave_read_file( uint32_t firstframe )
{
  pthread_mutex_lock( &mtx_file );
  pthread_mutex_lock( &mtx_next );
  if( firstframe == next->firstframe )
    return;
  next->firstframe = firstframe;
  //DEBUG(next->data);
  memset(next->data,0,sizeof(float) * chunksize_ * numchannels_);
  //DEBUG(firstframe);
  if( sfile ){
    if( ((int32_t)(firstframe + chunksize_) > file_firstframe)&&((int32_t)firstframe <= file_lastframe) ){
      //DEBUGMSG("the requested time interval is at least partially within the file");
      // the requested time interval is at least partially within the file
      uint32_t requested_file_pos = (std::max((int32_t)firstframe,file_firstframe)-file_firstframe);
      uint32_t databuffer_offset = std::max((int32_t)firstframe,file_firstframe)-firstframe;
      //DEBUG(requested_file_pos);
      //DEBUG(databuffer_offset);
      // databuffer_offset is smaller than chunksize_, because
      // (file_firstframe-firstframe < chunksize_)
      uint32_t requested_len = chunksize_ - databuffer_offset;
      //DEBUG(requested_len);
      // seek file if neccessary:
      if( file_pos != requested_file_pos ){
        sfile->seekf( requested_file_pos );
        file_pos = requested_file_pos;
      }
      sf_count_t rframes = sfile->readf_float( file_buffer, requested_len );
      file_pos += rframes;
      for( uint32_t ch=0;ch<numchannels_;ch++){
        //DEBUG(ch);
        for( uint32_t k=0;k<rframes;k++){
          next->data[(databuffer_offset+k)*numchannels_+ch] = gain_ * file_buffer[k*file_channels+file_firstchannel+ch];
        }
      }
    }
  }
  //DEBUG(firstframe);
  pthread_mutex_unlock( &mtx_next );
  pthread_mutex_unlock( &mtx_file );
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
