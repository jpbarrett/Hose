#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <stdarg.h>
#include <time.h>
#include <sys/poll.h>
#include <asm/io.h>
#include <netdb.h>
#include <math.h>
#include "/usr/local/include/sys/ugpib.h"
#include "safecalls.h"
#include "networkinglib.h"

#define PROTOCOL "tcp"
#define SERVICE "7409"
#define NFIELDS 5  /* max number of fields to receive from client */
#define FIELDLEN 120  /* max length of each field from client */

int Rcvr_addr;  /* GPIB address of the 1795 receiver */
int Rcvr_flag;  /* flag indicating whether receiver is responding */
int init_error;     /* initialization error code */

int fd1, fd2;    /* file descriptors for COM1 and COM2 */
float Sfreq;    /* source frequency at Pack in GHz */

int workersock;  /* file descriptor for socket I/O */

/*-------------- end of global declarations -------*/
/*-------------- beginning of functions -----------*/

pid_t
safefork (void)
{
  int retval;

  retval = fork ();

  if (retval == -1)
    {
      HandleError (errno, "fork", "fork failed");
    }
  return retval;
}

void
getfield (char *buffer, int fieldnum, char *res, int maxlen)
{
  char *startp, *endp;
  int i, buflen, fieldlen;

  endp = buffer;
  buflen = strlen (buffer);
  for (i = 0; ((i < fieldnum) && (endp != NULL)); ++i)
    {
      startp = endp;
      if (i > 0)
	++startp;
      if (startp < (buffer + buflen))
	{
	  endp = index (startp, ',');
	  if (endp == NULL)
	    endp = buffer + buflen;
	}
      else
	endp = NULL;
    }
  if (endp == NULL)
    {
      res[0] = 0;
      return;
    }
  fieldlen = endp - startp;
  if (fieldlen > maxlen)
    fieldlen = maxlen;
  if (startp < (buffer + buflen))
    {
      strncpy (res, startp, fieldlen);
      res[fieldlen] = 0;
    }
}

     /* Is 1795 data ready to read yet? */
int check_dataready()
 {
  int i;
  short ppbyte,spbyte;

  for (i=0,ppbyte=0; i<4000 && (ppbyte&15)<4; ++i)
    {
    PPoll(0,&ppbyte);
/*    usleep(200); */
    if ((ppbyte&8)!=0)
      {
      printf("Rcvr Error. Ppoll = %d\n",ppbyte);
      ReadStatusByte(0,Rcvr_addr,&spbyte);  /*serial poll receiver */
      SendCmds(0,"\x19",1);  /* send extra serial poll disable */
      printf("Spoll = %X (hex)\n",spbyte);
      }
    }
  /* readycount = i; */
  if ((ppbyte&12)==4) return(0);
  else return(ppbyte);
 }

  /* the byte order of data we get from the 1795 receiver needs to
     be reversed.  */
void swap4(char x[])
 {
 char c;

 c=x[0];
 x[0]=x[3];
 x[3]=c;
 c=x[1];
 x[1]=x[2];
 x[2]=c;
 }

    /* read data from 1795 receiver.  It is assumed that the desired
       frequency has been set, and the receiver is phase locked. */
int rread(float *ch1, float *ch2, float *ph)
 {
  int cc,errr;
  short ppbyte,spbyte;
  unsigned char tstr[5],readstr[40];
  union
    {
    char c[16];
    float f[4];
    } rcvrret;
  char c;
  
  if (Rcvr_addr < 0) {
    *ch1 = -99.9;
    *ch2 = -99.9;
    *ph  = -999.9;
    return(0);
    }
		   /* check for receiver phase lock */
  cc=0;
  do
    {
    ReadStatusByte(0,Rcvr_addr,&spbyte);  /*serial poll receiver */
/*    usleep(200); */
    SendCmds(0,"\x19",1);  /* send extra serial poll disable */
    ++cc;
    spbyte=spbyte&128;
    spbyte = 128;   /* bypass check for phase lock */
    }
  while (spbyte!=128 && cc<700);  /* check phase lock status bit */
  if (spbyte!=128) return(1);
    
    cc=0;
    /* wloop(1); */
    /* usleep(20); */
    Trigger(0,Rcvr_addr);   /* tell receiver to initiate measurement */
    errr=check_dataready(); /* wait until data is ready (or timeout) */
    if (errr != 0) return(errr);
    Receive(0,Rcvr_addr,rcvrret.c,16,STOPend);   /* read data */
       /* reverse byte order */
    swap4(&rcvrret.c[0]);
    swap4(&rcvrret.c[8]);
    swap4(&rcvrret.c[4]);
    *ch2=rcvrret.f[0];
    *ch1=rcvrret.f[2];
    *ph=rcvrret.f[1];  
 return(errr);
 }

void initrcvr()                  /* this routine sets up parameters on */
 {                              /* the 1795 receiver */
  char buf[20],ch,mode;
  int len,binn;
  short addr[3];

  if (Rcvr_addr < 0) return;
  
  addr[0] = Rcvr_addr;
  addr[1] = NOADDR;
  EnableRemote(0,addr);
  DevClear(0,Rcvr_addr);
  if (ibsta & ERR) {
    init_error = init_error | 1;
    printf("** 1795 Receiver not responding **\n");
    Rcvr_flag = 0;
    return;
    }
/*  Send(0,Rcvr_addr,"NC1",3,NLend);       /* select GPIB as controller */
  Send(0,Rcvr_addr,"G1234",5,NLend);     /* Parallel Poll configure */
  Send(0,Rcvr_addr,"DD2",3,NLend);       /* data destination=gpib */
  Send(0,Rcvr_addr,"Q1",2,NLend);        /* RF lock enable */
  Send(0,Rcvr_addr,"LF2",3,NLend);       /* IEEE 32 bit floating format */
  Send(0,Rcvr_addr,"LA0",3,NLend);       /* Log display */
  Send(0,Rcvr_addr,"LR1",3,NLend);       /* Medium resolution */
  Send(0,Rcvr_addr,"NT2",3,NLend);       /* trigger from gpib */
  Send(0,Rcvr_addr,"EB4",3,NLend);       /* number of samples */
  usleep(10000);
  Rcvr_flag = 1;
 }

void init_com1()
 {
 struct termios options;

 fd1 = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd1 == -1)
   {
   printf("Could not open COM1\n");
   init_error = init_error | 2;
   return;
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
 
 cfsetispeed(&options, B9600);
 cfsetospeed(&options, B9600);
 
 options.c_cc[VMIN] = 0;
 options.c_cc[VTIME] = (unsigned char)30;   /* 3 sec timeout on reads */
 tcsetattr(fd1, TCSANOW, &options);
 fcntl(fd1, F_SETFL, 0);
 }

void init_com2()
 {
 struct termios options;

 fd2 = open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd2 == -1)
   {
   printf("Could not open COM2\n");
   init_error = init_error | 4;
   return;
   }
 tcgetattr(fd2, &options);
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
 
 cfsetispeed(&options, B9600);
 cfsetospeed(&options, B9600);
 
 options.c_cc[VMIN] = 0;
 options.c_cc[VTIME] = (unsigned char)30;   /* 3 sec timeout on reads */
 tcsetattr(fd2, TCSANOW, &options);
 fcntl(fd2, F_SETFL, 0);
 }

   /* send position command over RS-232 interface to pointing computer.
      Angles should be passed to this function in absolute degrees
      (not offsets).  They are then converted to offsets. */
void command(float az, float el)
 {
 int n;
 char buf[40];

 if (el < 0.0) el = 0.00001; 
 az = az - 310.255;
 el = el - 0.494;
 n = sprintf(buf,"AZELOFF = %.4f, %.4f\n", az, el);
 printf("%s\n",buf);
 write(fd1,buf,n);
 }

   /* send 'PACK' command to send antenna to default PACK position */
void set_pack()
 {
 printf("Pointing to default PACK position\n");
 write(fd1,"PACK\n",5);
 }

  /* init_pio sets up the Keithly PIO-96 parallel I/O card for direct
     encoder input. No outputs are used on this card.  It is assumed
     that the PIO-96 base address is set to 300 (hex). */
void init_pio()
 {
                                                                                
    /* request permission from the OS to access computer's I/O ports
       in order to talk to PIO-96 */
 ioperm(0x300,16,255);
                                                                                
   /* PIO-96 control byte:  all inputs */
 outb(0x9B,0x303);  /* port 1 */
 outb(0x9B,0x307);  /* port 2 */
 }
                                                                                
   /* read parallel encoder data from PIO-96 I/O board */
void read_encp(float *az, float *el)
 {
 union {
   unsigned char b[4];
   long l;
   } azi, eli;
                                                                                
 azi.b[0] = inb(0x301);
 azi.b[1] = inb(0x302);
 azi.b[2] = inb(0x300);
 azi.b[3] = 0;
    /* the bits aren't exactly where we want them, so shift them
       over 1, and mask out the bits we are not using */
 azi.l = (azi.l >> 1) & 0x7ffff;
                                                                                
 eli.b[0] = inb(0x305);
 eli.b[1] = inb(0x306);
 eli.b[2] = inb(0x304);
 eli.b[3] = 0;
 eli.l = (eli.l >> 1) & 0x7ffff;
                                                                                
 *az = azi.l*360.0/524288.0;
 *el = eli.l*360.0/524288.0;
 if (*el > 180.0) *el -= 360.0;
 }

/* The chgfreq function changes the frequency of the comstron at Pack
   using the rexec call to the fastscan computer at Pack.  The fss computer
   actually changes the frequency via Cipolle's custom I/O board.  
   The frequency on the 1795 receiver is also changed. */
void chgfreq(float f)
 {
 char host[80], *hostptr=host;
 char user[20];
 char password[20];
 char cmd[120];
 int fd1, fd2, sl;
 float ch1,ch2,ph;

 fd2 = 0;
 if (f<17.5) f=17.5;
 if (f>21.5) f=21.5;
 Sfreq = rintf(f*10.0)/10.0;
 return;
 
 strcpy(host,"155.34.207.130");
 strcpy(user,"dcip");
 strcpy(password,"fss");
 sprintf(cmd,"c:\\bc4\\bin\\westford\\setfss /f%3d /pr",(int)(Sfreq*10.0+0.4));
 fd1 = rexec(&hostptr, getservbyname("exec","tcp")->s_port,
 user, password, cmd, &fd2 );
 if (fd1 < 0) printf("rexec call failed\n");
 else close(fd1);

   /* now set the frequency on the 1795 receiver */
 if (Rcvr_flag > 0) {
   sl = sprintf(cmd,"F%.2f",Sfreq);
   Send(0,Rcvr_addr,cmd,sl,NLend);
     /* a couple of dummy reads to make sure the receiver is
        locked and giving us data */
   rread(&ch1, &ch2, &ph);
   rread(&ch1, &ch2, &ph);
   }
 }

void setfreq(char field[NFIELDS][FIELDLEN], int nfields, char *retstring)
{
  float f;
  
  sscanf(field[1],"%f",&f);
  chgfreq(f);
  sprintf(retstring,"%5.2f OK",Sfreq);
}

void query1795(char field[NFIELDS][FIELDLEN], int nfields, char *retstring)
{

  strcpy(retstring,"-1.2 -20.44 128.3             ");
}

void queryencs(char field[NFIELDS][FIELDLEN], int nfields, char *retstring)
{
  float az, el;
  
  read_encp(&az, &el);
  sprintf(retstring,"%9.4f %9.4f\n", az, el);
}

void
runscan(char field[NFIELDS][FIELDLEN], int nfields, char *retstring)
{
  int i, sockpoll;
  struct pollfd fds[1];
  char inchar;
  
  printf("In runscan...\n");
  fds[0].fd = workersock;
  fds[0].events = 1;  /* poll for data to be read */
  
  inchar = 0;
  for (i=0; i<10; ++i) {
    sockpoll = poll(fds, 1, 0);
    if (sockpoll > 0) read(workersock, &inchar, 1);
    if (inchar == 27) break;
    printf("point %d, sockpoll = %d, inchar = %d\n",i,sockpoll, inchar);
    sleep(1);
    }
  strcpy(retstring,"done");
}

void
act_on_command (char *cmdstr)
{
  char *instr, field[NFIELDS][FIELDLEN];
  size_t nchars;
  int nfields, fi;

  instr = cmdstr;
  /* clear fields */
  for (fi = 0; fi < NFIELDS; ++fi)
    field[fi][0] = 0;
  fi = 0;
  /* parse out fields in command string */
  do
    {
      nchars = strcspn (instr, " ,\n\r");
      if (nchars == 0)
	break;
      if (nchars > (FIELDLEN-1))
	nchars = FIELDLEN-1;
      strncpy (field[fi], instr, nchars);
      field[fi][nchars] = 0;
      printf ("(%s) ", field[fi]);
      instr += (nchars + 1);
      ++fi;
      if ((instr - cmdstr) >= strlen (cmdstr))
	break;
    }
  while (fi < NFIELDS);

  printf ("\n");
  nfields = fi;
  if (strcmp (field[0], "qenc") == 0)
    queryencs (field, nfields, cmdstr);
  if (strcmp (field[0], "q1795") == 0)
    query1795 (field, nfields, cmdstr);
  if (strcmp (field[0], "sfreq") == 0)
    setfreq (field, nfields, cmdstr);
  if (strcmp (field[0], "rscan") == 0)
    runscan (field, nfields, cmdstr);
}

int
waitforstring (int fd, char *buf, int nchars, time_t timeout)
{
  struct pollfd fds;
  int freturn;
  time_t time1, time2;

  buf[0] = 0;			/* clear buffer */
  fds.fd = fd;
  fds.events = POLLIN;		/* poll for input data */
  time (&time1);
  do
    {
      freturn = poll (&fds, 1, 20);
      if (freturn == 0)
	usleep (5000);
      time (&time2);
    }
  while ((freturn == 0) && (((time2 - time1) < timeout) || (timeout < 0)));
  if (freturn <= 0)
    return (-1);
  freturn = readnlstring (fd, buf, nchars);
  return (freturn);
}

void
server_loop ()
{
  int mastersock;
  struct sigaction act;
  struct sockaddr_in socketaddr;
  int addrlen, pwcomp, loopcount, err;
  char buffer[1024], shapass[42];
  char size[100], filename[50];
  time_t curtimeval;
  FILE *logfile;

      do
	{
	  mastersock = serverinit (SERVICE, PROTOCOL);
	  if (mastersock < 0)
	    sleep (10);
	}
      while (mastersock < 0);
      /* set socket to non-blocking mode, so we can do other
         things while waiting for a connection */
      fcntl (mastersock, F_SETFL, O_NONBLOCK);

      printf ("The server is active.  To terminate, kill the process.\n");

      addrlen = sizeof (socketaddr);
      /* poll workersock until we get a connection ( > 0 ) */
      do
	{
	  workersock =
	    accept (mastersock, (struct sockaddr *) &socketaddr, &addrlen);
	  usleep (100000);
	}
      while (workersock < 0);

      if (workersock < 0)
	{
	  HandleError (errno, "accept", "couldn't open worker socket");
	}
      safeclose (mastersock);	/* do not accept any more connections */

      printf ("Received connection from a client at ");
      printf ("%s port %d\n", inet_ntoa (socketaddr.sin_addr),
	      ntohs (socketaddr.sin_port));
      sprintf(buffer,"err-%03d\n",init_error);
      write(workersock,buffer,7);
      time (&curtimeval);

      while (readnlstring (workersock, buffer, sizeof (buffer)) >= 0)
	 {
	   if (strncmp (buffer, "exit", 4) == 0)
	     break;
	   printf("-in-> %s\n",buffer);
	   act_on_command (buffer);
	   write_buffer (workersock, buffer, strlen (buffer));
	   printf("-out-> %s\n",buffer);
	   buffer[0] = 0;
	 }

      printf ("Closing...\n");
      time (&curtimeval);

      safeclose (workersock);
      printf ("Closed workersock.\n");

  printf ("returning and exiting...\n");
}


main ()
{
  time_t curtime;
  char filename[60], *datestr;
  FILE *logfile;

  if (safefork ())
    exit (0);			/* parent process exiting */

  /* at this point we are in the child process */

 init_error = 0;
 printf("Initializing GPIB...\n");
 SendIFC(0);
 if (ibsta & ERR) {
   printf("** Cannot initialize GPIB!!  Exiting...\n");
   exit(3);
   }

  Rcvr_addr = 6;
  Rcvr_flag = 0;
  init_pio ();
  initrcvr ();
  server_loop ();

}
