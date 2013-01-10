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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fftw3.h>
#include "hoafilt.h"


#define FS 48e3
#define LFFT 0x10000


void printT (const char *file, float *t)
{
    int     i;
    FILE   *F;
 
    F = fopen (file, "w");
    for (i = 0; i < LFFT; i++)
    {
	fprintf (F, "%10.6lf %12.9lf\n", i / FS, t [i]);
    }
    fclose (F);
}


void printlogF (const char *file, fftw_complex *f)
{
    int     i, d, m;
    float   x, y;
    FILE   *F;
 
    F = fopen (file, "w");
    d = 1;
    m = 64;
    for (i = 1; i <= LFFT / 2; i += d)
    {
	x = f[i][0];
        y = f[i][1];
	fprintf (F, "%10.6lf %10.6lf %10.6lf\n",
                 i * FS / LFFT, 
                 10 * log10f (x * x + y * y + 1e-30f), atan2f (y, x));
        if (i == m)
	{
	    m *= 2;
            d *= 2;
	}
    }
    fclose (F);
}


int main (int ac, char *av [])
{
    int            i;
    float          *S;
    double         *T;
    fftw_complex   *F;
    fftw_plan      fwd_plan;
    NF_filt1       filt1a;
    NF_filt2       filt2a;
    NF_filt3       filt3a;
    NF_filt4       filt4a;
    float          d1, d2;

    if (ac < 3) return 1; 
    d1 = atof (av [1]);
    d2 = atof (av [2]);

    S = new float [LFFT];
    T = (double *) fftw_malloc (LFFT * sizeof (double));
    F = (fftw_complex *) fftw_malloc ((LFFT / 2 + 1) * sizeof (fftw_complex));
    fwd_plan = fftw_plan_dft_r2c_1d (LFFT, T, F, FFTW_ESTIMATE);

    if (d1) d1 = 340.0f / (d1 * FS);
    if (d2) d2 = 340.0f / (d2 * FS);

    filt1a.init (d1, d2);
    filt2a.init (d1, d2);
    filt3a.init (d1, d2);
    filt4a.init (d1, d2);

    filt1a.reset ();
    filt2a.reset ();
    filt3a.reset ();
    filt4a.reset ();

    memset (S, 0, LFFT * sizeof (float));
    *S = 1.0f;
    if (d1) filt1a.process (LFFT, S, S);
    else filt1a.process1 (LFFT, S, S);
    for (i = 0; i < LFFT; i++) T [i] = S [i];
    fftw_execute_dft_r2c (fwd_plan, T, F);
    printT ("t1", S);
    printlogF ("f1", F);
    
    memset (S, 0, LFFT * sizeof (float));
    *S = 1.0f;
    if (d1) filt2a.process (LFFT, S, S);
    else filt2a.process1 (LFFT, S, S);
    for (i = 0; i < LFFT; i++) T [i] = S [i];
    fftw_execute_dft_r2c (fwd_plan, T, F);
    printT ("t2", S);
    printlogF ("f2", F);
    
    memset (S, 0, LFFT * sizeof (float));
    *S = 1.0f;
    if (d1) filt3a.process (LFFT, S, S);
    else filt3a.process1 (LFFT, S, S);
    for (i = 0; i < LFFT; i++) T [i] = S [i];
    fftw_execute_dft_r2c (fwd_plan, T, F);
    printT ("t3", S);
    printlogF ("f3", F);
    
    memset (S, 0, LFFT * sizeof (float));
    *S = 1.0f;
    if (d1) filt4a.process (LFFT, S, S);
    else filt4a.process1 (LFFT, S, S);
    for (i = 0; i < LFFT; i++) T [i] = S [i];
    fftw_execute_dft_r2c (fwd_plan, T, F);
    printT ("t4", S);
    printlogF ("f4", F);
    
    
    fftw_destroy_plan (fwd_plan);
    fftw_free (F);
    fftw_free (T);
    delete[] S;

    return 0;
}
