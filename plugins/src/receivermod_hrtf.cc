/*

  HRTF simulation.

This receiver describes the main features of measured head related
transfer functions by using a few low-order digital filters. The
parametrization is based on the Spherical Head Model (SHM) by
Brown & Duda (1998) and includes three further low-order filters.

The Duda SHM was extended by O. Buttler and S.D. Ewert in the context
of room acoustics simulator RAZR (Wendt, 2014; www.razrengine.com)
in Buttler (2018) to improve left-right, front-back, and
elevation perception.

In order to optimize the values for the filter parameters of the
original RAZR SHM-Model, the frequency response of the receiver has
been fitted to measured HRTFs of the KEMAR dummy head (Schwark, 2020)
provided by the OlHeaD-HRTF database (Denk & Kollmeier, 2020).

This implementation was written by Fenja Schwark.


C Phillip Brown and Richard O Duda. A structural model for binaural
  sound synthesis. IEEE Transition on Speech and Audio Processing,
  6(5):476–488, 1998. doi: 10.1109/89.709673.  URL
  https://doi.org/10.1109/89.709673.

Torben Wendt, Steven van de Par, and Stephan Ewert. A
  Computationally-Efficient and Perceptually-Plausible Algorithm for
  Binaural Room Impulse Response Simulation. Jour- nal of the Audio
  Engineering Society, 62(11):748–766, dec 2014. ISSN 15494950.  doi:
  10.17743/jaes.2014.0042. URL
  http://www.aes.org/e-lib/browse.cfm?elib= 17550.

Olliver Buttler. Optimierung und erweiterung einer effizienten methode
  zur synthese binau- raler raumimpulsantworten. MSc thesis, 2018.

Fenja Schwark. Data-driven optimization of parameterized head related
  transfer functions for the implementation in a real-time virtual
  acoustic rendering framework. BSc thesis, 2020.

Florian Denk and Birger Kollmeier. The hearpiece database of
  individual transfer functions of an openly available in-the-ear
  earpiece for hearing device research. https://zenodo.
  org/record/3900114, 2020.


 */

#include "delayline.h"
#include "filterclass.h"
#include "receivermod.h"
#include <random>

const std::complex<double> i(0.0, 1.0);

class hrtf_param_t : public TASCAR::xml_element_t {
public:
  hrtf_param_t(xmlpp::Element* xmlsrc);
  uint32_t sincorder;
  double c;
  TASCAR::pos_t dir_l;
  TASCAR::pos_t dir_r;
  TASCAR::pos_t dir_front;
  double radius;
  double angle;
  double thetamin; // minimum of the filter modelling the head shadow effect
  double omega;    // cut-off frequency of HSM
  double
      alphamin; // parameter which determines maximal depth of SHM at thetamin
  double startangle_front; // define range in which filter modelling pinna
                           // shadow operates
  double omega_front;      // cut-off frequency of pinna shadow filter
  double alphamin_front;   // parameter which determines maximal depth of pinna
                           // shadow filter
  double startangle_up; // define range in which filter modelling torso shadow
                        // operates
  double omega_up;      // cut-off frequency of torso shadow filter
  double alphamin_up;   // parameter which determines maximal depth of torso
                        // shadow filter
  double
      startangle_notch; // define range in which parametric equalizer operates
  double freq_start;    // notch frequency at startangle_notch
  double freq_end;      // notch frequency at 90 degr elev
  double maxgain; // gain applied at 90 degr elev (0 dB gain at startangle_notch
                  // and linear increase)
  double Q_notch; // inverse Q factor of the parametric equalizer
  bool diffuse_hrtf; // apply hrtf model also to diffuse rendering
};

hrtf_param_t::hrtf_param_t(xmlpp::Element* xmlsrc)
    : TASCAR::xml_element_t(xmlsrc), sincorder(0), c(340), dir_l(1, 0, 0),
      dir_r(1, 0, 0), dir_front(1, 0, 0), radius(0.08),
      angle(90 * DEG2RAD), // ears modelled at +/- 90 degree
      thetamin(160 * DEG2RAD), omega(3100), alphamin(0.14), startangle_front(0),
      omega_front(11200), alphamin_front(0.39), startangle_up(135 * DEG2RAD),
      //    omega_up(c/radius/2),
      alphamin_up(0.1), startangle_notch(102 * DEG2RAD), freq_start(1300),
      freq_end(650), maxgain(-5.4), Q_notch(2.3), diffuse_hrtf(false)
{
  GET_ATTRIBUTE(sincorder, "", "Sinc interpolation order of ITD delay line");
  GET_ATTRIBUTE(c, "m/s", "Speed of sound");
  GET_ATTRIBUTE(radius, "m", "Radius of sphere modeling the head");
  GET_ATTRIBUTE_DEG(angle, "Position of the ears on the sphere");
  GET_ATTRIBUTE_DEG(
      thetamin, "angle with respect to the position of the ears at which the "
                "maximum depth of the high-shelf realizing the SHM is reached");
  GET_ATTRIBUTE(omega, "Hz",
                "cut-off frequency of the high-self realizing the SHM");
  GET_ATTRIBUTE(alphamin, "",
                "parameter which determines the depth of the high-shelf "
                "realizing the SHM");
  GET_ATTRIBUTE_DEG(startangle_front,
                    "the second high-shelf, e.g. to model pinna shadow effect, "
                    "is applied when the angle with respect to front direction "
                    "[1 0 0] is larger than \\attr{startangle_front}");
  GET_ATTRIBUTE(omega_front, "Hz", "cut-off frequency of the second high-self");
  GET_ATTRIBUTE(
      alphamin_front, "",
      "parameter which determines the depth of the second high-shelf");
  GET_ATTRIBUTE_DEG(startangle_up,
                    "the third high-shelf which models the shadow effect of "
                    "the torso is applied when the angle with respect to up "
                    "direction [0 0 1] is larger than \\attr{startangle_up}");
  omega_up = c / radius / 2;
  GET_ATTRIBUTE(omega_up, "Hz",
                "cut-off frequency of the second high-shelf in Hz");
  GET_ATTRIBUTE(
      alphamin_up, "",
      "parameter which determines the depth of the second high-shelf");
  GET_ATTRIBUTE_DEG(
      startangle_notch,
      "notch filter to model concha notch is applied if angle with respect to "
      "up direction [0 0 1] is smaller than \\attr{startangle_notch}");
  GET_ATTRIBUTE(freq_start, "Hz",
                "notch center frequency at \\attr{startangle_notch}");
  GET_ATTRIBUTE(freq_end, "Hz", "notch center frequency at [0 0 1]");
  GET_ATTRIBUTE(maxgain, "dB",
                "gain applied at [0 0 1] -- gain is 0 dB at "
                "\\attr{startangle_notch} and increases linearly");
  GET_ATTRIBUTE(Q_notch, "", "quality factor of the notch filter");
  dir_l.rot_z(angle);
  dir_r.rot_z(-angle);
  GET_ATTRIBUTE_BOOL(diffuse_hrtf,
                     "apply hrtf model also to diffuse rendering");
}

class hrtf_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(double srate, uint32_t chunksize, hrtf_param_t& par_plugin);
    void filterdesign(const double theta_left, const double theta_right,
                      const double theta_front, const double elevation);
    inline void filter(const float& input)
    {
      out_l = (state_l = (dline_l.get_dist_push(tau_l, input)));
      out_r = (state_r = (dline_r.get_dist_push(tau_r, input)));
      out_l = bqelev_l.filter(bqazim_l.filter(out_l));
      out_r = bqelev_r.filter(bqazim_r.filter(out_r));
    }
    void set_param(const TASCAR::pos_t& prel_norm);
    double fs;
    double dt;
    hrtf_param_t& par;
    TASCAR::varidelay_t dline_l;
    TASCAR::varidelay_t dline_r;
    TASCAR::biquad_t bqazim_l;
    TASCAR::biquad_t bqazim_r;
    TASCAR::biquad_t bqelev_l;
    TASCAR::biquad_t bqelev_r;
    double out_l;
    double out_r;
    double state_l;
    double state_r;
    double target_tau_l;
    double target_tau_r;
    double tau_l;
    double tau_r;
    double inv_a0;
    double alpha_l;
    double alpha_r;
    double inv_a0_f;
    double alpha_f;
    double inv_a0_u;
    double alpha_u;
  };
  class diffuse_data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    diffuse_data_t(double srate, uint32_t chunksize, hrtf_param_t& par_plugin);
    hrtf_t::data_t xp, xm, yp, ym, zp, zm;
  };
  hrtf_t(xmlpp::Element* xmlsrc);
  ~hrtf_t();
  void add_pointsource(const TASCAR::pos_t& prel, double width,
                       const TASCAR::wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       receivermod_base_t::data_t*);
  // void tau_woodworth_schlosberg(const double theta, double& tau);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                               std::vector<TASCAR::wave_t>& output,
                               receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_data(double srate, uint32_t fragsize);
  receivermod_base_t::data_t* create_diffuse_data(double srate,
                                                  uint32_t fragsize);
  virtual void configure();
  virtual void release();
  virtual void postproc(std::vector<TASCAR::wave_t>& output);
  virtual void add_variables(TASCAR::osc_server_t* srv);

private:
  hrtf_param_t par;
  double decorr_length;
  bool decorr;
  std::vector<TASCAR::overlap_save_t*> decorrflt;
  std::vector<TASCAR::wave_t*> diffuse_render_buffer;
};

// calculate time delay according to woodworth and schlosberg
void tau_woodworth_schlosberg(const double theta, const double radius,
                              double& tau)
{
  if(theta < 0.5 * M_PI)
    tau = -radius * (cos(theta) - 1.0);
  else
    tau = radius * (theta - 0.5 * M_PI + 1.0);
}

hrtf_t::~hrtf_t() {}

hrtf_t::data_t::data_t(double srate, uint32_t chunksize,
                       hrtf_param_t& par_plugin)
    : fs(srate), dt(1.0 / std::max(1.0, (double)chunksize)), par(par_plugin),
      dline_l(4 * par.radius * srate / par.c + 2 + par.sincorder, srate, par.c,
              par.sincorder, 64),
      dline_r(4 * par.radius * srate / par.c + 2 + par.sincorder, srate, par.c,
              par.sincorder, 64),
      out_l(0), out_r(0), state_l(0), state_r(0), tau_l(0), tau_r(0),
      inv_a0(1.0 / (par.omega + fs)), alpha_l(0.0), alpha_r(0.0),
      inv_a0_f(1.0 / (par.omega_front + fs)), alpha_f(0.0),
      inv_a0_u(1.0 / (par.omega_up + fs)), alpha_u(0.0)
{
}

void hrtf_t::data_t::filterdesign(const double theta_l, const double theta_r,
                                  const double theta_f, const double elevation)
{
  // bqazim_l/r
  // parameter of SHM
  alpha_l = (1.0 + 0.5 * par.alphamin +
             (1.0 - 0.5 * par.alphamin) * cos(theta_l / par.thetamin * M_PI)) *
            fs;
  alpha_r = (1.0 + 0.5 * par.alphamin +
             (1.0 - 0.5 * par.alphamin) * cos(theta_r / par.thetamin * M_PI)) *
            fs;

  if(theta_f > par.startangle_front) // pinna shadow effect
  {
    alpha_f = (1.0 - (1.0 - par.alphamin_front) *
                         (0.5 - cos((theta_f - par.startangle_front) /
                                    (M_PI - par.startangle_front) * M_PI) *
                                    0.5)) *
              fs;
    // alpha_f = (1.0 - ( 1.0-par.alphamin_front ) * ( 0.5-0.5*cos(theta_f) ) )
    // *fs; // if startangle_front == 0

    bqazim_l.set_coefficients(
        (par.omega - fs) * inv_a0 + (par.omega_front - fs) * inv_a0_f,
        (par.omega - fs) * inv_a0 * (par.omega_front - fs) * inv_a0_f,
        (par.omega + alpha_l) * inv_a0 * (par.omega_front + alpha_f) * inv_a0_f,
        (par.omega + alpha_l) * inv_a0 * (par.omega_front - alpha_f) *
                inv_a0_f +
            (par.omega - alpha_l) * inv_a0 * (par.omega_front + alpha_f) *
                inv_a0_f,
        (par.omega - alpha_l) * inv_a0 * (par.omega_front - alpha_f) *
            inv_a0_f);

    bqazim_r.set_coefficients(
        (par.omega - fs) * inv_a0 + (par.omega_front - fs) * inv_a0_f,
        (par.omega - fs) * inv_a0 * (par.omega_front - fs) * inv_a0_f,
        (par.omega + alpha_r) * inv_a0 * (par.omega_front + alpha_f) * inv_a0_f,
        (par.omega + alpha_r) * inv_a0 * (par.omega_front - alpha_f) *
                inv_a0_f +
            (par.omega - alpha_r) * inv_a0 * (par.omega_front + alpha_f) *
                inv_a0_f,
        (par.omega - alpha_r) * inv_a0 * (par.omega_front - alpha_f) *
            inv_a0_f);
  } else {
    bqazim_l.set_coefficients((par.omega - fs) * inv_a0, 0.0,
                              (par.omega + alpha_l) * inv_a0,
                              (par.omega - alpha_l) * inv_a0, 0.0);
    bqazim_r.set_coefficients((par.omega - fs) * inv_a0, 0.0,
                              (par.omega + alpha_r) * inv_a0,
                              (par.omega - alpha_r) * inv_a0, 0.0);
  }

  // bqelev_l/r
  if(elevation <
     par.startangle_notch) // concha notch (parametric equalizer for cut)
  {
    double p_angle = (par.startangle_notch - elevation) / par.startangle_notch;
    double transform_const =
        1.0 /
        tan(M_PI *
            (p_angle * (par.freq_end - par.freq_start) + par.freq_start) / fs);
    double Q_n = transform_const / par.Q_notch;
    double inv_gain_Q = pow(10.0, (-par.maxgain * p_angle / 20.0)) * Q_n;
    double tc_sq = transform_const * transform_const;
    double inv_a0_n = 1.0 / (tc_sq + 1.0 + inv_gain_Q);
    bqelev_l.set_coefficients(
        2.0 * (1.0 - tc_sq) * inv_a0_n, (tc_sq + 1.0 - inv_gain_Q) * inv_a0_n,
        (tc_sq + 1.0 + Q_n) * inv_a0_n, 2.0 * (1.0 - tc_sq) * inv_a0_n,
        (tc_sq + 1.0 - Q_n) * inv_a0_n);
    bqelev_r.set_coefficients(
        2.0 * (1.0 - tc_sq) * inv_a0_n, (tc_sq + 1.0 - inv_gain_Q) * inv_a0_n,
        (tc_sq + 1.0 + Q_n) * inv_a0_n, 2.0 * (1.0 - tc_sq) * inv_a0_n,
        (tc_sq + 1.0 - Q_n) * inv_a0_n);
  } else if(elevation > par.startangle_up) // torso shadow effect
  {
    alpha_u = (1.0 - (1.0 - par.alphamin_up) *
                         (0.5 - 0.5 * cos((elevation - par.startangle_up) /
                                          (M_PI - par.startangle_up) * M_PI))) *
              fs;

    bqelev_l.set_coefficients((par.omega_up - fs) * inv_a0_u, 0.0,
                              (par.omega_up + alpha_u) * inv_a0_u,
                              (par.omega_up - alpha_u) * inv_a0_u, 0.0);
    bqelev_r.set_coefficients((par.omega_up - fs) * inv_a0_u, 0.0,
                              (par.omega_up + alpha_u) * inv_a0_u,
                              (par.omega_up - alpha_u) * inv_a0_u, 0.0);
  } else {
    bqelev_l.set_coefficients(0.0, 0.0, 1.0, 0.0, 0.0);
    bqelev_r.set_coefficients(0.0, 0.0, 1.0, 0.0, 0.0);
  }
}

hrtf_t::diffuse_data_t::diffuse_data_t(double srate, uint32_t chunksize,
                                       hrtf_param_t& par_plugin)
    : xp(srate, chunksize, par_plugin), xm(srate, chunksize, par_plugin),
      yp(srate, chunksize, par_plugin), ym(srate, chunksize, par_plugin),
      zp(srate, chunksize, par_plugin), zm(srate, chunksize, par_plugin)
{
  xp.set_param(TASCAR::pos_t(1, 0, 0));
  xm.set_param(TASCAR::pos_t(-1, 0, 0));
  yp.set_param(TASCAR::pos_t(0, 1, 0));
  ym.set_param(TASCAR::pos_t(0, -1, 0));
  zp.set_param(TASCAR::pos_t(0, 0, 1));
  zm.set_param(TASCAR::pos_t(0, 0, -1));
}

void hrtf_t::configure()
{
  TASCAR::receivermod_base_t::configure();
  n_channels = 2;
  // initialize decorrelation filter:
  decorrflt.clear();
  diffuse_render_buffer.clear();
  uint32_t irslen(decorr_length * f_sample);
  uint32_t paddedirslen((1 << (int)(ceil(log2(irslen + n_fragment - 1)))) -
                        n_fragment + 1);
  for(uint32_t k = 0; k < 2; ++k)
    decorrflt.push_back(new TASCAR::overlap_save_t(paddedirslen, n_fragment));
  TASCAR::fft_t fft_filter(irslen);
  std::mt19937 gen(1);
  std::uniform_real_distribution<double> dis(0.0, 2 * M_PI);
  for(uint32_t k = 0; k < 2; ++k) {
    for(uint32_t b = 0; b < fft_filter.s.n_; ++b)
      fft_filter.s[b] = std::exp(i * dis(gen));
    fft_filter.ifft();
    for(uint32_t t = 0; t < fft_filter.w.n; ++t)
      fft_filter.w[t] *= (0.5 - 0.5 * cos(t * PI2 / fft_filter.w.n));
    decorrflt[k]->set_irs(fft_filter.w, false);
    diffuse_render_buffer.push_back(new TASCAR::wave_t(n_fragment));
  }
  labels.clear();
  labels.push_back("_l");
  labels.push_back("_r");
  // end of decorrelation filter.
}

void hrtf_t::release()
{
  TASCAR::receivermod_base_t::release();
  for(auto it = decorrflt.begin(); it != decorrflt.end(); ++it)
    delete(*it);
  for(auto it = diffuse_render_buffer.begin();
      it != diffuse_render_buffer.end(); ++it)
    delete(*it);
  decorrflt.clear();
  diffuse_render_buffer.clear();
}

void hrtf_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  TASCAR::receivermod_base_t::postproc(output);
  for(uint32_t ch = 0; ch < 2; ++ch) {
    if(decorr)
      decorrflt[ch]->process(*(diffuse_render_buffer[ch]), output[ch], true);
    else
      output[ch] += *(diffuse_render_buffer[ch]);
    diffuse_render_buffer[ch]->clear();
  }
}

// Implementation of TASCAR::receivermod_base_t::add_variables()
void hrtf_t::add_variables(TASCAR::osc_server_t* srv)
{
  TASCAR::receivermod_base_t::add_variables(srv);
  srv->add_bool("/hrtf/decorr", &decorr);
  srv->add_double("/hrtf/angle", &par.angle);
  srv->add_double("/hrtf/thetamin", &par.thetamin);
  srv->add_double("/hrtf/omega", &par.omega);
  srv->add_double("/hrtf/alphamin", &par.alphamin);
  srv->add_double("/hrtf/startangle_front", &par.startangle_front);
  srv->add_double("/hrtf/omega_front", &par.omega_front);
  srv->add_double("/hrtf/alphamin_front", &par.alphamin_front);
  srv->add_double("/hrtf/startangle_up", &par.startangle_up);
  srv->add_double("/hrtf/omega_up", &par.omega_up);
  srv->add_double("/hrtf/alphamin_up", &par.alphamin_up);
  srv->add_double("/hrtf/startangle_notch", &par.startangle_notch);
  srv->add_double("/hrtf/freq_start", &par.freq_start);
  srv->add_double("/hrtf/freq_end", &par.freq_end);
  srv->add_double("/hrtf/maxgain", &par.maxgain);
  srv->add_double("/hrtf/Q_notch", &par.Q_notch);
  srv->add_bool("/hrtf/diffuse_hrtf", &par.diffuse_hrtf);
}

hrtf_t::hrtf_t(xmlpp::Element* xmlsrc)
    : TASCAR::receivermod_base_t(xmlsrc), par(xmlsrc), decorr_length(0.05),
      decorr(false)
{
  GET_ATTRIBUTE(decorr_length, "s", "Decorrelation length");
  GET_ATTRIBUTE_BOOL(decorr, "Flag to use decorrelation of diffuse sounds");
}

void hrtf_t::data_t::set_param(const TASCAR::pos_t& prel_norm)
{
  // angle with respect to front (range [0, pi])
  double theta_front = acos(dot_prod(prel_norm, par.dir_front));
  // angle with respect to left ear (range [0, pi])
  double theta_l = acos(dot_prod(par.dir_l, prel_norm));
  // angle with respect to right ear (range [0, pi])
  double theta_r = acos(dot_prod(par.dir_r, prel_norm));

  // warping for better grip on small angles
  theta_l = theta_l * (1.0 - 0.5 * cos(sqrt(theta_l / M_PI) * M_PI)) / 1.5;
  theta_r = theta_r * (1.0 - 0.5 * cos(sqrt(theta_r / M_PI) * M_PI)) / 1.5;

  // time delay in meter (panning parameters: target_tau is reached at end of
  // the block)
  tau_woodworth_schlosberg(theta_l, par.radius, target_tau_l);
  tau_woodworth_schlosberg(theta_r, par.radius, target_tau_r);
  // calculate delta tau for each panning step

  filterdesign(theta_l, theta_r, theta_front, -(prel_norm.elev() - 0.5 * M_PI));
}

void hrtf_t::add_pointsource(const TASCAR::pos_t& prel, double width,
                             const TASCAR::wave_t& chunk,
                             std::vector<TASCAR::wave_t>& output,
                             receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  d->set_param(prel.normal());
  // calculate delta tau for each panning step
  double dtau_l((d->target_tau_l - d->tau_l) * d->dt);
  double dtau_r((d->target_tau_r - d->tau_r) * d->dt);

  // apply panning:
  uint32_t N(chunk.size());
  for(uint32_t k = 0; k < N; ++k) {
    float v(chunk[k]);
    d->filter(v);
    output[0][k] += d->out_l;
    output[1][k] += d->out_r;
    d->tau_l += dtau_l;
    d->tau_r += dtau_r;
  }
  // explicitely apply final values, to avoid rounding errors:
  d->tau_r = d->target_tau_r;
  d->tau_l = d->target_tau_l;
}

void hrtf_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                                     std::vector<TASCAR::wave_t>& output,
                                     receivermod_base_t::data_t* sd)
{
  hrtf_t::diffuse_data_t* d((hrtf_t::diffuse_data_t*)sd);
  if(par.diffuse_hrtf) {
    float* o_l(diffuse_render_buffer[0]->d);
    float* o_r(diffuse_render_buffer[1]->d);
    const float* i_w(chunk.w().d);
    const float* i_x(chunk.x().d);
    const float* i_y(chunk.y().d);
    const float* i_z(chunk.z().d);
    // decode diffuse sound field in microphone directions:
    for(uint32_t k = 0; k < chunk.size(); ++k) {
      // FOA decoding into main axes:
      d->xp.filter(0.70711f * *i_w + *i_x);
      d->xm.filter(0.70711f * *i_w - *i_x);
      d->yp.filter(0.70711f * *i_w + *i_y);
      d->ym.filter(0.70711f * *i_w - *i_y);
      d->zp.filter(0.70711f * *i_w + *i_z);
      d->zm.filter(0.70711f * *i_w - *i_z);
      *o_l += 0.25f * (d->xp.out_l + d->xm.out_l + d->yp.out_l + d->ym.out_l +
                       d->zp.out_l + d->zm.out_l);
      *o_r += 0.25f * (d->xp.out_r + d->xm.out_r + d->yp.out_r + d->ym.out_r +
                       d->zp.out_r + d->zm.out_r);
      ++o_l;
      ++o_r;
      ++i_w;
      ++i_x;
      ++i_y;
      ++i_z;
    }
  } else {
    float* o_l(diffuse_render_buffer[0]->d);
    float* o_r(diffuse_render_buffer[1]->d);
    const float* i_w(chunk.w().d);
    const float* i_x(chunk.x().d);
    const float* i_y(chunk.y().d);
    // decode diffuse sound field in microphone directions:
    for(uint32_t k = 0; k < chunk.size(); ++k) {
      *o_l += *i_w + par.dir_l.x * (*i_x) + par.dir_l.y * (*i_y);
      *o_r += *i_w + par.dir_r.x * (*i_x) + par.dir_r.y * (*i_y);
      ++o_l;
      ++o_r;
      ++i_w;
      ++i_x;
      ++i_y;
    }
  }
}

TASCAR::receivermod_base_t::data_t* hrtf_t::create_data(double srate,
                                                        uint32_t fragsize)
{
  return new data_t(srate, fragsize, par);
}

TASCAR::receivermod_base_t::data_t*
hrtf_t::create_diffuse_data(double srate, uint32_t fragsize)
{
  return new diffuse_data_t(srate, fragsize, par);
}

REGISTER_RECEIVERMOD(hrtf_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
