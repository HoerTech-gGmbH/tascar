/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

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

scene_manager_t::scene_manager_t()
  : session(NULL)
{
}

scene_manager_t::~scene_manager_t()
{
  if( session )
    delete session;
}

void scene_manager_t::scene_load(const std::string& fname)
{
  std::lock_guard<std::mutex> lock(session_mutex);
  if( session )
    delete session;
  session = NULL;
  if( fname.size() ){
    session = new TASCAR::session_t( fname, TASCAR::xml_doc_t::LOAD_FILE, fname );
    try{
      session->start();
    }
    catch( ... ){
      delete session;
      session = NULL;
      throw;
    }
  }
}

void scene_manager_t::scene_new()
{
  std::lock_guard<std::mutex> lock(session_mutex);
  if( session )
    delete session;
  session = new TASCAR::session_t();
  session->start();
}

void scene_manager_t::scene_destroy()
{
  std::lock_guard<std::mutex> lock(session_mutex);
  if( session ){
    session->stop();
    delete session;
  }
  session = NULL;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
