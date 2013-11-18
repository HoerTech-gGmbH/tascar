#include <getopt.h>
#include "scene.h"
#include <iostream>

using namespace TASCAR;

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_creator [options]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

int main(int argc,char**argv)
{
  std::string cfg_input("");
  std::string cfg_output("");
  //bool b_list(false);
  bool b_example(false);
  const char *options = "i:o:xh";
  struct option long_options[] = { 
    { "input",   1, 0, 'i' },
    { "output",  1, 0, 'o' },
    //{ "list",    0, 0, 'l' },
    { "example", 0, 0, 'x' },
    { "help",    0, 0, 'h' },
    { 0, 0, 0, 0 }
  };
  int opt(0);
  int option_index(0);
  while( (opt = getopt_long(argc, argv, options,
                            long_options, &option_index)) != -1){
    switch(opt){
    case 'i':
      cfg_input = optarg;
      break;
    case 'o':
      cfg_output = optarg;
      break;
    case 'h':
      usage(long_options);
      return -1;
    //case 'l':
    //  b_list = true;
    //  break;
    case 'x':
      b_example = true;
      break;
    }
  }
  TASCAR::Scene::scene_t S;
  if( cfg_input.size() )
    S = TASCAR::Scene::xml_read_scene( cfg_input );
  if( b_example ){
    if( !S.name.size() )
      S.name = "Example scene";
    if( !S.description.size() )
      S.description = "Scene with example values";
    if( !S.object_sources.size() )
      S.add_source();
    if( !S.object_sources[0].location.size() ){
      S.object_sources[0].location[0] = pos_t(1,2,0.1);
      S.object_sources[0].location[10] = pos_t(2,-2,-0.1);
    }
    if( !S.object_sources[0].sound.size() ){
      S.object_sources[0].add_sound();
    }
    //if( !S.bg_amb.size() )
    //  S.bg_amb.push_back(TASCAR::Scene::bg_amb_t());
  }
  if( cfg_output.size() ){
    time_t tm(time(NULL));
    std::string strtime(ctime(&tm));
    strtime = "\nCreated with tascar_creator\n" + strtime;
    xml_write_scene(cfg_output,S,strtime,b_example);
  }
  //if( b_list )
  //  std::cout << S.print();
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
