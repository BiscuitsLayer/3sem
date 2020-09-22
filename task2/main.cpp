#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <cassert>
#include <unistd.h>
#include <pthread.h>

const int MAX_SIZE = 1e3;

void *PrintInfo (void *data) {
    pid_t own = getpid ();
    pid_t parent = getppid ();
    ++(*(int *)data);
    printf ("own: %d parent: %d number: %d\n", own, parent, *((int *)data));
}

void ThreadTest (int n = MAX_SIZE) {
    pthread_t threads [MAX_SIZE];
    int data = 0;

    for (int cnt = 0; cnt < n; ++cnt) {
        int returnVal = pthread_create (threads + cnt, nullptr, &PrintInfo, &data);
        int terminate = 0;
        pthread_join (threads[cnt], (void **)terminate);
        if (returnVal != 0) {
            printf ("Error caught\n");
            exit (1);
        }
    }
    printf ("Data value: %d\n", data);
}

void ProcessTest (int n = MAX_SIZE) {
    int cnt = 0;
    while (cnt < n) {
        pid_t isParent = fork ();
        int status = 0;
        if (isParent != 0) {
            wait (&status);
            ++cnt;
            continue;
        }
        printf ("own: %5d \t parent: %5d \t number: %5d\n", getpid (), getppid (), cnt);
        exit (0);
    }
}

void ExecTest (int argc, char **argv) {
    int returnVal = execvp (argv [1], argv + 1);
    if (returnVal == -1)
        returnVal = execv (argv [1], argv + 1);
    if (returnVal == -1)
        printf ("Execution attempt error\n");
}

int main (int argc, char **argv) {
    assert (argc == 2);
    int n = atoll (argv[1]);
    ThreadTest (n);
    //ExecTest (argc, argv);
    return 0;
}