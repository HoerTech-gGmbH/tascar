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
    gain_(1.0),
    xrun(0)
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
    gain_(1.0),
    xrun(0)
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
      throw ErrMsg("pthread_create failed");
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
    throw ErrMsg("request_data channel count mismatch");
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
      xrun++;
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
      xrun++;
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
    throw ErrMsg("the input sound file does not provide sufficient number of channels");
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

TASCAR::ringbuffer_t::ringbuffer_t(uint32_t size,uint32_t channels_)
  : data(new float[size*channels_]),
    pos(size),
    channels(channels_)
{
  DEBUG(this);
  DEBUG(data);
  reset();
  pthread_mutex_init( &mtx_read_access, NULL );
  pthread_mutex_init( &mtx_write_access, NULL );
}
 
TASCAR::ringbuffer_t::~ringbuffer_t()
{
  DEBUG(this);
  DEBUG(data);
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
  if( w > r ) return w-r-1;
  return w+l-r-1;
}

uint32_t TASCAR::ringbuffer_t::pos_t::wspace()
{
  if( r >= w ) return r-w;
  return r+l-w;
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

uint32_t TASCAR::ringbuffer_t::read( float* buf, uint32_t frames )
{
  if( pthread_mutex_trylock( &mtx_read_access ) == 0 ){
    pos_t tmp_p(get_pos());
    uint32_t frames_to_read = std::min(frames,tmp_p.rspace());
    uint32_t r1(std::min(tmp_p.r+frames_to_read,tmp_p.l)-tmp_p.r);
    tmp_p.r += frames_to_read;
    if( r1 && buf )
      memcpy( &(data[pos.r]), buf, r1*sizeof(float)*channels);
    if( r1 < frames_to_read ){
      if( buf )
        memcpy( data, &(buf[r1]), (frames_to_read-r1)*sizeof(float)*channels);
      tmp_p.r = (frames_to_read-r1);
    }
    pos.r = tmp_p.r;
    current_location += frames_to_read;
    pthread_mutex_unlock( &mtx_read_access );
    return frames_to_read;
  }
  return 0;
}

uint32_t TASCAR::ringbuffer_t::read_skip( uint32_t frames )
{
  return read( NULL, frames);
}

uint32_t TASCAR::ringbuffer_t::write( float* buf, uint32_t frames )
{
  if( pthread_mutex_trylock( &mtx_write_access ) == 0 ){
    pos_t tmp_p(get_pos());
    uint32_t frames_to_write = std::min(frames,tmp_p.wspace());
    uint32_t r1(std::min(tmp_p.w+frames_to_write,tmp_p.l)-tmp_p.w);
    tmp_p.w += frames_to_write;
    if( r1 ){
      if( buf )
        memcpy( buf, &(data[pos.w]), r1*sizeof(float)*channels);
      else
        memset( &(data[pos.w]), 0, r1*sizeof(float)*channels);
    }
    if( r1 < frames_to_write ){
      if( buf )
        memcpy( &(buf[r1]), data, (frames_to_write-r1)*sizeof(float)*channels);
      else
        memset( data, 0, (frames_to_write-r1)*sizeof(float)*channels);
      tmp_p.w = (frames_to_write-r1);
    }
    pos.w = tmp_p.w;
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
  DEBUG(this);
  pos.r = 0;
  pos.w = 1;
  current_location = 0;
  requested_location = (uint32_t)(-1);
}

void TASCAR::ringbuffer_t::lock_relocate()
{
  pthread_mutex_lock( &mtx_read_access );
  pthread_mutex_lock( &mtx_write_access );
}

void TASCAR::ringbuffer_t::unlock_relocate()
{
  uint32_t new_location(requested_location);
  reset();
  current_location = new_location;
  pthread_mutex_unlock( &mtx_read_access );
  pthread_mutex_unlock( &mtx_write_access );
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
