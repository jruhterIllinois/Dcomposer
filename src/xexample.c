/*
	Remember to compile try:
		1) gcc hi.c -o hi -lX11
		2) gcc hi.c -I /usr/include/X11 -L /usr/X11/lib -lX11
		3) gcc hi.c -I /where/ever -L /who/knows/where -l X11

	Brian Hammond 2/9/96.    Feel free to do with this as you will!
*/


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

/* here are our X variables */





#define OpaqueMask	0xffffffff
#define AlphaMask	0xff000000
#define RedMask		0x00ff0000
#define GreenMask	0x0000ff00
#define BlueMask	0x000000ff
#define ColorMask	0x00ffffff


/* Shape's mask transparency boundary */
#define TransBdry	0x48

/* Image file to display */
#define FileName	"curvy_border.png"

/* Set this to 2 or 3 if you want to see a translucent-xshaped window.
 * You shouldn't be able to see anything if you set this to four and up.
 * If you use floats no here, borders will look crappy, as well as if you
 * wouldn't have an xcompositing manager running
 */
#define TransRange	1

#define MAX(a,b)	((a>b)? a: b)

/* Functions prototypes */
int get_byte_order (void);
static Visual *find_argb_visual (Display *dpy, int scr);
static void name_window (Display *dpy, Window win, int width, int height,
			 const char *name);

static void init_image(void);

/* Global variables */
Window                   id,root, dummy;
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
u_int32_t                *Imlib_data;
u_int8_t                 *MaskBuffer;
unsigned int             alpha, red, green, blue;
double                   AlphaComp, RedComp, GreenComp, BlueComp;
unsigned int             *ColorBuffer, *AlphaBuffer;
char                     *display_string = NULL;
XRenderPictureAttributes pwin;
size_t                   numNewBufBytes;
int                      loop_count = 0;



int
main()
{

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


        image_width = 600;
        image_height = 450;


        if((Imlib_data = (u_int32_t *)valloc(image_height*image_width*sizeof(u_int32_t)))== NULL)
        {
            exit(-1);
        }
        
	a.override_redirect = 1;
	a.background_pixel = 0x0;
	a.colormap = colmap;
	a.border_pixel = 0;
	a.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask |
		Button1MotionMask;
	wmask = CWEventMask | CWBackPixel | CWColormap | CWOverrideRedirect |
		CWBorderPixel;

	id = XCreateWindow(dpy, root, (1024-image_width)/2, (786-image_height)/2,
			   image_width, image_height, 0, 32, InputOutput, visual,
			   wmask, &a);

        printf("Made it here 2\n");
	name_window (dpy, id, image_width, image_height,
		     "xshape, xcomposite and xrender test");

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


                    Imlib_data[i*image_width+j]  = rand()*100;
                    //printf("data %i \n",Imlib_data[i*image_width+j]);
			AlphaComp = (AlphaMask / 255.0);
			RedComp = (visual->red_mask / 255.0);
			GreenComp = (visual->green_mask / 255.0);
			BlueComp = (visual->blue_mask / 255.0);

			alpha = (((Imlib_data[i*image_width+j] & AlphaMask)>>24)*
				 AlphaComp) / TransRange;

			red = (((Imlib_data[i*image_width+j] & RedMask)>>16) * RedComp);
			green = (((Imlib_data[i*image_width+j] & GreenMask)>>8) * GreenComp);
			blue = ((Imlib_data[i*image_width+j] & BlueMask) * BlueComp);

			red &= visual->red_mask;
			green &= visual->green_mask;
			blue &= visual->blue_mask;

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


        printf("Made it here 0\n");
	/* Enter on the event loop */
	run = 1;

	do{

                init_image();
                XRenderComposite (dpy,
					  PictOpSrc,
					  p_win,
					  p_mask,
					  rootPicture,
					  0, 0, 0, 0, 0, 0, image_width,
					  image_height);
                usleep(10000);
        
                loop_count = loop_count + 10;
	}while(run!=0);

	/* Free retained resources and exit */
	XDestroyImage (ximage);
	XDestroyImage (xmask);
	XDestroyImage (xshape_mask);
	XFreeGC(dpy, gc);
	XFreeGC(dpy, gc_shape);
	XFreePixmap(dpy, mask);
	XFreePixmap(dpy, shape_mask);
	XCloseDisplay(dpy);
	AlphaBuffer=NULL;
	ColorBuffer=NULL;
	MaskBuffer=NULL;

	/* Bye, bye! */
	return(0);
}

static void
init_image(void)
{

    	//id = XCreateWindow(dpy, root, (1024-image_width)/2, (786-image_height)/2,
	//		   image_width, image_height, 0, 32, InputOutput, visual,
	//		   wmask, &a);

       // printf("Made it here 2\n");
	//name_window (dpy, id, image_width, image_height,
	//	     "xshape, xcomposite and xrender test");


        a.override_redirect = 1;
	a.background_pixel = 0x0;
	a.colormap = colmap;
	a.border_pixel = 0;
	a.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask |
		Button1MotionMask;
	wmask = CWEventMask | CWBackPixel | CWColormap | CWOverrideRedirect |
		CWBorderPixel;

        //printf("Made it here 2\n");
//	name_window (dpy, id, image_width, image_height,
//		     "xshape, xcomposite and xrender test");

      //  printf("Made it here 1.9\n");

      //  printf("Made it here 1.75 \n");

//	ximage = XCreateImage(dpy, visual, 32, ZPixmap, 0, (char*)ColorBuffer,
//			      image_width, image_height, 32, image_width*4);

      //  printf("Made it here 1.5 \n");

//	xmask = XCreateImage(dpy, visual, 32, ZPixmap, 0, (char*)AlphaBuffer,
//			     image_width, image_height, 32, image_width*4);


      //  printf("Made it here 1 \n");

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

	for(i=0; i<image_height; i++){
		for(j=0; j<image_width; j++){


                    Imlib_data[i*image_width+j]  = rand()*100;
                    //printf("data %i \n",Imlib_data[i*image_width+j]);
			AlphaComp = (AlphaMask / 255.0);
			RedComp = (visual->red_mask / 255.0);
			GreenComp = (visual->green_mask / 255.0);
			BlueComp = (visual->blue_mask / 255.0);

			alpha = (((Imlib_data[i*image_width+j] & AlphaMask)>>24)*
				 AlphaComp) / TransRange;
                        alpha = (511)*(AlphaComp / TransRange);

//			red = (((Imlib_data[i*image_width+j] & RedMask)>>16) * RedComp);
                        red =(((0x00000000 & RedMask)>>16 ) +loop_count)* RedComp;
//			green = (((Imlib_data[i*image_width+j] & GreenMask)>>8) * GreenComp);
//			blue = ((Imlib_data[i*image_width+j] & BlueMask) * BlueComp);
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



      //  printf("Made it here \n");

	/* Setup XRender */
//	XCompositeRedirectSubwindows(dpy, id, CompositeRedirectAutomatic);

  //      printf("Made it here 0.9\n");

//	pwin.subwindow_mode = IncludeInferiors;
//	rootPicture = XRenderCreatePicture (dpy, id,
//					    XRenderFindVisualFormat (dpy, visual),
//					    CPSubwindowMode, &pwin);

    //    printf("Made it here 0.875\n");

	/* Setup the Image */
//	format_pix = XRenderFindStandardFormat(dpy, PictStandardARGB32);

//	pixmap = XCreatePixmap(dpy, id, image_width, image_height, 32);

      //  printf("Made it here 0.75\n");

//	p_win = XRenderCreatePicture(dpy, pixmap, format_pix, 0, 0);

	XPutImage(dpy, pixmap, gc, ximage, 0, 0, 0, 0,
		  image_width, image_height);

	/* Setup the mask */
	format_mask = XRenderFindStandardFormat(dpy, PictStandardARGB32);

//	mask = XCreatePixmap(dpy, id, image_width, image_height, 32);


        // printf("Made it here 0.5\n");

//	p_mask = XRenderCreatePicture(dpy, mask, format_mask, 0, 0);

	XPutImage(dpy, mask, gc, xmask, 0, 0, 0, 0,
		  image_width, image_height);

	/* Assign the hand cursor to our window */
//	cursor = XCreateFontCursor (dpy, XC_hand2);
//	XDefineCursor(dpy, id, cursor);

	XMapWindow(dpy, id);
	XSync(dpy, False);


    //    printf("Made it here 0.25\n");
	/* Now the shape part, create a 1bit empty pixmap */
//	shape_mask = XCreatePixmap(dpy, id, image_width, image_height, 1);

	/* Create it's gc */
//	gc_shape = XCreateGC(dpy, shape_mask, 0, 0);

	/* Turn our ShapeBuffer over the pixmap */
	XPutImage(dpy, shape_mask,  gc_shape, xshape_mask, 0, 0, 0, 0,
		  image_width, image_height);

	/* Now, actually shape the window */
	XShapeCombineMask(dpy, id, ShapeBounding, 0, 0, shape_mask, ShapeSet);
	XShapeCombineMask(dpy, id, ShapeBounding, 1, 1, shape_mask, ShapeUnion);
//XSync(dpy, False);


}



/* Get host's byte order. There are native functions
 * within the xserver to do this
 */
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




#if 0
/* here are our X routines declared! */
void init_x();
void close_x();
void redraw();

void main(void * args)
{

    Display                  *dpy;
    char                     *display_string = NULL;
    XImage                   *ximage, *xmask, *xshape_mask;
    Visual                   *visual;
    Window                   id, root;
    int                      image_width, image_height, scr;
    Colormap                 colmap;
    XSetWindowAttributes     a;
    GC                       gc;
    unsigned long            wmask;
    u_int8_t                 *MaskBuffer;
    unsigned int             *ColorBuffer, *AlphaBuffer;
    int                      numNewBufBytes;
    Pixmap                   pixmap;
    XEvent                   event;





    display_string = getenv("DISPLAY");

	if (!(dpy = (Display*) XOpenDisplay(display_string))) {
		fprintf(stderr, "Couldn't open display '%s'\n",
			(display_string ? display_string : "NULL"));
		return 1;
	}

    	scr = DefaultScreen(dpy);
	root = RootWindow(dpy, scr);
	visual = DefaultVisual(dpy, scr);
	colmap = XCreateColormap (dpy, root, visual, AllocNone);

        image_width = 300;
        image_height = 300;

	ximage = XCreateImage(dpy, visual, 32, ZPixmap, 0, (char*)ColorBuffer,
			      image_width, image_height, 32, image_width*4);

        wmask = CWEventMask | CWBackPixel | CWColormap | CWOverrideRedirect |
		CWBorderPixel;

        MaskBuffer = malloc((int)((image_width*image_height)*sizeof(u_int8_t)));

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

        id = XCreateWindow(dpy, root, (1024-image_width)/2, (786-image_height)/2,
			   image_width, image_height, 0, 32, InputOutput, visual,
			   wmask, &a);


        a.override_redirect = 1;
	a.background_pixel = 0x0;
	a.colormap = colmap;
	a.border_pixel = 0;
	a.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask |
		Button1MotionMask;

        numNewBufBytes = ((int)((image_width*image_height)*sizeof(u_int32_t)));
	ColorBuffer = malloc(numNewBufBytes);
	AlphaBuffer = malloc(numNewBufBytes);


        ximage = XCreateImage(dpy, visual, 32, ZPixmap, 0, (char*)ColorBuffer,
			      image_width, image_height, 32, image_width*4);

	xmask = XCreateImage(dpy, visual, 32, ZPixmap, 0, (char*)AlphaBuffer,
			     image_width, image_height, 32, image_width*4);

        gc = XCreateGC (dpy, id, 0, 0);


        pixmap = XCreatePixmap(dpy, id, image_width, image_height, 32);

//	p_win = XRenderCreatePicture(dpy, pixmap, format_pix, 0, 0);

        XPutImage(dpy, pixmap, gc, ximage, 0, 0, 0, 0,
		  image_width, image_height);

        printf("Made it here \n");
        XMapWindow(dpy, id);
//	XSync(dpy, False);



        do{
            usleep(100000);
	XPutImage(dpy, pixmap, gc, ximage, 0, 0, 0, 0,
		  image_width, image_height);
        printf("In loop\n");

	}while(1);
		
}




#if 0
void init_x() {
/* get the colors black and white (see section for details) */
	unsigned long black,white;


	dis=XOpenDisplay((char *)0);


        screen=DefaultScreen(dis);
	black=BlackPixel(dis,screen),
	white=WhitePixel(dis, screen);
   	win=XCreateSimpleWindow(dis,DefaultRootWindow(dis),0,0,
		300, 300, 5,black, white);
	XSetStandardProperties(dis,win,"Howdy","Hi",None,NULL,0,NULL);
	XSelectInput(dis, win, ExposureMask|ButtonPressMask|KeyPressMask);
        gc=XCreateGC(dis, win, 0,0);
	XSetBackground(dis,gc,white);
	XSetForeground(dis,gc,black);
	XClearWindow(dis, win);
	XMapRaised(dis, win);
}



void close_x() {
	XFreeGC(dis, gc);
	XDestroyWindow(dis,win);
	XCloseDisplay(dis);
	exit(1);
}

void redraw() {
	XClearWindow(dis, win);
}
#endif


#endif