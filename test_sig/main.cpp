#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

void JustChill () {
    fprintf (stdout, "I'm gonna chill...\n");
}

void Handler (int sig) {
    fprintf (stderr, "GOTCHA!\n");
}

int main () {
    sigset_t initBlockedSignals;
    sigfillset (&initBlockedSignals);
    sigprocmask (SIG_SETMASK, &initBlockedSignals, nullptr);

    sigset_t sigMask = {};
    sigfillset (&sigMask);

    struct sigaction sig1Handler;
    sig1Handler.sa_handler = Handler;
    sig1Handler.sa_mask = sigMask;
    sigaction (SIGUSR1, &sig1Handler, nullptr);

    pid_t childPid = fork ();

    if (childPid == 0) {    //  CHILD
        kill (getppid(), SIGUSR1);
    }
    else {                  //  PARENT
        JustChill ();
        //fprintf (stdout, "I'm gonna chill...\n");
    }
    return 0;
}