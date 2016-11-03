#include "spectrum.h"
#include <algorithm>
#include <string.h>

TASCAR::spec_t::spec_t(uint32_t n) 
  : n_(n),b(new float _Complex [n_])
{
  memset(b,0,n_*sizeof(float _Complex));
}

TASCAR::spec_t::spec_t(const TASCAR::spec_t& src)
  : n_(src.n_),b(new float _Complex [n_])
{
  copy(src);
}

TASCAR::spec_t::~spec_t()
{
  delete [] b;
}

void TASCAR::spec_t::copy(const spec_t& src)
{
  memmove(b,src.b,std::min(n_,src.n_)*sizeof(float _Complex));
}

void TASCAR::spec_t::operator/=(const spec_t& o)
{
  for(unsigned int k=0;k<std::min(o.n_,n_);k++){
    if( cabs(o.b[k]) > 0 )
      b[k] /= o.b[k];
  }
}

void TASCAR::spec_t::operator*=(const spec_t& o)
{
  for(unsigned int k=0;k<std::min(o.n_,n_);k++)
    b[k] *= o.b[k];
}

void TASCAR::spec_t::operator+=(const spec_t& o)
{
  for(unsigned int k=0;k<std::min(o.n_,n_);k++)
    b[k] += o.b[k];
}

void TASCAR::spec_t::operator*=(const float& o)
{
  for(unsigned int k=0;k<n_;k++)
    b[k] *= o;
}

void TASCAR::spec_t::conj()
{
  for(uint32_t k=0;k<n_;k++)
    b[k] = conjf( b[k] );
}

std::ostream& operator<<(std::ostream& out, const TASCAR::wave_t& p)
{
  out << std::string("W(") << p.n << std::string("):");
  for(uint32_t k=0;k<p.n;k++)
    out << std::string(" ") << p.d[k];
  return out;
}

std::ostream& operator<<(std::ostream& out, const TASCAR::spec_t& p)
{
  out << std::string("S(") << p.n_ << std::string("):");
  for(uint32_t k=0;k<p.n_;k++)
    out << std::string(" ") << creal(p.b[k]) << std::string((cimag(p.b[k])>=0.0)?"+":"") << cimag(p.b[k]) << "i";
  return out;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
