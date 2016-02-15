// gets rid of annoying "deprecated conversion from string constant blah blah" warning
#pragma GCC diagnostic ignored "-Wwrite-strings"

#include <lsl_c.h>
#include <iostream>

int main(int argc, char** argv)
{
  unsigned buffer_elements(1024);
  lsl_streaminfo buffer[buffer_elements];
  int num_streams(lsl_resolve_all(buffer, buffer_elements, 1 ));
  for(int kstream=0;kstream<num_streams;kstream++){
    std::string fmt("undefined");
    switch( lsl_get_channel_format(buffer[kstream]) ){
    case cft_float32 : fmt="float32"; break;
    case cft_double64 : fmt="double64"; break;
    case cft_string : fmt="string"; break;
    case cft_int32 : fmt="int32"; break;
    case cft_int16 : fmt="int16"; break;
    case cft_int8 : fmt="int8"; break;
    case cft_int64 : fmt="int64"; break;
    }
    std::cout << lsl_get_name(buffer[kstream]) << 
      " type=" << lsl_get_type(buffer[kstream]) <<
      " fmt=" << fmt <<
      " channels=" << lsl_get_channel_count(buffer[kstream]) << 
      " fs=" << lsl_get_nominal_srate(buffer[kstream]) << " Hz " <<
      //lsl_get_uid(buffer[kstream]) << "@" <<
      lsl_get_hostname(buffer[kstream]) <<
      std::endl;
    lsl_destroy_streaminfo(buffer[kstream]);
  }
  if( num_streams == 0 )
    std::cout << "no LSL streams found." << std::endl;
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
