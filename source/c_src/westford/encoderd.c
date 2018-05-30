/*
encoder.d - This program is designed to be run as a daemon, and to provide
timely encoder data to the AoA subsystem (or any other client), over a
serial port.
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
#include <win_sdk_types.h>
#include <powerdaq.h>
#include <powerdaq32.h>
#include "safecalls.h"
#include "networkinglib.h"

#define PROTOCOL "tcp"
#define SERVICE "7433"

int boardid;   /* handle for DAQ board */

/*---------------------------------------------------------------
 init_com - initializes a serial port.  The port's device file is
      passed in as the first parameter.  eg:
          /dev/ttyS0  -  built in serial port COM1
---------------------------------------------------------------*/
int
init_com (char *devname, int *fd, speed_t baudrate, int parityflag)
{
  struct termios options;
  int fret;

  *fd = open (devname, O_RDWR | O_NOCTTY | O_NDELAY);
  if (*fd == -1)
    {
      printf ("Could not open serial port - %s\n", devname);
      return (-1);
    }
  tcgetattr (*fd, &options);
  options.c_cflag &= ~CRTSCTS;	/* disable hardware flow control */
  /* CLOCAL - do not change owner of port
     CREAD  - enable receiver */
  options.c_cflag |= (CLOCAL | CREAD);
  /* select raw input, no echo */
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

  /* 8 bits, no parity, 1 stop bit */
  if (parityflag == 0) {
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSIZE;  /* mask character size bits */
    options.c_cflag |= CS8;
    options.c_cflag &= ~CSTOPB;
  }
  else {
    /* 7 bits, odd parity, 1 stop bit */
    options.c_cflag |= PARENB;
    options.c_cflag |= PARODD;
    options.c_cflag &= ~CSIZE;  /* mask character size bits */
    options.c_cflag |= CS7;
    options.c_cflag &= ~CSTOPB;
  }

     /* disable any newline, CR translations on output or input */
  options.c_oflag &= ~ONLCR;
  options.c_oflag &= ~OCRNL;
  options.c_iflag &= ~INLCR;
  options.c_iflag &= ~ICRNL;

  fret = cfsetispeed (&options, baudrate);
  fret = cfsetospeed (&options, baudrate);

  options.c_cc[VMIN] = 1;
  options.c_cc[VTIME] = (unsigned char) 0;	/* 3 sec timeout on reads */
  /*options.c_oflag |= OPOST;*/

  fret = tcsetattr (*fd, TCSANOW, &options);
  if (fret != 0) printf("Error in setting %s\n",devname);
  fret = fcntl (*fd, F_SETFL, 0);
  if (fret == -1) printf("fcntl error for %s\n",devname);

  return (1);
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


   /* read parallel encoder data from UEI daq board */
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
  printf("w1=%04X, w2=%04X, w3=%02X, iaz=%05X, iel=%05X\n", w1, w2, w3, iaz, iel);
  
  *az = (iaz*360.0)/524288.0;
  *el = (iel*360.0)/524288.0;
}

void server_loop()
{
    int mastersock, workersock;
    struct sockaddr_in socketaddr;
    int buflen, fret;
    socklen_t addrlen;
    char buffer[1024];
    float az, el;
    time_t curtimeval;
                                                                                
                                                                                
    /* open a new socket. */
    while (1) {
        do {
            mastersock = serverinit(SERVICE, PROTOCOL);
            if (mastersock < 0)
                sleep(10);
        }
        while (mastersock < 0);
                                                                                
                                                                                
        printf("The server is active.  To terminate, kill the process.\n");
                                                                                
        addrlen = sizeof(socketaddr);
        printf("Waiting for connection...\n");
        workersock =
                accept(mastersock, (struct sockaddr *) &socketaddr,
                       &addrlen);
        if (workersock < 0)
                printf("** Not accepting connections **\n");
        safeclose(mastersock);  /* do not accept any more connections */
                                                                                
        printf("Received connection from a client at ");
        printf("%s port %d\n", inet_ntoa(socketaddr.sin_addr),
               ntohs(socketaddr.sin_port));
        time(&curtimeval);
	while (1) {
	  fret = readnlstring(workersock, buffer, 1022);
	  printf("fret from readnlstring = %d\n",fret);
	  if (fret < 0) break;
	  printf("Got string from client...(%d)\n", buffer[0]);
	  if (strncmp(buffer,"quit",4) == 0) break;
	  if (strncmp(buffer,"exit",4) == 0) break;
	  if (strncmp(buffer,"kill",4) == 0) {
	    safeclose(workersock);
	    return;
	  }
	  read_encoders(&az, &el);
	  buflen = sprintf(buffer,"\n%f  %f\n", az, el);
	  write(workersock,buffer,buflen);
	}
	perror("");
	printf("Exited loop..\n");
	safeclose(workersock);
    }
}                                                                                

int main () {
  int fd;
  
  /* fork a new process.  If after the fork, we are the parent process,
     exit.  This is so that the (child) process will run in the
     background as a daemon. */
  if (fork() > 0) return(0);

  /* init_com("/dev/ttyS1", &fd, B9600, 0); */
  InitDIO(&boardid);
  server_loop();
#if 0
  printf("setting readfds...\n");
  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  outfile = fopen("/home/dave/encoderd.txt","w");
  write(fd,"a",1);
  while(1) {
/*    select(1, &readfds, NULL, NULL, NULL); */
    read(fd,buf,1);
    if (buf[0] == 10) {
      read_encoders(&az,&el);
      buflen = sprintf(buf,"\n%f  %f\n",az,el);
      write(fd,buf,buflen);
      }
    }
  fclose(outfile);
#endif
  //close(fd);
  CloseDIO(boardid);
  return(0);
}
