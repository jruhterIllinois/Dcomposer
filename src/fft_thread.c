
       /*
           Internet domain, connection-oriented CLIENT
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

extern uint32_t it_address;
extern int acq_pipe;
static int cleanup_pop_arg1 = 0;
extern int lame_started;


//A[mxn] = m rows N columns

 /*
  Matrix A (3 x 4) is:
  1.0000  0.5000  0.3333  0.2500
  0.5000  0.3333  0.2500  0.2000
  0.3333  0.2500  0.2000  0.1667

   int m = 3;
   int n = 4;

   int p = (m < n ? m : n);

   double A[m * n];
   double U[m * m];
   double VT[n * n];
   double S[p * 1];
  *

  sgesvd_(&JOBU, &JOBVT, &M, &N,
                        (real*)A.begin(), &LDA,
                        (real*)S.begin(),
                        (real*)U.begin(), &LDU,
                        (real*)V.begin(), &LDVT,
                        wkf, &LWORK, &INFO);
  *
 */

#define MATRIX_IDX(n, i, j) j*n + i



//m is the number of rows
//n is the number of columns
//j sequences horizontally
//i sequences vertically

#define MATRIX_ELEMENT(A, m, n, i, j) A[ MATRIX_IDX(m, i, j) ]

extern void extract_hpcp(float * ps_in, int * chroma, int * octave, float * freq_out );
extern void env_proc(short * ts_in, char * arg2);

static void
client_kill(void * arg)
{
    printf("Killing the client socket \n");
    close( ((int *)arg)[0] );

}


static void
blackman_harris(float * win)
{
    float alpha_0 = 0.35875;
    float alpha_1 = 0.48829;
    float alpha_2 = 0.14128;
    float alpha_3 = 0.01168;
    int i;

    for(i = 0; i < FFT_SIZE; i++)
    {
        win[i] = alpha_0 - 
                 alpha_1*(cos((2*M_PI*i)/(FFT_SIZE))) +
                 alpha_2*(cos((4*M_PI*i)/(FFT_SIZE))) -
                 alpha_3*(cos((6*M_PI*i)/(FFT_SIZE)));

    }
}


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


void  *fft_thread(void * arg)
{
    int orig_sock, // Original socket in client
            len; // Misc. counter
    struct sockaddr_in serv_adr; // Internet addr of server
    int j = 100, err, win_length;
    uint32_t idx, i = 0, k, max_idx;
    float * ps, * window, *gwindow;
    float hpcp_freq;
    float signal_t;
    integer num_rows = NUM_ROWS;
    integer num_cols = NUM_COLS;
    integer lda = NUM_ROWS;
    integer lvdt =  NUM_COLS;
    integer ldu = NUM_ROWS;

    int p = NUM_ROWS > NUM_COLS ? NUM_ROWS : NUM_COLS;
    float s[ p ];
    float col_mv;
    float u[NUM_ROWS*NUM_ROWS];
    float vt[NUM_COLS*NUM_COLS];
    float temp_mat[NUM_ROWS*NUM_COLS];
    float max_val;
    short * raw_pcm;
    char jobu = 'A';
    char jobv = 'A';
    
    integer lwork =-1;  // dimension of WORK
    float work[8*NUM_ROWS];  // workspace (man page recommends at least 5*min(M,N);
                // see man page for more)
    integer info;
    int opt_val;
    int note_in, octave_in;
    int loop_cnt = 0;
    double * in;
    fftw_complex * out;
    fftw_complex *temp;
    int oldtype;
    fftw_plan plan;
    char chroma_notes[12][2] = {CHROMATIC_SCALE};
    struct sched_param thr_priority;



    if(pthread_detach(threads[*((int *)arg)]) != 0 )
    {
        printf("Error Detaching thead \n");
        terminate_all(NULL);

    }

    printf("Detached thread \n");
    if(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype))
    {
        printf("Failed to set cancel state \n");
        terminate_all(NULL);

    }


    thr_priority.__sched_priority = 1;
    if(pthread_setschedparam( pthread_self(), policy , &thr_priority ) != 0)
    {

        printf("Failed to set thread priority \n");
        terminate_all(NULL);

    }

   it_address = htonl(INADDR_ANY); // Any interface


    memset(&serv_adr, 0, sizeof ( serv_adr)); // Clear structure
    serv_adr.sin_family = AF_INET; // Set address type
    serv_adr.sin_addr.s_addr = it_address;
    serv_adr.sin_port = htons(PORT); // Use our fake port
    // SOCKET
    if ((orig_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("generate error");
        terminate_all(NULL);
    } // CONNECT



    opt_val = FFT_SIZE*100*sizeof(float);
    if(setsockopt(orig_sock,SOL_SOCKET, SO_REUSEADDR | SO_SNDBUF, &opt_val, sizeof(opt_val) ) < 0)
    {
        printf("Socket Open: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }



    if (connect(orig_sock, (struct sockaddr *) & serv_adr,
            sizeof (serv_adr)) < 0)
    {
        perror("connect error");
        terminate_all(NULL);
    }

    
    
    
    printf("Creating client \n");
    it_address = htonl(INADDR_ANY); // Any interface


    memset(&serv_adr, 0, sizeof ( serv_adr)); // Clear structure
    serv_adr.sin_family = AF_INET; // Set address type
    serv_adr.sin_addr.s_addr = it_address;
    serv_adr.sin_port = htons(SPCTRGM_PORT); // Use our fake port
    
    
    // SOCKET
    if ((spctrgm_sd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("generate error");
        terminate_all(NULL);
    } // CONNECT



    opt_val = FFT_SIZE*5*sizeof(float);
    if(setsockopt(spctrgm_sd,SOL_SOCKET, SO_SNDBUF, &opt_val, sizeof(opt_val) ) < 0)
    {
        printf("Socket Open: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }



    if (connect(spctrgm_sd, (struct sockaddr *) & serv_adr,
            sizeof (serv_adr)) < 0)
    {
        perror("connect error");
        terminate_all(NULL);
    }
    
    pthread_cleanup_push(client_kill, (void *)&orig_sock)



     if((raw_pcm = (short *)valloc(FFT_SIZE*sizeof(short))) == NULL)
    {
        perror("Valloc error");
        terminate_all(NULL);
    }
    bzero(raw_pcm, FFT_SIZE*sizeof(short));



     if((temp = (fftw_complex *)valloc(FFT_SIZE*sizeof(fftw_complex))) == NULL)
    {
        perror("Valloc error");
        terminate_all(NULL);
    }
    bzero(temp, FFT_SIZE*sizeof(fftw_complex));

    if((in = (double *)valloc(FFT_SIZE*sizeof(double))) == NULL)
    {
        perror("Valloc error");
        terminate_all(NULL);
    }
    bzero(in, FFT_SIZE*sizeof(double));

    if((out = (fftw_complex *)valloc(FFT_SIZE*sizeof(fftw_complex))) == NULL)
    {
        perror("Valloc error");
        terminate_all(NULL);
    }
    bzero(out, FFT_SIZE*sizeof(fftw_complex));

  //  for(i = 0; i< 12 ; i++)
    //    printf("%c%c\n", chroma_notes[i][0],chroma_notes[i][1]);

    if((ps = (float *)valloc(FFT_SIZE*3*sizeof(float))) == NULL)
    {
        perror("Valloc error");
        terminate_all(NULL);
    }
    bzero(ps, FFT_SIZE*3*sizeof(float));


    if((window = (float *)valloc(FFT_SIZE*sizeof(float))) == NULL)
    {
        perror("Valloc error");
        terminate_all(NULL);
    }
    blackman_harris(window);


    if((gwindow = (float *)valloc(FFT_SIZE*sizeof(float))) == NULL)
    {
        perror("Valloc error");
        terminate_all(NULL);
    }
    gaussian_win(gwindow);
    
    plan = fftw_plan_dft_r2c_1d(FFT_SIZE, in, out, FFTW_ESTIMATE);

    thread_active[*((int *)arg)] = 1;

    printf("Initialized Thread \n");
    i = 0;
    signal_t = 0.0;
    while(1)
    { // Process

         if(read( acq_pipe, raw_pcm, FFT_SIZE*sizeof(short)) < FFT_SIZE*sizeof(short))
         {
                printf("Failed to set cancel state \n");
                terminate_all(NULL);
         }

     //    printf("Read from the first thread %i\n", loop_cnt);
        loop_cnt++;

        for(i = 0; i< FFT_SIZE; i++)
        {
            temp[i][0]=raw_pcm[i]*window[i];
            temp[i][1]=0;
        }

/*
            for(i = 0; i< FFT_SIZE; i++)
            {
               //  signal_t += i;
                 temp[i][0]+= (float)cos(j*2*M_PI*(idx)/4096 );//+ (1/(2*M_PI*200))*signal_t  );//*window[i];
                 temp[i][1]+= (float)sin(j*2*M_PI*(idx)/4096); //+ (1/(2*M_PI*200))*signal_t  );//*window[i];

                idx++;
            }
*/
         
        for(i = 0; i< FFT_SIZE; i++)
        {
            in[i]= temp[i][0];
        }

        fftw_execute(plan);

        for(i = 0; i< FFT_SIZE; i++)
        {
            ps[i+FFT_SIZE] = (float)raw_pcm[i];
        }

/*
        for(i = 0; i< FFT_SIZE; i++)
        {
            in[i][0]= temp[i][0]*gwindow[i];
            in[i][1]= temp[i][1]*gwindow[i];
        }

        fftw_execute(plan);
*/

            idx = 0;

//column major fill
        for(i = 0; i< num_cols; i++)
        {
            col_mv = 0;
            for(j =0; j< num_rows; j++)
            {
                col_mv += (float) (out[j*num_rows +i][0]*out[j*num_rows +i][0] +  out[j*num_rows +i][1]*out[j*num_rows +i][1]);
         //         col_mv += (float) raw_pcm[j*num_rows +i];
            }

            col_mv /= num_rows;
            for(j =0; j< num_rows; j++)
            {
                temp_mat[idx] = (float) (out[j*num_rows +i][0]*out[j*num_rows +i][0] +  out[j*num_rows +i][1]*out[j*num_rows +i][1]) - col_mv;
         //       temp_mat[idx] = (float) raw_pcm[j*num_rows +i] - col_mv;
                idx++;
            }

        }



  //          printf("TEST IN %i %i\n", lda, lvdt);
            
        sgesvd_(&jobu, &jobv, &num_rows, &num_cols, temp_mat, &lda, s, u, &ldu, vt, &lvdt,
	   work, &lwork, &info);

        if(info == -1)
        {
                printf("Failed to set cancel state \n");
                terminate_all(NULL);
        }

        lwork = work[0];
     
        sgesvd_(&jobu, &jobv, &num_rows, &num_cols, temp_mat, &lda, s, u, &ldu, vt, &lvdt,
	   work, &lwork, &info);

//printf("TEST OUT \n");

       //m is the number of rows
//n is the number of columns
//j sequences horizontally
//i sequences vertically
// #define MATRIX_ELEMENT(A, m, n, i, j) A[ MATRIX_IDX(m, i, j) ]

        bzero(ps, FFT_SIZE*2*sizeof(float));
        env_proc( raw_pcm, NULL);

     //   bzero(&s[20], (num_cols- 20)*sizeof(float));

#if 0
        //column major fill

        idx = 0;
        for(i = 0; i< num_cols; i++)
        {
            for(j =0; j< num_rows; j++)
            {

                temp_mat[idx] = u[idx++]*s[i];
            //    temp_mat[idx] = abs(ps[idx]);
            }
        }



        //row major
        idx = 0;
        for(j =0; j< num_rows; j++)
        {

            for(i = 0; i< num_cols; i++)
            {
          

                ps[idx] = 0;
                for (k = 0; k < num_rows; k++)
                {
                     ps[idx]  += MATRIX_ELEMENT(temp_mat, num_rows, num_rows, k , j)*
                                   MATRIX_ELEMENT(vt, num_cols, num_cols, i , k);
                }

            //     ps[idx] = temp_mat[idx];
            //    ps[idx] = ps[idx];
                idx++;
            }
        }

#endif

       
        idx = 0;
        for(j =0; j< num_rows; j++)
        {

            for(i = 0; i< num_cols; i++)
            {

      //      ps[idx] = 20*log10(abs(s[idx%p]));
            //  ps[idx] = 20*log10(abs(u[j*num_rows + i]));
                ps[idx] = 10*log10(abs(s[j]));
             //   ps[idx] = vt[j*num_rows +i];
            //    ps[idx] = 20*log10(abs(temp_mat[j*num_rows +i]));
              idx++;
            }
        }


        for(i = 0; i< FFT_SIZE; i++)
        {
            ps[i+FFT_SIZE] = (float)in[i];
        }



       if(!(out[FFT_SIZE/4][0] == 0.0 ||  out[FFT_SIZE/4][1] == 0.0))
       {

      
       for(i = 0; i< FFT_SIZE/2; i++)
       {
            ps[i +FFT_SIZE/2] = out[i][0]*out[i][0] +  out[i][1]*out[i][1];
     //  ps[idx] = out[i][0]*out[i][0] +  out[i][1]*out[i][1];
       }




       extract_hpcp(&ps[FFT_SIZE/2], &note_in, &octave_in, &hpcp_freq );


       for(i = 0; i< FFT_SIZE/2; i++)
       {
            ps[i+FFT_SIZE/2] = 10*log10(ps[i +FFT_SIZE/2]);
       }

             printf("Returned the %c%c on the %i octave %f \r", chroma_notes[note_in][0],chroma_notes[note_in][1], octave_in, hpcp_freq);



        if ((err = write(orig_sock, ps, (int) FFT_SIZE * 2 * sizeof (float))) < FFT_SIZE * 2 * sizeof (float))
        {
             printf("Error writting to socket \n");
             exit(-1);
        }
             
             
             
             
        if (write(spctrgm_sd , ps, (int) FFT_SIZE * 2 * sizeof (float)) < FFT_SIZE * 2 * sizeof (float))
        {
           printf("Failed to set cancel state \n");
           terminate_all(NULL);
        }
             
        bzero(ps, FFT_SIZE*2*sizeof(float));
        
        
        


       }

        



            
            //printf("IT Clent :: wrote %i bytes to IT socket %s \n", err, strerror(errno));
      //      sleep(2);
    }



    close(orig_sock);
    pthread_cleanup_pop(cleanup_pop_arg1);
    return 0;
}


