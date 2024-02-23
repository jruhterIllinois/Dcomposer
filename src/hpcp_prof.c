/*
 *             File contains the Harmonic Pitch class Profile
 *             From the input of the spectral data
 *
 *             Generate the Pitch class profile
 *
 *             1, Frequency filtering based on a pitch class filter bank
 *                 p = 69 + 12*log2(f/440) <-- translation from freq to pitch class linearity
 *                 0 is C
 *                 1 is C sharp or D flat
 *                 2 is D
 *                 3 is D sharp or E flat
 *                 4 is E
 *                 5 is F
 *                 6 is F sharp of G flat
 *                 7 is G
 *                 8 is G sharp or A flat
 *                 9 is A
 *                 10 is A sharp of B float 
 *                 11 is B
 *
 *                 middle octave C4 (261.63 Hz) is 60
 *                 The note is taken as the modulo of 12
 *                 The ocative is taken as the division of 12
 *
 *            2, Generate the Harmonic Pitch Class Profile Using a
 *               Harmonic Summation Procedures that uses the following table
 *
 *              *--------------------------------------------
 *               | n    |  1  |  2  |  3  |  4  |  5  |  6  |
 *               --------------------------------------------
 *               | freq |  f  | 2f  | 3f  |  4f |  5f |  6f |
 *               -------------------------------------------
 *               | W    |  1  | s   | s^2 | s^3 | s^4 | s^5 |
 *               -------------------------------------------*
 */


#include <sys/ioctl.h>
#include <ctype.h>
#include <wait.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>             // UNIX protocol
#include <netdb.h>
#include <fftw3.h>
#include <f2c.h>
#include <blaswrap.h>
#include <clapack.h>
#include "../inc/dcompser_defs.h"

#define BIN2FREQ( arg )  (float)(arg+0.5)*( (float)22050 / (float )FFT_SIZE)
#define FREQ2BIN( arg )  (int)((float)(arg+0.5)*( (float)FFT_SIZE / (float)22050 ))


void extract_hpcp(float * ps_in, int * chroma, int  * octave, float * freq_out )
{
    int i, j, new_note, old_note, start_bin,stop_bin,harmonic_bin, num_notes;
    double freq, curr_ps, max_ps, start_freq, mean_ps;

    max_ps = 0.0;
    curr_ps = 0.0;

    start_bin = FREQ2BIN(100.0 );
    stop_bin = FREQ2BIN(3000.0 );
    for(i = start_bin; i < stop_bin; i++)
    {
        mean_ps += ps_in[i];
    }

    mean_ps /= (stop_bin- start_bin);

    num_notes = 0;
    for(i = start_bin; i < stop_bin; i++)
    {
        freq = BIN2FREQ(i);
        new_note = (int)(69 + 12*log2((double)freq/440));


        if(new_note != old_note)
        {
            num_notes++;
          //  printf("Frequency regions for %i are %f to %f w %f\n", old_note, start_freq, freq, curr_ps);
            if(curr_ps > max_ps)
            {
                max_ps = curr_ps;
                chroma[0] = (int) ((old_note)%12);
                octave[0] = (int) ((old_note)/12);
                freq_out[0] = (float)start_freq;

            //    printf("New Chroma found at %i in Octave %i @ bin %i %i %f, %f to %f \n", chroma[0] , octave[0],  i, old_note, curr_ps, start_freq, freq);
            }
            start_freq = freq;
            curr_ps = 0.0;
        }

        if (ps_in[i] > mean_ps * mean_ps)
        {


            for (j = 1; j < 7; j++)
            {
                harmonic_bin = FREQ2BIN(freq * j);

                if (harmonic_bin < FFT_SIZE_DIV2)
                {

                    curr_ps += pow(ps_in[harmonic_bin], j);
                }
            }
        }
        else
        {
            curr_ps +=  ps_in[i];
        }
        old_note =  new_note;
    }
 //   printf("Number of Notes analyzed %i \n", num_notes);
}


