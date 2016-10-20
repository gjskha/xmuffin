/* Default settings */

/* You can change the default colors by editing the source code, they are
 * near the beginning and should be pretty obvious */

/****************************************************************************/

/* Make offline mode the default.  Offline mode is usefull if you have a
 * dialup connection to the internet.  XLassie will display an "X" instead of
 * a number if it is unable to contact the POP3 server.  */

/* #define OFFLINE_DEFAULT */

/****************************************************************************/
/* Interval between checks for new mail, the default is 5 seconds for a local
 * file and 60 seconds for a POP3 server */

#define INTERVAL_POP3	60   /* POP3 default */
#define INTERVAL_SPOOL	5	/* Spool file default */

/****************************************************************************/

/* Default command to run when the window is clicked on.  The command will get
 * executed via system() which means you can use shell contructs.  The command
 * should be placed in the background or xlassie will be frozen until it
 * completes. */

/* run pine in an xterm */
#define COMMAND	"rxvt +sb -T pine -e pine &"

/****************************************************************************/

/* Default font for the display.  You can use xfontsel to try and find some
 * nice fonts on your system.  I would suggest using a scalable font. */

/* Some people don't have the utopia font, but they should, it's one of the
 * standard fonts that come with X. */
#define FONTNAME "-*-utopia-medium-r-normal--40-*-*-*-*-*-iso8859-1"

/****************************************************************************/
/* config stuff

#define CONFIGFILE "xmuffinrc"
#define MAXBUF 1024
#define DELIM "="
