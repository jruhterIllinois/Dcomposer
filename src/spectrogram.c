

/*
     Server - IT domain, connection-oriented
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


/* include the X library headers */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include "/usr/local/include/X11/extensions/Xcomposite.h"
#include "/usr/local/include/X11/extensions/Xrender.h"
#include <X11/extensions/xfixeswire.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>


#include "../inc/dcompser_defs.h"


/* here are our X variables */





#define OpaqueMask	0xffffffff
#define AlphaMask	0xff000000
#define RedMask		0x00ff0000
#define GreenMask	0x0000ff00
#define BlueMask	0x000000ff
#define ColorMask	0x00ffffff


/* Shape's mask transparency boundary */
#define TransBdry	0x48

#define TransRange	1

#define MAX(a,b)	((a>b)? a: b)

/* Functions prototypes */
int get_byte_order (void);
static Visual *find_argb_visual (Display *dpy, int scr);
static void name_window (Display *dpy, Window win, int width, int height,
			 const char *name);

static void init_image(void);

extern unsigned long spctrgrm_win_id;
extern unsigned char spctrgm_thrd_running;
extern uint32_t it_address;
extern Window id;
static int cleanup_pop_arg1 = 0;

static void
client_spctrgm_kill(void * arg)
{
    printf("Killing the client socket \n");
    close( ((int *)arg)[0] );

}

void  *spctrgm_thrd(void * arg)
{
    
    int opt_val;
    int oldtype;
    int orig_sock, // Original socket in client
            len; // Misc. counter
    struct sockaddr_in serv_adr; // Internet addr of server
    struct sched_param thr_priority;
    socklen_t clnt_len; // Length of client address
    int num_rd;
    int new_sock;
    float * buf;
    
    
    
/*X VARIABLES */


    Window                   root, dummy;
    Display                  *dpy;
    Visual                   *visual;
    GC                       gc, gc_pict, gc_mask, gc_shape;
    XEvent                   event;
    Cursor                   cursor;
    Colormap                 colmap;
    XSetWindowAttributes     a;
    Pixmap                   pixmap, mask, shape_mask;
    unsigned long            wmask;
    int                      image_width, image_height, i, j, scr, run, x, y, X, Y;
    //Imlib_Image              image;
    XRenderPictFormat        *format_pix, *format_mask;
    Picture                  p_win, p_mask, rootPicture;
    XImage                   *ximage, *xmask, *xshape_mask;
    uint32_t                    *Imlib_data;
    u_int8_t                 *MaskBuffer;
    unsigned int             alpha, red, green, blue;
    double                   AlphaComp, RedComp, GreenComp, BlueComp;
    unsigned int             *ColorBuffer, *AlphaBuffer;
    char                     *display_string = NULL;
    XRenderPictureAttributes pwin;
    size_t                   numNewBufBytes;
    int                      loop_count = 0;
    int                      yes = 1;



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

    
    if((buf = (float *)valloc(FFT_SIZE*3*sizeof(float))) == NULL)
    {
        perror("Valloc error");
        terminate_all(NULL);
    }
    bzero(buf, FFT_SIZE*3*sizeof(float));
    
    
    /*Set up the X client*/
    
    
    	display_string = getenv("DISPLAY");
	if (!(dpy = (Display*) XOpenDisplay(display_string))) {
		fprintf(stderr, "Couldn't open display '%s'\n",
			(display_string ? display_string : "NULL"));
		return 1;
	}

	scr = DefaultScreen(dpy);
	root = RootWindow(dpy, scr);
	visual = find_argb_visual(dpy, scr);
	colmap = XCreateColormap (dpy, root, visual, AllocNone);

	/* Imlib Specific */
/*	image = imlib_load_image(FileName);
	imlib_context_set_image(image);
	image_width = imlib_image_get_width();
	image_height = imlib_image_get_height();
	Imlib_data = imlib_image_get_data_for_reading_only();
*/


        image_width = SPCTRGM_DSPLY_PXL;
        image_height = SPCTRGM_DSPLY_HGHT;


        if((Imlib_data = (uint32_t *)valloc(image_height*image_width*sizeof(uint32_t)))== NULL)
        {
            exit(-1);
        }
        
	a.background_pixel = 0x0;
	a.colormap = colmap;
	a.border_pixel = 0;
	a.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask |
		Button1MotionMask | CWOverrideRedirect |ResizeRedirectMask;
        
	wmask = CWEventMask | CWBackPixel | CWColormap | CWBorderPixel |CWOverrideRedirect;

	id = XCreateWindow(dpy, root, NULL, NULL,
			   image_width, image_height, 0, 32, InputOutput, visual,
			   wmask, &a);
        
     //   id = (Window)spctrgrm_win_id;
        
       	name_window (dpy, id, image_width, image_height,
		     "Waterfall");
 
        printf("Made it here 2 %i %i\n", id, spctrgrm_win_id);
        
        printf("Made it here 1.9\n");

	numNewBufBytes = ((size_t)((image_width*image_height)*sizeof(u_int32_t)));
	ColorBuffer = malloc(numNewBufBytes);
	AlphaBuffer = malloc(numNewBufBytes);

        printf("Made it here 1.75 \n");

	ximage = XCreateImage(dpy, visual, 32, ZPixmap, 0, (char*)ColorBuffer,
			      image_width, image_height, 32, image_width*4);

        printf("Made it here 1.5 \n");

	xmask = XCreateImage(dpy, visual, 32, ZPixmap, 0, (char*)AlphaBuffer,
			     image_width, image_height, 32, image_width*4);

	MaskBuffer = malloc((size_t)((image_width*image_height)*sizeof(u_int8_t)));

        printf("Made it here 1 \n");

	xshape_mask = (XImage*)malloc(sizeof(XImage));
	xshape_mask->width = image_width;
	xshape_mask->height = image_height;
	xshape_mask->xoffset = 0;
	xshape_mask->format = ZPixmap;
	xshape_mask->data = (char*)MaskBuffer;
	xshape_mask->byte_order = LSBFirst;
	xshape_mask->bitmap_unit = 32;
	xshape_mask->bitmap_bit_order = LSBFirst;
	xshape_mask->bitmap_pad = 8;
	xshape_mask->depth = 1;
	xshape_mask->bytes_per_line = 0; /* Let XInitImage guess it */
	xshape_mask->bits_per_pixel = 8;
	xshape_mask->red_mask = 0xff;
	xshape_mask->green_mask = 0xff;
	xshape_mask->blue_mask = 0xff;
	XInitImage(xshape_mask);

        printf("Init image \n");

	for(i=0; i<image_height; i++){
		for(j=0; j<image_width; j++){

			AlphaComp = (AlphaMask / 255.0);
			RedComp = (visual->red_mask / 255.0);
			GreenComp = (visual->green_mask / 255.0);
			BlueComp = (visual->blue_mask / 255.0);

                        alpha = (511)*(AlphaComp / TransRange);

                        red =(((0x00FF0000 & RedMask)>>16 ) +loop_count)* RedComp;
                        green = 0;
                        blue = 0;

			red &= visual->red_mask;
	//		green &= visual->green_mask;
	//		blue &= visual->blue_mask;

			ColorBuffer[i*image_width+j] =
				(((alpha!=AlphaMask) ? AlphaMask : alpha) | red | green | blue);

			AlphaBuffer[i*image_width+j] = alpha;

			MaskBuffer[i*image_width+j] =
				(((alpha>>24) < TransBdry) ? 0: 0367);
		}
	}



        printf("Made it here \n");

	gc = XCreateGC (dpy, id, 0, 0);

	/* Setup XRender */
	XCompositeRedirectSubwindows(dpy, id, CompositeRedirectAutomatic);

        printf("Made it here 0.9\n");

	pwin.subwindow_mode = IncludeInferiors;
	rootPicture = XRenderCreatePicture (dpy, id,
					    XRenderFindVisualFormat (dpy, visual),
					    CPSubwindowMode, &pwin);

        printf("Made it here 0.875\n");

	/* Setup the Image */
	format_pix = XRenderFindStandardFormat(dpy, PictStandardARGB32);

	pixmap = XCreatePixmap(dpy, id, image_width, image_height, 32);

        printf("Made it here 0.75\n");

	p_win = XRenderCreatePicture(dpy, pixmap, format_pix, 0, 0);

	XPutImage(dpy, pixmap, gc, ximage, 0, 0, 0, 0,
		  image_width, image_height);

	/* Setup the mask */
	format_mask = XRenderFindStandardFormat(dpy, PictStandardARGB32);

	mask = XCreatePixmap(dpy, id, image_width, image_height, 32);


        printf("Made it here 0.5\n");

        
	p_mask = XRenderCreatePicture(dpy, mask, format_mask, 0, 0);

	XPutImage(dpy, mask, gc, xmask, 0, 0, 0, 0,
		  image_width, image_height);

	/* Assign the hand cursor to our window */
	cursor = XCreateFontCursor (dpy, XC_hand2);
	XDefineCursor(dpy, id, cursor);

	XMapWindow(dpy, id);
	XSync(dpy, False);

        

        printf("Made it here 0.25\n");
	/* Now the shape part, create a 1bit empty pixmap */
	shape_mask = XCreatePixmap(dpy, id, image_width, image_height, 1);

	/* Create it's gc */
	gc_shape = XCreateGC(dpy, shape_mask, 0, 0);

	/* Turn our ShapeBuffer over the pixmap */
	XPutImage(dpy, shape_mask,  gc_shape, xshape_mask, 0, 0, 0, 0,
		  image_width, image_height);

	/* Now, actually shape the window */
	XShapeCombineMask(dpy, id, ShapeBounding, 0, 0, shape_mask, ShapeSet);
	XShapeCombineMask(dpy, id, ShapeBounding, 1, 1, shape_mask, ShapeUnion);

      //  XConfigureWindow(dpy, id,  Below, CWStackMode);

        
          
    printf("Creating Server \n");
   
   
    memset(&serv_adr, 0, sizeof (serv_adr)); // Clear structure
    serv_adr.sin_family = AF_INET; // Set address type
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons( (uint16_t) SPCTRGM_PORT); // Use our fake port


    if ((orig_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("generate error");
        terminate_all(NULL);
    }
    
    
    if(setsockopt(orig_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))== -1)
    {
        perror("Set Sock opt error");
        terminate_all(NULL);
        
    }


    // BIND
    if (bind(orig_sock, (struct sockaddr *) & serv_adr,
            sizeof (serv_adr)) < 0)
    {
        perror("bind error");
        close(orig_sock);
        terminate_all(NULL);
    }
    
    
        spctrgm_thrd_running = 1;
        
    if (listen(orig_sock, 5) < 0)
    { // LISTEN
        perror("listen error");
        close(orig_sock);
        terminate_all(NULL);
    }
    pthread_cleanup_push(client_spctrgm_kill, (void *)&orig_sock)

            
    printf("Listening on Port \n");
            
            

    thread_active[*((int *)arg)] = 1;


    while(1)
    {
        
        clnt_len = sizeof (serv_adr); // ACCEPT a connect
        if ((new_sock = accept(orig_sock,
                              (struct sockaddr *) & serv_adr,
                               &clnt_len))     < 0)
        {
            perror("accept error");
            close(orig_sock);
            terminate_all(NULL);
        }

        printf("Processing new sock in child \n");
        num_rd = 0;
        while ((len = read(new_sock, buf, FFT_SIZE * 2 * sizeof (float))) > 0)
        {
    //        printf("Received new data in spectrogram thread \n");

  
        
        loop_count = loop_count + 10;
        a.override_redirect = 1;
	a.background_pixel = 0x0;
	a.colormap = colmap;
	a.border_pixel = 0;
	a.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask |
		Button1MotionMask;
	wmask = CWEventMask | CWBackPixel | CWColormap |
		CWBorderPixel;

	xshape_mask = (XImage*)malloc(sizeof(XImage));
	xshape_mask->width = image_width;
	xshape_mask->height = image_height;
	xshape_mask->xoffset = 0;
	xshape_mask->format = ZPixmap;
	xshape_mask->data = (char*)MaskBuffer;
	xshape_mask->byte_order = LSBFirst;
	xshape_mask->bitmap_unit = 32;
	xshape_mask->bitmap_bit_order = LSBFirst;
	xshape_mask->bitmap_pad = 8;
	xshape_mask->depth = 1;
	xshape_mask->bytes_per_line = 0; /* Let XInitImage guess it */
	xshape_mask->bits_per_pixel = 8;
	xshape_mask->red_mask = 0xff;
	xshape_mask->green_mask = 0xff;
	xshape_mask->blue_mask = 0xff;
	XInitImage(xshape_mask);

        
                
       // printf("Init image \n");

        memmove(  &Imlib_data[3*image_width],  Imlib_data ,image_width*(image_height-3)*sizeof(uint32_t));

        for(i = 0; i< image_width; i++)
        {
            Imlib_data[i] = 0;
            
            for(j = 0; j< SCL_FACTOR; j++)
                    Imlib_data[i] += buf[FFT_SIZE/2 +i*SCL_FACTOR +j];
            
            Imlib_data[i] /=  SCL_FACTOR;
            Imlib_data[i + image_width] =  Imlib_data[i];
            Imlib_data[i + image_width*2] =  Imlib_data[i];
        }
        
   //     printf("Value read and scaled \n");
        
	for(i=image_height-1; i>=0; i--){
		for(j=0; j<image_width; j++){
   
                    AlphaComp = (AlphaMask / 255.0);
		    RedComp = (visual->red_mask / 255.0);
		    GreenComp = (visual->green_mask / 255.0);
		    BlueComp = (visual->blue_mask / 255.0);

                    alpha = (511)*(AlphaComp / TransRange);

                    
                    	red = ((Imlib_data[i*image_width+j] & BlueMask) * RedComp);
                      //  red =(((0x00000000 & RedMask)>>16 ) +loop_count)* RedComp;
			green = ((Imlib_data[i*image_width+j] & BlueMask) * GreenComp);
			blue = ((Imlib_data[i*image_width+j] & BlueMask) * BlueComp);
                        
       //             red =(((0x00000000 & RedMask)>>16 ) +Imlib_data[i*image_width+j])* RedComp;
                    green = 0;
                    blue = 0;

                    red &= visual->red_mask +100;
	//		green &= visual->green_mask;
	//		blue &= visual->blue_mask;

                        
                        
                        
		    ColorBuffer[i*image_width+j] =
				(((alpha!=AlphaMask) ? AlphaMask : alpha) | red | green | blue);

		    AlphaBuffer[i*image_width+j] = alpha;

		    MaskBuffer[i*image_width+j] =
				(((alpha>>24) < TransBdry) ? 0: 0367);
		}
	}

        
	XPutImage(dpy, pixmap, gc, ximage, 0, 0, 0, 0,
		  image_width, image_height);

	/* Setup the mask */
	format_mask = XRenderFindStandardFormat(dpy, PictStandardARGB32);

	XPutImage(dpy, mask, gc, xmask, 0, 0, 0, 0,
		  image_width, image_height);
        
	//XMapWindow(dpy, id);
	//XSync(dpy, False);

	/* Turn our ShapeBuffer over the pixmap */
	XPutImage(dpy, shape_mask,  gc_shape, xshape_mask, 0, 0, 0, 0,
		  image_width, image_height);

	/* Now, actually shape the window */
	//XShapeCombineMask(dpy, id, ShapeBounding, 0, 0, shape_mask, ShapeSet);
	//XShapeCombineMask(dpy, id, ShapeBounding, 1, 1, shape_mask, ShapeUnion);
        
        
        XRenderComposite (dpy,
					  PictOpSrc,
					  p_win,
					  p_mask,
					  rootPicture,
					  0, 0, 0, 0, 0, 0, image_width,
					  image_height);
        
       
        
        XSync(dpy, False);
        
        num_rd++;
                //      write()
        }
        
        
        
        
        printf("Finish reading the data ");
        close(new_sock); // In CHILD process
        
    }

    printf("Thread running \n");



    pthread_cleanup_pop(cleanup_pop_arg1);

}


int
get_byte_order (void)
{
	union {
		char c[sizeof(short)];
		short s;
	} order;

	order.s = 1;

	if ((1 == order.c[0])) {
		return LSBFirst;
	} else {
		return MSBFirst;
	}
}

static Visual*
find_argb_visual (Display *dpy, int scr)
{
	XVisualInfo	*xvi;
	XVisualInfo	template;
	int nvi, i;
	XRenderPictFormat *format;
	Visual *visual;

	template.screen = scr;
	template.depth = 32;
	template.class = TrueColor;
	xvi = XGetVisualInfo (dpy,
			      VisualScreenMask |
			      VisualDepthMask |
			      VisualClassMask,
			      &template,
			      &nvi);
	if (xvi == NULL)
		return NULL;

	visual = NULL;
	for (i = 0; i < nvi; i++)
	{
		format = XRenderFindVisualFormat (dpy, xvi[i].visual);
		if (format->type == PictTypeDirect && format->direct.alphaMask)
		{
			visual = xvi[i].visual;
			break;
		}
	}
	XFree (xvi);
	if(visual==NULL){
		fprintf(stderr, "Couldn't find an argb visual\n");
		exit(1);
	}else
		return visual;
}

/* Gives a name to a window. This is required.
 * for xrender to work, I'm affraid.
 */
static void
name_window (Display *dpy, Window win, int width, int height,
	     const char *name)
{
	XSizeHints *size_hints;
	XWMHints *wm_hints;
	XClassHint *class_hint;

	size_hints = XAllocSizeHints ();
	size_hints->flags = 0;
	size_hints->x = 0;
	size_hints->y = 0;
	size_hints->width = width;
	size_hints->height = height;

	wm_hints = XAllocWMHints();
	wm_hints->flags = InputHint;
	wm_hints->input = True;

	class_hint = XAllocClassHint ();
	class_hint->res_name  = "any name";
	class_hint->res_class = "any class";

	Xutf8SetWMProperties (dpy, win, name, name, 0, 0,
			      size_hints, wm_hints, class_hint);
	XFree (size_hints);
	XFree (wm_hints);
	XFree (class_hint);
}
