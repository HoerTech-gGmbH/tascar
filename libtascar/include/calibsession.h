#ifndef CALIBSESSION_H
#define CALIBSESSION_H

#include "jackiowav.h"
#include "session.h"

namespace TASCAR {

  class calibsession_t : public TASCAR::session_t {
  public:
    calibsession_t(const std::string& fname, double reflevel,
                   const std::vector<std::string>& refport, double duration_,
                   double fmin, double fmax, double subduration_,
                   double subfmin, double subfmax, float bpo, float subbpo);
    ~calibsession_t();
    double get_caliblevel() const;
    double get_diffusegain() const;
    void inc_caliblevel(double dl);
    void inc_diffusegain(double dl);
    void set_active(bool b);
    void set_active_diff(bool b);
    /**
       @brief Measure relative levels of loudspeakers
       @param prewait Waiting time between activation and repositioning of sound
       sources and measurement phase
     */
    void get_levels(double prewait);
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
    bool levels_complete() const { return levelsrecorded; };
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
      return duration * (double)get_num_bb() *
                 (1.0 + (double)(max_fcomp_bb > 0u)) +
             subduration * (double)get_num_sub() *
                 (1.0 + (double)(max_fcomp_sub > 0u));
    };

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
    float bpo;
    float subbpo;

  public:
    std::vector<float> levelsfrg; // frequency-dependent level range (max-min)
    std::vector<float>
        sublevelsfrg; // frequency-dependent level range (max-min)
  private:
    std::vector<std::string> refport_; ///< list of measurement microphone ports
    double duration;
    double subduration;
    float lmin;
    float lmax;
    float lmean;
    float fmin_;
    float fmax_;
    float subfmin_;
    float subfmax_;
    std::string calibfor;
    jackrec2wave_t jackrec;
    std::vector<TASCAR::wave_t> bbrecbuf;
    std::vector<TASCAR::wave_t> subrecbuf;
    TASCAR::wave_t teststim_bb;
    TASCAR::wave_t teststim_sub;

  public:
    uint32_t max_fcomp_bb =
        0u; ///< maximum number of biquad stages for frequency compensation
    uint32_t max_fcomp_sub = 0u;

  private:
    std::vector<float> vF;
    std::vector<std::vector<float>> vGains;
    std::vector<float> vFsub;
    std::vector<std::vector<float>> vGainsSub;
    uint32_t fcomp_bb =
        0u; ///< final number of biquad stages for frequency compensation
    uint32_t fcomp_sub = 0u;
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
