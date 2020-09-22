#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

const int MAX_SIZE = 1e6;

int main () {
    //flags:    O_RDONLY
    FILE* readfile = fopen ("channel.fifo", "r");
    char *buf = (char *) calloc (MAX_SIZE, sizeof (char));
    fprintf (stdout, "INPUT WHEN READY TO START (READER)\n");
    fscanf (stdin, "%s", buf);
    if (fscanf (readfile, "%s", buf) == 1)
        fprintf (stderr, "READ SUCCESS\n");
    else
        fprintf (stderr, "READ FAIL\n");
    return 0;
}