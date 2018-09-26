#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "HSpectrometerManager.hh"

using namespace hose;

static void daemonize(void)
{
    pid_t pid, sid;
    int fd; 

    //already running a child process
    if ( getppid() == 1 ) return;

    pid = fork();
    if (pid < 0){exit(1); } // could not fork
    if (pid > 0){exit(0); } // kill parent

    //promote child to its own process
    sid = setsid();
    if (sid < 0){exit(1);}

    //change working directory to HOSE bin install
    if( chdir( STR2(BIN_INSTALL_DIR) ) < 0){exit(1);}

    //redirect stdout/stderr/etc to /dev/null
    fd = open("/dev/null",O_RDWR, 0);
    if (fd != -1)
    {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2){close (fd);}
    }

    /*resetting File Creation Mask */
    umask(022);
}


int main(int /*argc*/, char** /*argv*/)
{
    daemonize();

    /* this thread runs now runs as a daemon in the background */
    HSpectrometerManager<>* specManager = new HSpectrometerManager();
    specManager->Initialize();
    std::thread spectrometer_thread( &HSpectrometerManager<>::Run, specManager);
    spectrometer_thread.join();

    return 0;
}
