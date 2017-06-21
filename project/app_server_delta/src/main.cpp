#include <iostream>
#include <sstream>
#include <string>

#ifndef WIN32
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "common.h"
#include "base/log_binder.h"
#include "base/util.h"
#include "project_server.h"

using namespace serverframe;
using namespace std;

#ifndef WIN32
#include <sys/ipc.h>
#include <sys/shm.h>

//Bit-mask values for 'flags' argument of become_daemon()
#define BD_NO_CHDIR           01    //Don't chdir("/")
#define BD_NO_CLOSE_FILES     02    //Don't close all open files
#define BD_NO_REOPEN_STD_FDS  04    //Don't reopen stdin, stdout, and
//stderr to /dev/null 
#define BD_NO_UMASK0         010    //Don't do a umask(0)

#define BD_MAX_CLOSE  8192          //Maximum file descriptors to close if
//sysconf(_SC_OPEN_MAX) is indeterminate
//Returns 0 on success, -1 on error
int become_daemon(int flags)
{
    int maxfd, fd;

    switch (fork())                     //Become background process
    {
    case -1: return -1;
    case 0:  break;                     //Child falls through...
    default: _exit(EXIT_SUCCESS);       //while parent terminates
    }

    if (setsid() == -1)                 //Become leader of new session
        return -1;

    switch (fork()) {                   //Ensure we are not session leader
    case -1: return -1;
    case 0:  break;
    default: _exit(EXIT_SUCCESS);
    }

    if (!(flags & BD_NO_UMASK0))
        umask(0);                       //Clear file mode creation mask

    if (!(flags & BD_NO_CHDIR))
        chdir("/");                     //Change to root directory

    if (!(flags & BD_NO_CLOSE_FILES)) { //Close all open files
        maxfd = sysconf(_SC_OPEN_MAX);
        if (maxfd == -1)                //Limit is indeterminate...
            maxfd = BD_MAX_CLOSE;       //so take a guess 

        for (fd = 0; fd < maxfd; fd++)
            close(fd);
    }

    if (!(flags & BD_NO_REOPEN_STD_FDS)) {
        close(STDIN_FILENO);            //Reopen standard fd's to /dev/null

        fd = open("/dev/null", O_RDWR);

        if (fd != STDIN_FILENO)         //'fd' should be 0
            return -1;
        if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
            return -1;
        if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
            return -1;
    }

    return 0;
}
#endif

void version()
{
    cout << "date:2017-04-12" << endl
        << "ver:1.1.0      " << endl;
}

void usage(const char *bin)
{
    cout << endl
        << "-----------------------------------------------------" << endl
        << "usage: " << bin << "[options]" << endl
        << "   -v           : get version of program" << endl
        << "   -d           : service run as deamon" << endl
        << "   -o           : output log to the console" << endl
        << "   -h           : get help info" << endl
        << "   -l           : service run with local-config-flie" << endl
        << "-----------------------------------------------------" << endl;
}

static int proc_main(std::string& param)
{
    project_server::create_instance();
    project_server::get_instance()->start(param);
    project_server::get_instance()->join();
    project_server::destory_instance();
    return 0;
}

int main(int argc, char **argv)
{
    std::string curr_path = argv[0];
    int pos = curr_path.rfind("\\");
    curr_path = curr_path.substr(0, pos);

    std::string config_file = curr_path + 
        "/config/manager-server-config.xml";
    std::string log_config_file = curr_path + "/config/log-config.xml";
    const char * binary_name = strrchr(argv[0], '/');

    bool run_as_deamon = false;
    bool output_log_to_console = false;
#ifndef WIN32
    char c;
    while ((c = getopt(argc, argv, "lvdhos")) != -1) {
        switch (c) {
        case 'v':
            version();
            return 0;
        case 'd':
            run_as_deamon = true;
            break;
        case 'o':
            output_log_to_console = true;
            break;
        case 'h':
        case '?':
            usage(binary_name);
            return 0;
        case 'l':
            config_file = curr_path + 
                "/config/managerserver-config.xml";
            break;
        default:
            break;
        }
    }

    /* bind trace base log config file */
    if (!base::default_log_binder::bind_trace(log_config_file.c_str())) {
        cout << "trade_simulation server bind trace failed" << endl;
    }
    base::trace::enable_std_output(output_log_to_console);

    TRACE_SYSTEM(MODULE_NAME,"main function to be running %d:%s", 110, "hello manager_server");
    if (run_as_deamon) {
        /* run as deamon */
        become_daemon(BD_NO_CHDIR);
    } else {
        /* ignore broken pipe signal */
        signal(SIGPIPE, SIG_IGN);    
    }
#endif
    proc_main(config_file);

    return 0;
}
