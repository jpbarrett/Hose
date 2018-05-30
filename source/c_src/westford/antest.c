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
#include "/usr/local/natinst/ni4882/include/ni488.h"
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
 
 //cfsetispeed(&options, B19200);
 //cfsetospeed(&options, B19200);
 cfsetispeed(&options, B9600);
 cfsetospeed(&options, B9600);
 
 options.c_cc[VMIN] = 0;
 options.c_cc[VTIME] = (unsigned char)30;   /* 3 sec timeout on reads */
 tcsetattr(fd1, TCSANOW, &options);
 fcntl(fd1, F_SETFL, 0);
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
  short spbyte;
  union
    {
    char c[16];
    float f[4];
    } rcvrret;
  
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

void setrcvr()                  /* this routine sets up parameters on */
 {                              /* the 1795 receiver */
  short addr[3];

  if (Rcvr_addr < 0) return;
  
  printf("Initializing 1795 Receiver...\n");
  addr[0] = Rcvr_addr;
  addr[1] = NOADDR;
  EnableRemote(0,addr);
  DevClear(0,Rcvr_addr);
/*  Send(0,Rcvr_addr,"NC1",3,NLend); */      /* select GPIB as controller */
  Send(0,Rcvr_addr,"G1234",5,NLend);     /* Parallel Poll configure */
  Send(0,Rcvr_addr,"DD2",3,NLend);       /* data destination=gpib */
  Send(0,Rcvr_addr,"Q1",2,NLend);        /* RF lock enable */
  Send(0,Rcvr_addr,"LF2",3,NLend);       /* IEEE 32 bit floating format */
  Send(0,Rcvr_addr,"LA0",3,NLend);       /* Log display */
  Send(0,Rcvr_addr,"LR1",3,NLend);       /* Medium resolution */
  Send(0,Rcvr_addr,"NT2",3,NLend);       /* trigger from gpib */
  Send(0,Rcvr_addr,"EB4",3,NLend);       /* number of samples */
  Send(0,Rcvr_addr,"LB1A00R0x",9,NLend);
  Send(0,Rcvr_addr,"LB2R00R0x",9,NLend);
  usleep(10000);
  initrcvrflag = 1;
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

/*---------------------------------------------------
---------------------------------------------------*/
int send_acu(char *command, char *retstring, int maxlen, int *erret) {
  int fret, prompt, timeout, rindex, cmdlen;
  time_t curtime, starttime;

  if (fdacu <= 0) return(-98);

  cmdlen = strlen(command);
  if (cmdlen > 0) {
    write(fdacu,command,cmdlen);
    write(fdacu,"\r",1);  /* write trailing CR */
  }
  timeout = 0;
  rindex = 0;
  time(&starttime);
  do {
    fret = read(fdacu,&retstring[rindex],1);
    if (fret > 0) {
      ++rindex;
      retstring[rindex] = 0;
    }
    prompt = 0;
    if (rindex>4) {
      if (retstring[rindex-1] == '>') ++prompt;
      if ((retstring[rindex-2] >= '0') && (retstring[rindex-2] <= '9')) ++prompt;
      if ((retstring[rindex-3] >= '0') && (retstring[rindex-3] <= '9')) ++prompt;
      if ((retstring[rindex-4] >= '0') && (retstring[rindex-4] <= '9')) ++prompt;
    }
    time(&curtime);
    if ((curtime-starttime) > 3) timeout = 1;
    else timeout = 0;
  }
  while((prompt < 4) && (timeout == 0) && (rindex<maxlen));
  if (timeout == 1) return(-1);
  if (rindex >= maxlen) return(-2);
  sscanf(&retstring[rindex-4], "%d", erret);
  return(0);
}


/*---------------------------------------------------
---------------------------------------------------*/
int init_acu() {
  char retstring[80];
  int fret, erret;

  if (sock == 1) fdacu = clientconnect("155.34.203.222", "23", "tcp");
  else fdacu = fd1;
  if (fdacu > 0) {
    fcntl(fdacu, F_SETFL, O_NONBLOCK);
    printf("Socket to ACU opened! (%d)\n", fdacu);
  } else {
    printf("** ERR: init_acu: Cannot open socket to ACU **\n");
    return(-5);
  }

      /* No operation command */
  printf("Sending R0 command...\n");
  fret = send_acu("R0", retstring, 40, &erret);
  printf("return from R0 command: fret = %d\n", fret);
  if (fret < 0) return(fret);
  printf("%s\n", retstring);

      /* change security level to operator */
  fret = send_acu("R5 2", retstring, 40, &erret);
  if (fret < 0) return(10*fret);
  printf("%s\n", retstring);
  if (erret > 1) {
    printf("** ERR: Cannot change security level **\n");
    return(-13);
  }

  return(0);
}

/*---------------------------------------------------
    read parallel encoder data from 721X/7205 ACU
---------------------------------------------------*/
void read_encoders_acu(float *az, float *el) {
  int fret, erret;
  char retstring[80];

  fret = send_acu("R1 0", retstring, 50, &erret);
  if (fret < 0) {
    printf("reading encoders fret = %d\n", fret);
    *az = -999.1;
    *el = -999.1;
    return;
  }
  if (erret > 1) {
    *az = -999.2;
    *el = -999.2;
    return;
  }
  printf("------\n%s\n--------\n", retstring);
  fret = sscanf(&retstring[1], "%f %f", az, el);
  if (fret != 2) {
    *az = -999.3;
    *el = -999.3;
  }
}

/*---------------------------------------------------
---------------------------------------------------*/
int position_acu(float az, float el) {
  int fret, erret;
  char retstring[80], cmdstr[80];

  sprintf(cmdstr, "C2 %.3f %.3f 0.0", az, el);
  printf("%s\n", cmdstr);
  fret = send_acu(cmdstr, retstring, 50, &erret);
  if (fret < 0) {
    printf("**ERR positioning antenna**\n");
    return(-1);
  }
  if (erret > 1) {
    printf("**ERR positioning: ACU returned: %d **\n", erret);
    return(-2);
  }
  return(0);
}

/*---------------------------------------------------
---------------------------------------------------*/
void close_acu() {
  int erret, fret;
  char retstring[80];

  if (acuflag != 1) return;
     /* set security level back to monitor */
  fret = send_acu("R5 1", retstring, 50, &erret);
  if (fdacu > 0) close(fdacu);
}

/*-------------------------------------------------------
   Read parallel encoder data from UEI daq board
-------------------------------------------------------*/
void read_encoders(float *az, float *el)
 {
  DWORD w1, w2, w3;
  DWORD iaz, iel;
  int retval;
  
  if (acuflag == 1) {
    read_encoders_acu(az, el);
    return;
  }

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

/*-------------------------------------------------------
   Send position command over RS-232 interface to pointing computer.
      Angles should be in absolute degrees (not offsets).
-------------------------------------------------------*/
void command(float az, float el)
 {
 int n;
 char buf[40];

 if (acuflag == 1) {
   position_acu(az, el);
   return;
 }

 if (el < 0.0) el = 0.00001; 
 az = az - 310.255;
 el = el - 0.494;
 n = sprintf(buf,"AZELOFF = %.4f, %.4f\n", az, el);
 printf("%s\n",buf);
 write(fd1,buf,n);
 }

/*-------------------------------------------------------
   Send position command over RS-232 interface to pointing computer.
      Angles should be in absolute degrees (not offsets).
-------------------------------------------------------*/
void abscommand(float az, float el)
 {
 int n;
 char buf[40];

 if (acuflag == 1) {
   position_acu(az, el);
   return;
 }

 if (el < 0.0) el = 0.00001; 
 n = sprintf(buf,"AZEL = %.4f, %.4f\n", az, el);
 printf("%s\n",buf);
 write(fd1,buf,n);
 }

/*-------------------------------------------------------
   Send 'PACK' command to send antenna to default PACK position.
-------------------------------------------------------*/
void set_pack()
 {

 if (acuflag == 1) return;

 printf("Pointing to default PACK position\n");
 write(fd1,"PACK\n",5);
 }

/*-------------------------------------------------------
-------------------------------------------------------*/
void gen_fname(char *type, char *fname)
 {
 time_t curtime;
 char *dtstr;
 char months[12][3]={"Jan","Feb","Mar","Apr","May","Jun","Jul",
                     "Aug","Sep","Oct","Nov","Dec"};
 int i, monthindex, dayindex, yearindex;
 char tmstr[12];
 
 time(&curtime);
 dtstr = ctime(&curtime);
 printf("%s\n",dtstr);
 
 monthindex=0;
 for (i=0; i<12; ++i) if (strncmp(months[i],&dtstr[4],3)==0) monthindex=i+1;
 sscanf(&dtstr[8],"%d",&dayindex);
 sscanf(&dtstr[20],"%d",&yearindex);
 yearindex -= 2000;
 strncpy(tmstr,&dtstr[11],5);
 tmstr[2] = tmstr[3];
 tmstr[3] = tmstr[4];
 tmstr[4] = 0;
 sprintf(fname,"h%02d%02d%02d_%s_",yearindex,monthindex,dayindex,tmstr);
 i = strlen(type)-1;
 if ((type[i]==10) || (type[i]==13)) type[i] = 0;
 strcat(fname,type);
 strcat(fname,".dat");
 }

/*-------------------------------------------------------
   The chgfreq function changes the frequency of the comstron at Pack
   using the rexec call to the fastscan computer at Pack.  The fss computer
   actually changes the frequency via Cipolle's custom I/O board.  
   The frequency on the 1795 receiver is also changed.
-------------------------------------------------------*/
void chgfreq_old()
 {
 char host[80], *hostptr=host;
 char user[20];
 char password[20];
 char cmd[120];
 int fd1, fd2, sl;
 float ch1,ch2,ph,f;

 fd2 = 0;
 printf("Enter new frequency (GHz): ");
 scanf("%f",&f);
 if ((f<17.499) || (f>21.501))
   {
   printf("(%f) Invalid frequency!!\n",f);
   return;
   }
 Sfreq = f;
 strcpy(host,"155.34.207.130");
 strcpy(user,"dcip");
 strcpy(password,"fss");
 sprintf(cmd,"c:\\bc4\\bin\\westford\\setfss /f%3d /pr",(int)(Sfreq*10.0+0.4));
 fd1 = rexec(&hostptr, getservbyname("exec","tcp")->s_port,
 user, password, cmd, &fd2 );
 if (fd1 < 0) printf("rexec call failed\n");
 else close(fd1);

   /* now set the frequency on the 1795 receiver */
 if (Rcvr_addr > 0) {
   sl = sprintf(cmd,"F%.2f",Sfreq);
   Send(0,Rcvr_addr,cmd,sl,NLend);
     /* a couple of dummy reads to make sure the receiver is
        locked and giving us data */
   rread(&ch1, &ch2, &ph);
   rread(&ch1, &ch2, &ph);
   }
 }

/*-------------------------------------------------------
-------------------------------------------------------*/
int wait_for_ack(int fd) {
  int p, fret;
  char buf[15], c;
  struct timeval tv;
  struct timezone tz;
  time_t start_time, etime;
  
  p = 0;
  c = 0;
  gettimeofday(&tv,&tz);
  start_time = tv.tv_sec;
  do {
    fret = read(fd,&c,1);
    if (fret > 0) {
      buf[p] = c;
      ++p;
    }
    usleep(250000);
    gettimeofday(&tv,&tz);
    etime = tv.tv_sec - start_time;
  }
  while ((c != 10) && (p < 7) && (etime < 3));
  buf[p] = 0;
  if (strncmp(buf,"ack",3) == 0) return(1);
  else return(-1);
}

/*-------------------------------------------------------
   The chgfreq function changes the frequency of the comstron at Pack
   using a socket call to the fastscan computer at Pack.  The fss computer
   actually changes the frequency via Cipolle's custom I/O board.  
   The frequency on the 1795 receiver is also changed.
-------------------------------------------------------*/
void chgfreq()
 {
 char cmd[120];
 int fd1, sl, fret;
 float ch1, ch2, ph, f, sourcefreq;

 printf("Enter new (receive) frequency (GHz)::: ");
 scanf("%f",&f);
 if ((f>=32.499) && (f<=36.501)) sourcefreq = f-15.0;
 else sourcefreq = f;
 if ((sourcefreq<17.499) || (sourcefreq>21.501))
   {
   printf("(%f) Invalid frequency!!\n",f);
   return;
   }
 Sfreq = f;
#if 1
 printf("trying to connect to pack...\n");
 fd1 = clientconnect("pfastscan","2001","tcp");
 printf("socket fd = %d\n",fd1);
 if (fd1>0) {
   fcntl(fd1, F_SETFL, O_NONBLOCK);
   sl = sprintf(cmd,"Freq%d\r\n",(int)(sourcefreq*10.0+.1));
   write(fd1,cmd,sl);
   fret = wait_for_ack(fd1);
   if (fret < 0) printf("** Error writing to Pack fastscan **\n");
   else {
     sl = sprintf(cmd,"Pol1\r\n");
     write(fd1,cmd,sl);
     wait_for_ack(fd1);
   }
   close(fd1);
 }
 else printf("Cannot connect to Pack Fastscan Computer\n");
#endif

   /* now set the frequency on the 1795 receiver */
 if (Rcvr_addr > 0) {
   sl = sprintf(cmd,"F%.2f",Sfreq);
   Send(0,Rcvr_addr,cmd,sl,NLend);
     /* a couple of dummy reads to make sure the receiver is
        locked and giving us data */
   rread(&ch1, &ch2, &ph);
   rread(&ch1, &ch2, &ph);
   }
 }

/*-------------------------------------------------------
   function makemat reads the plain text file just created, and
   creates a MAT (matlab binary) file of the same name with a .mat
   extension.
-------------------------------------------------------*/
void makemat(char *flname, long nrecords, char *descrip, char *mtime, char *pfile)
  /* parameters:
         flname:   name of text file to read and convert
         nrecords: number of records in flname
         descrip:  description of measurement (to be put in MAT file)
         mtime:    time and date of measurement (put in MAT file)
         pfile:    name of parameter file used to run meas. (put in MAT file)
  */
 {
 int i;
 FILE *tfile;
 double *t, *az_abs, *el_abs, *z_ref_db, *z_meas_db, *z_meas_ph;
 MATFile *fp;
 mxArray *mxa;
 char *extptr, fname[80], descriptor[120], meastime[80];
 double freq;

 if (flname[0] == 0) {
   printf("Enter file name: ");
   scanf("%s",fname);
   printf("Enter number of records: ");
   scanf("%ld",&nrecords);
   freq = 44.1;
   strcpy(descriptor,"Westford Hologram test file");
   strcpy(meastime,"Sun Jul 4 00:00:00 1776");
   }
 else {
   freq = (double)Sfreq;
   strcpy(fname, flname);
   strcpy(descriptor,descrip);
   strcpy(meastime,mtime);
   }

 tfile = fopen(fname,"r");
 if (tfile == NULL) {
   printf("Cannot open text data file\n");
   return; }

 t = malloc(sizeof(double)*nrecords);
 az_abs = malloc(sizeof(double)*nrecords);
 el_abs = malloc(sizeof(double)*nrecords);
 z_ref_db = malloc(sizeof(double)*nrecords);
 z_meas_db = malloc(sizeof(double)*nrecords);
 z_meas_ph = malloc(sizeof(double)*nrecords);
 
 for (i=0; i<nrecords; ++i) {
   fscanf(tfile,"%lf %lf %lf %lf %lf %lf\n",&t[i], &az_abs[i],
                    &el_abs[i], &z_ref_db[i], &z_meas_db[i], &z_meas_ph[i]);
   }
 fclose(tfile);

 extptr = strstr(fname,".dat");
 if (extptr == NULL) strcat(fname,".mat");
 else strcpy(extptr,".mat");
 fp = matOpen(fname,"w");

 mxa = mxCreateDoubleMatrix(1,1,mxREAL);
 *mxGetPr(mxa) = freq*1.e9;   /* convert to Hz */
 mxSetName(mxa,"frequency");
 matPutArray(fp,mxa);

 *mxGetPr(mxa) = (double)azpeak;
 mxSetName(mxa,"azpeak");
 matPutArray(fp,mxa);

 *mxGetPr(mxa) = (double)elpeak;
 mxSetName(mxa,"elpeak");
 matPutArray(fp,mxa);

 mxDestroyArray(mxa);

 mxa = mxCreateString(descriptor);
 mxSetName(mxa,"descriptor");
 matPutArray(fp,mxa);
 mxDestroyArray(mxa);

 mxa = mxCreateString(meastime);
 mxSetName(mxa,"meastime");
 matPutArray(fp,mxa);
 mxDestroyArray(mxa);

 mxa = mxCreateString(pfile);
 mxSetName(mxa,"parameterfile");
 matPutArray(fp,mxa);
 mxDestroyArray(mxa);

 mxa = mxCreateDoubleMatrix(nrecords,1,mxREAL);
 memcpy(mxGetPr(mxa),t,nrecords*sizeof(double));
 mxSetName(mxa,"t");
 matPutArray(fp,mxa);

 memcpy(mxGetPr(mxa),az_abs,nrecords*sizeof(double));
 mxSetName(mxa,"az_abs");
 matPutArray(fp,mxa);

 memcpy(mxGetPr(mxa),el_abs,nrecords*sizeof(double));
 mxSetName(mxa,"el_abs");
 matPutArray(fp,mxa);

 memcpy(mxGetPr(mxa),z_ref_db,nrecords*sizeof(double));
 mxSetName(mxa,"z_ref_db");
 matPutArray(fp,mxa);

 memcpy(mxGetPr(mxa),z_meas_db,nrecords*sizeof(double));
 mxSetName(mxa,"z_meas_db");
 matPutArray(fp,mxa);

 memcpy(mxGetPr(mxa),z_meas_ph,nrecords*sizeof(double));
 mxSetName(mxa,"z_meas_ph");
 matPutArray(fp,mxa);

 mxDestroyArray(mxa); 

 matClose(fp);
 free(t);
 free(az_abs);
 free(el_abs);
 free(z_ref_db);
 free(z_meas_db);
 free(z_meas_ph);
 }

/*-------------------------------------------
  Open a socket connection to a device, and
  return file descriptor used to communicate
  with the device.
--------------------------------------------*/
int init_socket (char *host, char *port) {
  struct addrinfo hints;
  struct addrinfo *servinfo;
  int status, sockfd;

  printf("strlen(host) = %d\n", strlen(host));
  if ((strlen(host)<1) || (strlen(port)<1)) return(-1);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(host, port, &hints, &servinfo);
  if (status != 0) {
    printf("** getaddrinfo failed **\n");
    return(-3);
  }
  sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
              servinfo->ai_protocol);
  if (sockfd < 0) {
    printf("** socket call failed **\n");
    freeaddrinfo(servinfo);
    return(-1);
  }
  status = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
  freeaddrinfo(servinfo);
  if (status < 0) {
    printf("** connect failed **\n");
    return(-2);
  }
  fcntl (sockfd, F_SETFL, O_NONBLOCK);

  return(sockfd);
}

/*----------------------------------------------------------------------
   Reads bytes from a non-blocking socket until specified character is
   found, or request times out (returning an error)
----------------------------------------------------------------------*/
int readdelimstringnet(int sockid, char *buf, int maxlen, char delim) {
  int count, status;
  time_t curtime, starttime, elapsedtime;

  count = 0;
  elapsedtime = 0;
  time(&starttime);
  while ((count<maxlen) && (elapsedtime<=3)) {
    status = read(sockid, buf+count, 1);
    if (status > 0) {
      if (buf[count] == delim) {
        buf[count+1] = 0;
        return (count+1);
      }
      count++;
    }
    time(&curtime);
    elapsedtime = curtime - starttime;
  }
  return count;
}

/*-------------------------------------------------------- 
  Wait until antenna has settled at desired point
-------------------------------------------------------*/
void wait2arrive(float az, float el) {
  float curaz, curel, nextaz, nextel, adif, pdif;
  float azoff, eloff;  // pointing system built in offsets
  int closecount;

  azoff = .061;
  eloff = .004;
  closecount = 0;
  sleep(2);
  az = az+azoff;
  el = el+eloff;
  while(1) {
    read_encoders(&curaz, &curel);
    sleep(2);
    read_encoders(&nextaz, &nextel);
    adif = fabs(curaz-nextaz) + fabs(curel-nextel);
    pdif = fabs(nextaz-az) + fabs(nextel-el);
    printf("adif=%.3f, pdif=%.3f\n",adif,pdif);
    if ((adif<0.005) && (pdif<0.005)) break;
  }
}

/*--------------------------------------------------------
  Reads operation event register looking for end of sweep
  Returns 1 when sweep is done.
--------------------------------------------------------*/
int saswpquery(int specid) {
  char buf[150];
  int buflen, reg;
  
    read(specid, buf, 140);  /* clear buffer of any leftover stuff */
    write(specid, "STATUS:OPER:COND?\n", 19);
    buflen = readdelimstringnet(specid, buf, 49, 10);
    if (buflen < 1) {
      printf("cannot read status\n");
      return(-1);
    } else printf("stat = %s\n", buf);

    sscanf(buf, "%d", &reg);
    reg = reg&8;
    if (reg == 0) return(1);
    else return(0);
}

/*-------------------------------------------------------- 
  Use Max Hold to read peak signal at Spectrum Analyzer
-------------------------------------------------------*/
float readpeak(int specid) {
  int statbyte, i, fret;
  float dbmax, db;
  char buf[50];

  dbmax = -999.9;
  for (i=0; i<1; ++i) {
    usleep(7000);
    write(specid, "INIT\n", 5);  // trigger sweep
    do {
      usleep(100000);
      statbyte = saswpquery(specid);
    }
    while(statbyte == 0);
    write(specid, "CALC:MARK1:MAX\n", 15);  // move marker to peak
    usleep(50000);
    write(specid, "CALC:MARK1:Y?\n", 14);   // request marker Y value
    fret = readdelimstringnet(specid, buf, 49, 10);
    printf("marker read (%d): %s\n", fret, buf);
    if (fret < 1) db = -993.06;
    else sscanf(buf, "%f", &db);
    if (db > dbmax) dbmax = db;
  }  // end: for (i...
  return(dbmax);
}

/*---------------------------------------------------------
 *    Set mode of STDIN to immediate read, no echo
 *    ---------------------------------------------------------*/
void set_imm_stdin() {
  struct termios options;

  tcgetattr(0, &options);
  options.c_lflag &= ~(ICANON | ECHO | ECHOE);
  tcsetattr(0, TCSANOW, &options);
  fcntl(0, F_SETFL, FNDELAY);
}

/*---------------------------------------------------------
 *    Reset mode of STDIN to normal
 *    ---------------------------------------------------------*/
void reset_stdin() {
  struct termios options;

  tcgetattr(0, &options);
  options.c_lflag |= (ICANON | ECHO | ECHOE | ISIG);
  tcsetattr(0, TCSANOW, &options);
  fcntl(0, F_SETFL, 0);
}

/*---------------------------------------------------------
 *    Record specAN and angles while antenna is moving
 *    ---------------------------------------------------------*/
void recscan()
{
  float db, az, el, dechour;
  FILE *outfile;
  char fname[80], key;
  int specid;
  struct timezone tz;
  struct timeval t;
  struct tm timestruct;

  printf("Enter file name: ");
  scanf("%s", fname);
  outfile = fopen(fname, "a");
  specid = init_socket("psa1", "5025");
  if (specid < 0) {
    printf("** Cannot open socket to SpecAN **\n");
    return;
  }
  write(specid, "INIT:CONT OFF\n", 14);  //turn specan free run off
  set_imm_stdin();
  key = 0;
  while(1) {
    scanf("%c", &key);
    if (key == 27) break;
    gettimeofday(&t, &tz);
    localtime_r(&t.tv_sec, &timestruct);
    db = readpeak(specid);
    dechour = timestruct.tm_hour+(timestruct.tm_min/60.0)+
               (timestruct.tm_sec/3600.0)+(t.tv_usec/3600.0e6);

    read_encoders(&az, &el);
    fprintf(outfile,"%f %.4f %.4f %.2f\n", dechour, az, el, db);
  }
  fclose(outfile);
  reset_stdin();
    
}

/*-------------------------------------------------------- 
  Perform an antenna scan (single cut) using Spectrum Analyzer
-------------------------------------------------------*/
void specscan()
{
  float az, azstart, azstop, azinc, elstart;
  float db;
  int specid;
  FILE *outfile;

  specid = init_socket("psa1", "5025");
  if (specid < 0) {
    printf("** Cannot open socket to SpecAN **\n");
    return;
  }
  write(specid, "INIT:CONT OFF\n", 14);  //turn specan free run off
  outfile = fopen("specscan.txt","a");

  printf("Enter Starting Elevation Angle: ");
  scanf("%f", &elstart);
  printf("Enter Azstart, Azstop, Azinc: ");
  scanf("%f,%f,%f", &azstart, &azstop, &azinc);

  for (az=azstart; az<=azstop; az+=azinc) {
    abscommand(az, elstart);
    wait2arrive(az, elstart);
    db = readpeak(specid);
    fprintf(outfile,"%.4f %.4f %.2f\n", az, elstart, db);
  }

  fclose(outfile);
  close(specid);
}

/*-------------------------------------------------------- 
  Perform an antenna scan
-------------------------------------------------------*/
void ascan()
 {
 float az, el;
 float amp1, amp2, ph, *aztab, *eltab;
 float thresh_s, azdiff, eldiff, vdiff;
 char fname[80], ftype[15], azelfname[80], key;
 char descriptor[120];
 FILE *datfile, *azelfile;
 struct timeval t;
 struct timezone tz;
 double dt, dtstart, dtarrive, dwell, lastread;
 struct termios options;
 int i, nazel, pointindex;
 long nrecords;
 time_t start_timet;
 
 if (Sfreq < 1.0) chgfreq();
 if (initrcvrflag == 0) setrcvr();
 printf("Enter filename of az-el pairs: ");
 scanf("%s", azelfname);
 azelfile = fopen(azelfname,"r");
 if (azelfile == NULL) {
   printf("Cannot open AZ-EL file\n");
   return; }
 
 fscanf(azelfile,"%d",&nazel);
 printf("num points = %d\n",nazel);
 aztab = (float *)malloc(sizeof(float)*nazel);
 eltab = (float *)malloc(sizeof(float)*nazel);
 if ((aztab == NULL) || (eltab == NULL)) {
   printf("memory allocation failed!!\n");
   return; }
   
 for (i=0; i<nazel; ++i)
        fscanf(azelfile,"%f %f",&aztab[i],&eltab[i]);
 fclose(azelfile);
 
 printf("Peak Position: %f, %f\n",azpeak, elpeak);
 command(azpeak+aztab[0], elpeak+eltab[0]);
 printf("Positioning to first point in table...\n");
 printf("Enter Data File type: ");
 fgets(ftype,13,stdin);  /* dummy read to clear newline from buffer */
 fgets(ftype,13,stdin);
 gen_fname(ftype,fname);
 printf("Opening %s\n",fname);
 datfile = fopen(fname,"w");
 printf("Enter measurement descriptor('.' to exit):\n");
 fgets(descriptor,118,stdin);
 if ((strlen(descriptor)<3) && (descriptor[0]=='.')) return;
      /* set mode of stdin for immediate read, no echo */
 tcgetattr(0, &options);
 options.c_lflag &= ~(ICANON | ECHO | ECHOE);
 tcsetattr(0, TCSANOW, &options);
 fcntl(0, F_SETFL, FNDELAY);
 key = 0;
 thresh_s = thresh * thresh;
 gettimeofday(&t, &tz);
 dtstart = t.tv_sec + (t.tv_usec/1.e6);
 start_timet = t.tv_sec;
 pointindex = 0;
 dwell = -1.0;
 dtarrive = -1.0;
 lastread = -1.0;
 nrecords = 0;

 while(1) {
   scanf("%c",&key);
   if (key == 27) break;
   gettimeofday(&t, &tz);
   dt = (t.tv_sec + (t.tv_usec/1.e6)) - dtstart;
   read_encoders(&az, &el);
   if ((dt-lastread) >= readfreq) {
     rread(&amp1, &amp2, &ph);
     lastread = dt;
     fprintf(datfile,"%.4lf\t%.4f\t%.4f\t%.2f\t%.2f\t%.1f\n",
                     dt, az, el, amp1, amp2, ph);
     ++nrecords;
     }
   azdiff = (az-azpeak) - aztab[pointindex];
   eldiff = (el-elpeak) - eltab[pointindex];
   vdiff = (azdiff * azdiff) + (eldiff * eldiff);
   if (vdiff < thresh_s) {
     if (dtarrive < 0.0) {  /* first arrival */
       printf("Arrived at point %d\n",pointindex);
       dtarrive = dt;
          /* Changed the dwell from 3 and 3.5 seconds to effectively
	     zero for first and last points.  8/16/2001  DSB  */
       if (pointindex == 0) dwell = -1.0;
       else if (pointindex == (nazel-1)) dwell = -1.0;
       else dwell = -1.0;
       }
     if ((dt-dtarrive) >= dwell) {
       ++pointindex;
       if (pointindex >= nazel) break;
       command(azpeak+aztab[pointindex], elpeak+eltab[pointindex]);
       dtarrive = -1.0;
       }
     }
     
   }
 printf("Done.  Cleaning up...\n");  
 free(aztab);
 free(eltab);
 fclose(datfile);
 printf("Created data file: %s\n",fname);
 makemat(fname,nrecords,descriptor,ctime(&start_timet),azelfname);
  
     /* reset mode of stdin */
 options.c_lflag |= (ICANON | ECHO | ECHOE | ISIG);
 tcsetattr(0, TCSANOW, &options);
 fcntl(0, F_SETFL, 0);
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
 fprintf(outfile, "#Year\tDayofYear\tHour\tMin\tSec\tElapsed_Sec\tAz\tEl\n");
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
   fprintf(outfile,"%d\t%d\t%02d\t%02d\t%lf\t%lf\t%f\t%f",
		   timestruct.tm_year+1900,timestruct.tm_yday,
		   timestruct.tm_hour,timestruct.tm_min,tcur, dt, az, el);
   fprintf(stdout,"%lf\t%f\t%f", dt, az, el);
   fprintf(outfile,"\n");
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

/*----------------------------------------------------------------
  Read initialization file containing parameters.
---------------------------------------------------------------*/
void readini()
 {
 FILE *inifile;
 char line[80];
 
 inifile = fopen("antest.ini","r");
 if (inifile == NULL) {
   printf("** Cannot read ini file **\n");
   exit(3);
   }
   
 fgets(line,78,inifile);
 sscanf(line,"%f",&thresh);
 fgets(line,78,inifile);
 sscanf(line,"%d",&Rcvr_addr);

/* Dummy read.  Used to be 'encflag' (parallel or serial) */
 fgets(line,78,inifile);

 fgets(line,78,inifile);
 sscanf(line,"%f",&azpeak);
 fgets(line,78,inifile);
 sscanf(line,"%f",&elpeak);
 fgets(line,78,inifile);
 sscanf(line,"%lf",&readfreq);
 fclose(inifile);
 printf("thresh = %f\naddr = %d\n",thresh,Rcvr_addr);
 printf("azpeak = %f\nelpeak = %f\nreadfreq = %lf\n",azpeak,elpeak,readfreq);
 }
 
/*----------------------------------------------------------------
newpeak allows user to define new peak.  Global variables are set
     to new values, and antest.ini file is also updated.
---------------------------------------------------------------*/
void newpeak()
 {
 FILE *inifile;
 char line[80];
 long posaz, posel;
 
/* inifile = fopen("antest.ini","r+");
 if (inifile == NULL) {
   printf("** Cannot read ini file **\n");
   return;
   }
*/

 printf("Old Peak: (%f, %f)\nEnter new peak AZ, EL: ",azpeak, elpeak);
 scanf("%f, %f", &azpeak, &elpeak);
 return;
 
 fgets(line,78,inifile);  /* thresh */
 fgets(line,78,inifile);  /* receiver address */
 fgets(line,78,inifile);  /* encoder flag */
 posaz = ftell(inifile);    /* get current position */
 fgets(line,78,inifile);  /* get az peak line */
 posel = ftell(inifile);

 rewind(inifile);
 fseek(inifile,posaz,SEEK_SET);
 fprintf(inifile,"%-14f",azpeak);
 fseek(inifile,posel,SEEK_SET);
 fprintf(inifile,"%-14f",elpeak);
 fclose(inifile);
 }
 

void findpeak()
 {
 float vdiff, diffprev;
 
 return;
 command(azpeak, elpeak);
 do
   {
   }
 while((vdiff>0.005) && (diffprev>0.0009));
 }

int main(int argc, char *argv[])
 {
 float az, el, ch1, ch2, ph;
 int optn, fret;
 
 sock = 1;  /* use network socket for ACU control */
 printf("Initializing COM port...\n");
 init_com1();

 acuflag = 0;
 fdacu = -1;
 if (argc > 1) {
   if (argv[1][0] == 'A') {
     printf("Using ACU for antenna control\n");
     acuflag = 1;
     fret = init_acu();
   }
 }

 readini();
 
 close_encd();  /* shut down encoder daemon if running */
 

 printf("Initializing GPIB...\n");
 SendIFC(0);
 if (ibsta & ERR) {
   printf("Cannot initialize GPIB!!  Exiting...\n");
   exit(3);
   }
 setrcvr();  /* initialize receiver */
 
 printf("Initializing DIO card...\n");
 InitDIO(&boardid);

 //set_pack();

 Sfreq = 0;
 
 do {
   printf("\n Main Menu:\n\n");
   printf("1. Single read of encoders\n");
   printf("2. Init. 1795 Receiver\n");
   printf("3. Single read of receiver\n");
   printf("4. Scan Antenna\n");
   printf("5. loop read of encoders\n");
   printf("6. Position to peak\n");
   printf("7. Re-read initialization file (antest.ini)\n");
   printf("8. Quit Lincoln Pointing system\n");
   printf("9. Enter new Az, El peak values\n");
   printf("10. Change Pack Frequency\n");
   printf("11. Reformat text file to MAT file\n");
   printf("12. Find Peak\n");
   printf("13. Pos to ABS Angles\n");
   printf("14. Single SpecAN cut\n");
   printf("15. Record SpecAN cut (cont)\n");
   printf("\n0. EXIT\n\nEnter option number: ");
   scanf("%d",&optn);
   if (optn == 1) {
     read_encoders(&az, &el);
     printf("az = %f, el = %f\n", az, el); }
   if (optn == 2) setrcvr();
   if (optn == 3) {
     rread(&ch1, &ch2, &ph);
     printf("ch1 = %f, ch2 = %f, ph = %f\n", ch1, ch2, ph); }
   if (optn == 4) ascan();
   if (optn == 5) lpread();
   if (optn == 6) command(azpeak, elpeak);
   if (optn == 7) readini();
   if (optn == 8) if (acuflag != 1) write(fd1,"QUIT\n",5);
   if (optn == 9) newpeak();
   if (optn == 10) chgfreq();
   if (optn == 11) makemat("",0,"","","unknown");
   if (optn == 12) findpeak();
   if (optn == 13) {
     printf("Enter az, el angles: ");
     scanf("%f,%f", &az, &el);
     abscommand(az,el);
     wait2arrive(az,el);
   }
   if (optn == 14) specscan();
   if (optn == 15) recscan();
 }
 while (optn > 0);
 if ((fd1 > 0) && (sock == 1)) close(fd1);
 if (boardid > 0) CloseDIO(boardid);
 if (acuflag == 1) close_acu();
 return(0); 
 }
