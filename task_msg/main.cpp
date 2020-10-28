#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <errno.h>

const int MAX_PROC_COUNT = 10000;

int main (int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char *error = nullptr;

    if (argc != 2) {
        fprintf (stderr, "Wrong number of arguments!\n");
        exit (EXIT_FAILURE);
    }
    
    errno = 0;
    long long childrenCount = strtoll (argv[1], &error, 10);

    if (errno != 0) {
        fprintf (stderr, "Number overflow!");
        fprintf (stdout, "\n");
        exit (EXIT_FAILURE);
    }

    if (childrenCount <= 0 || childrenCount >= MAX_PROC_COUNT) {
        fprintf (stderr, "Invalid input!");
        fprintf (stdout, "\n");
        exit (EXIT_FAILURE);
    }

    int msgid = msgget (IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0666);

    unsigned myNum = 0;
    pid_t forkPid = 0;
    unsigned dataSize = sizeof (msgbuf) - sizeof (long);

    for (long long i = 1; i <= childrenCount; ++i) {
        forkPid = fork ();
        if (forkPid == 0) { //  CHILD
            myNum = i;
            break;
        }
    }

    if (forkPid != 0) { //  PARENT
        for (long long i = 1; i <= childrenCount; ++i) {
            msgbuf msg = { i };
            if (msgsnd (msgid, &msg, dataSize, IPC_NOWAIT) < 0) {
                fprintf (stderr, "Parent sending error!\n");
                exit (EXIT_FAILURE);
            }
            if (msgrcv (msgid, &msg, dataSize, childrenCount + 1, MSG_NOERROR) < 0) {
                fprintf (stderr, "Parent receiving error!\n");
                exit (EXIT_FAILURE);
            }
        }
        msgctl (msgid, IPC_RMID, nullptr);
        fprintf (stdout, "\n");
        return 0;
    }
        
    if (forkPid == 0) { //  CHILD
        msgbuf msg = { myNum };
        if (msgrcv (msgid, &msg, dataSize, myNum, MSG_NOERROR) < 0) {
            fprintf (stderr, "Child %d receiving error!\n", myNum);
            exit (EXIT_FAILURE);
        }
        fprintf (stdout, myNum == 1 ? "%d" : " %d", myNum);

        msg.mtype = childrenCount + 1;
        if (msgsnd (msgid, &msg, dataSize, IPC_NOWAIT) < 0) {
            fprintf (stderr, "Child %d sending error!\n", myNum);
            exit (EXIT_FAILURE);
        }
        return 0;
    }
}