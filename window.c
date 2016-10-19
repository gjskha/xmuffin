#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <string.h>
#include "defaults.h"

#define VERSION "0.1"

/* X related globals */
Display *dpy;
Window win;
GC gc;
XFontStruct *font;
int Ascent=33,Descent=0;
int Width=55,Height=33;
XColor FgXColor,HiXColor;
Pixmap ShapeMask;           /* shape stuff */
GC ShapeGC;                 /* shape stuff */
int MaskWidth,MaskHeight;   /* shape stuff */
Atom DeleteWindow;          /* Atom of delete window message */

/* app-default related globals */
char *FgColor="Black",*BgColor="White",*HiColor="Red";
char *FontName=NULL;
char *DisplayName=NULL;
char *AppName;
int Number;
int Interval=INTERVAL_SPOOL;
char SpoolFile[256];
char Command[256];
char Username[32];
char Password[32];
char NewMailCommand[256];
char *Geometry=NULL;
int Options=0;
#define BELL_MODE       0x0001
#define COMMAND_MODE    0x0002
#define SHAPED_WINDOW   0x0004
#define HILIGHT_MAIL    0x0008
#define OFFLINE         0x0010

char *BackupFonts[0] = { FONTNAME }; 

struct config
{
   char server[MAXBUF];
   char user[MAXBUF];
   char font[MAXBUF];
   char folders[MAXBUF];
   char command[MAXBUF];
};

void usage();
void update();
void handler(int);
void parse_options(int argc,char *argv[]);
void init(int argc,char *argv[]);
int count_mail();
/**/
struct get_config(char *filename);

void update()
{
    static int old_number =- 1;
    char str[32];
    static int oldw =-1, oldh =- 1;
    int w,h;

    if( Options&OFFLINE && Number == -1 ) {

        strcpy(str,"X");

    } else {
        sprintf( str, "%d", Number );
    }

    w = ( Width - XTextWidth(font,str,strlen(str)) ) / 2;
    h = ( Height + Ascent - Descent) / 2;

    if ( Options&SHAPED_WINDOW ) {

        if ( Number != old_number || oldw != w || oldh != h ) {

            old_number = Number; 
            oldw = w; 
            oldh = h;

            /* these next 3 lines clear the pixmap, is there a cleaner way? */
            XSetFunction(dpy,ShapeGC,GXclear);
            XFillRectangle(dpy,ShapeMask,ShapeGC,0,0,MaskWidth,MaskHeight);
            XSetFunction(dpy,ShapeGC,GXset);
            XDrawString(dpy,ShapeMask,ShapeGC,0,Ascent,str,strlen(str));
            XShapeCombineMask(dpy, win, ShapeBounding, 
                w, h-Ascent, ShapeMask, ShapeSet);
        }

    } else {

        XClearWindow(dpy,win);

    }

    if(Options&HILIGHT_MAIL)  {
        XSetForeground(dpy,gc,Number?HiXColor.pixel:FgXColor.pixel);
    }

    XDrawString(dpy,win,gc,w,h,str,strlen(str));
}

void handler(int nothing)
{
    int old;

    old = Number;
    count_mail();

    if(old==Number) return;

    update();

    if (Number>old) {

        if (Options&BELL_MODE)
            XBell(dpy,100);

        if (Options&COMMAND_MODE) {
            char str[256];
            sprintf(str, NewMailCommand, Number);
            system(str);
        }
    }
    XFlush(dpy);
}

void font_height(void)
{
    int foo,bar,baz;
    XCharStruct extents;

    XTextExtents(font,"0123456789",10,&foo,&bar,&baz,&extents);
   
    Ascent = extents.ascent;
    Descent = extents.descent;
}

void usage(void)
{
    printf("XMuffin V" VERSION "\n");
    printf("Usage: xmuffin [-h] | [-options]\n");
    printf("where options include:\n");
    printf("    -h,  -help              Print usage\n");
    printf("    -v,  -version           Print version\n");
    printf("    -name <name>            Set app name\n");
    printf("    -bg <color>             Background color [%s]\n",BgColor);
    printf("    -fg <color>             Foreground color [%s]\n",FgColor);
    printf("    -hilight [color]        Use a different foreground color when there\n"
           "                                is non-zero mail [%s]\n", HiColor);
    printf("    -update <number>        Check mail every <number> seconds\n"
           "                                default %d sec local, %d sec remote\n", INTERVAL_SPOOL, INTERVAL_POP3);
           /* "                                default %d seconds\n", */
    printf("    -bell                   Ring bell when count increases from zero\n");
    printf("    -mailcommand <command>  Command to execute on new mail arrival\n");
    printf("    -fn <fontname>          Font for the new mail count\n");
    printf("    -display <displayname>  X server to connect to\n");
    printf("    -shape                  Use a shaped window\n");
    printf("    -command <command>      Command to execute when clicked on\n");
    /* printf("    -imap <server>          Use the IMAP protocol instead of pop3\n"); */
    printf("    -username <name>        Username for imap server\n");
          /* "                                if different from local username\n");*/
    printf("    -password <word>        Password to use on imap server\n");
    printf("                                Use the password 'ask' to get prompted for\n"
           "                                the password on stdin.\n");
    printf("    -offline                Don't exit when the server is unreachable\n");
}

void ask_password()
{
    int i;

    printf("Mail account password: ");
    fgets(Password, 32, stdin);

    i = strlen(Password) - 1;
    if (Password[i] == '\n') 
        Password[i]='\0';
}

void parse_options( int argc,char *argv[] ) {

    int i;
    int intervalused = 0;

    for ( i = 1; i < argc; i++ ) {

        if( !strcmp( argv[i], "-fg" )) {

            if ( ++i == argc ) { 
                usage(); 
                exit(2); 
            }

            FgColor = argv[i];

        } else if ( !strcmp(argv[i], "-bg" )) {
 
            if ( ++i == argc ) { 
                usage(); 
                exit(2); 
            }

            BgColor = argv[i];

        } else if ( !strcmp(argv[i],"-update") ) {

            if ( ++i == argc) { 
                usage(); 
                exit(2); 
            }

            Interval = atoi(argv[i]);
            intervalused = 1;

        } else if (!strcmp(argv[i],"-bell")) {

            Options |= BELL_MODE;

        } else if (!strcmp(argv[i],"-shape")) {

            Options |= SHAPED_WINDOW;

        } else if (!strcmp(argv[i],"-fn") || !strcmp(argv[i],"-font")) {

            if (++i == argc) { 
                usage(); 
                exit(2); 
            }

            FontName = argv[i];

        } else if (!strcmp(argv[i],"-display")) {

            if (++i == argc) { 
                usage(); 
                exit(2); 
            }

            DisplayName = argv[i];

        } else if (!strcmp(argv[i],"-hilight")) {

            Options |= HILIGHT_MAIL;

            if (i+1!=argc && argv[i+1][0]!='-') 
                HiColor = argv[++i];

        } else if (!strcmp(argv[i],"-command")) {

            if(++i==argc) { usage(); exit(2); };
            
           strcpy(Command,argv[i]);

        } else if (!strcmp(argv[i],"-geometry")) {

            if(++i==argc)  { usage(); exit(2); };

            Geometry=argv[i];

        } else if (!strcmp(argv[i],"-name")) {

            if(++i==argc)  { usage(); exit(2); };

            AppName = argv[i];

        } else if (!strcmp(argv[i],"-imap"))  {
            
            if(++i==argc)  { usage(); exit(2); };
            
            strcpy(SpoolFile,argv[i]);

            Options |= USE_IMAP;
 
           if (!intervalused) 
                Interval = INTERVAL_POP3;

        } else if (!strcmp(argv[i],"-username"))  {

            if(++i==argc)  { usage(); exit(2); };
           
            strcpy(Username,argv[i]);

        } else if (!strcmp(argv[i],"-password")) {

            if(++i==argc)  { usage(); exit(2); };

            strcpy(Password,argv[i]);
            memset(argv[i],0,strlen(argv[i]));

            if(!strcmp(Password,"ask"))  ask_password();

        } else if (!strcmp(argv[i],"-offline")) {

            Options |= OFFLINE;

        } else if (!strcmp(argv[i],"-mailcommand")) {

            Options |= COMMAND_MODE;

            if(++i==argc) { usage(); exit(2); };
           
            strcpy(NewMailCommand, argv[i]);

        } else if (!strcmp(argv[i],"-h") || !strcmp(argv[i],"-help")) {

            usage();
            exit(0);

        } else if (!strcmp(argv[i],"-v") || !strcmp(argv[i],"-version")) {

            printf("XMuffin " VERSION "\n"); exit(0);

        } else {
            fprintf(stderr,"Unknown option %s\n",argv[i]);
            fprintf(stderr,"Use -h for help\n");
            exit(2);
        };
    };
}

void init(int argc,char *argv[])
{
    int screen;
    XWMHints *wmh;
    XSizeHints *xsh;
    XClassHint *classh;
    XColor color,tmp;
    Window icon=0;
    int x,y,g;

/* config stuff
 */

    struct config configstruct;
    configstruct = get_config(CONFIGFILE);
       
    printf("%s",configstruct.server);
    printf("%s",configstruct.user);
    printf("%s",configstruct.font);
    printf("%s",configstruct.folders);
    printf("%s",configstruct.command);

/* end config stuff
 */

    parse_options( argc, argv );

    dpy = XOpenDisplay(DisplayName);

    if (dpy == NULL) {
        fprintf(stderr,"Error: Can't open display: %s\n",DisplayName);
        exit(1);
    }

    screen = DefaultScreen(dpy);

    DeleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

    xsh = XAllocSizeHints();

    wmh = XAllocWMHints();

    classh = XAllocClassHint();

    FontName = BackupFonts[0];

    font = XLoadQueryFont(dpy, FontName);

    if(font==NULL) {

        fprintf(stderr, "Can't find specified font, I give up!\n");
        exit(1);

    }

    font_height();
    
    Height = Ascent + Descent;

    Width = XTextWidth(font,"88",2);

    xsh->flags = PSize; 
    xsh->width = Width;
    xsh->height = Height;

    /*  printf("%d x %d\n", Width, Height);*/

    g = XWMGeometry(dpy,screen,Geometry,NULL,0,xsh,&x,&y,&Width,&Height,&g);

    if (g&XValue) { 
        xsh->x = x; 
        xsh->flags |= USPosition; 
    }

    if (g&YValue) { 
        xsh->y = y;
        xsh->flags |= USPosition; 
    }

    if (g&WidthValue) {
        xsh->width=Width; 
        xsh->flags|=USSize; 
    } else {
        Width = xsh->width; 
    }

    if (g&HeightValue) {
        xsh->height=Height; 
        xsh->flags|=USSize;
    } else            {Height = xsh->height; };


    win=XCreateSimpleWindow(dpy,RootWindow(dpy,screen),x,y,Width,Height,0,
        BlackPixel(dpy,screen),WhitePixel(dpy,screen));

    wmh->initial_state = NormalState;
    wmh->input = False;
    wmh->window_group = win;
    wmh->flags = StateHint | InputHint | WindowGroupHint;
    classh->res_name = ( AppName == NULL ) ? "xmuffin" : AppName;
    classh->res_class = "XBiff";

    XmbSetWMProperties(dpy, win, "xmuffin", "xmuffin", argv, argc,
                       xsh, wmh, classh);
    XSetWMProtocols(dpy, win, &DeleteWindow, 1);
    XSelectInput(dpy, win, ExposureMask|ButtonPressMask|StructureNotifyMask);

    XMapWindow(dpy, win);
    /* if(Options&WM_MODE) win = icon; */     /* Use the icon window from now on */

    gc=XCreateGC(dpy,win,0,NULL);

    XAllocNamedColor(dpy,DefaultColormap(dpy,screen),FgColor,&color,&tmp);
    XSetForeground(dpy,gc,color.pixel); FgXColor=color;
    XAllocNamedColor(dpy,DefaultColormap(dpy,screen),BgColor,&color,&tmp);
    XSetBackground(dpy,gc,color.pixel);
    XSetWindowBackground(dpy,win,color.pixel);
    if(Options&HILIGHT_MAIL)  XAllocNamedColor(dpy,DefaultColormap(dpy,screen),
                                                HiColor,&HiXColor,&tmp);
    XSetFont(dpy,gc,font->fid);

    if(Options&SHAPED_WINDOW)  {
        MaskWidth = Width; MaskHeight = Height;
        ShapeMask = XCreatePixmap(dpy,win,MaskWidth,MaskHeight,1);
        ShapeGC = XCreateGC(dpy,ShapeMask,0,NULL);
        XSetFont(dpy,ShapeGC,font->fid);
    };
}


int main(int argc,char *argv[])
{
    XEvent xev;
    Atom WM_PROTOCOLS;
    struct itimerval itv;
    struct sigaction sig;
    char *mail;

    init(argc,argv);

    WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);

/* #ifdef OFFLINE_DEFAULT */
    Options|=OFFLINE;
/* #endif */

    count_mail();

    if (Options&BELL_MODE && Number) 
        XBell(dpy,100);

    sig.sa_handler = handler;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = SA_RESTART;
    sigaction(SIGALRM,&sig,NULL);
    itv.it_interval.tv_sec = Interval; 
    itv.it_interval.tv_usec = 0;
    itv.it_value = itv.it_interval;
    setitimer(ITIMER_REAL,&itv,NULL);

    for (;;) {

        XNextEvent(dpy,&xev);

        switch(xev.type) {
        case Expose:
            while(XCheckTypedEvent(dpy,Expose,&xev));
            update();
            break;
        case ButtonPress:
            /* system(Command);*/
            system(configstruct.command);
            break;
        case ConfigureNotify:
            Width = xev.xconfigure.width;
            Height = xev.xconfigure.height;
            update();
            break;
        case ClientMessage:
            if(xev.xclient.message_type == WM_PROTOCOLS)  {
                if(xev.xclient.data.l[0] == DeleteWindow) {
                    XCloseDisplay(dpy);
                    exit(0);
                };
            };
            break;
        case DestroyNotify:
            XCloseDisplay(dpy);
            exit(0);
        /* default: */
        };
    };
}

////////////////////////////////////////////////////////
// config stuff

struct config get_config(char *filename)
{
        struct config configstruct;
        FILE *file = fopen (filename, "r");

        if (file != NULL)
        {
                char line[MAXBUF];
                int i = 0;

                while(fgets(line, sizeof(line), file) != NULL)
                {
                        char *cfline;
                        cfline = strstr((char *)line,DELIM);
                        cfline = cfline + strlen(DELIM);
   
                        if (i == 0){
                                memcpy(configstruct.server,cfline,strlen(cfline));
                        } else if (i == 1){
                                memcpy(configstruct.user,cfline,strlen(cfline));
                        } else if (i == 2){
                                memcpy(configstruct.font,cfline,strlen(cfline));
                        } else if (i == 3){
                                memcpy(configstruct.folders,cfline,strlen(cfline));
                        } else if (i == 4){
                                memcpy(configstruct.command,cfline,strlen(cfline));
                        }
                       
                        i++;
                } 
                fclose(file);
        } 
        return configstruct;
}


