#ifndef CALIBSESSION_H
#define CALIBSESSION_H

#include "jackiowav.h"
#include "session.h"

namespace TASCAR {

  /**
     @brief Parameters for loudspeaker equalization and calibration.
   */
  class spk_eq_param_t {
  public:
    spk_eq_param_t(bool issub = false);
    void factory_reset();
    void read_defaults();
    void write_defaults();
    void read_xml(const tsccfg::node_t& layoutnode);
    void save_xml(const tsccfg::node_t& layoutnode) const;
    void validate() const;
    float fmin = 62.5f;    ///< Lower limit of frequency in Hz.
    float fmax = 4000.0f;  ///< Upper limit of frequency in Hz.
    float duration = 1.0f; ///< Stimulus duration in s.
    float prewait =
        0.125f; ///< Time between stimulus onset and measurement start in s.
    float reflevel = 80.0f;      ///< reference level for calibration.
    float bandsperoctave = 3.0f; ///< Bands per octave in frequency dependent
                                 ///< level analysis.
    float bandoverlap =
        2.0f; ///< Overlap of analysis filterbank in filter bands.
    uint32_t max_eqstages =
        0u; ///< maximum number of biquad stages for frequency compensation
    const bool issub = false;
  };

  /**
     @brief Calibration parameters
   */
  class calib_cfg_t {
  public:
    calib_cfg_t();
    void factory_reset();
    void read_defaults();
    void save_defaults();
    void read_xml(const tsccfg::node_t& layoutnode);
    void save_xml(const tsccfg::node_t& layoutnode) const;
    void validate() const;
    void set_has_sub(bool h) { has_sub = h; };
    spk_eq_param_t par_speaker; ///< Broadband speaker calibration parameters
    spk_eq_param_t par_sub;     ///< Subwoofer calibration parameters
    std::vector<std::string> refport; ///< Jack port name to which measurement
                                      ///< microphones are connected
    std::vector<float>
        miccalib;        ///< Calibration values of reference microphones
    bool initcal = true; ///< Initial calibration (or re-calibration if false)
  private:
    bool has_sub = false;
  };

  class spkeq_report_t {
  public:
    spkeq_report_t();
    spkeq_report_t(std::string label, const std::vector<float>& vF,
                   const std::vector<float>& vG_precalib,
                   const std::vector<float>& vG_postcalib, float gain_db);
    std::string label;
    std::vector<float> vF;
    std::vector<float> vG_precalib;
    std::vector<float> vG_postcalib;
    float gain_db = 0.0f;
    std::vector<float> eq_f;
    std::vector<float> eq_g;
    std::vector<float> eq_q;
    std::vector<float> level_db_re_fs;
    std::vector<float> coh;
  };

  /**
     @brief Dedicated TASCAR session for calibration of speaker layout files.

     Creating an instance of this class will create a jack based TASCAR session
     suitable for calibration. The speaker layout parameters can be changed
     interactively, and the calibrated layout file can be saved.
   */
  class calibsession_t : public TASCAR::session_t {
  public:
    calibsession_t(const std::string& fname, const calib_cfg_t& cfg);
    ~calibsession_t();
    double get_caliblevel() const;
    double get_diffusegain() const;
    void set_caliblevel(float level);
    void set_diffusegain(float gain);
    void inc_caliblevel(float dl);
    void inc_diffusegain(float dl);
    void set_active(bool b);
    void set_active_diff(bool b);
    /**
       @brief Measure relative levels of loudspeakers
       @param prewait Waiting time between activation and repositioning of sound
       sources and measurement phase
     */
    void get_levels();
    void reset_levels();
    void saveas(const std::string& fname);
    void save();
    bool complete() const
    {
      return levelsrecorded && calibrated && calibrated_diff;
    };
    bool modified() const
    {
      return levelsrecorded || calibrated || calibrated_diff || gainmodified;
    };
    std::string name() const { return spkname; };
    double get_lmin() const { return lmin; };
    double get_lmax() const { return lmax; };
    double get_lmean() const { return lmean; };
    bool complete_spk_equal() const { return levelsrecorded; };
    size_t get_num_bb() const
    {
      if(spk_file)
        return spk_file->size();
      return 0u;
    };
    size_t get_num_sub() const
    {
      if(spk_file)
        return spk_file->subs.size();
      return 0u;
    };
    size_t get_num_channels() const { return get_num_bb() + get_num_sub(); };
    double get_measurement_duration() const
    {
      return (cfg_.par_speaker.duration + cfg_.par_speaker.prewait) *
                 (double)get_num_bb() *
                 (1.0 + (double)(cfg_.par_speaker.max_eqstages > 0u)) +
             (cfg_.par_sub.duration + cfg_.par_sub.prewait) *
                 (double)get_num_sub() *
                 (1.0 + (double)(cfg_.par_sub.max_eqstages > 0u)) +
             0.2f * (float)cfg_.par_speaker.max_eqstages *
                 (double)get_num_bb() +
             0.2f * (float)cfg_.par_sub.max_eqstages * (double)get_num_sub();
    };
    const spk_array_diff_render_t& get_current_layout() const;
    void enable_spkcorr_spec(bool b);

  private:
    bool gainmodified;
    bool levelsrecorded;
    bool calibrated;
    bool calibrated_diff;
    // std::vector<std::string> refport;
    double startlevel;
    double startdiffgain;
    double delta;
    double delta_diff;
    std::string spkname;
    spk_array_diff_render_t* spk_file = NULL;
    TASCAR::Scene::receiver_obj_t* rec_nsp = NULL;
    TASCAR::Scene::receiver_obj_t* rec_spec = NULL;
    TASCAR::receivermod_base_speaker_t* spk_nsp = NULL;
    TASCAR::receivermod_base_speaker_t* spk_spec = NULL;
    std::vector<float> levels;
    std::vector<float> sublevels;

  public:
    std::vector<float> levelsfrg; // frequency-dependent level range (max-min)
    std::vector<float>
        sublevelsfrg; // frequency-dependent level range (max-min)

    TASCAR::Scene::route_t* levelroute = NULL;

  private:
    const calib_cfg_t cfg_;
    // spk_eq_param_t par_speaker;
    // spk_eq_param_t par_sub;
    // std::vector<std::string> refport_; ///< list of measurement microphone
    // ports
    float lmin;
    float lmax;
    float lmean;
    std::string calibfor;
    jackrec2wave_t jackrec;
    std::vector<TASCAR::wave_t> bbrecbuf;
    std::vector<TASCAR::wave_t> subrecbuf;
    TASCAR::wave_t teststim_bb;
    TASCAR::wave_t teststim_sub;
    // std::vector<float> vF;
    // std::vector<std::vector<float>> vGains;
    // std::vector<float> vFsub;
    // std::vector<std::vector<float>> vGainsSub;
    bool isactive_pointsource = false;
    bool isactive_diffuse = false;

  public:
    std::vector<spkeq_report_t> spkeq_report;
  };

  class spkcalibrator_t {
  public:
    spkcalibrator_t();
    ~spkcalibrator_t();
    void set_filename(const std::string&);
    std::string get_filename() const { return filename; };
    std::string get_orig_speaker_desc() const;
    std::string get_current_speaker_desc() const;
    std::vector<spkeq_report_t> get_speaker_report() const
    {
      if(p_session)
        return p_session->spkeq_report;
      return {};
    };
    /**
       @brief Call this function after a file was selected.
     */
    void step1_file_selected();
    /**
       @brief Call this function after the configuration was revised.
    */
    void step2_config_revised();
    /**
       @brief Call this function after the calibration was initialized.

       Part of the initialization is the coarse adjustment of the levels.
     */
    void step3_calib_initialized();
    /**
       @brief Call this function after the speakers were equalized.

       Equalization can be either broadband only, or frequency
       compensated and broadband equalized.
     */
    void step4_speaker_equalized();
    /**
       @brief Call this function after the level of the first speaker
       and the diffuse level were adjusted.
     */
    void step5_levels_adjusted();

    void go_back();

    calib_cfg_t cfg;

    const TASCAR::levelmeter_t& get_meter(uint32_t k) const;

    void set_active_pointsource(bool act)
    {
      if(p_session)
        p_session->set_active(act);
    };

    void set_active_diffuse(bool act)
    {
      if(p_session)
        p_session->set_active_diff(act);
    };

    void inc_diffusegain(float d)
    {
      if(p_session)
        p_session->inc_diffusegain(d);
    };

    void inc_caliblevel(float d)
    {
      if(p_session)
        p_session->inc_caliblevel(d);
    };
    void set_caliblevel(float d)
    {
      if(p_session)
        p_session->set_caliblevel(d);
    };
    void set_diffusegain(float g)
    {
      if(p_session)
        p_session->set_diffusegain(g);
    };
    double get_caliblevel() const
    {
      if(p_session)
        return p_session->get_caliblevel();
      return 0;
    };
    double get_diffusegain() const
    {
      if(p_session)
        return p_session->get_diffusegain();
      return 0;
    };
    double get_measurement_duration() const
    {
      if(p_session)
        return p_session->get_measurement_duration();
      return 0;
    };
    void get_levels()
    {
      if(p_session)
        p_session->get_levels();
    };
    void reset_levels()
    {
      if(p_session)
        p_session->reset_levels();
    };
    bool complete_spk_equal() const
    {
      if(p_session)
        return p_session->complete_spk_equal();
      return false;
    };
    bool complete() const
    {
      if(p_session)
        return p_session->complete();
      return false;
    };
    void save()
    {
      if(p_session)
        p_session->save();
    };
    void cfg_load_from_layout()
    {
      if(p_layout_doc)
        cfg.read_xml(p_layout_doc->root());
    }
    bool has_sub() const
    {
      if(p_layout)
        return p_layout->subs.size() > 0u;
      return false;
    }

  private:
    std::string filename;
    uint32_t currentstep = 0u;
    calibsession_t* p_session = NULL;
    xml_doc_t* p_layout_doc = NULL;
    spk_array_diff_render_t* p_layout = NULL;
    TASCAR::levelmeter_t fallbackmeter;
  };

} // namespace TASCAR

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
