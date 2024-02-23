/*
 *               Envelope Processing Source Code
 *               Creates a tree that branches every time
 *               the envelope of the signal seems to change
 *
 *               Envelope is found by sampling and holding the peaks in the
 *               time series and then linearly interpretting the points in between
 *
 * */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <lame/lame.h>
#include <dlfcn.h>
#include "asoundlib.h"
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>             // UNIX protocol
#include <netdb.h>
#include <fftw3.h>

#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "../inc/dcompser_defs.h"
#include "../inc/dcompser_struct.h"

extern int env_sock;

void env_proc(short * ts_in, char * arg2)
{
    int i;
    int err;
    float * env_ts;


    
    if((env_ts = (float *)valloc(FFT_SIZE*sizeof(float))) == NULL)
    {
        perror("Valloc error");
        terminate_all(NULL);
    }
    bzero(env_ts, FFT_SIZE*sizeof(float));


    for(i = 1; i < FFT_SIZE; i++)
    {
        if (fabs((float) ts_in[i]) > env_ts[i - 1])
        {
            env_ts[i] = fabs((float) ts_in[i]);
        } else
        {
            env_ts[i] = env_ts[i - 1] - 1;
        }
    }
 
#if 0
    for(i = 0; i < FFT_SIZE; i++)
    {

                env_ts[i] = (float) fabs((float)env_ts[i]);
         
    }
#endif
    
    if ((err = write(env_sock, env_ts, (int) FFT_SIZE * sizeof (float))) < FFT_SIZE  * sizeof (float))
    {
        printf("Error writting to socket \n");
        exit(-1);
    }
    free(env_ts);

}