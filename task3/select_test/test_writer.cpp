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
    int writefile = -1;
    while (writefile == -1)
        writefile = open ("test.fifo", O_WRONLY | O_NONBLOCK);
    //write (writefile, "abc", 3);
    sleep (5);
    return 0;
}