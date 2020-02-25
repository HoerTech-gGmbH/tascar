#include <tascar/receivermod.h>
#include <tascar/delayline.h>

/*
  This plugin implements a receiver with a simple "spatial window",
  i.e., sound from the frontal direction is passed trough, whereas
  other directions are completely blocked.

  Receiver plugins inherit from
  TASCAR::receivermod_base_t. Loudspeaker based rendering methods can
  alternatively inherit from TASCAR::receivermod_base_speaker_t. In
  that case, the methods add_diffuse_sound_field() does
  not need to be implemented.

  To configure the number of output channels, you need to implement
  the configure method. In that method, the receiver plugin can be
  initialized based on the members of the chunk_cfg_t member
  variables. To report the number of output channels, the member
  n_channels need to be changed to the correct value.
 */
class receiverexample_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    /*
      A receiver module needs to provide a data class which is keeping
      the internal state for each source. The create_data() method
      will be called to generate instances of this type. The data will
      be cleaned up automatically in the end.
     */
    data_t();
    double weight;
  };
  // construct a receiver based on the XML configuration of the receiver element
  receiverexample_t(xmlpp::Element* xmlsrc);
  // add one point source - this is the core panning/rendering function
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  // add a First-Order-Ambisonics diffuse sound field:
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  // configure plugins, including number of output channels. To report
  // the number of output channels, the member n_channels need to be
  // changed to the correct value.
  void configure();
  // register OSC variables:
  void add_variables(TASCAR::osc_server_t* srv);
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
private:
  double start_angle; //< start angle in radians
};

receiverexample_t::data_t::data_t()
  : weight(0)
{
}

receiverexample_t::receiverexample_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),
    start_angle(10*DEG2RAD) // default value is 10 degrees
{
  // register XML variable:
  GET_ATTRIBUTE_DEG( start_angle );
}

// Implementation of TASCAR::receivermod_base_t::add_variables()
void receiverexample_t::add_variables(TASCAR::osc_server_t* srv)
{
  // register OSC variable:
  srv->add_double_degree( "/start_angle", &start_angle );
}

// this is the core rendering function. This function will be called
// for each primary or image source (keep this always in mind - this
// is typically the most important bottle neck in performance).
void receiverexample_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  // convert data_t base class pointer to our data_t* pointer:
  data_t* d((data_t*)sd);
  // get the normalized relative position of the source:
  TASCAR::pos_t prel_norm(prel.normal());
  // now, prel_norm.x is the cosine of the angular distance of the
  // source. Use a threshold:
  double start_cos(cos(start_angle));
  // calculate panning weight (as incremental values). weight is the
  // value which should be reached at the end of the chunk:
  double weight(prel_norm.x > start_cos);
  double dweight((weight - d->weight)*t_inc);
  // apply panning:
  uint32_t N(chunk.size());
  for(uint32_t k=0;k<N;++k)
    output[0][k] += chunk[k] * (d->weight+=dweight);
}

void receiverexample_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
  // this algoithm will put a cardioid pattern in frontal direction:
  float* o_l(output[0].d);
  const float* i_w(chunk.w().d);
  const float* i_x(chunk.x().d);
  for(uint32_t k=0;k<chunk.size();++k){
    // sum of w and x is cardioid:
    *o_l += *i_w + *i_x;
    ++o_l;
    ++i_w;
    ++i_x;
  }
}

void receiverexample_t::configure()
{
  // this receiver type returns always exactly one channel:
  n_channels = 1;
  // also all other members of chunk_cfg_t can be used to configure
  // this plugin, e.g., fragment size or sampling rate.
}

TASCAR::receivermod_base_t::data_t* receiverexample_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t();
}

REGISTER_RECEIVERMOD(receiverexample_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
