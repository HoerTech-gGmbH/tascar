#ifndef SESSION_H
#define SESSION_H

#include "jackrender.h"
#include "session_reader.h"

namespace TASCAR {

  class session_t;

  class module_cfg_t {
  public:
    module_cfg_t(xmlpp::Element* xmlsrc, TASCAR::session_t* session_ );
    TASCAR::session_t* session;
    xmlpp::Element* xmlsrc;
  };

  class module_base_t : public xml_element_t {
  public:
    module_base_t( const module_cfg_t& cfg );
    virtual void cleanup() {};
    virtual void write_xml();
    virtual ~module_base_t();
    /**
       \brief Method to update geometry etc on each processing cycle in the session processing thread (jack).
       \param t Transport time.
       \param running Transport running (true) or stopped (false).
     */
    virtual void update(uint32_t frame,bool running);
    virtual void configure(double srate,uint32_t fragsize);
    void configure_(double srate,uint32_t fragsize);
  protected:
    TASCAR::session_t* session;
    double f_sample;
    double f_fragment;
    double t_sample;
    double t_fragment;
    uint32_t n_fragment;
  };

  class module_t : public TASCAR::xml_element_t {
  public:
    module_t( const TASCAR::module_cfg_t& cfg );
    void write_xml();
    virtual ~module_t();
    void cleanup();
    void configure(double srate,uint32_t fragsize);
    void update(uint32_t frame,bool running);
  private:
    std::string name;
    void* lib;
    TASCAR::module_base_t* libdata;
    bool is_configured;
  };

  class connection_t : public TASCAR::xml_element_t {
  public:
    connection_t(xmlpp::Element*);
    void write_xml();
    std::string src;
    std::string dest;
  };

  class range_t : public scene_node_base_t {
  public:
    range_t(xmlpp::Element* e);
    void write_xml();
    void prepare(double fs, uint32_t fragsize){};
    std::string name;
    double start;
    double end;
  };

  class named_object_t {
  public:
    named_object_t(TASCAR::Scene::object_t* o,const std::string& n) : obj(o),name(n){};
    TASCAR::Scene::object_t* obj;
    std::string name;
  };

  class session_oscvars_t : public TASCAR::xml_element_t {
  public:
    session_oscvars_t( xmlpp::Element* src );
    std::string name;
    std::string srv_port;
    std::string srv_addr;
  };

  class session_t : public TASCAR::tsc_reader_t, public session_oscvars_t, public jackc_transport_t, public TASCAR::osc_server_t {
  public:
    session_t();
    session_t(const std::string& filename_or_data,load_type_t t,const std::string& path);
    void set_v014();
  private:
    session_t(const session_t& src);
  public:
    virtual ~session_t();
    void add_scene(xmlpp::Element*);
    void add_range(xmlpp::Element*);
    void add_connection(xmlpp::Element*);
    void add_module(xmlpp::Element*);
    void write_xml();
    void start();
    void stop();
    void run(bool &b_quit);
    double get_duration() const { return duration; };
    uint32_t get_active_pointsources() const;
    uint32_t get_total_pointsources() const;
    uint32_t get_active_diffusesources() const;
    uint32_t get_total_diffusesources() const;
    std::vector<TASCAR::named_object_t> find_objects(const std::string& pattern);
    // configuration variables:
    //std::string name;
    double duration;
    bool loop;
    double levelmeter_tc;
    TASCAR::levelmeter_t::weight_t levelmeter_weight;
    std::vector<TASCAR::scene_render_rt_t*> scenes;
    std::vector<TASCAR::range_t*> ranges;
    std::vector<TASCAR::connection_t*> connections;
    std::vector<TASCAR::module_t*> modules;
    std::vector<std::string> get_render_output_ports() const;
    virtual int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_rolling);
    //virtual int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer);
    void unload_modules();
    bool lock_vars();
    void unlock_vars();
    bool trylock_vars();
    bool is_running() { return started_; };
  protected:
    // derived variables:
    std::string session_path;
  private:
    void add_transport_methods();
    void read_xml();
    double period_time;
    bool started_;
    pthread_mutex_t mtx;
  };

  class actor_module_t : public module_base_t {
  public:
    actor_module_t( const TASCAR::module_cfg_t& cfg, bool fail_on_empty=false );
    virtual ~actor_module_t();
    void write_xml();
    void set_location(const TASCAR::pos_t& l);
    void set_orientation(const TASCAR::zyx_euler_t& o);
    void add_location(const TASCAR::pos_t& l);
    void add_orientation(const TASCAR::zyx_euler_t& o);
  protected:
    std::string actor;
    std::vector<TASCAR::named_object_t> obj;
  };

}

#define REGISTER_MODULE(x) TASCAR_PLUGIN( module_base_t, const module_cfg_t&, x )

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
