/*  This program (antest.c) was written to take antenna measurements
    at Westford, primarily for hologram analysis.  An interface has been
    setup to read the 19 bit encoder data directly with a Keithly PIO-96
    parallel I/O card.  This program must be run as root (or setuid root)
    because it is performing direct port reads and writes to the PIO-96.
                   8/10/2001   D. Besse
*/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <termios.h>
#include <string.h>
#include <math.h>
#include <netdb.h>
#include "libmat/mat.h"
#include "libmat/matrix.h"
#include "libmat/tmwtypes.h"
#include <win_sdk_types.h>
#include <powerdaq.h>
#include <powerdaq32.h>
#include "networkinglib.h"
#include "safecalls.h"

int fd1;   /* file descriptor for COM1 */
int fdacu;   /* file descriptor for socket to ACU */
int boardid;  /* handle for DAQ board */
int Rcvr_addr;  /* GPIB address of the 1795 receiver */
double readfreq; /* read receiver every <readfreq> seconds */
float thresh;   /* distance from point to know when we're there (deg) */
int initrcvrflag;  /* has receiver been initialized? 0=no, 1=yes */
float azpeak, elpeak;
float Sfreq;    /* source frequency at Pack in GHz */
int acuflag;    /* Using ACU to control Antenna? */
int sock;     /* using socket (1) for ACU control, or RS232 (0) */

/*-------------------------------------------------------
-------------------------------------------------------*/
void init_com1()
 {
 struct termios options;

 fd1 = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd1 == -1)
   {
   printf("Could not open COM1\n");
   exit(1);
   }
 tcgetattr(fd1, &options);
 options.c_cflag &= ~CRTSCTS;  /* disable hardware flow control */
        /* CLOCAL - do not change owner of port
           CREAD  - enable receiver */
 options.c_cflag |= (CLOCAL | CREAD);
     /* select raw input, no echo */
 options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
 
     /* 8 bits, no parity, 1 stop bit */
 options.c_cflag &= ~PARENB;
 options.c_cflag &= ~CSTOPB;
 options.c_cflag &= ~CSIZE;
 options.c_cflag |= CS8;
 
 cfsetispeed(&options, B19200);
 cfsetospeed(&options, B19200);
 
 options.c_cc[VMIN] = 0;
 options.c_cc[VTIME] = (unsigned char)30;   /* 3 sec timeout on reads */
 tcsetattr(fd1, TCSANOW, &options);
 fcntl(fd1, F_SETFL, 0);
 }

void InitDIO(int *boardid) {
  int retval;

  *boardid = PdAcquireSubsystem(0, DigitalIn, 1);
  if (*boardid < 0) {
    printf("** PdAcquireSubsystem failed **\n");
    return;
  }
  printf("Board ID = %d\n", *boardid);
  retval = _PdDIOReset(*boardid);
  if (retval < 0) {
      printf("SingleDIO: PdDInReset error %d\n", retval);
      return;
  }

       /* Do not enable any words for output, all input */
  retval = _PdDIOEnableOutput(*boardid, 0);
   if(retval < 0)
   {
      printf("_PdDioEnableOutput failed\n");
      return;
   }
}

void CloseDIO(int boardid) {
  int retval;
  
  retval = _PdDIOReset(boardid);
  retval = PdAcquireSubsystem(boardid, DigitalIn, 0);
  if (retval < 0) printf("** Release error **\n");
  else printf("Release OK\n");
}

int ReadDIO(int boardid, int iword, DWORD *wordval, int latchflag) {
  int retval;
  
  if (latchflag == 1) {
    retval = _PdDIOLatchAll(boardid, 0);
    if (retval < 0) {
      printf("** Error Latching Encoder data (%d) **\n", retval);
      return(-1);
    }
  }

  retval = _PdDIOSimpleRead(boardid, iword, wordval);
  if (retval < 0) {
    printf("** ReadDIO error %d **\n",retval);
    return(-1);
  }
  return(0);
}

/*-------------------------------------------------------
  If encoder daemon is running, close it.  Only one
  program can open the digital I/O subsystem needed to
  read the encoder data, at any one time.
-------------------------------------------------------*/
void close_encd() {
  int fd;
  
  if (acuflag == 1) return;
  fd = clientconnect("localhost","7433","tcp");
  if (fd > 0) {
    printf("closing encoder daemon...\n");
    write(fd,"kill\n",5);
    close(fd);
    sleep(1);
  }
}

/*-------------------------------------------------------
   Read parallel encoder data from UEI daq board
-------------------------------------------------------*/
void read_encoders(float *az, float *el)
 {
  DWORD w1, w2, w3;
  DWORD iaz, iel;
  int retval;
  
  retval = ReadDIO(boardid, 0, &w1, 1);
  if (retval < 0) {
    *az = 400.0;
    *el = 400.0;
    return;
  }
  ReadDIO(boardid, 1, &w2, 0);
  ReadDIO(boardid, 2, &w3, 0);
  w1 = w1&0xffff;
  w2 = w2&0xffff;
  w3 = w3&0xff;

  iaz = ((w3&7)<<16) + w1;
  iel = ((w3&0x38)<<13) + w2;
  
  *az = (iaz*360.0)/524288.0;
  *el = (iel*360.0)/524288.0;
}

/*----------------------------------------------------------------
loop read of encoders.  Data written to "encs.txt" until user
   presses the ESC key.
---------------------------------------------------------------*/
void lpread()
 {
 struct termios options;
 struct timeval t;
 struct timezone tz;
 struct tm timestruct;
 float az, el;
 FILE *outfile;
 char key;
 double dt, ts, tcur;
 int tdelay;
 
 printf("Enter time between points (milliseconds): ");
 scanf("%d", &tdelay);
 outfile = fopen("encs.txt","w");
 if (outfile == NULL) {
	 printf("** Cannot open file **\n");
	 sleep(3);
	 return;
 }
 fprintf(outfile, "#Year DayofYear Hour Min Sec Elapsed_Sec Az El\n");
 printf("Press the ESC key to end loop.\n");
 tcgetattr(0, &options);
 options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
 tcsetattr(0, TCSANOW, &options);
 fcntl(0, F_SETFL, FNDELAY);
 key = 0;
 gettimeofday(&t, &tz);
 ts = t.tv_sec + (t.tv_usec/1.e6);
 do
   {
   gettimeofday(&t, &tz);
   localtime_r(&t.tv_sec, &timestruct);
   dt = (t.tv_sec + (t.tv_usec/1.e6)) - ts;
   tcur = timestruct.tm_sec + (t.tv_usec/1.e6);
   scanf("%c", &key);
   read_encoders(&az, &el);
   usleep(tdelay*1000);
   fprintf(outfile,"%d %d %02d %02d %lf %lf %f %f",
		   timestruct.tm_year+1900,timestruct.tm_yday+1,
		   timestruct.tm_hour,timestruct.tm_min,tcur, dt, az, el);
   fprintf(stdout,"%lf %f %f", dt, az, el);
   fprintf(outfile,"\n");
   fflush(outfile);
   fprintf(stdout,"\n");
   }
 while (key != 27);
 fclose(outfile);
 options.c_lflag |= (ICANON | ECHO | ECHOE | ISIG);
 tcsetattr(0, TCSANOW, &options);
 fcntl(0, F_SETFL, 0);
 printf("--> Done!\n--> Data written to 'encs.txt'\n\n");
 sleep(3);  /* wait 3 secs before returning to main menu (asthetics)*/
 }

int main(int argc, char *argv[])
 {

 close_encd();  /* shut down encoder daemon if running */
 
 printf("Initializing DIO card...\n");
 InitDIO(&boardid);

 lpread();

 if (boardid > 0) CloseDIO(boardid);
 return(0); 
 }
