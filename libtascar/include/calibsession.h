#ifndef CALIBSESSION_H
#define CALIBSESSION_H

#include "session.h"

namespace TASCAR {

  class calibsession_t : public TASCAR::session_t {
  public:
    calibsession_t(const std::string& fname, double reflevel,
                   const std::vector<std::string>& refport, double duration_,
                   double fmin, double fmax, double subduration_,
                   double subfmin, double subfmax);
    ~calibsession_t();
    double get_caliblevel() const;
    double get_diffusegain() const;
    void inc_caliblevel(double dl);
    void inc_diffusegain(double dl);
    void set_active(bool b);
    void set_active_diff(bool b);
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
      if(spkarray)
        return spkarray->size();
      return 0u;
    };
    size_t get_num_sub() const
    {
      if(spkarray)
        return spkarray->subs.size();
      return 0u;
    };
    size_t get_num_channels() const { return get_num_bb() + get_num_sub(); };
    double get_measurement_duration() const
    {
      return duration * (double)get_num_bb() +
             subduration * (double)get_num_sub();
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
    spk_array_diff_render_t* spkarray;
    std::vector<double> levels;
    std::vector<double> sublevels;
    std::vector<std::string> refport_; ///< list of measurement microphone ports
    double duration;
    double subduration;
    double lmin;
    double lmax;
    double lmean;
    std::string calibfor;
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
