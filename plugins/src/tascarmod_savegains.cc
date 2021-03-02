#include "session.h"
#include <fstream>

class savegains_t : public TASCAR::module_base_t {
public:
  savegains_t( const TASCAR::module_cfg_t& cfg );
  ~savegains_t();
  static int osc_save(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void save();
  static int osc_restore(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void restore();
private:
  std::vector<std::string> pattern;
  std::string path;
  std::string filename;
  lo_message m;
};

savegains_t::savegains_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    pattern(1,"*"),
    filename("savedgains"),
    m(lo_message_new())
{
  lo_message_add_float(m,0.0f);
  GET_ATTRIBUTE_(path);
  GET_ATTRIBUTE_(filename);
  GET_ATTRIBUTE_(pattern);
  session->add_string("/savegains/filename",&filename);
  session->add_method("/savegains/save","",savegains_t::osc_save,this);
  session->add_method("/savegains/restore","",savegains_t::osc_restore,this);
  session->add_method("/savegains/save","f",savegains_t::osc_save,this);
  session->add_method("/savegains/restore","f",savegains_t::osc_restore,this);
}

savegains_t::~savegains_t()
{
}

int savegains_t::osc_save(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( (argc == 0) || (argv[0]->f > 0) )
    ((savegains_t*)user_data)->save();
  return 0;
}

void savegains_t::save()
{
  if( session ){
    std::string fname;
    if( !path.empty() ){
      fname = path;
      if( fname[fname.size()-1] != '/' )
        fname = fname+"/";
    }
    fname = fname+filename;
    try{
      std::ofstream ofs(fname.c_str());
      if( ofs.good() ){
        auto ports(session->find_audio_ports( pattern ));
        for( auto pit=ports.begin();pit!=ports.end();++pit)
          ofs << (*pit)->get_ctlname() << "/gain " << (*pit)->get_gain_db() << "\n";
      }else{
        throw TASCAR::ErrMsg("Unable to create file \""+fname+"\".");
      }
    }
    catch( const std::exception& e ){
      std::string msg("Warning: Unable to save gains: ");
      msg += e.what();
      TASCAR::add_warning(msg);
    }
  }
}

int savegains_t::osc_restore(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( (argc == 0) || (argv[0]->f > 0) )
    ((savegains_t*)user_data)->restore();
  return 0;
}

void savegains_t::restore()
{
  if( session ){
    std::string fname;
    if( !path.empty() ){
      fname = path;
      if( fname[fname.size()-1] != '/' )
        fname = fname+"/";
    }
    fname = fname+filename;
    try{
      std::ifstream ifs(fname.c_str());
      if( !ifs.good() )
        throw TASCAR::ErrMsg("Unable to open file \""+fname+"\".");
      while( ifs.good() && !ifs.eof() ){
        std::string p;
        float v(0.0f);
        ifs >> p;
        ifs >> v;
        if( !p.empty()){
          lo_arg** arg(lo_message_get_argv(m));
          arg[0]->f = v;
          session->dispatch_data_message(p.c_str(),m);
        }
      }
    }
    catch( const std::exception& e ){
      std::string msg("Unable to restore gains: ");
      msg += e.what();
      TASCAR::add_warning(msg);
    }
  }
}

REGISTER_MODULE(savegains_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
