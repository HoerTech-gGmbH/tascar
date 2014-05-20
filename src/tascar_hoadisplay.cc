/**
   \file tascar_hoadisplay.cc
   \ingroup apptascar
   \brief Display a azimuth-frequency map of a HOA signal
   \author Giso Grimm
   \date 2014

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

#include "jackclient.h"
#include <gtkmm.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/drawingarea.h>
#include <cairomm/context.h>
#include <getopt.h>
#include <iostream>
#include <complex.h>
#include "audiochunks.h"
#include "defs.h"

class gammatone_t 
{
public:
  gammatone_t(double f, double bw, double fs, uint32_t order);
  float complex filter(float complex x);
private:
  float complex A;
  float B;
  float complex state;
  uint32_t order;
};

gammatone_t::gammatone_t(double f, double bw, double fs, uint32_t order_)
  : state(0),
    order(order_)
{
  // filter design after: V. Hohmann (2002). Frequency analysis and
  // synthesis using a Gammatone filterbank. Acta Acustica united with
  // Acustica vol. 88
  float u = pow(2.0,-1.0/order);
  float ct = cos(M_PI*bw/fs);
  float ct2 = ct*ct;
  float lambda = (sqrt((ct2-1.0)*u*u+(2.0-2.0*ct)*u)+ct*u-1.0)/(u-1.0);
  A = lambda*cexp(I*M_PI*f/fs);
  B = pow(2.0,1.0/order)*(1.0-cabs(A));
}

float complex gammatone_t::filter(float complex x)
{
  for(uint32_t o=0;o<order;o++){
    state *= A;
    state += B*x;
    x = state;
  }
  return x;
}

class achannel_t : public TASCAR::wave_t
{
public:
  achannel_t(float azrad,uint32_t hoa_order,uint32_t bands,float fmin,float fmax,float fs);
  void process(jack_nframes_t n,const std::vector<float*>& input);
private:
  TASCAR::wave_t decoder;
  std::vector<gammatone_t> fb;
  TASCAR::wave_t lpstate;
};

achannel_t::achannel_t(float azrad,uint32_t hoa_order,uint32_t bands,float fmin,float fmax,float fs)
  : TASCAR::wave_t(bands),
    decoder(2*hoa_order+1),
    lpstate(bands)
{
  float f_ratio(pow(fmax/fmin,1.0/(double)(bands-1)));
  for(uint32_t k=0;k<bands;k++){
    float fc(fmin*pow(fmax/fmin,(double)k/(double)(bands-1)));
    float bw(fc*f_ratio-fc/f_ratio);
    fb.push_back(gammatone_t(fc, bw, fs, 2));
  }
  for(uint32_t o=1;o<=hoa_order;o++){
    decoder[2*o-1] = cos(o*azrad);
    decoder[2*o] = sin(o*azrad);
  }
  decoder[0] = 1.0/sqrt(2.0);
}

void achannel_t::process(jack_nframes_t n,const std::vector<float*>& input)
{
  for(uint32_t k=0;k<n;k++){
    // decode ambisonics signal:
    float y(0.0f);
    for(uint32_t ko=0;ko<decoder.size();ko++)
      y += input[ko][k]*decoder[ko];
    // 
    for(uint32_t b=0;b<fb.size();b++){
      float by(cabsf(fb[b].filter(y)));
      lpstate[b] *= 0.9999;
      by = lpstate[b] += 0.0001*by*by;
      operator[](b) = sqrt(by);
    }
  }
}

class hoadisplay_t : public jackc_t, public Gtk::DrawingArea
{
public:
  hoadisplay_t(const std::string& jackname,
               uint32_t order,
               uint32_t channels,
               float bands_per_octave,
               float fmin,
               float fmax);
  ~hoadisplay_t();
  virtual int process(jack_nframes_t n,const std::vector<float*>& input,
                      const std::vector<float*>& output);
protected:
  virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
#ifdef GTKMM24
  virtual bool on_expose_event(GdkEventExpose* event);
#endif
  bool on_timeout();
private:
  uint32_t order;
  uint32_t channels;
  float bands_per_octave;
  float fmin;
  float fmax;
  uint32_t bands;
public:
  Glib::RefPtr<Gdk::Pixbuf> image;
private:
  pthread_mutex_t mtx_scene;
  std::vector<achannel_t> analyzer;
};

hoadisplay_t::hoadisplay_t(const std::string& jackname,
                           uint32_t order_,
                           uint32_t channels_,
                           float bands_per_octave_,
                           float fmin_,
                           float fmax_)
  : jackc_t(jackname),
    order(order_),
    channels(channels_),
    bands_per_octave(bands_per_octave_),
    fmin(fmin_),
    fmax(fmax_),
    bands(bands_per_octave*log2(fmax/fmin)+1.0)
{
  for(uint32_t k=0;k<channels;k++)
    analyzer.push_back(achannel_t((double)k*PI2/(double)channels,order,bands,fmin,fmax,srate));
  image = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB,false,8,channels,bands);
  Glib::signal_timeout().connect( sigc::mem_fun(*this, &hoadisplay_t::on_timeout), 60 );
#ifdef GTKMM30
  signal_draw().connect(sigc::mem_fun(*this, &hoadisplay_t::on_draw), false);
#else
  signal_expose_event().connect(sigc::mem_fun(*this, &hoadisplay_t::on_expose_event), false);
#endif
  pthread_mutex_init( &mtx_scene, NULL );
  set_size_request(channels,bands);
  for(uint32_t k=0;k<2*order+1;k++){
    char ctmp[1024];
    uint32_t lorder((k+1)/2);
    int32_t ldeg(2*lorder*(((k & 1) > 0)-0.5));
    sprintf(ctmp,"in_%d.%d",lorder,ldeg);
    add_input_port(ctmp);
  }
}

hoadisplay_t::~hoadisplay_t()
{
  image.clear();
  pthread_mutex_trylock( &mtx_scene );
  pthread_mutex_unlock(  &mtx_scene);
  pthread_mutex_destroy( &mtx_scene );
}

#ifdef GTKMM24
bool hoadisplay_t::on_expose_event(GdkEventExpose* event)
{
  if( event ){
    // This is where we draw on the window
    Glib::RefPtr<Gdk::Window> window = wdg_scenemap.get_window();
    if(window){
      Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context();
      return on_draw(cr);
    }
  }
  return true;
}
#endif


bool hoadisplay_t::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  pthread_mutex_lock( &mtx_scene );
  try{
    float vmax(1e-7);
    for(uint32_t k=0;k<analyzer.size();k++)
      for(uint32_t b=0;b<bands;b++)
        vmax = std::max(vmax,analyzer[k][b]);
    guint8* pixels = image->get_pixels();
    for(uint32_t k=0;k<analyzer.size();k++)
      for(uint32_t b=0;b<bands;b++){
        uint32_t pix(k+b*analyzer.size());
        float val(analyzer[k][b]/vmax);
        pixels[3*pix] = val*255;
        pixels[3*pix+1] = val*255;
        pixels[3*pix+2] = val*255;
      }
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();
    cr->save();
    cr->scale((double)width/(double)channels,(double)height/(double)bands);
    Gdk::Cairo::set_source_pixbuf(cr, image,
                                  0,0);
    //(width - image->get_width())/2, (height - image->get_height())/2);
    cr->paint();
    cr->restore();
    pthread_mutex_unlock( &mtx_scene );
  }
  catch(...){
    pthread_mutex_unlock( &mtx_scene );
    throw;
  }
  return true;
}

bool hoadisplay_t::on_timeout()
{
  Glib::RefPtr<Gdk::Window> win = get_window();
  if (win){
    Gdk::Rectangle r(0,0, 
		     get_allocation().get_width(),
		     get_allocation().get_height() );
    win->invalidate_rect(r, true);
  }
  return true;
}

int hoadisplay_t::process(jack_nframes_t nframes,
                          const std::vector<float*>& inBuffer,
                          const std::vector<float*>& outBuffer)
{
  for(uint32_t k=0;k<analyzer.size();k++)
    analyzer[k].process(nframes,inBuffer);
  return 0;
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_gui -c configfile [options]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

int main(int argc, char** argv)
{
  Gtk::Main kit(argc, argv);
  Gtk::Window win;
  std::string jackname("hoadisplay");
  uint32_t order(1);
  uint32_t channels(32);
  float bpoctave(1);
  float fmin(125);
  float fmax(8000);
  const char *options = "hj:o:c:b:l:u:";
  struct option long_options[] = { 
    { "help",     0, 0, 'h' },
    { "jackname", 1, 0, 'j' },
    { "order",    1, 0, 'o' },
    { "channels", 1, 0, 'c' },
    { "bpoctave", 1, 0, 'b' },
    { "fmin",     1, 0, 'l' },
    { "fmax",     1, 0, 'u' },
    { 0, 0, 0, 0 }
  };
  int opt(0);
  int option_index(0);
  while( (opt = getopt_long(argc, argv, options,
                            long_options, &option_index)) != -1){
    switch(opt){
    case 'h':
      usage(long_options);
      return -1;
    case 'j':
      jackname = optarg;
      break;
    case 'o':
      order = atoi(optarg);
      break;
    case 'c':
      channels = atoi(optarg);
      break;
    case 'b':
      bpoctave = atof(optarg);
      break;
    case 'l':
      fmin = atof(optarg);
      break;
    case 'u':
      fmax = atof(optarg);
      break;
    }
  }
  win.set_title("tascar hoadisplay");
  hoadisplay_t c(jackname,order,channels,bpoctave,fmin,fmax);
  win.add(c);
  win.set_default_size(1024,768);
  win.show_all();
  c.jackc_t::activate();
  Gtk::Main::run(win);
  c.jackc_t::deactivate();
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
