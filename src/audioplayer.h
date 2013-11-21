#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "tascar.h"
#include "scene.h"
#include "async_file.h"

class audioplayer_t : public TASCAR::Scene::scene_t, public jackc_transport_t  {
public:
  audioplayer_t(const std::string& jackname,const std::string& xmlfile="");
  ~audioplayer_t();
  void open_files();
  int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_running);
  void run(bool &b_quit);
  void start();
  void stop();
  std::vector<TASCAR::Scene::sndfile_info_t> infos;
  std::vector<TASCAR::async_sndfile_t> files;
  std::vector<uint32_t> portno;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
