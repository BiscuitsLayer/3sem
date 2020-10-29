#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>

int main () {
    mkfifo ("test.fifo", 0777);
    int readfile = open ("test.fifo", O_RDONLY | O_NONBLOCK);
    struct timeval tv = { 5, 0 };
    fd_set test;
    FD_ZERO (&test);
    FD_SET (readfile, &test);
    int retVal = 0;
    retVal = select (readfile + 1, &test, nullptr, nullptr, &tv);
    char msg [10];
    read (readfile, msg, 3);
    fprintf (stdout, "%d\n", retVal);
    retVal = FD_ISSET (readfile, &test);
    fprintf (stdout, "%d\n", retVal);
    unlink ("test.fifo");
    return 0;
}