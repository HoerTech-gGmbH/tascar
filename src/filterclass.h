#ifndef FILTERCLASS_H
#define FILTERCLASS_H

#include <vector>
#include "audiochunks.h"

namespace TASCAR {

    class filter_t {
    public:
	/** 
	    \brief Constructor 
	    \param ch Number of channels
	    \param lena Number of recursive coefficients
	    \param lenb Number of non-recursive coefficients
	*/
        filter_t(unsigned int lena,  // length A
                 unsigned int lenb); // length B
	/** 
	    \brief Constructor with initialization of coefficients.
	    \param ch Number of channels.
	    \param vA Recursive coefficients.
	    \param vB Non-recursive coefficients.
	*/
	filter_t(const std::vector<double>& vA, const std::vector<double>& vB);

	filter_t(const TASCAR::filter_t& src);

        ~filter_t();
	/** \brief Filter all channels in a waveform structure. 
	    \param out Output signal
	    \param in Input signal
	*/
      void filter(TASCAR::wave_t* out,const TASCAR::wave_t* in);
	/** \brief Filter parts of a waveform structure
	    \param dest Output signal.
	    \param src Input signal.
	    \param dframes Number of frames to be filtered.
	    \param frame_dist Index distance between frames of one channel
	    \param channel_dist Index distance between audio channels
	    \param channel_begin Number of first channel to be processed
	    \param channel_end Number of last channel to be processed
	*/
	void filter(float* dest,
                    const float* src,
                    unsigned int dframes,
                    unsigned int frame_dist);
	/**
	   \brief Filter one sample 
	   \param x Input value
	   \param ch Channel number to use in filter state
	*/
        double filter(float x);
	/** \brief Return length of recursive coefficients */
        unsigned int get_len_A() const {return len_A;};
	/** \brief Return length of non-recursive coefficients */
        unsigned int get_len_B() const {return len_B;};
	/** \brief Pointer to recursive coefficients */
        double* A;
	/** \brief Pointer to non-recursive coefficients */
        double* B;
    private:
        unsigned int len_A;
        unsigned int len_B;
        unsigned int len;
        double* state;
    };

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
