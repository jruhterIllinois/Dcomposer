OBJDIR := ../obj
INCDIR := ../inc/
OBJS := $(OBJDIR)/dcomposer.o \
	$(OBJDIR)/hpcp_prof.o \
	$(OBJDIR)/env_proc.o \
	$(OBJDIR)/fft_thread.o \
	$(OBJDIR)/spectrogram.o

VIDEO_OBJS := 	$(OBJDIR)/video_enc.o

XEXAMPLE_OBJS := $(OBJDIR)/xexample.o


XLIBS = /usr/local/lib/libXext.a
XLIBS += /usr/local/lib/libXrender.a
XLIBS += /usr/local/lib/libXcomposite.a
XLIBS += /usr/local/lib/libXfixes.a

F2CLIB := /usr/local/lib/libf2c.a
#MPLAYER_DIR := /home/jay/MPLAYER_SRC/mplayer-checkout-2014-01-05
LAPACKLIB := /usr/local/lib/lapack_LINUX.a
BLASLIB := /usr/local/lib/libcblaswr.a -lcblas -latlas

VIDEO_LIBS_INC := /home/jay/MPLAYER_SRC/mplayer-checkout-2014-01-05/ffmpeg
VIDEO_LIBS_DIR =/home/jay/MPLAYER_SRC/mplayer-checkout-2014-01-05/ffmpeg




EXEDIR := ../exe
CFLAGS := -I$(INCDIR) -I$(VIDEO_LIBS_INC)



all : $(EXEDIR)/DCOMPOSR $(EXEDIR)/XEXAMPLE

$(OBJDIR)/%.o : %.c
	gcc -fPIC -c $(CFLAGS)  $^ -o $@

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

$(EXEDIR)/DCOMPOSR : $(OBJS)
	gcc $(CLFAGS) -rdynamic -fast -o $(EXEDIR)/DCOMPOSR $(OBJS) -L. -fast  $(LAPACKLIB) $(BLASLIB) $(F2CLIB) $(MPEGLIB) -lasound -lm -lpthread -lrt -lfftw3 -lmp3lame -ldl -lX11 $(XLIBS)

#$(EXEDIR)/VIDEO_ENC : $(VIDEO_OBJS)
#	gcc $(CLFAGS) -fast -o $(EXEDIR)/VIDEO_ENC $(VIDEO_OBJS) -L. -fast -lm -lpthread -lrt -ldl $(VIDEO_LIBS) 

$(EXEDIR)/XEXAMPLE : $(XEXAMPLE_OBJS)
	gcc $(CFLAGS) -fast -o $(EXEDIR)/XEXAMPLE $(XEXAMPLE_OBJS) -fast -lm -lpthread -lrt -ldl -lX11 $(XLIBS)

clean :
	rm $(OBJDIR)/*.o $(EXEDIR)/DCOMPOSR $(EXEDIR)/VIDEO_ENC 
