/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (C) 2018 Giso Grimm
 * Copyright (C) 2021 Giso Grimm
 * Copyright (C) 2022 Giso Grimm
 */
/**
   \file jackiowav.h
   \ingroup apptascar
   \brief JACK I/O interface for simultaneous playback and recording of WAV
   files or memory buffers.
   \author Giso Grimm
   \date 2012,2014,2017
*/
/*
  License (GPL)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; version 2 of the
  License.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/
#ifndef JACKIOWAV_H
#define JACKIOWAV_H

#include "audiochunks.h"
#include "errorhandling.h"
#include "jackclient.h"
#include <atomic>
#include <jack/ringbuffer.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <unistd.h>

/**
   \ingroup apptascar
   \brief JACK client for simultaneous playback of a sound file and recording to
   a sound file.

   This class loads an entire input file into memory for playback and allocates
   memory for the recording. It handles JACK port creation, connection, and
   synchronization. It supports both standard JACK transport and freewheeling
   mode.
*/
class jackio_t : public jackc_transport_t {
public:
  /**
     \brief Constructor for file-based playback and recording.

     Loads a sound file into memory for playback. Creates an output file for
     recording. The number of output ports (recording channels) is determined by
     the size of the `ports` vector minus the number of input channels.

     \param ifname Path to the input WAV file for playback.
     \param ofname Path to the output WAV file for recording. If empty, no
     recording is performed.
     \param ports List of JACK port names to connect to. The first N ports are
     used for output (playback), where N is the number of channels in `ifname`.
     Remaining ports are used for input (recording).
     \param jackname Name of the JACK client.
     \param freewheel If non-zero, JACK freewheeling mode is enabled for
     faster-than-realtime processing.
     \param autoconnect If non-zero, automatically connects to system
     playback/capture ports ("system:playback_N", "system:capture_1").
     \param verbose If true, prints status messages to stderr.
  */
  jackio_t(const std::string& ifname, const std::string& ofname,
           const std::vector<std::string>& ports,
           const std::string& jackname = "jackio", int freewheel = 0,
           int autoconnect = 0, bool verbose = false);

  /**
     \brief Constructor for memory-based playback and recording (offline
     processing).

     Uses provided wave_t buffers for input and output instead of files. Useful
     for offline processing or testing.

     \param isig Vector of input signals (wave_t) to play back.
     \param osig Reference to vector of output signals (wave_t) to record into.
     \param ports List of JACK port names (currently unused in this specific
     constructor logic).
     \param jackname Name of the JACK client.
     \param freewheel If non-zero, enables freewheeling mode.
     \param verbose If true, prints status messages to stderr.
  */
  jackio_t(const std::vector<TASCAR::wave_t>& isig,
           std::vector<TASCAR::wave_t>& osig,
           const std::vector<std::string>& ports,
           const std::string& jackname = "jackio", int freewheel = 0,
           bool verbose = false);

  /**
     \brief Constructor for recording-only mode (no playback file).

     Allocates memory for recording of a specific duration. No input file is
     loaded.

     \param duration Duration of the recording in seconds.
     \param ofname Path to the output WAV file.
     \param ports List of JACK port names for recording inputs.
     \param jackname Name of the JACK client.
     \param freewheel If non-zero, enables freewheeling mode.
     \param autoconnect If non-zero, connects to "system:capture_1".
     \param verbose If true, prints status messages to stderr.
  */
  jackio_t(double duration, const std::string& ofname,
           const std::vector<std::string>& ports,
           const std::string& jackname = "jackio", int freewheel = 0,
           int autoconnect = 0, bool verbose = false);

  /**
     \brief Configure JACK transport synchronization.

     \param start Start time in seconds.
     \param wait If true, waits for the JACK transport to reach the start frame
     before beginning playback/recording.
  */
  void set_transport_start(double start, bool wait);

  ~jackio_t();

  /**
     \brief Start the JACK client, process audio, and handle recording.

     This method activates the client, connects ports, and enters a loop
     processing audio until the end of the file is reached or stopped.
  */
  void run();

private:
  SNDFILE* sf_in = NULL;  ///< Handle for the input sound file (libsndfile).
  SNDFILE* sf_out = NULL; ///< Handle for the output sound file (libsndfile).
  SF_INFO sf_inf_in;      ///< Metadata for the input file.
  SF_INFO sf_inf_out;     ///< Metadata for the output file.

public:
  float* buf_in =
      NULL; ///< Interleaved buffer storing input samples for playback.
  float* buf_out = NULL; ///< Interleaved buffer storing recorded samples.

private:
  unsigned int pos = 0u; ///< Current playback/recording position in frames.
  bool b_quit = false;   ///< Flag to signal the processing loop to stop.
  bool start = false;    ///< Flag indicating if playback/recording has started.
  bool freewheel_ = false; ///< Flag indicating if freewheeling mode is active.
  bool use_transport =
      false; ///< Flag indicating if JACK transport is used for sync.
  uint32_t startframe =
      0u; ///< The frame number at which to start (transport mode).

public:
  uint32_t nframes_total = 1u; ///< Total number of frames to process.

private:
  std::vector<std::string> p; ///< List of port names for connection.

  /**
     \brief JACK process callback.

     Called by the JACK server when audio is available. It handles copying
     data from `buf_in` to JACK output ports and from JACK input ports to
     `buf_out`.

     \param nframes Number of frames in the current cycle.
     \param inBuffer Vector of pointers to input port buffers.
     \param outBuffer Vector of pointers to output port buffers.
     \param tp_frame Current transport frame.
     \param tp_rolling Current transport rolling state.
     \return 0 on success.
  */
  int process(jack_nframes_t nframes, const std::vector<float*>& inBuffer,
              const std::vector<float*>& outBuffer, uint32_t tp_frame,
              bool tp_rolling);

  void log(const std::string& msg);
  std::atomic<bool> b_cb =
      false; ///< Flag indicating if the process callback has run at least once.
  bool b_verbose = true; ///< Flag for verbose logging.
  bool wait_ = false; ///< If true, wait for transport position before starting.

public:
  float cpuload = 0.0f; ///< Measured CPU load after processing.
  uint32_t xruns = 0u;  ///< Count of JACK xruns encountered.

private:
  std::vector<TASCAR::wave_t> osig__ =
      {}; ///< Internal storage for output signals if no file is used.
  std::vector<TASCAR::wave_t>& osig_ =
      osig__; ///< Reference to the output signal buffer.
};

/**
   \ingroup apptascar
   \brief Asynchronous JACK recorder using a ring buffer.

   This class records audio from JACK ports into a file. It uses a separate
   thread (service) to write data from a ring buffer to disk, preventing
   disk I/O from blocking the real-time JACK thread.
*/
class jackrec_async_t : public jackc_transport_t {
public:
  /**
     \brief Constructor for the asynchronous recorder.

     \param ofname Path to the output WAV file.
     \param ports List of JACK port names to record from.
     \param jackname Name of the JACK client.
     \param buflen Ring buffer duration in seconds.
     \param format libsndfile format specifier (e.g., SF_FORMAT_WAV |
     SF_FORMAT_PCM_16).
     \param usetransport If true, only record when JACK transport is rolling.
  */
  jackrec_async_t(const std::string& ofname,
                  const std::vector<std::string>& ports,
                  const std::string& jackname = "jackrec", double buflen = 10,
                  int format = SF_FORMAT_WAV | SF_FORMAT_PCM_16 |
                               SF_ENDIAN_FILE,
                  bool usetransport = false);
  ~jackrec_async_t();

  double rectime; ///< Total recorded time in seconds.
  size_t xrun;    ///< Number of buffer overruns (xruns).
  size_t werror;  ///< Number of write errors to disk.

private:
  int process(jack_nframes_t nframes, const std::vector<float*>& inBuffer,
              const std::vector<float*>& outBuffer, uint32_t tp_frame,
              bool tp_rolling);

  /**
     \brief Service thread routine.

     Reads data from the ring buffer and writes it to the output file.
     Runs asynchronously to the JACK process callback.
  */
  void service();

  SNDFILE* sf_out;       ///< Handle for the output sound file.
  SF_INFO sf_inf_out;    ///< Metadata for the output file.
  jack_ringbuffer_t* rb; ///< Ring buffer for real-time to disk data transfer.
  std::thread srv;       ///< The service thread handling disk I/O.
  bool run_service;      ///< Flag to control the service thread loop.
  float* buf;            ///< Temporary buffer used in the JACK process callback
                         ///< (interleaved).
  float* rbuf; ///< Buffer used by the service thread for reading from the ring
               ///< buffer.
  size_t rlen; ///< Size of the read buffer (rbuf) in samples.
  double tscale;     ///< Time scaling factor (1.0 / samplerate).
  size_t recframes;  ///< Total number of frames recorded.
  size_t channels;   ///< Number of recording channels.
  bool usetransport; ///< Flag to use JACK transport state.
};

/**
   \ingroup apptascar
   \brief Simple JACK recorder to memory buffers.

   This class records audio directly into pre-allocated `wave_t` structures
   in memory. It is designed for short-duration recording where disk I/O
   is not required.
*/
class jackrec2wave_t : public jackc_t {
public:
  /**
     \brief Constructor.

     \param channels Number of input channels/ports to create.
     \param jackname Name of the JACK client.
  */
  jackrec2wave_t(size_t channels, const std::string& jackname = "jackrec");
  ~jackrec2wave_t();

  /**
     \brief Start recording into the provided wave buffers.

     This method blocks until recording is finished (i.e., buffers are filled).

     \param w Vector of wave_t buffers to record into.
     \param ports List of JACK ports to connect to.
  */
  void rec(const std::vector<TASCAR::wave_t>& w,
           const std::vector<std::string>& ports);

private:
  int process(jack_nframes_t n, const std::vector<float*>& s_in,
              const std::vector<float*>& s_out);
  std::atomic_bool
      isrecording; ///< Atomic flag indicating active recording state.
  const std::vector<TASCAR::wave_t>*
      buff;                ///< Pointer to the target buffer vector.
  uint32_t appendpos = 0u; ///< Current write position within the buffers.
};

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
