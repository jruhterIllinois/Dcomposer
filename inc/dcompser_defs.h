#define NUM_THRDS 100
#define FFT_SIZE 8192
#define FFT_SIZE_DIV2 4096
#define RAND_STD 6e-1
#define FRAC_SIZE 100000
#define EX_SZ_MAX FFT_SIZE*sizeof(float)
#define ACQ2PROC_PIPE ".acq2proc"
#define SPCTRGM_SOCK ".spectrogram_sock"

#define CHROMATIC_SCALE "C ", "C#","D ", "D#","E ", "F ", "F#","G ", "G#","A ", "A#",  "B"


//Binaries

#define PORT 2022
#define ENV_PORT 2023
#define SPCTRGM_PORT 2024
#define NUM_ROWS 64
#define NUM_COLS 64

#define SPCTRGM_DSPLY_HGHT 512
#define SPCTRGM_DSPLY_PXL 512
#define SCL_FACTOR FFT_SIZE_DIV2/SPCTRGM_DSPLY_PXL

int policy;
int spctrgm_sd;

float *test_func(int argc, float * argv);
void *fft_thread(void * arg);
pthread_t threads[NUM_THRDS];
char thread_active[NUM_THRDS];
void * terminate_all(void * arg);
void * spctrgm_thrd(void * arg);
//void extract_hpcp(float * ps_in, int chroma, int octave );