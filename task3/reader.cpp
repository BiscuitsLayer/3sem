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

const int MAX_SIZE = PIPE_BUF;

int main () {
    if (mkfifo ("service.fifo", 0777) == -1) {
        fprintf (stderr, "Error creating service fifo\n");
        exit (EXIT_FAILURE);
    }

    if (mkfifo ("channel.fifo", 0777) == -1) {
        fprintf (stderr, "Error creating channel fifo\n");
        exit (EXIT_FAILURE);
    }

    int readfile = open ("channel.fifo", O_RDONLY | O_NONBLOCK);
    if (readfile == -1) {
        fprintf (stderr, "Error opening file\n");
        exit (EXIT_FAILURE);
    }
    char *buf = (char *) calloc (MAX_SIZE, sizeof (char));
    
    fprintf (stdout, "INPUT WHEN READY TO START (READER)\n");
    fscanf (stdin, "%s", buf);

    int num = 0;

    while ((num = read (readfile, buf, PIPE_BUF)), num >= 0) {
        if (num == PIPE_BUF) 
            fprintf (stderr, "READ SUCCESS\n");
        else if (num == 0) {
            fprintf (stderr, "ZERO\n");
            break;
        } 
        else 
            fprintf (stderr, "FAILURE\n");
        sleep (1);
    }
    
    if (num == -1 && errno == EAGAIN) {
    //  EAGAIN <- сейчас нет данных в FIFO
        fprintf (stdout, "Writer has been broken\n");
    }
    close (readfile);
    unlink ("channel.fifo");
    exit (EXIT_SUCCESS);
}