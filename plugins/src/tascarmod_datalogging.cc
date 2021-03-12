#include "datalogging_glade.h"
#include "session.h"
#include <fstream>
#include <gtkmm.h>
#include <gtkmm/builder.h>
#include <gtkmm/window.h>
#include <lsl_cpp.h>
#include <matio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cmath>

using std::isfinite;

#define STRBUFFER_SIZE 1024

class recorder_t;

void error_message(const std::string& msg)
{
  std::cerr << "Error: " << msg << std::endl;
  Gtk::MessageDialog dialog("Error", false, Gtk::MESSAGE_ERROR);
  dialog.set_secondary_text(msg);
  dialog.run();
}

std::string datestr()
{
  time_t t(time(NULL));
  struct tm* ltm = localtime(&t);
  char ctmp[1024];
  strftime(ctmp, 1024, "%Y%m%d_%H%M%S", ltm);
  return ctmp;
}

class lslvar_t : public TASCAR::xml_element_t {
public:
  lslvar_t(tsccfg::node_t xmlsrc, double lsltimeout);
  ~lslvar_t();
  uint32_t size;
  std::string name;
  recorder_t* recorder;
  void set_delta(double deltatime);
  void poll_data();
  void get_stream_delta_start();
  void get_stream_delta_end();
  std::string get_xml();

private:
  double get_stream_delta();
  std::string predicate;

public:
  //  lsl::stream_info* info;
  lsl::stream_inlet* inlet;

private:
  double delta;

public:
  double stream_delta_start;
  double stream_delta_end;
  bool time_correction_failed;
  double tctimeout;
  lsl::channel_format_t chfmt;
  uint32_t skipplot;
};

std::string lslvar_t::get_xml()
{
  if(!inlet)
    return "";
  return inlet->info().as_xml();
}

lslvar_t::lslvar_t(tsccfg::node_t xmlsrc, double lsltimeout)
    : xml_element_t(xmlsrc), size(0), recorder(NULL), inlet(NULL),
      delta(lsl::local_clock()), stream_delta_start(0), stream_delta_end(0),
      time_correction_failed(false), tctimeout(2.0), chfmt(lsl::cf_undefined),
      skipplot(0)
{
  // str_buffer.resize(STRBUFFER_SIZE);
  GET_ATTRIBUTE_(predicate);
  if(predicate.empty())
    throw TASCAR::ErrMsg("Invalid (empty) predicate.");
  GET_ATTRIBUTE_(tctimeout);
  bool required(true);
  GET_ATTRIBUTE_BOOL_(required);
  GET_ATTRIBUTE_(skipplot);
  std::vector<lsl::stream_info> results(lsl::resolve_stream(predicate, 1, 1));
  if(results.empty()) {
    if(required)
      throw TASCAR::ErrMsg("No matching LSL stream found (" + predicate + ").");
    else
      TASCAR::add_warning("No matching LSL stream found (" + predicate + ").",
                          e);
  }
  if(results.size() > 1)
    TASCAR::add_warning("More than one LSL stream found (" + predicate +
                            "), using first one.",
                        e);
  if(!results.empty()) {
    chfmt = results[0].channel_format();
    size = results[0].channel_count() + 1;
    name = results[0].name();
    inlet = new lsl::stream_inlet(results[0]);
    std::cerr << "created LSL inlet for predicate " << predicate << std::endl;
    std::cerr << "measuring LSL time correction: ";
    get_stream_delta_start();
    std::cerr << stream_delta_start << " s\n";
  }
}

void lslvar_t::get_stream_delta_start()
{
  stream_delta_start = get_stream_delta();
}

void lslvar_t::get_stream_delta_end()
{
  stream_delta_end = get_stream_delta();
}

double lslvar_t::get_stream_delta()
{
  if(!inlet)
    return 0.0;
  double stream_delta(0);
  try {
    stream_delta = inlet->time_correction(tctimeout);
  }
  catch(const std::exception& e) {
    std::cerr << "Warning: " << e.what() << std::endl;
    stream_delta = 0;
    time_correction_failed = true;
  }
  return stream_delta;
}

lslvar_t::~lslvar_t()
{
  if(inlet)
    delete inlet;
}

void lslvar_t::set_delta(double deltatime)
{
  delta = deltatime;
}

class oscsvar_t : public TASCAR::xml_element_t {
public:
  oscsvar_t(tsccfg::node_t xmlsrc);
  std::string path;
  uint32_t skipplot;
};

oscsvar_t::oscsvar_t(tsccfg::node_t xmlsrc)
    : xml_element_t(xmlsrc), skipplot(0)
{
  GET_ATTRIBUTE_(path);
  GET_ATTRIBUTE_(skipplot);
}

class oscvar_t : public TASCAR::xml_element_t {
public:
  oscvar_t(tsccfg::node_t xmlsrc);
  std::string get_fmt();
  std::string path;
  uint32_t size;
  bool ignorefirst;
  bool usedouble;
  uint32_t skipplot;
};

oscvar_t::oscvar_t(tsccfg::node_t xmlsrc)
  : xml_element_t(xmlsrc), size(1), ignorefirst(false), usedouble(false),skipplot(0)
{
  GET_ATTRIBUTE_(path);
  GET_ATTRIBUTE_(size);
  GET_ATTRIBUTE_BOOL_(ignorefirst);
  GET_ATTRIBUTE_BOOL_(usedouble);
  GET_ATTRIBUTE_(skipplot);
}

std::string oscvar_t::get_fmt()
{
  if( usedouble )
    return std::string(size, 'd');
  return std::string(size, 'f');
}

typedef std::vector<oscvar_t> oscvarlist_t;
typedef std::vector<oscsvar_t> oscsvarlist_t;
typedef std::vector<lslvar_t*> lslvarlist_t;

class dlog_vars_t : public TASCAR::module_base_t {
public:
  dlog_vars_t(const TASCAR::module_cfg_t& cfg);
  ~dlog_vars_t();
  virtual void update(uint32_t frame, bool running);
  void set_jack_client(jack_client_t* jc) { jc_ = jc; };

protected:
  std::string multicast;
  std::string port;
  std::string srv_proto;
  std::string fileformat;
  std::string outputdir;
  bool displaydc;
  double lsltimeout;
  oscvarlist_t oscvars;
  oscsvarlist_t oscsvars;
  lslvarlist_t lslvars;
  jack_client_t* jc_;
};

dlog_vars_t::dlog_vars_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg), srv_proto("UDP"), fileformat("matcell"),
      displaydc(true), lsltimeout(10.0), jc_(NULL)
{
  GET_ATTRIBUTE_(multicast);
  GET_ATTRIBUTE_(port);
  GET_ATTRIBUTE_(srv_proto);
  GET_ATTRIBUTE_(fileformat);
  GET_ATTRIBUTE_(outputdir);
  GET_ATTRIBUTE_(lsltimeout);
  GET_ATTRIBUTE_BOOL_(displaydc);
  if(fileformat.size() == 0)
    fileformat = "matcell";
  if((fileformat != "txt") && (fileformat != "mat") &&
     (fileformat != "matcell"))
    throw TASCAR::ErrMsg("Invalid file format \"" + fileformat + "\".");
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn = subnodes.begin();
      sn != subnodes.end(); ++sn) {
    tsccfg::node_t sne(dynamic_cast<tsccfg::node_t>(*sn));
    if(sne && (sne->get_name() == "variable"))
      oscvars.push_back(oscvar_t(sne));
    if(sne && (sne->get_name() == "osc"))
      oscvars.push_back(oscvar_t(sne));
    if(sne && (sne->get_name() == "oscs"))
      oscsvars.push_back(oscsvar_t(sne));
    if(sne && (sne->get_name() == "lsl"))
      lslvars.push_back(new lslvar_t(sne, lsltimeout));
  }
}

void dlog_vars_t::update(uint32_t frame, bool running)
{
  if(jc_) {
    double delta(jack_get_current_transport_frame(jc_) * t_sample);
    delta -= lsl::local_clock();
    delta *= -1;
    for(lslvarlist_t::iterator it = lslvars.begin(); it != lslvars.end(); ++it)
      (*it)->set_delta(delta);
  }
}

dlog_vars_t::~dlog_vars_t()
{
  for(lslvarlist_t::iterator it = lslvars.begin(); it != lslvars.end(); ++it)
    delete *it;
}

std::string nice_name(std::string s, std::string extend = "")
{
  for(uint32_t k = 0; k < s.size(); k++)
    switch(s[k]) {
    case '/':
    case ':':
    case '.':
    case ' ':
    case '-':
    case '+':
      s[k] = '_';
    }
  while(s.size() && (s[0] == '_'))
    s.erase(0, 1);
  if(extend.size())
    return s + "." + extend;
  return s;
}

class label_t {
public:
  label_t(double t1_, double t2_, const std::string& msg_)
      : t1(t1_), t2(t2_), msg(msg_){};
  double t1;
  double t2;
  std::string msg;
};

class recorder_t : public Gtk::DrawingArea {
public:
  recorder_t(uint32_t size, const std::string& name,
             pthread_mutex_t& record_mtx, jack_client_t* jc, double srate,
             bool ignore_first, size_t skipplot);
  virtual ~recorder_t();
  void store(uint32_t n, double* data);
  void store_msg(double t1, double t2, const std::string& msg);
  const std::vector<double>& get_data() const { return data_; };
  uint32_t get_size() const { return size_; };
  const std::string& get_name() const { return name_; };
  static int osc_setvar(const char* path, const char* types, lo_arg** argv,
                        int argc, lo_message msg, void* user_data);
  void osc_setvar(const char* path, uint32_t size, double* data);
  static int osc_setvar_s(const char* path, const char* types, lo_arg** argv,
                          int argc, lo_message msg, void* user_data);
  void osc_setvar_s(const char* path, const char* msg);
  void clear();
  virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
  bool on_timeout();
  void set_displaydc(bool displaydc) { displaydc_ = displaydc; };
  void set_textdata() { b_textdata = true; };
  bool is_textdata() const { return b_textdata; };
  const std::vector<label_t>& get_textdata() const { return messages; };

private:
  void get_valuerange(const std::vector<double>& data, uint32_t channels,
                      uint32_t firstchannel, uint32_t n1, uint32_t n2,
                      double& dmin, double& dmax);
  bool displaydc_;
  uint32_t size_;
  bool b_textdata;
  std::vector<double> data_;
  std::vector<double> plotdata_;
  std::vector<double> vdc;
  std::vector<label_t> messages;
  std::vector<label_t> plot_messages;
  std::string name_;
  pthread_mutex_t& record_mtx_;
  jack_client_t* jc_;
  double israte_;
  pthread_mutex_t drawlock;
  pthread_mutex_t plotdatalock;
  sigc::connection connection_timeout;
  bool ignore_first_;
  size_t skipplotdata_;
  size_t plotdata_cnt;
};

recorder_t::recorder_t(uint32_t size, const std::string& name,
                       pthread_mutex_t& record_mtx, jack_client_t* jc,
                       double srate, bool ignore_first, size_t skipplotdata)
    : displaydc_(true), size_(size), b_textdata(false), vdc(size_, 0),
      name_(name), record_mtx_(record_mtx), jc_(jc), israte_(1.0 / srate),
      ignore_first_(ignore_first), skipplotdata_(skipplotdata), plotdata_cnt(0)
{
  pthread_mutex_init(&drawlock, NULL);
  pthread_mutex_init(&plotdatalock, NULL);
  connection_timeout = Glib::signal_timeout().connect(
      sigc::mem_fun(*this, &recorder_t::on_timeout), 200);
}

void recorder_t::clear()
{
  data_.clear();
  messages.clear();
  pthread_mutex_lock(&plotdatalock);
  plotdata_.clear();
  plot_messages.clear();
  pthread_mutex_unlock(&plotdatalock);
  plotdata_cnt = 0;
}

recorder_t::~recorder_t()
{
  connection_timeout.disconnect();
}

double sqr(double x)
{
  return x * x;
}

/**
   \brief Find index range corresponding to time interval
   \param data Multichannel data vector, time is in first channel
   \param channels Number of channels
   \param t1 Start time, value included
   \param t2 End time, value excluded
   \retval n1 Start index, included
   \retval n2 End index, excluded
 */
void find_timeinterval(const std::vector<double>& data, uint32_t channels,
                       double t1, double t2, uint32_t& n1, uint32_t& n2)
{
  uint32_t N(data.size() / channels);
  n1 = 0;
  n2 = 0;
  if(N == 0)
    return;
  for(n2 = N - 1; (n2 > 0) && (data[n2 * channels] >= t2); --n2)
    ;
  ++n2;
  for(n1 = n2 - 1; (n1 > 0) && (data[n1 * channels] >= t1); --n1)
    ;
}

void recorder_t::get_valuerange(const std::vector<double>& data,
                                uint32_t channels, uint32_t firstchannel,
                                uint32_t n1, uint32_t n2, double& dmin,
                                double& dmax)
{
  dmin = HUGE_VAL;
  dmax = -HUGE_VAL;
  for(uint32_t dim = firstchannel; dim < channels; dim++) {
    vdc[dim] = 0;
    if(!displaydc_) {
      uint32_t cnt(0);
      for(uint32_t k = n1; k < n2; k++) {
        double v(data[dim + k * channels]);
        if((v != HUGE_VAL) && (v != -HUGE_VAL)) {
          vdc[dim] += v;
          cnt++;
        }
      }
      if(cnt)
        vdc[dim] /= (double)cnt;
    }
    for(uint32_t k = n1; k < n2; k++) {
      double v(data[dim + k * channels]);
      if((v != HUGE_VAL) && (v != -HUGE_VAL)) {
        dmin = std::min(dmin, v - vdc[dim]);
        dmax = std::max(dmax, v - vdc[dim]);
      }
    }
  }
  if(dmax == dmin) {
    dmin -= 1.0;
    dmax += 1.0;
  }
  if(!(dmax > dmin)) {
    dmin = 1.0;
    dmax = 1.0;
  }
}

TASCAR::Scene::rgb_color_t set_hsv(float h, float s, float v)
{
  // const std::complex<float> i(0.0, 1.0);
  // h = (std::arg(std::exp(i * h * DEG2RADf)) * RAD2DEGf);
  // if(h < 0)
  //  h += 360.0;
  h = fmodf(h, 360.0f);
  float c(v * s);
  float x(c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f)));
  float m(v - c);
  TASCAR::Scene::rgb_color_t rgb_d;
  if(h < 60)
    rgb_d = TASCAR::Scene::rgb_color_t(c, x, 0);
  else if(h < 120)
    rgb_d = TASCAR::Scene::rgb_color_t(x, c, 0);
  else if(h < 180)
    rgb_d = TASCAR::Scene::rgb_color_t(0, c, x);
  else if(h < 240)
    rgb_d = TASCAR::Scene::rgb_color_t(0, x, c);
  else if(h < 300)
    rgb_d = TASCAR::Scene::rgb_color_t(x, 0, c);
  else
    rgb_d = TASCAR::Scene::rgb_color_t(c, 0, x);
  rgb_d.r += m;
  rgb_d.g += m;
  rgb_d.b += m;
  return rgb_d;
}

bool recorder_t::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  if(pthread_mutex_trylock(&drawlock) == 0) {
    Glib::RefPtr<Gdk::Window> window = get_window();
    if(window) {
      Gtk::Allocation allocation = get_allocation();
      const int width = allocation.get_width();
      const int height = allocation.get_height();
      // double ratio = (double)width/(double)height;
      cr->save();
      cr->set_source_rgb(1.0, 1.0, 1.0);
      cr->paint();
      cr->restore();
      cr->save();
      // cr->scale((double)width/30.0,(double)height);
      cr->translate(width, 0);
      cr->set_line_width(1);
      // data plot:
      pthread_mutex_lock(&plotdatalock);
      double ltime(0);
      double drange(2);
      double dscale(1);
      double dmin(-1);
      double dmax(1);
      if(b_textdata) {
        drange = dmax - dmin;
        dscale = height / drange;
        ltime = israte_ * jack_get_current_transport_frame(jc_);
        for(auto it = plot_messages.begin(); it != plot_messages.end(); ++it) {
          if((it->t1 >= ltime - 30) && (it->t1 <= ltime)) {
            double t((it->t1 - ltime) * width / 30.0);
            double v(height - (-0.8 - dmin) * dscale);
            double v2(height - (-0.85 - dmin) * dscale);
            double v1(height);
            cr->save();
            cr->set_source_rgb(0.4, 0, 0);
            cr->move_to(t, v2);
            cr->line_to(t, v1);
            cr->stroke();
            cr->move_to(t, v);
            cr->save();
            cr->rotate(-0.5 * M_PI);
            cr->show_text(it->msg);
            cr->restore();
            cr->restore();
          }
        }
      } else {
        // N is number of (multi-dimensional) samples:
        uint32_t N(plotdata_.size() / size_);
        if(N > 1) {
          //        double tmax(0);
          uint32_t n1(0);
          uint32_t n2(0);
          ltime = plotdata_[(N - 1) * size_];
          find_timeinterval(plotdata_, size_, ltime - 30, ltime, n1, n2);
          get_valuerange(plotdata_, size_, 1 + ignore_first_, n1, n2, dmin,
                         dmax);
          uint32_t stepsize((n2 - n1) / 2048);
          ++stepsize;
          drange = dmax - dmin;
          dscale = height / drange;
          for(uint32_t dim = 1 + ignore_first_; dim < size_; ++dim) {
            cr->save();
            double size_norm(1.0 / (size_ - 1.0 - ignore_first_));
            TASCAR::Scene::rgb_color_t rgb(set_hsv(
                (dim - 1.0 - ignore_first_) * size_norm * 360, 0.8, 0.7));
            cr->set_source_rgb(rgb.r, rgb.g, rgb.b);
            bool first(true);
            // limit number of lines:
            for(uint32_t k = n1; k < n2; k += stepsize) {
              double t(plotdata_[k * size_] - ltime);
              double v(plotdata_[dim + k * size_]);
              if(finite(v)) {
                if(!displaydc_)
                  v -= vdc[dim];
                if(v != HUGE_VAL) {
                  t *= width / 30.0;
                  v = height - (v - dmin) * dscale;
                  //++nlines;
                  if(first) {
                    cr->move_to(t, v);
                    first = false;
                  } else
                    cr->line_to(t, v);
                }
              }
            }
            cr->stroke();
            cr->restore();
          }
        }
      }
      pthread_mutex_unlock(&plotdatalock);
      // draw scale:
      cr->set_source_rgb(0, 0, 0);
      cr->move_to(-width, 0);
      cr->line_to(0, 0);
      cr->line_to(0, height);
      cr->line_to(-width, height);
      cr->line_to(-width, 0);
      cr->stroke();
      // x-grid, x-scale:
      for(int32_t k = 0; k < 30; k++) {
        double t(floor(ltime) - k);
        double x((t - ltime) * width / 30.0);
        if(t >= 0) {
          // cr->move_to(-(double)k*width/30.0,height);
          // cr->line_to(-(double)k*width/30.0,(double)height*(1-0.03));
          cr->move_to(x, height);
          double dx(0.02);
          if((int)t % 5 == 0)
            dx *= 2;
          cr->line_to(x, (double)height * (1 - dx));
          if((int)t % 5 == 0) {
            char ctmp[1024];
            sprintf(ctmp, "%d", (int)t);
            cr->show_text(ctmp);
          }
        }
      }
      cr->stroke();
      // y-grid & scale:
      double ystep(pow(10.0, floor(log10(0.5 * drange))));
      if(std::isfinite(dmax) && std::isfinite(dmin) && std::isfinite(ystep) &&
         isfinite(dscale)) {
        double ddmin(ystep * round(dmin / ystep));
        if(std::isfinite(dmax) && std::isfinite(ddmin) && std::isfinite(ystep) &&
           (dmax > ddmin)) {
          uint8_t ky(0);
          for(double dy = ddmin; (dy < dmax) && (ky < 20); dy += ystep) {
            ++ky;
            double v(height - (dy - dmin) * dscale);
            cr->move_to(-width, v);
            cr->line_to(-width * (1.0 - 0.01), v);
            char ctmp[1024];
            sprintf(ctmp, "%g", dy);
            cr->show_text(ctmp);
            cr->stroke();
          }
        }
      }
      cr->restore();
    }
    pthread_mutex_unlock(&drawlock);
  }
  return true;
}

bool recorder_t::on_timeout()
{
  Glib::RefPtr<Gdk::Window> win = get_window();
  if(win) {
    Gdk::Rectangle r(0, 0, get_allocation().get_width(),
                     get_allocation().get_height());
    win->invalidate_rect(r, false);
  }
  return true;
}

void recorder_t::store(uint32_t n, double* data)
{
  if(n != size_)
    throw TASCAR::ErrMsg("Invalid size (recorder_t::store)");
  if(pthread_mutex_trylock(&record_mtx_) == 0) {
    for(uint32_t k = 0; k < n; k++)
      data_.push_back(data[k]);
    if(pthread_mutex_trylock(&plotdatalock) == 0) {
      for(uint32_t k = 0; k < n; k++)
        plotdata_.push_back(data[k]);
      pthread_mutex_unlock(&plotdatalock);
    }
    pthread_mutex_unlock(&record_mtx_);
  }
}

void recorder_t::store_msg(double t1, double t2, const std::string& msg)
{
  if(pthread_mutex_trylock(&record_mtx_) == 0) {
    messages.emplace_back(t1, t2, msg);
    b_textdata = true;
    if(pthread_mutex_trylock(&plotdatalock) == 0) {
      plot_messages = messages;
      pthread_mutex_unlock(&plotdatalock);
    }
    pthread_mutex_unlock(&record_mtx_);
  }
}

void lslvar_t::poll_data()
{
  if(!inlet)
    return;
  double recorder_buffer[size + 1];
  double* data_buffer(&(recorder_buffer[2]));
  double t(1);
  // char* strb(str_buffer.data());
  while(t != 0) {
    if(chfmt == lsl::cf_string) {
      std::vector<std::string> sample;
      t = inlet->pull_sample(sample, 0.0);
      if((t != 0) && recorder) {
        recorder->set_textdata();
        for(auto it = sample.begin(); it != sample.end(); ++it) {
          recorder->store_msg(t - delta + stream_delta_start,
                              t + stream_delta_start, *it);
        }
      }
    } else {
      try {
        t = inlet->pull_sample(data_buffer, size - 1, 0.0);
        if((t != 0) && recorder) {
          recorder_buffer[0] = t - delta + stream_delta_start;
          recorder_buffer[1] = t + stream_delta_start;
          recorder->store(size + 1, recorder_buffer);
        }
      }
      catch(const std::exception& e) {
        TASCAR::add_warning(e.what());
      }
    }
  }
}

class datalogging_t : public dlog_vars_t, public TASCAR::osc_server_t {
  //, public jackc_portless_t {
public:
  datalogging_t(const TASCAR::module_cfg_t& cfg);
  ~datalogging_t();
  void start_trial(const std::string& name);
  void stop_trial();
  void save_text(const std::string& filename);
  void save_mat(const std::string& filename);
  void save_matcell(const std::string& filename);
  void save_session_related_meta_data(mat_t* matfp, const std::string& fname);
  void on_osc_set_trialid();
  void on_ui_showdc();
  void on_ui_start();
  void on_ui_stop();
  bool on_100ms();
  void poll_lsl_data();
  static int osc_session_start(const char* path, const char* types,
                               lo_arg** argv, int argc, lo_message msg,
                               void* user_data);
  static int osc_session_stop(const char* path, const char* types,
                              lo_arg** argv, int argc, lo_message msg,
                              void* user_data);
  static int osc_session_set_trialid(const char* path, const char* types,
                                     lo_arg** argv, int argc, lo_message msg,
                                     void* user_data);

private:
  std::vector<recorder_t*> recorder;
  pthread_mutex_t record_mtx;
  bool b_recording;
  std::string filename;
  // GUI:
  Glib::RefPtr<Gtk::Builder> refBuilder;
  Gtk::Window* win;
  Gtk::Entry* trialid;
  Gtk::Label* datelabel;
  Gtk::Label* jacktime;
  Gtk::Grid* draw_grid;
  Gtk::Label* rec_label;
  Gtk::Entry* outputdirentry;
  Gtk::CheckButton* showdc;
  sigc::connection connection_timeout;
  Glib::Dispatcher osc_start;
  Glib::Dispatcher osc_stop;
  Glib::Dispatcher osc_set_trialid;
  std::string osc_trialid;
  double israte_;
  uint32_t fragsize;
  double srate;
};

#define GET_WIDGET(x)                                                          \
  refBuilder->get_widget(#x, x);                                               \
  if(!x)                                                                       \
  throw TASCAR::ErrMsg(std::string("No widget \"") + #x +                      \
                       std::string("\" in builder."))

datalogging_t::datalogging_t(const TASCAR::module_cfg_t& cfg)
    : dlog_vars_t(cfg), TASCAR::osc_server_t(multicast, port, srv_proto),
      // jackc_portless_t(jacknamer(session->name,"datalog.")),
      b_recording(false), israte_(1.0 / cfg.session->srate),
      fragsize(cfg.session->fragsize), srate(cfg.session->srate)
{
  pthread_mutex_init(&record_mtx, NULL);
  pthread_mutex_lock(&record_mtx);
  TASCAR::osc_server_t* osc(this);
  if(port.empty())
    osc = session;
  osc->add_method("/session_trialid", "s",
                  &datalogging_t::osc_session_set_trialid, this);
  osc->add_method("/session_start", "", &datalogging_t::osc_session_start,
                  this);
  osc->add_method("/session_stop", "", &datalogging_t::osc_session_stop, this);
  set_jack_client(session->jc);
  // first, add all regular OSC variables:
  for(auto ov : oscvars) {
    recorder.push_back(new recorder_t(ov.size + 1, ov.path, record_mtx,
                                      session->jc, session->srate,
                                      ov.ignorefirst, ov.skipplot));
    osc->add_method(ov.path, ov.get_fmt().c_str(), &recorder_t::osc_setvar,
                    recorder.back());
  }
  // second, add all string OSC variables:
  for(auto ov : oscsvars) {
    recorder.push_back(new recorder_t(2, ov.path, record_mtx, session->jc,
                                      session->srate, false, ov.skipplot));
    osc->add_method(ov.path, "s", &recorder_t::osc_setvar_s, recorder.back());
  }
  // finally, add all LSL variables (the vector contains pointers):
  for(auto lslvp : lslvars) {
    recorder.push_back(new recorder_t(lslvp->size + 1, lslvp->name, record_mtx,
                                      session->jc, session->srate, true,
                                      lslvp->skipplot));
    lslvp->recorder = recorder.back();
  }
  // update display of DC:
  for(auto rec : recorder)
    rec->set_displaydc(displaydc);
  //
  TASCAR::osc_server_t::activate();
  refBuilder = Gtk::Builder::create_from_string(ui_datalogging);
  GET_WIDGET(win);
  GET_WIDGET(trialid);
  GET_WIDGET(datelabel);
  GET_WIDGET(jacktime);
  GET_WIDGET(draw_grid);
  GET_WIDGET(rec_label);
  GET_WIDGET(outputdirentry);
  GET_WIDGET(showdc);
  rec_label->set_text("");
  outputdir = TASCAR::env_expand(outputdir);
  outputdirentry->set_text(outputdir);
  win->set_title("tascar datalogging - " + session->name);
  int32_t dlogwin_x(0);
  int32_t dlogwin_y(0);
  int32_t dlogwin_width(300);
  int32_t dlogwin_height(300);
  win->get_size(dlogwin_width, dlogwin_height);
  win->get_position(dlogwin_x, dlogwin_y);
  get_attribute_value(e, "w", dlogwin_width);
  get_attribute_value(e, "h", dlogwin_height);
  get_attribute_value(e, "x", dlogwin_x);
  get_attribute_value(e, "y", dlogwin_y);
  win->resize(dlogwin_width, dlogwin_height);
  win->move(dlogwin_x, dlogwin_y);
  win->resize(dlogwin_width, dlogwin_height);
  win->move(dlogwin_x, dlogwin_y);
  win->show();
  // Create actions for menus and toolbars:
  Glib::RefPtr<Gio::SimpleActionGroup> refActionGroupMain =
      Gio::SimpleActionGroup::create();
  refActionGroupMain->add_action(
      "start", sigc::mem_fun(*this, &datalogging_t::on_ui_start));
  refActionGroupMain->add_action(
      "stop", sigc::mem_fun(*this, &datalogging_t::on_ui_stop));
  refActionGroupMain->add_action(
      "showdc", sigc::mem_fun(*this, &datalogging_t::on_ui_showdc));
  osc_start.connect(sigc::mem_fun(*this, &datalogging_t::on_ui_start));
  osc_stop.connect(sigc::mem_fun(*this, &datalogging_t::on_ui_stop));
  osc_set_trialid.connect(
      sigc::mem_fun(*this, &datalogging_t::on_osc_set_trialid));
  win->insert_action_group("datalog", refActionGroupMain);
  connection_timeout = Glib::signal_timeout().connect(
      sigc::mem_fun(*this, &datalogging_t::on_100ms), 100);
  uint32_t num_cols(ceil(sqrt(recorder.size())));
  uint32_t num_rows(ceil(recorder.size() / std::max(1u, num_cols)));
  for(uint32_t k = 0; k < num_cols; k++)
    draw_grid->insert_column(0);
  for(uint32_t k = 0; k < num_rows; k++)
    draw_grid->insert_row(0);
  for(uint32_t k = 0; k < recorder.size(); k++) {
    uint32_t kc(k % num_cols);
    uint32_t kr(k / num_cols);
    Gtk::Box* box = new Gtk::Box(Gtk::ORIENTATION_VERTICAL);
    draw_grid->attach(*box, kc, kr, 1, 1);
    Gtk::Label* label = new Gtk::Label(recorder[k]->get_name());
    box->pack_start(*label, Gtk::PACK_SHRINK);
    box->pack_start(*(recorder[k]));
  }
  draw_grid->show_all();
  showdc->set_active(displaydc);
}

void datalogging_t::poll_lsl_data()
{
  for(lslvarlist_t::iterator it = lslvars.begin(); it != lslvars.end(); ++it)
    (*it)->poll_data();
}

int datalogging_t::osc_session_start(const char* path, const char* types,
                                     lo_arg** argv, int argc, lo_message msg,
                                     void* user_data)
{
  ((datalogging_t*)user_data)->osc_start.emit();
  return 0;
}

int datalogging_t::osc_session_stop(const char* path, const char* types,
                                    lo_arg** argv, int argc, lo_message msg,
                                    void* user_data)
{
  ((datalogging_t*)user_data)->osc_stop.emit();
  return 0;
}

int datalogging_t::osc_session_set_trialid(const char* path, const char* types,
                                           lo_arg** argv, int argc,
                                           lo_message msg, void* user_data)
{
  ((datalogging_t*)user_data)->osc_trialid = std::string(&(argv[0]->s));
  ((datalogging_t*)user_data)->osc_set_trialid.emit();
  return 0;
}

void datalogging_t::on_osc_set_trialid()
{
  trialid->set_text(osc_trialid);
}

void datalogging_t::on_ui_showdc()
{
  displaydc = showdc->get_active();
  for(std::vector<recorder_t*>::iterator it = recorder.begin();
      it != recorder.end(); ++it)
    (*it)->set_displaydc(displaydc);
}

void datalogging_t::on_ui_start()
{
  try {
    std::string trialidtext(trialid->get_text());
    if(trialidtext.empty()) {
      trialidtext = "unnamed";
      trialid->set_text(trialidtext);
    }
    std::string datestr_(datestr());
    datelabel->set_text(datestr_);
    // throw TASCAR::ErrMsg("Empty trial ID string.");
    trialidtext += "_" + datestr_;
    win->set_title("tascar datalogging - " + session->name + " [" +
                   trialidtext + "]");
    start_trial(trialidtext);
    rec_label->set_text(" REC ");
  }
  catch(const std::exception& e) {
    std::string tmp(e.what());
    tmp += " (start)";
    error_message(tmp);
  }
}

void datalogging_t::on_ui_stop()
{
  try {
    stop_trial();
    rec_label->set_text("");
    win->set_title("tascar datalogging - " + session->name);
  }
  catch(const std::exception& e) {
    std::string tmp(e.what());
    tmp += " (stop)";
    error_message(tmp);
  }
}

bool datalogging_t::on_100ms()
{
  // poll LSL data:
  poll_lsl_data();
  // update labels:
  if(!b_recording)
    datelabel->set_text(datestr());
  char ctmp[1024];
  // sprintf(ctmp,"%1.1f s", jack_frame_time(jc)/(double)srate );
  sprintf(ctmp, "%1.1f s", session->tp_get_time());
  jacktime->set_text(ctmp);
  return true;
}

int recorder_t::osc_setvar(const char* path, const char* types, lo_arg** argv,
                           int argc, lo_message msg, void* user_data)
{
  double data[argc + 1];
  data[0] = 0;
  for(uint32_t k = 0; k < (uint32_t)argc; k++){
    switch( types[k] ){
    case 'f':
      data[k + 1] = argv[k]->f;
      break;
    case 'd':
      data[k + 1] = argv[k]->d;
      break;
    case 'i':
      data[k + 1] = argv[k]->i;
      break;
    }
  }
  ((recorder_t*)user_data)->osc_setvar(path, argc + 1, data);
  return 0;
}

void recorder_t::osc_setvar(const char* path, uint32_t size, double* data)
{
  data[0] = israte_ * jack_get_current_transport_frame(jc_);
  store(size, data);
}

int recorder_t::osc_setvar_s(const char* path, const char* types, lo_arg** argv,
                             int argc, lo_message msg, void* user_data)
{
  ((recorder_t*)user_data)->osc_setvar_s(path, &(argv[0]->s));
  return 0;
}

void recorder_t::osc_setvar_s(const char* path, const char* msg)
{
  double t1(israte_ * jack_get_current_transport_frame(jc_));
  store_msg(t1, 0, msg);
}

datalogging_t::~datalogging_t()
{
  try {
    stop_trial();
  }
  catch(const std::exception& e) {
    std::cerr << "Warning: " << e.what() << "\n";
  }
  connection_timeout.disconnect();
  TASCAR::osc_server_t::deactivate();
  for(uint32_t k = 0; k < recorder.size(); k++)
    delete recorder[k];
  pthread_mutex_destroy(&record_mtx);
  delete win;
  delete trialid;
  delete datelabel;
  delete jacktime;
  delete draw_grid;
  delete rec_label;
}

void datalogging_t::start_trial(const std::string& name)
{
  stop_trial();
  if(name.empty())
    throw TASCAR::ErrMsg("Empty trial name.");
  session->tp_stop();
  session->tp_locate(0u);
  uint32_t timeout(1000);
  while(timeout && (session->tp_get_time() > 0))
    usleep(1000);
  for(uint32_t k = 0; k < recorder.size(); k++)
    recorder[k]->clear();
  // lsl re-sync:
  for(lslvarlist_t::iterator it = lslvars.begin(); it != lslvars.end(); ++it) {
    (*it)->get_stream_delta_start();
  }
  filename = name;
  b_recording = true;
  pthread_mutex_unlock(&record_mtx);
  session->tp_start();
}

void datalogging_t::stop_trial()
{
  session->tp_stop();
  // lsl re-sync:
  for(lslvarlist_t::iterator it = lslvars.begin(); it != lslvars.end(); ++it) {
    (*it)->get_stream_delta_end();
  }
  outputdir = outputdirentry->get_text();
  if(!outputdir.empty()) {
    if(outputdir[outputdir.size() - 1] != '/')
      outputdir = outputdir + "/";
    struct stat info;
    if(stat(outputdir.c_str(), &info) != 0)
      throw TASCAR::ErrMsg("Cannot access output directory \"" + outputdir +
                           "\".");
    if(!(info.st_mode & S_IFDIR)) // S_ISDIR() doesn't exist on my windows
      throw TASCAR::ErrMsg("\"" + outputdir + "\" is not a directory.");
  }
  if(!b_recording)
    return;
  pthread_mutex_lock(&record_mtx);
  b_recording = false;
  if(fileformat == "txt")
    save_text(filename);
  else if(fileformat == "mat")
    save_mat(filename);
  else if(fileformat == "matcell")
    save_matcell(filename);
}

void datalogging_t::save_text(const std::string& filename)
{
  std::string fname(outputdir + nice_name(filename, "txt"));
  std::ofstream ofs(fname.c_str());
  if(!ofs.good())
    throw TASCAR::ErrMsg("Unable to create file \"" + fname + "\".");
  ofs << "# tascar datalogging\n";
  ofs << "# version: " << TASCARVERSION << "\n";
  ofs << "# trialid: " << filename << "\n";
  ofs << "# filename: " << fname << "\n";
  ofs << "# savedat: " << datestr() << "\n";
  ofs << "# tscfilename: " << session->get_file_name() << "\n";
  ofs << "# tscpath: " << session->get_session_path() << "\n";
  ofs << "# srate: " << srate << "\n";
  ofs << "# fragsize: " << fragsize << "\n";
  for(uint32_t k = 0; k < recorder.size(); k++) {
    ofs << "# " << recorder[k]->get_name() << std::endl;
    if(recorder[k]->is_textdata()) {
      std::vector<label_t> data(recorder[k]->get_textdata());
      for(auto it = data.begin(); it != data.end(); ++it)
        ofs << it->t1 << " " << it->t2 << " \"" << it->msg << "\"\n";
    } else {
      std::vector<double> data(recorder[k]->get_data());
      uint32_t N(recorder[k]->get_size());
      uint32_t M(data.size());
      uint32_t cnt(N - 1);
      for(uint32_t l = 0; l < M; l++) {
        ofs << data[l];
        if(cnt--) {
          ofs << " ";
        } else {
          ofs << "\n";
          cnt = N - 1;
        }
      }
    }
  }
  for(lslvarlist_t::iterator it = lslvars.begin(); it != lslvars.end(); ++it) {
    ofs << "# " << (*it)->recorder->get_name()
        << "_stream_delta_start: " << (*it)->stream_delta_start << "\n";
    ofs << "# " << (*it)->recorder->get_name()
        << "_stream_delta_end: " << (*it)->stream_delta_end << "\n";
  }
}

void mat_add_strvar(mat_t* matfb, const std::string& name,
                    const std::string& str)
{
  size_t dims[2];
  dims[0] = 1;
  dims[1] = str.size();
  char* v((char*)str.c_str());
  matvar_t* mStr(
      Mat_VarCreate(name.c_str(), MAT_C_CHAR, MAT_T_INT8, 2, dims, v, 0));
  if(mStr == NULL)
    throw TASCAR::ErrMsg("Unable to create variable \"" + name + "\".");
  Mat_VarWrite(matfb, mStr, MAT_COMPRESSION_NONE);
  Mat_VarFree(mStr);
}

void mat_add_double(mat_t* matfb, const std::string& name, double v)
{
  size_t dims[2];
  dims[0] = 1;
  dims[1] = 1;
  matvar_t* mVar(
      Mat_VarCreate(name.c_str(), MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, &v, 0));
  if(mVar == NULL)
    throw TASCAR::ErrMsg("Unable to create variable \"" + name + "\".");
  Mat_VarWrite(matfb, mVar, MAT_COMPRESSION_NONE);
  Mat_VarFree(mVar);
}

void mat_set_double_field(matvar_t* s, const std::string& name, double v,
                          size_t idx)
{
  size_t dims[2];
  dims[0] = 1;
  dims[1] = 1;
  matvar_t* mVar(
      Mat_VarCreate(NULL, MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, &v, 0));
  if(mVar == NULL)
    throw TASCAR::ErrMsg("Unable to create variable \"" + name + "\".");
  Mat_VarSetStructFieldByName(s, name.c_str(), idx, mVar);
}

void mat_add_double_field(matvar_t* s, const std::string& name, double v)
{
  Mat_VarAddStructField(s, name.c_str());
  mat_set_double_field(s, name, v, 0);
}

void mat_set_char_field(matvar_t* s, const std::string& name,
                        const std::string& str, size_t idx)
{
  size_t dims[2];
  dims[0] = 1;
  dims[1] = str.size();
  char* v((char*)str.c_str());
  matvar_t* mStr(Mat_VarCreate(NULL, MAT_C_CHAR, MAT_T_INT8, 2, dims, v, 0));
  if(mStr == NULL)
    throw TASCAR::ErrMsg("Unable to create variable \"" + name + "\".");
  Mat_VarSetStructFieldByName(s, name.c_str(), idx, mStr);
}

void mat_add_char_field(matvar_t* s, const std::string& name,
                        const std::string& str)
{
  Mat_VarAddStructField(s, name.c_str());
  mat_set_char_field(s, name, str, 0);
}

void datalogging_t::save_session_related_meta_data(mat_t* matfp,
                                                   const std::string& fname)
{
  mat_add_strvar(matfp, "tascarversion", TASCARVERSION);
  mat_add_strvar(matfp, "trialid", filename);
  mat_add_strvar(matfp, "filename", fname);
  mat_add_strvar(matfp, "savedat", datestr());
  mat_add_strvar(matfp, "tscfilename", session->get_file_name());
  mat_add_strvar(matfp, "tscpath", session->get_session_path());
  mat_add_strvar(matfp, "sourcexml", session->doc->write_to_string_formatted());
  mat_add_double(matfp, "fragsize", fragsize);
  mat_add_double(matfp, "srate", srate);
}

matvar_t* create_message_struct(const std::vector<label_t>& msg)
{
  size_t dims[2];
  dims[0] = msg.size();
  dims[1] = 1;
  const char* fieldnames[3] = {"t_tascar", "t_lsl", "message"};
  matvar_t* matDataStruct(Mat_VarCreateStruct(NULL, 2, dims, fieldnames, 3));
  if(matDataStruct == NULL)
    throw TASCAR::ErrMsg("Unable to create message variable.");
  for(uint32_t c = 0; c < msg.size(); ++c) {
    mat_set_double_field(matDataStruct, "t_tascar", msg[c].t1, c);
    mat_set_double_field(matDataStruct, "t_lsl", msg[c].t2, c);
    mat_set_char_field(matDataStruct, "message", msg[c].msg, c);
  }
  return matDataStruct;
}

void datalogging_t::save_mat(const std::string& filename)
{
  // first create mat file:
  mat_t* matfp;
  std::string fname(outputdir + nice_name(filename, "mat"));
  matfp = Mat_CreateVer(fname.c_str(), NULL, MAT_FT_MAT5);
  if(NULL == matfp)
    throw TASCAR::ErrMsg("Unable to create file \"" + fname + "\".");
  // if anything fails, try to close file.
  try {
    // now, store session-related meta data:
    save_session_related_meta_data(matfp, fname);
    // store recorded variables:
    size_t dims[2];
    for(uint32_t k = 0; k < recorder.size(); k++) {
      std::string name(nice_name(recorder[k]->get_name()));
      matvar_t* mvar(NULL);
      if(recorder[k]->is_textdata()) {
        mvar = create_message_struct(recorder[k]->get_textdata());
      } else {
        std::vector<double> data(recorder[k]->get_data());
        uint32_t N(recorder[k]->get_size());
        uint32_t M(data.size());
        dims[1] = M / N;
        dims[0] = N;
        double* x(&(data[0]));
        mvar = Mat_VarCreate(name.c_str(), MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims,
                             x, 0);
      }
      if(mvar == NULL)
        throw TASCAR::ErrMsg("Unable to create variable \"" + name + "\".");
      Mat_VarWrite(matfp, mvar, MAT_COMPRESSION_NONE);
      Mat_VarFree(mvar);
    }
    // add LSL variable meta data:
    for(lslvarlist_t::iterator it = lslvars.begin(); it != lslvars.end();
        ++it) {
      mat_add_double(matfp, (*it)->recorder->get_name() + "_stream_delta_start",
                     (*it)->stream_delta_start);
      mat_add_double(matfp, (*it)->recorder->get_name() + "_stream_delta_end",
                     (*it)->stream_delta_end);
      mat_add_strvar(matfp, (*it)->recorder->get_name() + "_info",
                     (*it)->get_xml().c_str());
    }
  }
  catch(...) {
    // close mat file, to have as much written as possible.
    Mat_Close(matfp);
    throw;
  }
  Mat_Close(matfp);
}

void datalogging_t::save_matcell(const std::string& filename)
{
  // first create mat file:
  mat_t* matfp;
  std::string fname(outputdir + nice_name(filename, "mat"));
  matfp = Mat_CreateVer(fname.c_str(), NULL, MAT_FT_MAT5);
  if(NULL == matfp)
    throw TASCAR::ErrMsg("Unable to create file \"" + fname + "\".");
  // if anything fails, try to close file.
  try {
    // now, store session-related meta data:
    save_session_related_meta_data(matfp, fname);
    // store recorded variables:
    size_t dims[2];
    dims[0] = recorder.size();
    dims[1] = 1;
    matvar_t* cell_array(
        Mat_VarCreate("data", MAT_C_CELL, MAT_T_CELL, 2, dims, NULL, 0));
    if(cell_array == NULL)
      throw TASCAR::ErrMsg("Unable to create data cell array.");
    matvar_t* cell_array_names(
        Mat_VarCreate("names", MAT_C_CELL, MAT_T_CELL, 2, dims, NULL, 0));
    if(cell_array == NULL)
      throw TASCAR::ErrMsg("Unable to create data cell array.");
    for(uint32_t k = 0; k < recorder.size(); k++) {
      std::string name(nice_name(recorder[k]->get_name()));
      // create data cell entry as struct:
      const char* fieldnames[2] = {"data", "name"};
      dims[0] = 1;
      dims[1] = 1;
      matvar_t* matDataStruct(
          Mat_VarCreateStruct(NULL, 2, dims, fieldnames, 2));
      if(matDataStruct == NULL)
        throw TASCAR::ErrMsg("Unable to create variable \"" + name + "\".");
      matvar_t* mData(NULL);
      if(recorder[k]->is_textdata()) {
        mData = create_message_struct(recorder[k]->get_textdata());
      } else {
        std::vector<double> data(recorder[k]->get_data());
        uint32_t N(recorder[k]->get_size());
        uint32_t M(data.size());
        dims[1] = M / N;
        dims[0] = N;
        double* x(&(data[0]));
        mData = Mat_VarCreate(NULL, MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, x, 0);
      }
      if(mData == NULL)
        throw TASCAR::ErrMsg("Unable to create variable \"" + name + "\".");
      Mat_VarSetStructFieldByName(matDataStruct, "data", 0, mData);
      dims[0] = 1;
      dims[1] = name.size();
      char* v((char*)name.c_str());
      matvar_t* mStr(
          Mat_VarCreate(NULL, MAT_C_CHAR, MAT_T_INT8, 2, dims, v, 0));
      if(mStr == NULL)
        throw TASCAR::ErrMsg("Unable to create variable \"" + name + "\".");
      Mat_VarSetStructFieldByName(matDataStruct, "name", 0, mStr);
      // test if this is an LSL variable, if yes, provide some header
      // information: here would come some header information...
      for(lslvarlist_t::iterator it = lslvars.begin(); it != lslvars.end();
          ++it)
        if(((*it)->recorder == recorder[k]) && ((*it)->inlet)) {
          mat_add_char_field(matDataStruct, "lsl_name",
                             (*it)->inlet->info().name());
          mat_add_char_field(matDataStruct, "lsl_type",
                             (*it)->inlet->info().type());
          mat_add_double_field(matDataStruct, "lsl_srate",
                               (*it)->inlet->info().nominal_srate());
          mat_add_char_field(matDataStruct, "lsl_source_id",
                             (*it)->inlet->info().source_id());
          mat_add_char_field(matDataStruct, "lsl_hostname",
                             (*it)->inlet->info().hostname());
          mat_add_double_field(matDataStruct, "lsl_protocolversion",
                               (*it)->inlet->info().version());
          mat_add_double_field(matDataStruct, "lsl_libraryversion",
                               lsl::library_version());
          mat_add_double_field(matDataStruct, "stream_delta_start",
                               (*it)->stream_delta_start);
          mat_add_double_field(matDataStruct, "stream_delta_end",
                               (*it)->stream_delta_end);
          mat_add_char_field(matDataStruct, "lsl_info", (*it)->get_xml());
        }
      // here add var to cell array!
      Mat_VarSetCell(cell_array, k, matDataStruct);
      matvar_t* mStrName(
          Mat_VarCreate(NULL, MAT_C_CHAR, MAT_T_INT8, 2, dims, v, 0));
      if(mStrName == NULL)
        throw TASCAR::ErrMsg("Unable to create variable \"" + name + "\".");
      Mat_VarSetCell(cell_array_names, k, mStrName);
    }
    // add data cell array to Mat file:
    Mat_VarWrite(matfp, cell_array, MAT_COMPRESSION_NONE);
    Mat_VarWrite(matfp, cell_array_names, MAT_COMPRESSION_NONE);
    Mat_VarFree(cell_array);
    Mat_VarFree(cell_array_names);
  }
  catch(...) {
    Mat_Close(matfp);
    throw;
  }
  Mat_Close(matfp);
}

REGISTER_MODULE(datalogging_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
