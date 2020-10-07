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

int main () {
    char msg [PIPE_BUF];
    for (unsigned i = 0; i < PIPE_BUF; ++i)
        msg[i] = 'a' + rand() % 20;
    msg[PIPE_BUF - 1] = '\0';

    int writefile = open ("channel.fifo", O_WRONLY | O_NONBLOCK);

    while (writefile == -1) {
        if (errno == ENXIO) {
            //  ENXIO <- никто не открыл FIFO для чтения
            fprintf (stdout, "Reader has been broken\n");
            exit (EXIT_FAILURE);
        }
        //  EEXIST <- файл не существует
        writefile = open ("channel.fifo", O_WRONLY | O_NONBLOCK);
    }

    char buf = '\0';

    fprintf (stdout, "INPUT WHEN READY TO START (WRITER)\n");
    fscanf (stdin, "%c", &buf);

    int num = 0;
    unsigned cnt = 0;

    while (true) {
        num = write (writefile, msg, PIPE_BUF);

        if (num == PIPE_BUF) {
            fprintf (stderr, "WRITE SUCCESS\n");
        }
        else {
            fprintf (stderr, "WRITE FAIL, write returned: %d\n", num);

        }
        ++cnt;
        sleep (1);
    }

    close (writefile);
    unlink ("channel.fifo");
    exit (EXIT_SUCCESS);
}