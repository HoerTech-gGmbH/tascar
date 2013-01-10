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


#include "hoafilt.h"


void NF_filt1::init (float w1, float w2, float g)
{
    float b1, g1;

    b1 = 0.5f * w1;
    g1 = 1 + b1;
    _c1 = (2 * b1) / g1;
    g *= g1; 

    b1 = 0.5f * w2;
    g1 = 1 + b1;
    _d1 = (2 * b1) / g1;
    g /= g1;

    _g = g;
}


void NF_filt1::init (NF_filt1& F)
{
    _g  = F._g;
    _c1 = F._c1;
    _d1 = F._d1;
}


void NF_filt1::process (int n, float *ip, float *op, int d)
{
    float x, y, z1;

    z1 = _z1;
    while (n--)
    {
	x = _g * *ip;
        ip += d;
        y = x - _d1 * z1 + 1e-30f;
        x = y + _c1 * z1; 
        z1 += y;
        *op = x;
        op += d; 
    }
    _z1 = z1;
}


void NF_filt1::process1 (int n, float *ip, float *op, int d)
{
    float x, z1;

    z1 = _z1;
    while (n--)
    {
	x = _g * *ip;
        ip += d;
        x -= _d1 * z1 + 1e-30f;
        z1 += x;
        *op = x;
        op += d; 
    }
    _z1 = z1;
}



void NF_filt2::init (float w1, float w2, float g)
{
    float r1, r2, b1, b2, g2;

    r1 = 0.5f * w1;
    r2 = r1 * r1;
    b1 = 3.0f * r1;
    b2 = 3.0f * r2;         
    g2 = 1 + b1 + b2;
    _c1 = (2 * b1 + 4 * b2) / g2;
    _c2 = (4 * b2) / g2;
    g *= g2; 

    r1 = 0.5f * w2;
    r2 = r1 * r1;
    b1 = 3.0f * r1;
    b2 = 3.0f * r2;         
    g2 = 1 + b1 + b2;
    _d1 = (2 * b1 + 4 * b2) / g2;
    _d2 = (4 * b2) / g2;
    g /= g2; 

    _g = g;
}


void NF_filt2::init (NF_filt2& F)
{
    _g  = F._g;
    _c1 = F._c1;
    _c2 = F._c2;
    _d1 = F._d1;
    _d2 = F._d2;
}


void NF_filt2::process (int n, float *ip, float *op, int d)
{
    float x, y, z1, z2;

    z1 = _z1;
    z2 = _z2;
    while (n--)
    {
	x = _g * *ip;
        ip += d;
        y = x - _d1 * z1 - _d2 * z2 + 1e-30f;
        x = y + _c1 * z1 + _c2 * z2; 
        z2 += z1;
        z1 += y;
        *op = x;
        op += d; 
    }
    _z1 = z1;
    _z2 = z2;
}


void NF_filt2::process1 (int n, float *ip, float *op, int d)
{
    float x, z1, z2;

    z1 = _z1;
    z2 = _z2;
    while (n--)
    {
	x = _g * *ip;
        ip += d;
        x -= _d1 * z1 + _d2 * z2 + 1e-30f;
        z2 += z1;
        z1 += x;
        *op = x;
        op += d; 
    }
    _z1 = z1;
    _z2 = z2;
}



void NF_filt3::init (float w1, float w2, float g)
{
    float r1, r2, b1, b2, g1, g2;

    r1 = 0.5f * w1;
    r2 = r1 * r1;
    b1 = 3.6778f * r1;
    b2 = 6.4595f * r2;         
    g2 = 1 + b1 + b2;
    _c1 = (2 * b1 + 4 * b2) / g2;
    _c2 = (4 * b2) / g2;
    g *= g2; 
    b1 = 2.3222f * r1;
    g1 = 1 + b1;
    _c3 = (2 * b1) / g1;
    g *= g1; 

    r1 = 0.5f * w2;
    r2 = r1 * r1;
    b1 = 3.6778f * r1;
    b2 = 6.4595f * r2;         
    g2 = 1 + b1 + b2;
    _d1 = (2 * b1 + 4 * b2) / g2;
    _d2 = (4 * b2) / g2;
    g /= g2; 
    b1 = 2.3222f * r1;
    g1 = 1 + b1;
    _d3 = (2 * b1) / g1;
    g /= g1; 

    _g = g;
}


void NF_filt3::init (NF_filt3& F)
{
    _g  = F._g;
    _c1 = F._c1;
    _c2 = F._c2;
    _c3 = F._c3;
    _d1 = F._d1;
    _d2 = F._d2;
    _d3 = F._d3;
}


void NF_filt3::process (int n, float *ip, float *op, int d)
{
    float x, y, z1, z2, z3;

    z1 = _z1;
    z2 = _z2;
    z3 = _z3;
    while (n--)
    {
	x = _g * *ip;
        ip += d;
        y = x - _d1 * z1 - _d2 * z2 + 1e-30f;
        x = y + _c1 * z1 + _c2 * z2; 
        z2 += z1;
        z1 += y;
        y = x - _d3 * z3 + 1e-30f;
        x = y + _c3 * z3; 
        z3 += y;
        *op = x;
        op += d; 
    }
    _z1 = z1;
    _z2 = z2;
    _z3 = z3;
}


void NF_filt3::process1 (int n, float *ip, float *op, int d)
{
    float x, z1, z2, z3;

    z1 = _z1;
    z2 = _z2;
    z3 = _z3;
    while (n--)
    {
	x = _g * *ip;
        ip += d;
        x -= _d1 * z1 + _d2 * z2 + 1e-30f;
        z2 += z1;
        z1 += x;
        x -= _d3 * z3 + 1e-30f;
        z3 += x;
        *op = x;
        op += d; 
    }
    _z1 = z1;
    _z2 = z2;
    _z3 = z3;
}



void NF_filt4::init (float w1, float w2, float g)
{
    float r1, r2, b1, b2, g2;

    r1 = 0.5f * w1;
    r2 = r1 * r1;
    b1 =  4.2076f * r1;
    b2 = 11.4877f * r2;         
    g2 = 1 + b1 + b2;
    _c1 = (2 * b1 + 4 * b2) / g2;
    _c2 = (4 * b2) / g2;
    g *= g2; 
    b1 = 5.7924f * r1;
    b2 = 9.1401f * r2;         
    g2 = 1 + b1 + b2;
    _c3 = (2 * b1 + 4 * b2) / g2;
    _c4 = (4 * b2) / g2;
    g *= g2; 

    r1 = 0.5f * w2;
    r2 = r1 * r1;
    b1 =  4.2076f * r1;
    b2 = 11.4877f * r2;         
    g2 = 1 + b1 + b2;
    _d1 = (2 * b1 + 4 * b2) / g2;
    _d2 = (4 * b2) / g2;
    g /= g2; 
    b1 = 5.7924f * r1;
    b2 = 9.1401f * r2;         
    g2 = 1 + b1 + b2;
    _d3 = (2 * b1 + 4 * b2) / g2;
    _d4 = (4 * b2) / g2;
    g /= g2; 

    _g = g;
}


void NF_filt4::init (NF_filt4& F)
{
    _g  = F._g;
    _c1 = F._c1;
    _c2 = F._c2;
    _c3 = F._c3;
    _c4 = F._c4;
    _d1 = F._d1;
    _d2 = F._d2;
    _d3 = F._d3;
    _d4 = F._d4;
}


void NF_filt4::process (int n, float *ip, float *op, int d)
{
    float x, y, z1, z2, z3, z4;

    z1 = _z1;
    z2 = _z2;
    z3 = _z3;
    z4 = _z4;
    while (n--)
    {
	x = _g * *ip;
        ip += d;
        y = x - _d1 * z1 - _d2 * z2 + 1e-30f;
        x = y + _c1 * z1 + _c2 * z2; 
        z2 += z1;
        z1 += y;
        y = x - _d3 * z3 - _d4 * z4 + 1e-30f;
        x = y + _c3 * z3 + _c4 * z4; 
        z4 += z3;
        z3 += y;
        *op = x;
        op += d; 
    }
    _z1 = z1;
    _z2 = z2;
    _z3 = z3;
    _z4 = z4;
}


void NF_filt4::process1 (int n, float *ip, float *op, int d)
{
    float x, z1, z2, z3, z4;

    z1 = _z1;
    z2 = _z2;
    z3 = _z3;
    z4 = _z4;
    while (n--)
    {
	x = _g * *ip;
        ip += d;
        x -= _d1 * z1 + _d2 * z2 + 1e-30f;
        z2 += z1;
        z1 += x;
        x -= _d3 * z3 + _d4 * z4 + 1e-30f;
        z4 += z3;
        z3 += x;
        *op = x;
        op += d; 
    }
    _z1 = z1;
    _z2 = z2;
    _z3 = z3;
    _z4 = z4;
}
