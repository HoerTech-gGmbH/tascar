#ifndef ACOUSTICMODEL_H
#define ACOUSTICMODEL_H

#include "receivermod.h"
#include "sourcemod.h"
#include "dynamicobjects.h"
#include "audioplugin.h"

/*

  new delayline concept:

  source_base_t is base class of primary_source_t and
  image_source_t. primary_source_t owns a delayline and feeds into
  it. In "add_pointsource" audio will be read from delayline and
  filtered according to acoustic model.

  Delay Q
  Reflection filter R -> reflectors
  Air absorption A -> distance
  Radiation directivity D -> (receiver-source)/O_source orientation
  Receiver panning P -> (source-receiver)/O_receiver orientation
 
 */

namespace TASCAR {

  enum gainmodel_t {
    GAIN_INVR, GAIN_UNITY
  };
  
  /** \brief Components relevant for the acoustic modelling
   */
  namespace Acousticmodel {

    /**
       \brief Diffraction model
     */
    class diffractor_t : public ngon_t {
    public:
      class state_t {
      public:
        double A1;
        float s1;
        float s2;
        state_t() : A1(0),s1(0),s2(0) {};
      };
      pos_t process(pos_t p_src, const pos_t& p_rec, wave_t& audio, double c, double fs, state_t& state,float drywet);
    };

    class diffuse_source_t : public shoebox_t {
    public:
      diffuse_source_t(uint32_t chunksize,TASCAR::levelmeter_t& rmslevel_);
      virtual ~diffuse_source_t() {};
      virtual void preprocess();
      amb1rotator_t audio;
      double falloff;
      bool active;
      TASCAR::levelmeter_t& rmslevel;
    };

    class boundingbox_t : public dynobject_t {
    public:
      boundingbox_t(xmlpp::Element*);
      void write_xml();
      pos_t size;
      double falloff;
      bool active;
    };

    class mask_t : public shoebox_t {
    public:
      mask_t();
      double gain(const pos_t& p);
      double inv_falloff;
      bool mask_inner;
      bool active;
    };

    class source_t : public sourcemod_t, public c6dof_t {
    public:
      source_t(xmlpp::Element* xmlsrc);
      ~source_t();
      void write_xml();
      void prepare( chunk_cfg_t& cf_ );
      void release();
      virtual void process_plugins(const TASCAR::transport_t& tp);
      uint32_t ismmin;
      uint32_t ismmax;
      uint32_t layers;
      double maxdist;
      double minlevel;
      uint32_t sincorder;
      gainmodel_t gainmodel;
      double size;
      // derived / internal / updated variables:
      std::vector<wave_t> inchannels;
      std::vector<wave_t*> inchannelsp;
      bool active;
    private:
      bool is_prepared;
    public:
      std::vector<TASCAR::audioplugin_t*> plugins;
    };

    class receiver_t : public receivermod_t, public c6dof_t {
    public:
      receiver_t(xmlpp::Element* xmlsrc);
      ~receiver_t();
      void write_xml();
      void prepare( chunk_cfg_t& cf_ );
      void release();
      void clear_output();
      void add_pointsource(const pos_t& prel, double width, const wave_t& chunk, receivermod_base_t::data_t*);
      void add_diffusesource(const amb1wave_t& chunk, receivermod_base_t::data_t*);
      void update_refpoint(const pos_t& psrc_physical, const pos_t& psrc_virtual, pos_t& prel, double& distamnce, double& gain, bool b_img, gainmodel_t gainmodel );
      void set_next_gain(double gain);
      void set_fade( double targetgain, double duration );
      void apply_gain();
      void post_proc();
      // configuration/control variables:
      TASCAR::pos_t size;
      bool render_point;
      bool render_diffuse;
      bool render_image;
      uint32_t ismmin;
      uint32_t ismmax;
      uint32_t layers;
      bool use_global_mask;
      double diffusegain;
      double falloff;
      double delaycomp;
      double layerfadelen;
      // derived / internal / updated variables:
      std::vector<wave_t> outchannels;
      std::vector<wave_t*> outchannelsp;
      bool active;
      TASCAR::Acousticmodel::boundingbox_t boundingbox;
      bool gain_zero;
    private:
      double x_gain;
      double dx_gain;
      double next_gain;
      // fade timer, is > 0 during fade:
      int32_t fade_timer;
      // time constant for fade:
      float fade_rate;
      // target gain at end of fade:
      float next_fade_gain;
      // current fade gain at time of fade update:
      float previous_fade_gain;
      // preliminary values to have atomic operations (correct?):
      float prelim_next_fade_gain;
      float prelim_previous_fade_gain;
      // current fade gain:
      float fade_gain;
      bool is_prepared;
    };

    class filter_coeff_t {
    public:
      filter_coeff_t() { c[0] = 1.0; c[1] = 0.0;};
      double c[2];
    };

    class obstacle_t : public diffractor_t {
    public:
      obstacle_t();
      bool active;
      float transmission;
    };

    class reflector_t : public diffractor_t {
    public:
      reflector_t();
      void apply_reflectionfilter( TASCAR::wave_t& audio, double& lpstate ) const;
      bool active;
      double reflectivity;
      double damping;
      bool edgereflection;
    };

    class soundpath_t : public c6dof_t {
    public:
      soundpath_t(const source_t* src, const soundpath_t* parent_ = NULL, const reflector_t* generator_ = NULL);//< constructor, for primary sources set parent_ to NULL
      void update_position();//< Update image source position from parent and reflector
      pos_t get_effective_position( const pos_t& receiverp, double& gain );//< correct perceived position and caculate gain
      uint32_t getorder() const; //< Return image source order of sound path, 0 is direct path
      void apply_reflectionfilter( TASCAR::wave_t& audio ); //< Apply reflection filter of all reflectors
      const soundpath_t* parent; //< Parent sound path (or this if primary)
      const source_t* primary; //< Primary source
      const reflector_t* reflector;//< Reflector, which created new sound path from parent, or NULL for primary
      std::vector<double> reflectionfilterstates;//< Filter states for first-order reflection filters
      bool visible;
      pos_t p_cut;
    };

    /** \brief A model for a sound wave propagating from a point source to a receiver
     *
     * Processing includes delay, gain, air absorption, and optional
     * obstacles.
     */
    class acoustic_model_t : public soundpath_t {
    public:
      acoustic_model_t(double c,double fs,uint32_t chunksize,
                       source_t* src, receiver_t* receiver,
                       const std::vector<obstacle_t*>& obstacles = std::vector<obstacle_t*>(0u,NULL),
                       const acoustic_model_t* parent = NULL, 
                       const reflector_t* reflector = NULL);
      ~acoustic_model_t();
      /** \brief Read audio from source, process and add to receiver.
       */
      uint32_t process();
      double get_gain() const { return gain;};
    protected:
      double c_;
      double fs_;
    public:
      source_t* src_;
      receiver_t* receiver_;
      //pos_t effective_srcpos;
    protected:
      receivermod_base_t::data_t* receiver_data;
      sourcemod_base_t::data_t* source_data;
      std::vector<obstacle_t*> obstacles_;
      std::vector<diffractor_t::state_t> vstate;
      wave_t audio;
      uint32_t chunksize;
      double dt;
      double distance;
      double gain;
      double dscale;
      double air_absorption;
      varidelay_t delayline;
      float airabsorption_state;
      double layergain;
      double dlayergain;
    public:
      uint32_t ismorder;
    };

    /** \brief A model for a sound wave propagating from a point source to a receiver
     *
     * Processing includes delay, gain, air absorption, and optional
     * obstacles.
     */
    class diffuse_acoustic_model_t {
    public:
      diffuse_acoustic_model_t(double fs,uint32_t chunksize,diffuse_source_t* src,receiver_t* receiver);
      ~diffuse_acoustic_model_t();
      /** \brief Read audio from source, process and add to receiver.
       */
      uint32_t process();
    protected:
      diffuse_source_t* src_;
      receiver_t* receiver_;
      receivermod_base_t::data_t* receiver_data;
      amb1rotator_t audio;
      uint32_t chunksize;
      double dt;
      double gain;
    };

    class receiver_graph_t {
    public:
      /** \brief Create a graph for one receiver
       */
      receiver_graph_t(double c, double fs, uint32_t chunksize,
                       const std::vector<source_t*>& sources,
                       const std::vector<diffuse_source_t*>& diffusesources,
                       const std::vector<reflector_t*>& reflectors,
                       const std::vector<obstacle_t*>& obstacles,
                       receiver_t* receiver,
                       uint32_t mirror_order);
      ~receiver_graph_t();
      void process();
      uint32_t get_active_pointsource() const {return active_pointsource;};
      uint32_t get_active_diffusesource() const {return active_diffusesource;};
      uint32_t get_total_pointsource() const {return acoustic_model.size();};
      uint32_t get_total_diffusesource() const {return diffuse_acoustic_model.size();};
      std::vector<acoustic_model_t*> acoustic_model;
      std::vector<diffuse_acoustic_model_t*> diffuse_acoustic_model;
      uint32_t active_pointsource;
      uint32_t active_diffusesource;
    };

    /** \brief The render model of an acoustic scenario.
     *
     * A world creates a set of acoustic models, one for each
     * combination of a sound source (primary or mirrored) and a receiver.
     */
    class world_t {
    public:
      /** \brief Create a world of acoustic models.
       *
       * \param sources Pointers to primary sound sources
       * \param reflectors Pointers to reflector objects
       * \param receivers Pointers to render receivers
       *
       * A mirror model is created from the reflectors and primary sources.
       * An instance of this class is created in TASCAR::render_core_t::prepare().
       */
      world_t( double c, double fs, uint32_t chunksize,
               const std::vector<source_t*>& sources,
               const std::vector<diffuse_source_t*>& diffusesources,
               const std::vector<reflector_t*>& reflectors,
               const std::vector<obstacle_t*>& obstacles,
               const std::vector<receiver_t*>& receivers,
               const std::vector<mask_t*>& masks,
               uint32_t mirror_order);
      ~world_t();
      /** \brief Process the mirror model and all acoustic models.
       */
      void process();
      uint32_t get_active_pointsource() const {return active_pointsource;};
      uint32_t get_active_diffusesource() const {return active_diffusesource;};
      uint32_t get_total_pointsource() const {return total_pointsource;};
      uint32_t get_total_diffusesource() const {return total_diffusesource;};
      std::vector<receiver_graph_t*> receivergraphs;
      std::vector<receiver_t*> receivers_;
      std::vector<mask_t*> masks_;
      uint32_t active_pointsource;
      uint32_t active_diffusesource;
      uint32_t total_pointsource;
      uint32_t total_diffusesource;
    };

  }

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
