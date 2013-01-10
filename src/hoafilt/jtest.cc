/*
    Copyright (C) 2005-2007 Fons Adriaensen <fons@kokkinizita.net>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <jack/jack.h>
#include "hoafilt.h"


// -------------------------------------------------------------------------------------------------


class Jack_client
{
public:

    Jack_client (void);
    ~Jack_client (void);

    void init (void);
    void process (jack_nframes_t nframes);

    jack_client_t   *_jack_handle;
    jack_port_t     *_ip_port;
    jack_port_t     *_op_port;
    long             _rate;
    long             _size;
    NF_filt4         _filtA;
    NF_filt4         _filtB;
};


Jack_client::Jack_client (void)
{
}


Jack_client::~Jack_client (void)
{
}


void Jack_client::init (void)
{
    _filtA.init (340.0f / (0.5f * _rate), 340.0f / (5.0f * _rate));
    _filtB.init (340.0f / (5.0f * _rate), 340.0f / (0.5f * _rate));
    puts ("INIT DONE");
}


void Jack_client::process (jack_nframes_t nframes)
{
    float *ip, *op;

    ip = (float *)(jack_port_get_buffer (_ip_port, nframes));
    op = (float *)(jack_port_get_buffer (_op_port, nframes));

    _filtA.process (nframes, ip, op);
    _filtB.process (nframes, op, op);
}


// -------------------------------------------------------------------------------------------------


static int process (jack_nframes_t nframes, void *arg)
{
    Jack_client *JC = (Jack_client *) arg;
    JC->process (nframes);
    return 0;  
}


int main (int ac, char *av [])
{
    Jack_client *JC = new Jack_client;

    if ((JC->_jack_handle = jack_client_new ("hoafilt")) == 0)
    {
        fprintf (stderr, "Can't connect to JACK\n");
        exit (1);
    }

    JC->_rate = jack_get_sample_rate (JC->_jack_handle);
    JC->_size = jack_get_buffer_size (JC->_jack_handle);
    JC->init ();
    jack_set_process_callback (JC->_jack_handle, process, JC);
    JC->_ip_port = jack_port_register (JC->_jack_handle, "in",  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput,  0);
    JC->_op_port = jack_port_register (JC->_jack_handle, "out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    if (jack_activate (JC->_jack_handle))
    {
        fprintf (stderr, "Can't activate JACK\n");
        exit (1);
    }

    sleep (10000);
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------------
