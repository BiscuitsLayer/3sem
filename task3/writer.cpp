#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int main () {
    //flags:    O_WRONLY | O_CREAT | O_TRUNC
    FILE *writefile = fopen ("channel.fifo", "w");   //in case of fifo O_TRUNC flag is ignored

    if (!writefile) {
        fprintf (stderr, "Error opening file\n");
    }

    char buf = '\0';
    fprintf (stdout, "INPUT WHEN READY TO START (WRITER)\n");
    fscanf (stdin, "%c", &buf);
    if (fprintf (writefile, "%s", "abc") == 3)
        fprintf (stderr, "WRITE SUCCESS\n");
    else
        fprintf (stderr, "WRITE FAIL\n");
    return 0;
}