/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include "jackiowav.h"
#include <iostream>

jackio_t::jackio_t(const std::string& ifname, const std::string& ofname,
                   const std::vector<std::string>& ports,
                   const std::string& jackname, int freewheel, int autoconnect,
                   bool verbose)
    : jackc_transport_t(jackname), sf_in(NULL), sf_out(NULL), buf_in(NULL),
      buf_out(NULL), pos(0), b_quit(false), start(false), freewheel_(freewheel),
      use_transport(false), startframe(0), nframes_total(0), p(ports),
      b_cb(false), b_verbose(verbose), wait_(false), cpuload(0), xruns(0)
{
  memset(&sf_inf_in, 0, sizeof(sf_inf_in));
  memset(&sf_inf_out, 0, sizeof(sf_inf_out));
  log("opening input file " + ifname);
  if(!(sf_in = sf_open(ifname.c_str(), SFM_READ, &sf_inf_in)))
    throw TASCAR::ErrMsg("unable to open input file \"" + ifname +
                         "\" for reading.");
  sf_inf_out = sf_inf_in;
  sf_inf_out.samplerate = get_srate();
  sf_inf_out.channels =
      std::max(0, (int)(p.size()) - (int)(sf_inf_in.channels));
  sf_inf_out.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT | SF_ENDIAN_FILE;
  if(autoconnect) {
    p.clear();
    for(int ch = 0; ch < sf_inf_in.channels; ch++) {
      char ctmp[100];
      sprintf(ctmp, "system:playback_%d", ch + 1);
      p.push_back(ctmp);
    }
    p.push_back("system:capture_1");
  }
  if(ofname.size()) {
    log("creating output file " + ofname);
    if(!(sf_out = sf_open(ofname.c_str(), SFM_WRITE, &sf_inf_out))) {
      std::string errmsg("Unable to open output file \"" + ofname + "\": ");
      errmsg += sf_strerror(NULL);
      throw TASCAR::ErrMsg(errmsg);
    }
  }
  char c_tmp[1024];
  nframes_total = (uint32_t)sf_inf_in.frames;
  sprintf(c_tmp, "%u", nframes_total);
  log("allocating memory for " + std::string(c_tmp) + " audio frames");
  buf_in =
      new float[std::max((sf_count_t)1, sf_inf_in.channels * sf_inf_in.frames)];
  buf_out = new float[std::max((sf_count_t)1,
                               sf_inf_out.channels * sf_inf_in.frames)];
  memset(buf_out, 0, sizeof(float) * sf_inf_out.channels * sf_inf_in.frames);
  memset(buf_in, 0, sizeof(float) * sf_inf_in.channels * sf_inf_in.frames);
  log("reading input file into memory");
  sf_readf_float(sf_in, buf_in, sf_inf_in.frames);
  for(unsigned int k = 0; k < (unsigned int)sf_inf_out.channels; k++) {
    sprintf(c_tmp, "in_%u", k + 1);
    log("adding input port " + std::string(c_tmp));
    add_input_port(c_tmp);
  }
  for(unsigned int k = 0; k < (unsigned int)sf_inf_in.channels; k++) {
    sprintf(c_tmp, "out_%u", k + 1);
    log("adding output port " + std::string(c_tmp));
    add_output_port(c_tmp);
  }
}

jackio_t::jackio_t(double duration, const std::string& ofname,
                   const std::vector<std::string>& ports,
                   const std::string& jackname, int freewheel, int autoconnect,
                   bool verbose)
    : jackc_transport_t(jackname), freewheel_(freewheel),
      nframes_total(std::max(1u, uint32_t(get_srate() * duration))), p(ports),
      b_verbose(verbose)
{
  memset(&sf_inf_in, 0, sizeof(sf_inf_in));
  memset(&sf_inf_out, 0, sizeof(sf_inf_out));
  sf_inf_out.samplerate = get_srate();
  sf_inf_out.channels = std::max(1, (int)(p.size()));
  sf_inf_out.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT | SF_ENDIAN_FILE;
  if(autoconnect) {
    p.clear();
    p.push_back("system:capture_1");
  }
  if(ofname.size()) {
    log("creating output file " + ofname);
    if(!(sf_out = sf_open(ofname.c_str(), SFM_WRITE, &sf_inf_out))) {
      std::string errmsg("Unable to open output file \"" + ofname + "\": ");
      errmsg += sf_strerror(NULL);
      throw TASCAR::ErrMsg(errmsg);
    }
  }
  char c_tmp[1024];
  sprintf(c_tmp, "%u", nframes_total);
  log("allocating memory for " + std::string(c_tmp) + " audio frames");
  buf_out = new float[sf_inf_out.channels * nframes_total];
  memset(buf_out, 0, sizeof(float) * sf_inf_out.channels * nframes_total);
  for(unsigned int k = 0; k < (unsigned int)sf_inf_out.channels; k++) {
    sprintf(c_tmp, "in_%u", k + 1);
    log("add input port " + std::string(c_tmp));
    add_input_port(c_tmp);
  }
}

jackio_t::jackio_t(const std::vector<TASCAR::wave_t>& isig,
                   std::vector<TASCAR::wave_t>& osig,
                   const std::vector<std::string>& ports,
                   const std::string& jackname, int freewheel, bool verbose)
    : jackc_transport_t(jackname), freewheel_(freewheel), p(ports),
      b_verbose(verbose), osig_(osig)
{
  for(const auto& sig : isig)
    nframes_total = std::max(nframes_total, sig.n);
  for(const auto& sig : osig)
    nframes_total = std::max(nframes_total, sig.n);
  memset(&sf_inf_in, 0, sizeof(sf_inf_in));
  memset(&sf_inf_out, 0, sizeof(sf_inf_out));
  sf_inf_in.samplerate = get_srate();
  sf_inf_in.channels = isig.size();
  sf_inf_out.samplerate = get_srate();
  sf_inf_out.channels = osig.size();
  buf_in = new float[std::max((size_t)1u, isig.size() * nframes_total)];
  memset(buf_in, 0, sizeof(float) * isig.size() * nframes_total);
  buf_out = new float[std::max((size_t)1u, osig.size() * nframes_total)];
  memset(buf_out, 0, sizeof(float) * osig.size() * nframes_total);
  log("reading input file into memory");
  for(size_t k = 0; k < osig.size(); ++k) {
    char c_tmp[1024];
    sprintf(c_tmp, "in_%lu", k + 1);
    log("adding input port " + std::string(c_tmp));
    add_input_port(c_tmp);
  }
  for(size_t ch = 0; ch < isig.size(); ++ch) {
    char c_tmp[1024];
    sprintf(c_tmp, "out_%lu", ch + 1);
    log("adding output port " + std::string(c_tmp));
    add_output_port(c_tmp);
    // copy input signal:
    for(size_t f = 0; f < isig[ch].n; ++f)
      buf_in[ch + sf_inf_in.channels * f] = isig[ch].d[f];
  }
}

jackio_t::~jackio_t()
{
  log("cleaning up file handles");
  if(sf_in) {
    sf_close(sf_in);
  }
  if(sf_out) {
    sf_close(sf_out);
  }
  log("deallocating memory");
  if(buf_in)
    delete[] buf_in;
  if(buf_out)
    delete[] buf_out;
}

int jackio_t::process(jack_nframes_t nframes,
                      const std::vector<float*>& inBuffer,
                      const std::vector<float*>& outBuffer, uint32_t tp_frame,
                      bool tp_rolling)
{
  b_cb = true;
  bool record(start);
  if(use_transport)
    record &= tp_rolling;
  if(wait_)
    record &= tp_frame >= startframe;
  for(unsigned int k = 0; k < nframes; k++) {
    if(record && (pos < nframes_total)) {
      if(buf_in)
        for(unsigned int ch = 0; ch < outBuffer.size(); ch++)
          outBuffer[ch][k] = buf_in[sf_inf_in.channels * pos + ch];
      for(unsigned int ch = 0; ch < inBuffer.size(); ch++)
        buf_out[sf_inf_out.channels * pos + ch] = inBuffer[ch][k];
      pos++;
    } else {
      if(pos >= nframes_total)
        b_quit = true;
      for(unsigned int ch = 0; ch < outBuffer.size(); ch++)
        outBuffer[ch][k] = 0;
    }
  }
  return 0;
}

void jackio_t::log(const std::string& msg)
{
  if(b_verbose)
    std::cerr << msg << std::endl;
}

void jackio_t::run()
{
  log("activating jack client");
  activate();
  for(unsigned int k = 0; k < (unsigned int)sf_inf_in.channels; k++)
    if(k < p.size()) {
      log("connecting output port to " + p[k]);
      connect_out(k, p[k]);
    }
  for(unsigned int k = 0; k < (unsigned int)sf_inf_out.channels; k++)
    if(k + sf_inf_in.channels < p.size()) {
      log("connecting input port to " + p[k + sf_inf_in.channels]);
      connect_in(k, p[k + sf_inf_in.channels], false, true);
    }
  if(freewheel_) {
    log("switching to freewheeling mode");
    jack_set_freewheel(jc, 1);
  }
  if(use_transport && (!wait_)) {
    log("locating to startframe");
    tp_stop();
    tp_locate(startframe);
  }
  b_cb = false;
  while(!b_cb) {
    usleep(5000);
  }
  start = true;
  if(use_transport && (!wait_)) {
    log("starting transport");
    tp_start();
  }
  log("waiting for data to complete");
  while(!b_quit) {
    usleep(5000);
  }
  cpuload = get_cpu_load();
  xruns = get_xruns();
  if(use_transport && (!wait_)) {
    log("stopping transport");
    tp_stop();
  }
  if(freewheel_) {
    log("deactivating freewheeling mode");
    jack_set_freewheel(jc, 0);
  }
  log("deactivating jack client");
  deactivate();
  if(sf_out)
    sf_writef_float(sf_out, buf_out, nframes_total);
  // if osig_ is not empty, copy signal to osig_:
  for(size_t ch = 0; ch < osig_.size(); ++ch) {
    for(size_t f = 0; f < osig_[ch].n; ++f)
      osig_[ch].d[f] = buf_out[ch + sf_inf_out.channels * f];
  }
}

void jackio_t::set_transport_start(double start_, bool wait)
{
  use_transport = true;
  wait_ = wait;
  startframe = (uint32_t)(get_srate() * start_);
}

jackrec_async_t::jackrec_async_t(const std::string& ofname,
                                 const std::vector<std::string>& ports,
                                 const std::string& jackname, double buflen,
                                 int format, bool usetransport_)
    : jackc_transport_t(jackname), rectime(0), xrun(0), werror(0), sf_out(NULL),
      rb(NULL), run_service(true), tscale(1), recframes(0),
      channels(ports.size()), usetransport(usetransport_)
{
  if(!channels)
    throw TASCAR::ErrMsg("No sources selected.");
  memset(&sf_inf_out, 0, sizeof(sf_inf_out));
  sf_inf_out.samplerate = get_srate();
  sf_inf_out.channels = (int)(ports.size());
  sf_inf_out.format = format;
  if(ofname.size()) {
    if(!(sf_out = sf_open(ofname.c_str(), SFM_WRITE, &sf_inf_out))) {
      std::string errmsg("Unable to open output file \"" + ofname + "\": ");
      errmsg += sf_strerror(NULL);
      throw TASCAR::ErrMsg(errmsg);
    }
  }
  unsigned int k(0);
  char c_tmp[1024];
  for(auto p : ports) {
    ++k;
    sprintf(c_tmp, "in_%u", k);
    add_input_port(c_tmp);
  }
  if(buflen < 2.0)
    buflen = 2.0;
  rb = jack_ringbuffer_create((size_t)(buflen * get_srate() * (double)channels *
                                       (double)(sizeof(float))));
  buf = new float[channels * get_fragsize()];
  rlen = channels * get_srate();
  rbuf = new float[rlen];
  activate();
  k = 0;
  for(auto& p : ports) {
    connect_in(k, p, true, true);
    ++k;
  }
  tscale = 1.0 / get_srate();
  srv = std::thread(&jackrec_async_t::service, this);
}

jackrec_async_t::~jackrec_async_t()
{
  deactivate();
  run_service = false;
  srv.join();
  if(sf_out) {
    sf_close(sf_out);
  }
  if(rb)
    jack_ringbuffer_free(rb);
  delete[] buf;
  delete[] rbuf;
}

void jackrec_async_t::service()
{
  size_t rchunk(rlen * sizeof(float));
  while(run_service) {
    if(jack_ringbuffer_read_space(rb) >= rchunk) {
      size_t rcnt(jack_ringbuffer_read(rb, (char*)rbuf, rchunk));
      rcnt /= sizeof(float) * channels;
      size_t wcnt(sf_writef_float(sf_out, rbuf, rcnt));
      if(wcnt < rcnt)
        ++werror;
    }
    usleep(100);
  }
  size_t rcnt(0);
  do {
    rcnt = jack_ringbuffer_read(rb, (char*)rbuf, rchunk);
    rcnt /= sizeof(float) * channels;
    sf_writef_float(sf_out, rbuf, rcnt);
  } while(rcnt > 0);
}

int jackrec_async_t::process(jack_nframes_t nframes,
                             const std::vector<float*>& inBuffer,
                             const std::vector<float*>&, uint32_t,
                             bool b_rolling)
{
  if(usetransport && (!b_rolling))
    return 0;
  size_t ch(inBuffer.size());
  for(size_t k = 0; k < nframes; ++k)
    for(size_t c = 0; c < ch; ++c)
      buf[ch * k + c] = inBuffer[c][k];
  size_t wcnt(ch * nframes * sizeof(float));
  size_t cnt(jack_ringbuffer_write(rb, (const char*)buf, wcnt));
  if(cnt < wcnt)
    ++xrun;
  recframes += nframes;
  rectime = (double)recframes * tscale;
  return 0;
}

jackrec2wave_t::jackrec2wave_t(size_t channels, const std::string& jackname)
    : jackc_t(jackname)
{
  isrecording = false;
  for(size_t k = 0; k < channels; ++k)
    add_input_port(std::string("in.") + std::to_string(k));
  activate();
}

jackrec2wave_t::~jackrec2wave_t()
{
  deactivate();
}

void jackrec2wave_t::rec(const std::vector<TASCAR::wave_t>& w,
                         const std::vector<std::string>& ports)
{
  size_t ch = std::min(w.size(), std::min(get_num_input_ports(), ports.size()));
  for(size_t k = 0; k < ch; ++k)
    connect_in((unsigned int)k, ports[k], true, true);
  buff = &w;
  appendpos = 0u;
  isrecording = true;
  while(isrecording)
    usleep(1000);
  buff = NULL;
  for(size_t k = 0; k < ch; ++k)
    disconnect_in((unsigned int)k);
}

int jackrec2wave_t::process(jack_nframes_t n, const std::vector<float*>& s_in,
                            const std::vector<float*>&)
{
  if(isrecording) {
    if(buff) {
      size_t channels = std::min(buff->size(), s_in.size());
      for(size_t ch = 0; ch < channels; ++ch) {
        const TASCAR::wave_t& w = (*buff)[ch];
        if(appendpos > w.n) {
          // buffer is filled.
          isrecording = false;
          return 0;
        }
        uint32_t write_space = w.n - appendpos;
        memmove(&w.d[appendpos], s_in[ch],
                std::min(write_space, n) * sizeof(float));
      }
      appendpos += n;
    } else {
      isrecording = false;
    }
  }
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
