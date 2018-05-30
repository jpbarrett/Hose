#ifndef __SAFECALLS_H__
#define __SAFECALLS_H__

#include <stdio.h>        /* required for FILE * stuff */
#include <sys/stat.h>        /* required for struct stat stuff */
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#ifndef __SAFECALLS__C__
FILE *SafeLibErrorDest;
#endif

char *safestrdup(const char *s);
char *safestrncpy(char *dest, const char *src, size_t n);
char *safestrcat(char *dest, const char *src, size_t n);
int safekill(pid_t pid, int sig);
char *safegetenv(const char *name);
int safechdir(const char *path);
int safemkdir(const char *path, mode_t mode);
int safestat(const char *file_name, struct stat *buf);
int safeopen(const char *pathname, int flags);
int safeopen2(const char *pathname, int flags, mode_t mode);
int safepipe(int filedes[2]);
int safedup2(int oldfd, int newfd);
int safeexecvp(const char *file, char *const argv[]);
int saferead(int fd, void *buf, size_t count);
int safewrite(int fd, const char *buf, size_t count);
int safeclose(int fd);
FILE *safefopen(char *path, char *mode);
size_t safefread(void *ptr, size_t size, size_t nmemb, FILE *stream);
char *safefgets(char *s, int size, FILE *stream);
size_t safefwrite(void *ptr, size_t size, size_t nmemb, FILE *stream);
int safefclose(FILE *stream);
int safefflush(FILE *stream);
void *safemalloc(size_t size);
void HandleError(int ecode, const char *const caller,
         const char *fmt, ...);


#endif
