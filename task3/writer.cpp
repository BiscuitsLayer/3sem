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

const int FILENAME_SIZE = 20;

int main () {

    pid_t pid = getpid ();
    char serviceName [FILENAME_SIZE];
    sprintf (serviceName, "channel_%d.fifo", pid);

    char msg [PIPE_BUF];
    for (unsigned i = 0; i < PIPE_BUF; ++i)
        msg[i] = 'a' + rand() % 20;
    msg[PIPE_BUF - 1] = '\0';

    if (mkfifo (serviceName, 0777) == -1) {
        fprintf (stderr, "Error creating service fifo\n");
        exit (EXIT_FAILURE);
    }

    int serviceReadfile = open (serviceName, O_RDONLY | O_NONBLOCK);
    if (serviceReadfile == -1) {
        fprintf (stderr, "Error opening service fifo\n");
        exit (EXIT_FAILURE);
    }

    int openSuccededCount = 0;
    int openSuccededPid = 0;

    while (openSuccededCount == 0) {
        if (read (serviceReadfile, &openSuccededPid, 4) == 1)
            openSuccededCount++;
    }

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
    //  В этот момент можно запустить второй writer, который будет печатать в ту же FIFO
    //  Чтобы избежать такого, нужно отправить через второй FIFO в обратную сторону (write
    //  конец NON_BLOCK) сообщение об успешном коннекте
    sleep (5);
    //
    chmod ("channel.fifo", 0000);
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
        if (cnt == 4)
            break;
        sleep (1);
    }

    close (writefile);
    unlink ("channel.fifo");
    exit (EXIT_SUCCESS);
}