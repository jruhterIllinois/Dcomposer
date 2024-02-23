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
#include <X11/Xlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "../inc/dcompser_defs.h"


#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

uint32_t it_address;
hip_t hgfp;
void *lib_handle;
snd_pcm_t *handle;
int env_sock;
unsigned char spctrgm_thrd_running = 0;
Window id;

static void
gaussian_win(float * win)
{
    int i;
    float arg, num, den;
    float sigma = 0.1;

    for(i = 0; i < FFT_SIZE; i++)
    {
        num = i - (FFT_SIZE-1)/2;
        den = sigma*(FFT_SIZE-1)/2;
        arg = num/den;
        win[i] = exp(-0.5*(arg*arg));

    }
}


FILE* lame_fopen(char const* file, char const* mode)
{
    return fopen(file, mode);
}


pid_t pid;
int policy;

lame_global_flags *gfp;
int thread_cnt = 0;
int acq_pipe;
int lame_started = 0;
static char *device = "default"; /* playback device */
//static char *device = "plughw:0,0"; /* playback device */
static snd_pcm_format_t format = SND_PCM_FORMAT_S16; /* sample format */
static unsigned int rate = 44100; /* stream rate */
static unsigned int channels = 1; /* count of channels */
static unsigned int buffer_time = 500000; /* ring buffer length in us */
static unsigned int period_time = 100000; /* period time in us */
static double freq = 440; /* sinusoidal wave frequency in Hz */
static int verbose = 0; /* verbose flag */
static int resample = 0; /* enable alsa-lib resampling */
static int period_event = 0; /* produce poll event after each period */
static snd_pcm_sframes_t buffer_size;
static snd_pcm_sframes_t period_size;
static snd_output_t *output = NULL;
unsigned long spctrgrm_win_id;



void sig_handler(int signum);


void main(int argc, char **argv)
{
    lame_version_t ver;
    char inPath[PATH_MAX + 1];
    char outPath[PATH_MAX + 1];
    FILE * file_in;
    double wavsize;
    int iread;
    short * ssamps_l;
    short * ssamps_r;
    short * pcm_l;
    short * pcm_r;
    int loop_cntr = 0;
    int num_bytes = 0;
    unsigned char * enc_data;
    int err, morehelp;
    int num_samp;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    int method = 0;
    signed short *samples;
    unsigned int chn;
    int orig_sock, // Original socket in client
            len; // Misc. counter
    struct sockaddr_in serv_adr; // Internet addr of server
    snd_pcm_channel_area_t *areas;
    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);
    int i, curr_idx;
    int read_samps;
    struct stat sts;
    struct sched_param test;
    int opt_val;
    char GUI_strt_cmd[30];

    

    
    
    
    
    thread_cnt  = 0;

    signal(SIGINT, sig_handler);


    lib_handle = dlopen("/usr/local/lib/libasound_module_pcm_oss.so", RTLD_LAZY);

    if (!lib_handle)
    {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }


    if((enc_data = (unsigned char *)valloc(65536*sizeof(unsigned char)))==NULL)
    {
        printf("No enough memory\n");
        exit(EXIT_FAILURE);
    }


    pid = getpid();
    test.__sched_priority = 2;

    if((err = sched_setscheduler(0,SCHED_RR,&test))<0)
    {
        printf("Scheduler error :: %s \n",strerror(errno));
        exit(EXIT_FAILURE);
    }

    policy = sched_getscheduler(pid);


    samples = malloc((period_size * channels * snd_pcm_format_physical_width(format)) / 8);
    if (samples == NULL)
    {
        printf("No enough memory\n");
        exit(EXIT_FAILURE);
    }


    //if (argc == 3)
    //{
        
      //  spctrgrm_win_id = strtol(argv[2], NULL, 10);
        //printf("Window Id %i \n", spctrgrm_win_id);
    //}
    


    if (stat(ACQ2PROC_PIPE, &sts) == -1 && errno == ENOENT)
    {
        if(mkfifo(ACQ2PROC_PIPE,0777) < 0)
        {
            printf("Failed to create the pipe \n");
        }
    }

    if((acq_pipe = open(ACQ2PROC_PIPE, O_RDWR))<0)
    {
         printf("Failed open the pipe  %s\n", strerror(errno));
         exit(-1);
    }

    
    printf("Starting the Spec thread \n");
    


    if (pthread_create(&threads[thread_cnt], NULL, spctrgm_thrd, (void *)&thread_cnt))
    {
         printf("Failed to create the thread with errno %s\n", strerror(errno));
         exit(-1);
    }

    while(thread_active[thread_cnt] == 0 )
    {
        usleep(10);

    }

    thread_cnt++;
    


    printf("Got a connection \n");
    
    
    

    //    sprintf(GUI_strt_cmd,"./test_server.tcl %i &" , id);
    //printf("Starting X on id %s \n", GUI_strt_cmd);
    
    
    //system(GUI_strt_cmd);

    //sleep(4);
    
    

    if (pthread_create(&threads[thread_cnt], NULL, fft_thread, (void *)&thread_cnt))
    {
         printf("Failed to create the thread with errno %s\n", strerror(errno));
         exit(-1);
    }

    while(thread_active[thread_cnt] == 0 )
    {
        usleep(10);

    }
    printf("Started Thread \n");

    thread_cnt++;

    


    err = snd_output_stdio_attach(&output, stdout, 0);
    if (err < 0)
    {
        printf("Output failed: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }


    get_lame_version_numerical(&ver);







    /** Create the Envelope Socket ***/
#if 1

   it_address = htonl(INADDR_ANY); // Any interface


    memset(&serv_adr, 0, sizeof ( serv_adr)); // Clear structure
    serv_adr.sin_family = AF_INET; // Set address type
    serv_adr.sin_addr.s_addr = it_address;
    serv_adr.sin_port = htons(ENV_PORT); // Use our fake port
    // SOCKET
    if ((env_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("generate error");
        terminate_all(NULL);
    } // CONNECT



    opt_val = FFT_SIZE*5*sizeof(float);
    if(setsockopt(env_sock,SOL_SOCKET, SO_SNDBUF, &opt_val, sizeof(opt_val) ) < 0)
    {
        printf("Socket Open: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }



    if (connect(env_sock, (struct sockaddr *) & serv_adr,
            sizeof (serv_adr)) < 0)
    {
        perror("connect error");
        terminate_all(NULL);
    }
#endif

    /***************************/

    if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        printf("Playback open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    





    if((ssamps_r = (short *)valloc(20e6*sizeof(short))) == NULL)
    {
        perror("Valloc error");
                exit(EXIT_FAILURE);
    }
    bzero(ssamps_r, 20e6*sizeof(short));

    if((ssamps_l = (short *)valloc(20e6*sizeof(short))) == NULL)
    {
        perror("Valloc error");
                exit(EXIT_FAILURE);
    }
    bzero(ssamps_l, 20e6*sizeof(short));


    if((pcm_r = (short *)valloc(65536*sizeof(short))) == NULL)
    {
        perror("Valloc error");
                exit(EXIT_FAILURE);
    }
    bzero(pcm_r, 65536*sizeof(short));

    if((pcm_l = (short *)valloc(65536*sizeof(short))) == NULL)
    {
        perror("Valloc error");
                exit(EXIT_FAILURE);
    }
    bzero(pcm_l, 65536*sizeof(short));


    if ((err = snd_pcm_set_params(handle,
            SND_PCM_FORMAT_S16,
            SND_PCM_ACCESS_RW_INTERLEAVED,
            1,
            44100,
            1,
            500000)) < 0)
    { /* 0.5sec */
        printf("Playback open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    printf("\n Using LAME Version :: %s \n", get_lame_version());

    gfp = lame_init();

    printf("Initialized the decoder with file %s \n", argv[1]);

    if (argc == 2)
    {

        if ((file_in = lame_fopen(argv[1], "r")) < 0)
        {
            printf("Openning Error \n");
            exit(-1);
        }
        
        printf("Openned file \n");

        /* required call to initialize decoder */
        hgfp = hip_decode_init();


        printf("\n\n\n Reading in from file %s \n", argv[1]);


        loop_cntr = 0;
        num_bytes = 0;

        lame_started = 1;
        
        curr_idx = 0;
        read_samps = 8192;
        while ((num_bytes = fread(enc_data, 1, read_samps, file_in)) == read_samps)
        {
            printf("Decoding data %i \r", curr_idx);

            if ((num_samp = hip_decode(hgfp
                    , enc_data
                    , read_samps
                    , pcm_l
                    , pcm_r
                    )) < 0)
            {
                printf("Decoding Error \n");
                break;
            }
            for(i = 0; i< num_samp; i++)
            {
                ssamps_l[curr_idx + i] = pcm_r[i]/2 + pcm_l[i]/2;
            }
            
            memcpy( &ssamps_r[curr_idx], pcm_r, num_samp*sizeof(short));
            curr_idx+=num_samp;
        }

        printf("Playing Data %i \n", num_bytes);

        num_samp = curr_idx;

        for(curr_idx = 16384*10; curr_idx<num_samp; curr_idx+=FFT_SIZE )
        {

            err = snd_pcm_writei(handle, &ssamps_l[curr_idx], FFT_SIZE_DIV2);
            if (err < 0)
                err = snd_pcm_recover(handle, err, 0);
            if (err < 0)
            {
                printf("snd_pcm_writei failed: %s\n", snd_strerror(err));
                break;
            }
            if (err > 0 && err < (long) sizeof (num_samp))
                printf("Short write (expected %li, wrote %li)\n", (long) sizeof (num_samp), err);


            if (write(acq_pipe, &ssamps_l[curr_idx], FFT_SIZE* sizeof (short)) < FFT_SIZE*sizeof (short))
            {
                printf("Failed to set cancel state \n");
                terminate_all(NULL);
            }

            err = snd_pcm_writei(handle, &ssamps_l[curr_idx + FFT_SIZE_DIV2], FFT_SIZE_DIV2);

            if (err < 0)
                err = snd_pcm_recover(handle, err, 0);
            if (err < 0)
            {
                printf("snd_pcm_writei failed: %s\n", snd_strerror(err));
                break;
            }
            if (err > 0 && err < (long) sizeof (num_samp))
                printf("Short write (expected %li, wrote %li)\n", (long) sizeof (num_samp), err);


            if (write(acq_pipe, &ssamps_l[curr_idx + FFT_SIZE_DIV2], FFT_SIZE* sizeof (short)) < FFT_SIZE*sizeof (short))
            {
                printf("Failed to set cancel state \n");
                terminate_all(NULL);
            }
            
            
            
            


        }




        hip_decode_exit(hgfp);


        lame_close(gfp);
    }

    snd_pcm_close(handle);

    dlclose(lib_handle);
}


void sig_handler(int signum)
{

    terminate_all(NULL);
}


void * terminate_all(void * arg)
{
    int i;


    if(lame_started)
    {
        hip_decode_exit(hgfp);
        lame_close(gfp);
    }

    snd_pcm_close(handle);

    dlclose(lib_handle);

    if(spctrgm_thrd_running)
    {
        close(spctrgm_sd); // Close socket
        unlink(SPCTRGM_SOCK); // Remove it
    }

    close(env_sock);

    for(i = 0; i< thread_cnt; i++)
    {
        printf("Killing thread %i \n", i);
        if(pthread_cancel(threads[i]))
        {
            printf("Thread failed to cancel \n");
        }
        else
        {
            printf("Canceled the %i thread \n", i);
        }
    }

    system("pkill test_server.tcl");
    sleep(1);


    printf("Ending the process \n");
    exit(-1);

}
