/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
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

#include "errorhandling.h"
#include "glabsensorplugin.h"
#include <atomic>
#include <lsl_cpp.h>
#include <mutex>
#include <sys/time.h>
#include <thread>

using namespace TASCAR;

double gettime()
{
  struct timeval tv;
  memset(&tv, 0, sizeof(timeval));
  gettimeofday(&tv, NULL);
  return (double)(tv.tv_sec) + 0.000001 * tv.tv_usec;
}

class rigid_t {
public:
  rigid_t(const std::string& name, double freq, lo_address datatarget_,
          std::string dataprefix_, bool use_lsl);
  ~rigid_t();
  void process(double x, double y, double z, double rz, double ry, double rx);

private:
  lsl::stream_info* lsl_info = NULL;
  lsl::stream_outlet* lsl_stream = NULL;
  std::string name;
  lo_address datatarget = NULL;
  std::string dataprefix;

public:
  std::vector<double> c6dof;
  double last_call;
};

rigid_t::rigid_t(const std::string& name_, double freq, lo_address datatarget_,
                 std::string dataprefix_, bool use_lsl)
    : name(name_), datatarget(datatarget_),
      dataprefix(dataprefix_), c6dof(6, 0.0),
      last_call(gettime() - 24.0 * 3600.0)
{
  if( use_lsl ){
    lsl_info = new lsl::stream_info(name_, "MoCap", 6, freq, lsl::cf_float32,
                                    std::string("qtm_") + name_);
    if( !lsl_info )
      throw TASCAR::ErrMsg("Unable to create LSL output stream info");
    lsl_stream = new lsl::stream_outlet(*lsl_info);
  }
  std::cerr << "Qualisys: added rigid " << name << std::endl;
}

rigid_t::~rigid_t()
{
  std::cerr << "Qualisys: removed rigid " << name << std::endl;
}

void rigid_t::process(double x, double y, double z, double rz, double ry,
                      double rx)
{
  x *= 0.001;
  y *= 0.001;
  z *= 0.001;
  c6dof[0] = x;
  c6dof[1] = y;
  c6dof[2] = z;
  c6dof[3] = DEG2RAD * rz;
  c6dof[4] = DEG2RAD * ry;
  c6dof[5] = DEG2RAD * rx;
  if( lsl_stream )
    lsl_stream->push_sample(c6dof);
  if(datatarget) {
    lo_send(datatarget, (dataprefix + "/" + name).c_str(), "ffffff", x, y, z,
            rz, ry, rx);
  }
  last_call = gettime();
}

#define OSCHANDLER(classname, methodname)                                      \
  static int methodname(const char* path, const char* types, lo_arg** argv,    \
                        int argc, lo_message msg, void* user_data)             \
  {                                                                            \
    return ((classname*)(user_data))                                           \
        ->methodname(path, types, argv, argc, msg);                            \
  }                                                                            \
  int methodname(const char* path, const char* types, lo_arg** argv, int argc, \
                 lo_message msg)

class qualisys_tracker_t : public sensorplugin_drawing_t {
public:
  qualisys_tracker_t(const TASCAR::sensorplugin_cfg_t& cfg);
  ~qualisys_tracker_t();
  void add_variables(TASCAR::osc_server_t* srv);
  void draw(const Cairo::RefPtr<Cairo::Context>& cr, double width,
            double height);
  void prepare();
  void release();
  void srv_prepare();
  OSCHANDLER(qualisys_tracker_t, qtmres);
  OSCHANDLER(qualisys_tracker_t, qtmxml);
  OSCHANDLER(qualisys_tracker_t, qtm6d);

private:
  TASCAR::osc_server_t oscserver;
  std::string qtmurl = "osc.udp://localhost:22225/";
  std::string dataurl = "";
  std::string dataprefix = "";
  double timeout = 1.0;
  lo_address qtmtarget = NULL;
  lo_address datatarget = NULL;
  std::mutex mtx;
  int32_t srv_port = 0;
  double nominal_freq = 1.0;
  std::map<std::string, rigid_t*> rigids;
  double last_prepared;
  std::atomic_bool run_preparethread = true;
  std::thread preparethread;
  std::atomic_bool isprepared = false;
  bool uselsl = true;
};

int qualisys_tracker_t::qtmres(const char*, const char*, lo_arg**, int,
                               lo_message)
{
  return 0;
}

int qualisys_tracker_t::qtmxml(const char*, const char*, lo_arg** argv, int,
                               lo_message)
{
  std::cerr << "Qualisys: receiving configuration" << std::endl;
  TASCAR::xml_doc_t qtmcfg(&(argv[0]->s), TASCAR::xml_doc_t::LOAD_STRING);
  TASCAR::xml_element_t root(qtmcfg.root);
  nominal_freq = 1.0;
  auto subn(root.get_children("General"));
  if(subn.size()) {
    TASCAR::xml_element_t general(subn[0]);
    general.get_attribute("Frequency", nominal_freq, "Hz",
                          "Nominal sensor sampling rate");
  }
  // nominal_freq = tsccfg::node_xpath_to_number(root.e,"/*/General/Frequency");
  for(auto the6d : tsccfg::node_get_children(root.e, "The_6D")) {
    for(auto body : tsccfg::node_get_children(the6d, "Body")) {
      for(auto bname : tsccfg::node_get_children(body, "Name")) {
        std::string name(tsccfg::node_get_text(bname));
        std::lock_guard<std::mutex> lock(mtx);
        if(rigids.find(name) != rigids.end())
          TASCAR::add_warning("Rigid " + name +
                              " was allocated more than once.");
        rigids[name] = new rigid_t(name, nominal_freq, datatarget, dataprefix, uselsl);
      }
    }
  }
  std::cerr << "Qualisys: requesting stream start" << std::endl;
  lo_send(qtmtarget, "/qtm", "sss", "StreamFrames", "AllFrames", "6DEuler");
  return 0;
}

int qualisys_tracker_t::qtm6d(const char* path, const char*, lo_arg** argv, int,
                              lo_message)
{
  if(strncmp(path, "/qtm/6d_euler/", 14) == 0) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it(rigids.find(&(path[14])));
    if(it != rigids.end()) {
      alive();
      it->second->process(argv[0]->f, argv[1]->f, argv[2]->f, argv[3]->f,
                          argv[4]->f, argv[5]->f);
      // return zero to indicate that this message was processed successfully
      return 0;
    }
  }
  // return non-zero to allow further processing
  return 1;
}

qualisys_tracker_t::qualisys_tracker_t(const sensorplugin_cfg_t& cfg)
    : sensorplugin_drawing_t(cfg), oscserver("", "auto", "UDP"),
      last_prepared(gettime())
{
  oscserver.activate();
  GET_ATTRIBUTE(qtmurl, "", "Qualisys Track Manager URL of USC interface");
  GET_ATTRIBUTE(timeout, "s", "Timeout");
  GET_ATTRIBUTE(dataurl, "",
                "OSC URL where data is sent to (or empty for no OSC sending)");
  GET_ATTRIBUTE(dataprefix, "",
                "OSC path prefix, will be followed by slash + rigid names");
  GET_ATTRIBUTE_BOOL(uselsl, "Create LSL output stream");
  qtmtarget = lo_address_new_from_url(qtmurl.c_str());
  if(!qtmtarget)
    throw TASCAR::ErrMsg("Invalid QTM URL");
  if(!dataurl.empty()) {
    datatarget = lo_address_new_from_url(dataurl.c_str());
    if(!datatarget)
      throw TASCAR::ErrMsg("Invalid data OSC URL \"" + dataurl + "\".");
  }
  preparethread = std::thread(&qualisys_tracker_t::srv_prepare, this);
}

void qualisys_tracker_t::srv_prepare()
{
  while(run_preparethread) {
    bool any_visible(false);
    if(mtx.try_lock()) {
      for(auto rigid : rigids) {
        if(gettime() - rigid.second->last_call < timeout)
          any_visible = true;
      }
      mtx.unlock();
      if((!any_visible) && (gettime() - last_prepared > 5.0)) {
        prepare();
      }
    }
    usleep(50000);
  }
}

void qualisys_tracker_t::prepare()
{
  if( isprepared )
    release();
  {
    std::lock_guard<std::mutex> lock(mtx);
    last_prepared = gettime();
    for(auto it = rigids.begin(); it != rigids.end(); ++it)
      delete it->second;
    rigids.clear();
  }
  std::cerr << "Qualisys: sending connection request" << std::endl;
  lo_send(qtmtarget, "/qtm", "si", "Connect", srv_port);
  lo_send(qtmtarget, "/qtm", "ss", "GetParameters", "All");
  isprepared = true;
}

void qualisys_tracker_t::release()
{
  isprepared = false;
  std::cerr << "Qualisys: sending disconnection request" << std::endl;
  lo_send(qtmtarget, "/qtm", "s", "Disconnect");
  {
    std::lock_guard<std::mutex> lock(mtx);
    for(auto it = rigids.begin(); it != rigids.end(); ++it)
      delete it->second;
    rigids.clear();
  }
}

qualisys_tracker_t::~qualisys_tracker_t()
{
  run_preparethread = false;
  if(preparethread.joinable())
    preparethread.join();
  lo_address_free(qtmtarget);
  oscserver.deactivate();
}

void qualisys_tracker_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv = &oscserver;
  srv_port = srv->get_srv_port();
  srv->set_prefix("");
  srv->add_method("/qtm/cmd_res", "s", &qtmres, this);
  srv->add_method("/qtm/xml", "s", &qtmxml, this);
  srv->add_method("", "ffffff", &qtm6d, this);
}

void qualisys_tracker_t::draw(const Cairo::RefPtr<Cairo::Context>& cr,
                              double width, double height)
{
  double y(0.05);
  {
    // std::lock_guard<std::mutex> lock(mtx);
    if(mtx.try_lock()) {
      // any_visible = false;
      for(auto it = rigids.begin(); it != rigids.end(); ++it) {
        y += 0.15;
        cr->move_to(0.05 * width, y * height);
        cr->show_text(it->first);
        cr->stroke();
        if(gettime() - it->second->last_call < timeout) {
          // any_visible = true;
          cr->move_to(0.23 * width, y * height);
          char ctmp[256];
          sprintf(ctmp, "%7.3f %7.3f %7.3f m %7.1f %7.1f %7.1f deg",
                  it->second->c6dof[0], it->second->c6dof[1],
                  it->second->c6dof[2], it->second->c6dof[3] * RAD2DEG,
                  it->second->c6dof[4] * RAD2DEG,
                  it->second->c6dof[5] * RAD2DEG);
          cr->show_text(ctmp);
          cr->stroke();
          // rotation:
          cr->save();
          cr->set_line_width(1);
          for(uint32_t c = 0; c < 3; ++c) {
            double lx((0.8 + 0.05 * c) * width);
            double ly((y - 0.04) * height);
            double r(0.055 * height);
            cr->arc(lx, ly, r, 0, TASCAR_2PI);
            cr->stroke();
            double lrot(DEG2RAD * it->second->c6dof[3 + c]);
            cr->move_to(lx, ly);
            cr->line_to(lx + r * cos(lrot), ly + r * sin(lrot));
            cr->stroke();
          }
          cr->restore();
        }
      }
      mtx.unlock();
    }
  }
}

REGISTER_SENSORPLUGIN(qualisys_tracker_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
