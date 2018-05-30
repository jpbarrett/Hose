/* This module contains wrappers around a number of system calls and
   library functions so that a default error behavior can be defined.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>

#define __SAFECALLS_C__
#include "safecalls.h"

/* The first two are automatically set by HandleError.  The third you can
   set to be the file handle to which error messages are written.  If
   NULL, is taken to be stderr. */

const char *SafeLibErrorLoc;
int SafeLibErrno = 0;
FILE *SafeLibErrorDest = NULL;

char *safestrdup(const char *s)
{
  char *retval;

  retval = strdup(s);
  if (!retval)
    HandleError(0, "strdup", "dup %s failed", s);
  return retval;
}

char *safestrncpy(char *dest, const char *src, size_t n)
{
  if (strlen(src) >= n)
    HandleError(0, "strncpy", "Attempt to copy string \"%s\"\n"
                   "to buffer %d bytes long", src, (int) n);
  return strncpy(dest, src, n);
}

char *safestrcat(char *dest, const char *src, size_t n)
{
  if ((strlen(src) + strlen(dest)) >= n)
    HandleError(0, "strcat", "Attempt to strcat too big a string");
  return strncat(dest, src, n - 1);
}


int safekill(pid_t pid, int sig)
{
  int retval;

  retval = kill(pid, sig);
  if (retval == -1)
    HandleError(errno, "kill", "kill (pid %d, sig %d) failed", (int) pid, sig);
  return retval;
}


char *safegetenv(const char *name)
{
  char *retval;

  retval = getenv(name);
  if (!retval)
    HandleError(errno, "getenv", "getenv on %s failed", name);
  return retval;
}

int safechdir(const char *path)
{
  int retval;

  retval = chdir(path);
  if (retval == -1)
    HandleError(errno, "chdir", "chdir to %s failed", path);
  return retval;
}

int safemkdir(const char *path, mode_t mode)
{
  int retval;

  retval = mkdir(path, mode);
  if (retval == -1)
    HandleError(errno, "mkdir", "mkdir %s failed", path);
  return retval;
}

int safestat(const char *file_name, struct stat *buf)
{
int retval;
  retval = stat(file_name, buf);
  if (retval == -1)
    HandleError(errno, "stat", "Couldn't stat %s", file_name);
  return retval;
}  

int safeopen(const char *pathname, int flags)
{
int retval;
  if ((retval = open(pathname, flags)) == -1) {
    HandleError(errno, "open", "open %s failed", pathname);
  }
  return retval;
}

int safeopen2(const char *pathname, int flags, mode_t mode)
{
  int retval;

  retval = open(pathname, flags, mode);
  if (retval == -1)
    HandleError(errno, "open2", "Open %s failed", pathname);
  return retval;
}

int safepipe(int filedes[2])
{
  int retval;

  retval = pipe(filedes);
  if (retval == -1)
    HandleError(errno, "pipe", "failed");
  return retval;
}

int safedup2(int oldfd, int newfd)
{
  int retval;

  retval = dup2(oldfd, newfd);
  if (retval == -1)
    HandleError(errno, "dup2", "failed"); 
  return retval;
}

int safeexecvp(const char *file, char *const argv[])
{
  int retval;

  retval = execvp(file, argv);
  if (retval == -1)
    HandleError(errno, "execvp", "execvp %s failed", file);
  return retval;
}

int saferead(int fd, void *buf, size_t count)
{
  int retval;

  retval = read(fd, buf, count);
  if (retval == -1)
    HandleError(errno, "read",
        "read %d bytes from fd %d failed", (int) count, fd);
  return retval;
}

int safewrite(int fd, const char *buf, size_t count)
{
  int retval;

  retval = write(fd, buf, count);
  if (retval == -1)
    HandleError(errno, "write",
        "write %d bytes to fd %d failed", (int) count, fd);
  return retval;
}

int safeclose(int fd)
{
  int retval;

  retval = close(fd);

  if (fd == -1) {
    HandleError(errno, "close", "Possible serious problem: close failed");
  }
  return retval;
}

FILE *safefopen(char *path, char *mode)
{
  FILE *retval;

  retval = fopen(path, mode);
  if (!retval)
    HandleError(errno, "fopen", "fopen %s failed", path);
  return retval;
}

size_t safefread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t retval;

  retval = fread(ptr, size, nmemb, stream);
  if (ferror(stream))
    HandleError(errno, "fread", "failed");
  return retval;
}

char *safefgets(char *s, int size, FILE *stream) {
  char *retval;

  retval = fgets(s, size, stream);
  if (!retval) 
    HandleError(errno, "fgets", "failed");
  return retval;
}

size_t safefwrite(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t retval;

  retval = fread(ptr, size, nmemb, stream);
  if (ferror(stream))
    HandleError(errno, "fwrite", "failed");
  return retval;
}

int safefclose(FILE *stream)
{
  int retval;

  retval = fclose(stream);
  if (retval != 0)
    HandleError(errno, "fclose", "Possibly serious error: fclose failed");
  return retval;
}

int safefflush(FILE *stream)
{
  int retval;

  retval = fflush(stream);
  if (retval != 0)
    HandleError(errno, "fflush", "fflush failed");
  return retval;
}

void *safemalloc(size_t size)
{
  void *retval;

  retval = malloc(size);
  if (!retval)
    HandleError(0, "malloc", "malloc failed");
  return retval;
}

void HandleError(int ecode, const char *const caller,
         const char *fmt, ...) {

  va_list fmtargs;
  struct sigaction sastruct;
  FILE *of = (SafeLibErrorDest) ? SafeLibErrorDest : stderr;

  /* Safe these into global variables for any possible signal handler. */

  SafeLibErrorLoc = caller;
  SafeLibErrno = ecode;

  /* Print the error message(s) */

  va_start(fmtargs, fmt);

  fprintf(of, "*** Error in %s: ", caller);
  vfprintf(of, fmt, fmtargs);
  va_end(fmtargs);
  fprintf(of, "\n");
  if (ecode) {
    fprintf(of, "*** Error cause: %s\n", strerror(ecode));
  }

  /* Return if no signal handler.  Otherwise, raise a signal. */

  sigaction(SIGUSR1, NULL, &sastruct);
  if (sastruct.sa_handler != SIG_DFL) {
    raise(SIGUSR1);
  }
}
