#include "alsamidicc.h"
#include "errorhandling.h"
#include "glabsensorplugin.h"
#include <lsl_cpp.h>
#include <mutex>
#include <thread>

using namespace TASCAR;

class oscthreshold_t : public sensorplugin_drawing_t {
public:
  oscthreshold_t(const sensorplugin_cfg_t& cfg);
  virtual ~oscthreshold_t();
  virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr, double width,
                    double height);
  virtual std::string type() const;
  void add_variables(TASCAR::osc_server_t* srv);
  void update(double val);

private:
  void send_service();
  std::string path;
  std::string msg_warning;
  std::string msg_critical;
  std::string disp_prefix;
  std::string disp_suffix;
  std::vector<double> range;
  double threshold_warning;
  double threshold_critical;
  std::string unknown_label;
  double value;
  lsl::stream_outlet* outlet;
  Gdk::RGBA col;
  std::mutex colmtx;
};

int osc_update(const char* path, const char* types, lo_arg** argv, int argc,
               lo_message msg, void* user_data)
{
  if(user_data && (argc == 1) && (types[0] == 'f'))
    ((oscthreshold_t*)user_data)->update(argv[0]->f);
  return 0;
}

oscthreshold_t::oscthreshold_t(const sensorplugin_cfg_t& cfg)
    : sensorplugin_drawing_t(cfg), threshold_warning(0.8),
      threshold_critical(0.9), value(0)
{
  if(name.empty())
    name = cfg.modname;
  range.resize(2);
  range[0] = 0;
  range[1] = 1;
  GET_ATTRIBUTE(path);
  GET_ATTRIBUTE(range);
  if(range.size() != 2)
    throw ErrMsg("Range needs two entries");
  if(range[0] == range[1])
    throw ErrMsg("Range values must differ");
  std::vector<std::string> controllers;
  GET_ATTRIBUTE(msg_warning);
  GET_ATTRIBUTE(msg_critical);
  GET_ATTRIBUTE(disp_prefix);
  GET_ATTRIBUTE(disp_suffix);
  GET_ATTRIBUTE(threshold_critical);
  GET_ATTRIBUTE(threshold_warning);
  lsl::stream_info streaminfo(get_name(), modname, 1, 0, lsl::cf_double64);
  outlet = new lsl::stream_outlet(streaminfo);
  col.set_rgba(1, 0, 0, 1);
}

oscthreshold_t::~oscthreshold_t() {}

void oscthreshold_t::draw(const Cairo::RefPtr<Cairo::Context>& cr, double width,
                          double height)
{
  cr->save();
  double linewidth(3.0);
  cr->set_line_width(linewidth);
  cr->set_font_size(7 * linewidth);
  cr->set_source_rgb(0, 0, 0);
  double y0(height - 40);
  double y1((height - 50) * 0.5 + 10);
  // cr->move_to(0,y0);
  // cr->line_to(width,y0);
  // cr->move_to(0,20);
  // cr->line_to(width,20);
  // cr->stroke();
  cr->save();
  cr->set_source_rgb(0.5, 0.5, 0.5);
  double x1(10);
  double x2(width - 10);
  cr->move_to(x1, y0);
  cr->line_to(x1, 10);
  cr->line_to(x2, 10);
  cr->line_to(x2, y0);
  cr->line_to(x1, y0);
  cr->stroke();
  cr->restore();
  cr->save();
  cr->set_line_width(0.8 * (height - 50));
  std::lock_guard<std::mutex> lock(colmtx);
  cr->set_source_rgb(col.get_red(), col.get_green(), col.get_blue());
  if(value != HUGE_VAL) {
    double x11(10 + (width - 20) * (value - range[0]) / (range[1] - range[0]));
    x11 = std::min(x11, width - 10);
    cr->move_to(x1, y1);
    cr->line_to(x11, y1);
    cr->stroke();
  }
  cr->restore();
  cr->move_to(10, height - 10);
  cr->show_text(unknown_label);
  cr->restore();
}

std::string oscthreshold_t::type() const
{
  return "osc";
}

void oscthreshold_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv->set_prefix("");
  srv->add_method(path, "f", &osc_update, this);
}

void oscthreshold_t::update(double newvalue)
{
  std::lock_guard<std::mutex> lock(colmtx);
  value = newvalue;
  col.set_rgba(0, 1, 0, 1);
  if(threshold_critical > threshold_warning) {
    // larger values are more critical
    if(value >= threshold_critical) {
      add_critical(msg_critical);
      col.set_rgba(1, 0, 0, 1);
    } else {
      if(value >= threshold_warning) {
        add_warning(msg_warning);
        col.set_rgba(1, 0.8, 0, 1);
      }
    }
  } else {
    // smaller values are more critical
    if(value <= threshold_critical) {
      add_critical(msg_critical);
      col.set_rgba(1, 0, 0, 1);
    } else {
      if(value <= threshold_warning) {
        add_warning(msg_warning);
        col.set_rgba(1, 0.8, 0, 1);
      }
    }
  }
  char ctmp[4096];
  snprintf(ctmp, 4096, "%s%g%s", disp_prefix.c_str(), value,
           disp_suffix.c_str());
  ctmp[4095] = 0;
  unknown_label = ctmp;
  outlet->push_sample(&value);
  alive();
}

REGISTER_SENSORPLUGIN(oscthreshold_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
