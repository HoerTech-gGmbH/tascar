/*
    Copyright (C) 2006 Fons Adriaensen <fons.adriaensen@skynet.be>
    
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


#ifndef __HOAFILT_H
#define __HOAFILT_H


class NF_filt1
{
public:

    NF_filt1 (void) {}
    ~NF_filt1 (void) {}

    void init (float w1, float w2, float g = 1.0f);
    void init (NF_filt1& F);
    void reset (void) { _z1 = 0; }
    void process (int n, float *ip, float *op, int d = 1);
    void process1 (int n, float *ip, float *op, int d = 1);

private:

    float _g;   // gain
    float _c1;  // forward filter coefficient
    float _d1;  // crierse filter coefficient
    float _z1;  // delay element 
};


class NF_filt2
{
public:

    NF_filt2 (void) {}
    ~NF_filt2 (void) {}

    void init (float w1, float w2, float g = 1.0f);
    void init (NF_filt2& F);
    void reset (void) { _z1 = _z2 = 0; }
    void process (int n, float *ip, float *op, int d = 1);
    void process1 (int n, float *ip, float *op, int d = 1);

private:

    float _g;        // gain
    float _c1, _c2;  // forward filter coefficients
    float _d1, _d2;  // crierse filter coefficients
    float _z1, _z2;  // delay elements 
};


class NF_filt3
{
public:

    NF_filt3 (void) {}
    ~NF_filt3 (void) {}

    void init (float w1, float w2, float g = 1.0f);
    void init (NF_filt3& F);
    void reset (void) { _z1 = _z2 = _z3 = 0; }
    void process (int n, float *ip, float *op, int d = 1);
    void process1 (int n, float *ip, float *op, int d = 1);

private:

    float _g;             // gain
    float _c1, _c2, _c3;  // forward filter coefficients
    float _d1, _d2, _d3;  // crierse filter coefficients
    float _z1, _z2, _z3;  // delay elements 
};


class NF_filt4
{
public:

    NF_filt4 (void) {}
    ~NF_filt4 (void) {}

    void init (float w1, float w2, float g = 1.0f);
    void init (NF_filt4& F);
    void reset (void) { _z1 = _z2 = _z3 = _z4 = 0; }
    void process (int n, float *ip, float *op, int d = 1);
    void process1 (int n, float *ip, float *op, int d = 1);

private:

    float _g;                  // gain
    float _c1, _c2, _c3, _c4;  // forward filter coefficients
    float _d1, _d2, _d3, _d4;  // crierse filter coefficients
    float _z1, _z2, _z3, _z4;  // delay elements 
};


#endif

/*
 * Local Variables:
 * mode: c++
 * End:
 */
