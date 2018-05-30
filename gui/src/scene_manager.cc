#include "scene_manager.h"
#include <fstream>
#include <string.h>
#include <fstream>

std::string file2str(const std::string& fname)
{
  std::ifstream t(fname.c_str());
  if( !t.good() )
    throw TASCAR::ErrMsg("Unable to open file \""+fname+"\".");
  std::stringstream buffer;
  buffer << t.rdbuf();
  return buffer.str();
}

std::string load_any_file(const std::string& fname)
{
  std::string raw(file2str(fname));
  if( raw.size() < 4 )
    throw TASCAR::ErrMsg("Invalid file type \""+fname+"\".");
  return raw;
}

scene_manager_t::scene_manager_t()
  : session(NULL)
{
  pthread_mutex_init( &mtx_scene, NULL );
}

scene_manager_t::~scene_manager_t()
{
  if( session )
    delete session;
}

void scene_manager_t::scene_load(const std::string& fname)
{
  pthread_mutex_lock( &mtx_scene );
  try{
    if( session )
      delete session;
    session = NULL;
    if( fname.size() ){
      session = new TASCAR::session_t( load_any_file(fname), TASCAR::xml_doc_t::LOAD_STRING, fname );
      session->start();
    }
  }
  catch( ... ){
    pthread_mutex_unlock( &mtx_scene );
    throw;
  }
  pthread_mutex_unlock( &mtx_scene );
}

void scene_manager_t::scene_new()
{
  pthread_mutex_lock( &mtx_scene );
  try{
    if( session )
      delete session;
    session = new TASCAR::session_t();
    session->start();
  }
  catch( ... ){
    pthread_mutex_unlock( &mtx_scene );
    throw;
  }
  pthread_mutex_unlock( &mtx_scene );
}

void scene_manager_t::scene_destroy()
{
  pthread_mutex_lock( &mtx_scene );
  try{
    if( session ){
      session->stop();
      delete session;
    }
    session = NULL;
  }
  catch( ... ){
    pthread_mutex_unlock( &mtx_scene );
    throw;
  }
  pthread_mutex_unlock( &mtx_scene );
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
