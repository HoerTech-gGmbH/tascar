#include "errorhandling.h"
#include "scene.h"

class nsp_t : public TASCAR::receivermod_base_speaker_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t( uint32_t chunksize,uint32_t channels );
    virtual ~data_t();
    // point source speaker weights:
    float* point_w;
    float* point_dw;
    // ambisonic weights:
    float* diff_w;
    float* diff_dw;
    float* diff_x;
    float* diff_dx;
    float* diff_y;
    float* diff_dy;
    float* diff_z;
    float* diff_dz;
    double dt;
  };
  nsp_t( xmlpp::Element* xmlsrc );
  virtual ~nsp_t() {};
  void add_pointsource( const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* );
  receivermod_base_t::data_t* create_data( double srate,uint32_t fragsize );
  void add_variables( TASCAR::osc_server_t* srv );
  bool useall;
};


nsp_t::data_t::data_t( uint32_t chunksize,uint32_t channels )
{
  point_w = new float[channels];
  point_dw = new float[channels];
  diff_w = new float[channels];
  diff_dw = new float[channels];
  diff_x = new float[channels];
  diff_dx = new float[channels];
  diff_y = new float[channels];
  diff_dy = new float[channels];
  diff_z = new float[channels];
  diff_dz = new float[channels];
  for( uint32_t k=0;k<channels;k++ )
    point_w[k] = point_dw[k] = diff_w[k] = diff_dw[k] = diff_x[k] = 
      diff_dx[k] = diff_y[k] = diff_dy[k] = diff_z[k] = diff_dz[k] = 0;
  dt = 1.0/std::max( 1.0,(double )chunksize);
}

nsp_t::data_t::~data_t()
{
  delete [] point_w;
  delete [] point_dw;
  delete [] diff_w;
  delete [] diff_dw;
  delete [] diff_x;
  delete [] diff_dx;
  delete [] diff_y;
  delete [] diff_dy;
  delete [] diff_z;
  delete [] diff_dz;
}

nsp_t::nsp_t( xmlpp::Element* xmlsrc )
  : TASCAR::receivermod_base_speaker_t( xmlsrc ),
  useall(false)
{
  GET_ATTRIBUTE_BOOL_(useall);
}

void nsp_t::add_variables( TASCAR::osc_server_t* srv )
{
  TASCAR::receivermod_base_speaker_t::add_variables( srv );
  srv->add_bool( "/useall", &useall );
}

void nsp_t::add_pointsource( const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd )
{
  data_t* d( (data_t* )sd);
  TASCAR::pos_t psrc( prel.normal());
  uint32_t kmin(0);
  double dmin( distance(psrc,spkpos[kmin].unitvector ));
  double dist(0);
  for( unsigned int k=1;k<spkpos.size();k++)
    if( ( dist = distance(psrc,spkpos[k].unitvector ))<dmin ){
      kmin = k;
      dmin = dist;
    }
  if( useall )
    for( unsigned int k=0;k<spkpos.size();k++)
      d->point_dw[k] = ( 1.0f - d->point_w[k])*d->dt;
  else
    for( unsigned int k=0;k<spkpos.size();k++)
      d->point_dw[k] = ( (k==kmin ) - d->point_w[k])*d->dt;
  for( unsigned int i=0;i<chunk.size();i++){
    for( unsigned int k=0;k<spkpos.size();k++){
      output[k][i] += ( d->point_w[k] += d->point_dw[k] ) * chunk[i];
    }
  }
}

TASCAR::receivermod_base_t::data_t* nsp_t::create_data( double srate,uint32_t fragsize )
{
  return new data_t( fragsize,spkpos.size() );
}

REGISTER_RECEIVERMOD( nsp_t );

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
