#include <tascar/sourcemod.h>
#include <tascar/errorhandling.h>

/*
  This plugin implements a source with a simple "spatial window",
  i.e., sound is radiated to the frontal direction, whereas other
  directions are completely blocked.

  Source plugins inherit from TASCAR::sourcemod_base_t.
 */
class sourceexample_t : public TASCAR::sourcemod_base_t {
public:
  class data_t : public TASCAR::sourcemod_base_t::data_t {
  public:
    data_t();
    double weight;
  };
  sourceexample_t(xmlpp::Element* xmlsrc);
  // read_source is the main rendering function, called for normal
  // receivers. For volumetric receivers, always an omni-directional
  // radiation pattern is assumed. This can be changed by implementing
  // the method read_source_diffuse() with the same interface as
  // read_source().
  bool read_source(TASCAR::pos_t& prel, const std::vector<TASCAR::wave_t>& input, TASCAR::wave_t& output, sourcemod_base_t::data_t*);
  void configure();
  // register OSC variables:
  void add_variables(TASCAR::osc_server_t* srv);
  TASCAR::sourcemod_base_t::data_t* create_data(double srate,uint32_t fragsize);
private:
  double start_angle; //< start angle in radians
};

sourceexample_t::data_t::data_t()
  : weight(0)
{
}

sourceexample_t::sourceexample_t(xmlpp::Element* xmlsrc)
  : TASCAR::sourcemod_base_t(xmlsrc),
    start_angle(10*DEG2RAD) // default value is 10 degrees
{
  // register XML variable:
  GET_ATTRIBUTE_DEG( start_angle );
}

// Implementation of TASCAR::receivermod_base_t::add_variables()
void sourceexample_t::add_variables(TASCAR::osc_server_t* srv)
{
  // register OSC variable:
  srv->add_double_degree( "/start_angle", &start_angle );
}

bool sourceexample_t::read_source(TASCAR::pos_t& prel, const std::vector<TASCAR::wave_t>& input, TASCAR::wave_t& output, sourcemod_base_t::data_t* sd)
{
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
  uint32_t N(output.size());
  for(uint32_t k=0;k<N;++k)
    output[k] = input[0][k] * (d->weight+=dweight);
  return false;
}

void sourceexample_t::configure()
{
  // this source model requires exactly one audio channel:
  if( n_channels != 1 )
    throw TASCAR::ErrMsg("his source model requires one input channel, received "+TASCAR::to_string(n_channels)+".");
}

TASCAR::sourcemod_base_t::data_t* sourceexample_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t();
}

REGISTER_SOURCEMOD(sourceexample_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
